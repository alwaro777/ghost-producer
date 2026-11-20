# ADR 0004 — Modelo de hilos, colas lock-free, snapshots y detector RT

Estado: aceptado (F0, G1) · Fecha: 2026-11-20 · Contratos: `include/rt/*.h`,
`include/engine/*.h`

## Contexto
Regla 1 del proyecto: el hilo de audio no reserva memoria, no usa locks, no hace I/O ni logs.
La estabilidad del audio es la máxima prioridad. Este esquema lo heredan todas las fases.

## Decisión
**Hilos**
| Hilo | Qué hace | Puede bloquear |
|---|---|---|
| Mensajes (UI) | UI, Document, CommandHistory, compilación modelo→motor, guardar/abrir (F0) | Sí (con moderación) |
| Audio (driver) | `IRenderCore::process` | **No** |
| MIDI (driver) | Recibe MIDI; en F0 solo cuenta eventos (atómico) | No |
| Fondo (futuro) | Escaneo de plugins, lectura de disco, IA | Sí |

**Modelo → motor**: tras cada cambio del documento, el hilo de mensajes compila un
`EngineSnapshot` inmutable (`buildSnapshot`) y lo publica con `rt::SnapshotExchange`
(puntero atómico pendiente + cola SPSC de retiro). El audio toma la instantánea al principio de
cada bloque; las antiguas vuelven por la cola y se destruyen en el hilo de mensajes
(`collectGarbage`, ≥10 Hz). El audio nunca ejecuta un destructor.

**Motor → UI**: picos y métricas con atómicos (`takePeaks`, `takeMetrics`) leídos por un timer
de UI a 30 Hz. Flujos de eventos (posición, MIDI) usarán `rt::SpscQueue` de capacidad fija.

**Valores continuos**: las rampas (5–20 ms) las hace el motor al ver un valor nuevo en la
instantánea; en F0 basta una instantánea por cambio. Los parámetros de alta frecuencia
(automatización en directo) usarán atómicos por parámetro en fases posteriores.

**Callback**: `rt::ScopedRealtimeSection` + `juce::ScopedNoDenormals` al entrar. El mismo
`IRenderCore` sirve para tiempo real y render offline (determinismo, tests golden).

**Detector RT (debug)**: `GP_RT_CHECKS=1` en presets `dev`, `asan`, `headless`. Reemplazo global
de `operator new/delete` que cuenta como violación toda reserva/liberación dentro de una
sección RT (flag `thread_local`); `GP_RT_ASSERT_NOT_REALTIME()` en nuestro código que bloquea o
hace I/O. Handler RT-safe (por defecto solo cuenta). La UI de debug muestra el contador; los
tests de motor exigen 0. TSan cubre las primitivas lock-free (preset `tsan`). En release el
detector no existe (coste cero).

**Excepción documentada**: `IAudioEngine::deviceManagerForSettingsUI()` expone el
`juce::AudioDeviceManager` solo para alojar `AudioDeviceSelectorComponent` (selección de
dispositivo de audio y MIDI). Es una preferencia de la app, no del proyecto, y no pasa por
Commands.

## Alternativas
- Mutex con `try_lock` en el audio: no bloquea, pero el escritor sí puede retrasar al audio
  e invierte prioridades. Descartado.
- `std::shared_ptr` atómico (C++20 `atomic<shared_ptr>`): no es lock-free en libc++/MSVC y el
  último `release` podría ocurrir en el audio. Descartado.
- Detector por interposición de `pthread_mutex_lock`/`malloc`: más completo pero no portable a
  macOS/Windows. Posible ampliación solo en Linux CI.

## Consecuencias
- Toda estructura que lea el audio debe estar preasignada en la instantánea.
- El hook de `operator new` debe estar en la misma unidad de compilación que algo que el
  ejecutable use seguro (o el enlazador puede descartarlo de la librería estática).
- ASan y el hook conviven (el hook acaba en `malloc`, que ASan intercepta); se pierde la
  detección de *new/delete type mismatch* de ASan, aceptable.

## Licencias
Sin dependencias nuevas.
