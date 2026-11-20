# F0-BUG1 — Rutas Unicode, guardado duradero y redo en transacciones (modelo)
Dueño: arquitecto · Modelo: sonnet · Depende de: — · Paralelo con: F0-BUG2, F0-BUG3
Prioridad: P0 (B1: pérdida de datos/crash en Windows) + importantes I1, I2, I3 (parte modelo), I5

## Objetivo
El modelo guarda y abre proyectos con rutas no ASCII en todas las plataformas, el guardado
sobrevive a un corte de luz, dos instancias no se pisan y el redo no se pierde por transacciones abortadas.

## Contexto mínimo (lee SOLO esto)
- docs/tareas/F0-REVISION.informe.md § B1 y § IMPORTANTES 1, 2, 3, 5
- docs/adr/0002-formato-proyecto.md, docs/adr/0004-hilos-y-tiempo-real.md (§ GP_RT_ASSERT_NOT_REALTIME)
- src/model/serialization/ProjectFile.cpp, src/commands/CommandHistory.cpp, src/model/Document.cpp

## Puedes editar
- src/model/**, src/commands/**, tests/model/**, tests/commands/**
- docs/adr/0002-formato-proyecto.md (solo para documentar fsync y el .tmp único)

## No hagas
- Cambiar include/ salvo que sea imprescindible (si lo es, documéntalo en el informe)
- Tocar src/app/ ni src/ui/ (eso es F0-BUG2)

## Criterios de aceptación
- [ ] Ninguna llamada a `path::string()` ni construcción de `path` desde `std::string` UTF-8 sin
      pasar por `std::u8string`/`u8string()` en src/model (B1).
- [ ] Test de regresión: guardar → abrir en una carpeta y un archivo con «José/Canción ñ 日本.gpproj» pasa.
- [ ] Guardado: fsync del .tmp antes del rename (F_FULLFSYNC en macOS, FlushFileBuffers en
      Windows), fsync del directorio después en POSIX y comprobación de errores de close (I1).
- [ ] El .tmp tiene un nombre único en la misma carpeta (pid + contador o aleatorio); test que
      guarda dos veces en paralelo sin corrupción (I2).
- [ ] `GP_RT_ASSERT_NOT_REALTIME()` está en saveProject, loadProject, Document::notifyChanged y CommandHistory::execute (I3).
- [ ] El redo solo se vacía al hacer commit de una transacción no vacía; abortTransaction conserva el redo;
      clear() aborta una transacción abierta. Tests de regresión para los tres casos (I5).
- [ ] `ctest --preset headless` y `ctest --preset dev` verdes.
