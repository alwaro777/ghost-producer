# F0-BUG3 — GP_RT_ASSERT_NOT_REALTIME en el motor
Dueño: motor-audio · Modelo: haiku · Depende de: — · Paralelo con: F0-BUG1, F0-BUG2
Prioridad: importante I3 (parte motor)

## Objetivo
Las funciones del motor que bloquean o hacen I/O fallan en debug si se las llama desde el hilo de audio.

## Contexto mínimo (lee SOLO esto)
- docs/tareas/F0-REVISION.informe.md § IMPORTANTES 3
- docs/adr/0004-hilos-y-tiempo-real.md (§ GP_RT_ASSERT_NOT_REALTIME)
- src/engine/device/AudioEngine.cpp

## Puedes editar
- src/engine/**, tests/engine/**

## Criterios de aceptación
- [ ] `GP_RT_ASSERT_NOT_REALTIME()` al inicio de AudioEngine::setProject, timerCallback y de
      toda sección que tome `writerLock_`.
- [ ] Test: llamar a una de ellas dentro de ScopedRealtimeSection registra una violación (si el
      detector lo permite sin abortar el test).
- [ ] `ctest --preset headless` verde.
