Estado: HECHO
Archivos: src/engine/rt/RtGuard.cpp (+213), include/rt/SpscQueue.h (+51, bajo marcador), include/rt/SnapshotExchange.h (+53, bajo marcador), tests/engine/rt/{RtGuardTests,SpscQueueTests,SnapshotExchangeTests}.cpp (+559)
Criterios: 7/7 ✓ (19 casos [rt]; headless 36/36, tsan 36/36 con 8 de detector en SKIP porque tsan usa GP_RT_CHECKS=OFF)
Bloqueos: —
Decisiones locales: hook (20 variantes new/delete) en la misma TU que ScopedRealtimeSection, solo con GP_RT_CHECKS=1 (0 símbolos en tsan/release); delete(nullptr) no cuenta; el handler corre con la detección suspendida (sin recursión si reserva).
Deuda/TODO: tsan no ejecuta los tests del detector (preset con GP_RT_CHECKS=OFF, decisión del arquitecto); SpscQueue sin caché local de índices (no puedo añadir miembros); al hacer pop el slot queda movido-desde hasta que se sobrescribe.
Aviso: el commit «wip» fd9818a (no mío) ya incluye una versión intermedia de los dos headers; el árbol de trabajo tiene la final (static_cast<T&&> en vez de std::move). No hice commit.
Medición: carga DSP N/A (sin audio); latencia añadida 0 muestras. tryPush/tryPop/acquire wait-free, O(1), 0 reservas (verificado con el detector).
---

## Detalle técnico

### RtGuard.cpp
- Estado por hilo `constinit thread_local` de tipo trivial (sin inicializador dinámico de
  TLS, seguro dentro de `operator new` y antes de `main`). Contador global y handler en
  atómicos `constinit`.
- `isInRealtimeSection() = rtDepth > 0 && allowDepth == 0`; ambas profundidades anidables.
- Variantes reemplazadas: new/new[] × {normal, nothrow, aligned, aligned+nothrow} y
  delete/delete[] × {normal, nothrow, sized, aligned, aligned+nothrow, sized+aligned}.
  Todo va a `malloc/free` (`posix_memalign` para alineadas; `_aligned_malloc/_aligned_free`
  en Windows). Respeta `new_handler` / `bad_alloc`.
- Con `GP_RT_CHECKS == 0`: todas las funciones son no-op, `checksEnabled()` = false y no se
  define ningún operador (comprobado con `nm`: 20 definiciones en headless, 0 en tsan).

### SpscQueue
Índices monótonos (tail − head = tamaño), máscara por potencia de 2, acquire/release. Se usa
`static_cast<T&&>` en vez de `std::move` porque `<utility>` no está entre los includes del
contrato y no puedo añadir nada por encima del marcador.
Propuesta al arquitecto (opcional): añadir `#include <utility>` al contrato.

### SnapshotExchange
- `publish`: `exchange(acq_rel)` sobre `pending_`; la pendiente desplazada la destruye el
  escritor. No llama a `collectGarbage` (el contrato lo deja al llamante y hacerlo impediría
  el caso «cola de retiro llena»).
- `acquire`: si hay pendiente y hay hueco en la cola de retiro (sizeApprox desde el
  productor es exacto para «hay espacio»), la toma con `exchange(nullptr)` y retira la actual.
  Sin hueco: devuelve la actual y la pendiente sigue ahí.
- Destructor: collectGarbage + pendiente + actual.

### Tests (tests/engine/rt/)
- RtGuard: new → 1 allocation; delete → deallocation; las 9 parejas de variantes → 18;
  fuera de sección → 0; ScopedAllowViolations → 0; anidadas; GP_RT_ASSERT_NOT_REALTIME →
  forbiddenCall; handler que reserva sin recursión; otro hilo no RT reserva → 0;
  SpscQueue y acquire (incluido el intercambio con retiro) dentro de sección → 0.
  Los casos del detector hacen `SKIP` si `!checksEnabled()`.
- SpscQueue: FIFO, vacía/llena (tipo solo-movible: no se mueve si está llena), vueltas
  completas del índice, estrés 2 hilos con 10^6 en orden y suma exacta.
- SnapshotExchange: nullptr inicial, protocolo, cola de retiro llena (capacidad 2), estrés
  10^5 publicaciones: destrucciones == publicadas, 0 en el hilo lector (id de hilo en
  `~Probe`), sin corrupción (magic), secuencia monótona, última vista == 10^5, 0 violaciones
  RT con el lector en sección RT.
- Clang 18 no declara `operator delete(void*, size_t)` sin `-fsized-deallocation`: el test
  no lo llama explícitamente; con GCC la variante sized queda cubierta por `delete b`.

### Verificación
Builds en el scratchpad con las mismas variables de caché que los presets (para no pisar
`build/headless` de los agentes en paralelo):
- headless (GCC, -Werror, GP_RT_CHECKS=ON): ctest 36/36; [rt] 19/19.
- tsan (GCC, TSan, GP_RT_CHECKS=OFF): ctest 36/36 (8 en SKIP); estrés repetido 5 veces sin
  avisos.
- asan+ubsan (GCC): [rt] 19/19. clang 18 -Werror: [rt] 19/19 (no hay runtime de ASan para
  clang en el contenedor).
- Tiempos (Debug): estrés SPSC 10^6 ≈ 0,25 s; estrés snapshot 10^5 ≈ 0,05 s.
