# F0-T3 — Primitivas RT (SPSC, SnapshotExchange) y detector RT
Dueño: motor-audio · Modelo: opus · Depende de: — · Paralelo con: T1, T2, T8

## Objetivo
Existen `rt::SpscQueue`, `rt::SnapshotExchange` y el detector RT (hook de operator new/delete +
`GP_RT_ASSERT_NOT_REALTIME`), probados con TSan y con tests que provocan violaciones.

## Contexto mínimo (lee SOLO esto)
- include/rt/RtGuard.h, include/rt/SpscQueue.h, include/rt/SnapshotExchange.h
- docs/adr/0004-hilos-y-tiempo-real.md
- .claude/skills/daw-orquestador/references/checklist-tiempo-real.md

## Puedes editar
- include/rt/SpscQueue.h y include/rt/SnapshotExchange.h **solo debajo del marcador**
  `// ---- implementación (motor-audio, F0-T3)` (ampliación explícita de propiedad). Las
  declaraciones de arriba no se tocan.
- src/engine/rt/** (RtGuard.cpp con el hook)
- tests/engine/rt/**

## No hagas
- Cambiar firmas ni documentación de include/ (si no alcanzan: BLOQUEADO al arquitecto).
- Usar mutex, `std::atomic<std::shared_ptr>` o reservas en `tryPush/tryPop/acquire`.
- Activar el hook cuando `GP_RT_CHECKS == 0` (release no debe reemplazar operator new).

## Criterios de aceptación (comprobables)
- [ ] El hook de new/delete (todas las variantes: sized, aligned, nothrow, array) vive en la
      misma TU que `ScopedRealtimeSection`, para que el enlazador no lo descarte.
- [ ] Test: dentro de `ScopedRealtimeSection`, `new int` → `violationCount() == 1` y kind
      allocation; fuera, 0; con `ScopedAllowViolations`, 0; secciones anidadas OK;
      `GP_RT_ASSERT_NOT_REALTIME()` dentro → forbiddenCall.
- [ ] Test: `tryPush/tryPop` y `SnapshotExchange::acquire` dentro de una sección RT → 0
      violaciones.
- [ ] SpscQueue: FIFO, llena → false sin mover, vacía → false; test de estrés 2 hilos con
      10^6 elementos en orden, sin pérdidas, limpio con `--preset tsan`.
- [ ] SnapshotExchange: estrés escritor (publish + collectGarbage) / lector (acquire) 10^5
      publicaciones; contador de destrucciones = publicadas al final; ninguna destrucción en el
      hilo lector (comprobado con id de hilo en el destructor del tipo de prueba); limpio con TSan.
- [ ] Con la cola de retiro llena, `acquire()` mantiene la actual y no pierde la pendiente.
- [ ] `ctest --preset headless` y `ctest --preset tsan` verdes.

## Prueba manual (para la guía del usuario)
(Sin prueba manual: la verás como «Violaciones RT: 0» en la ventana de debug de T7.)
