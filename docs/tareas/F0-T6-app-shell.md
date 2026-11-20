# F0-T6 — App: composición, menús, archivos y smoke test
Dueño: ui-ux · Modelo: sonnet · Depende de: T1, T2, T5, T7 · Paralelo con: —

## Objetivo
`GhostProducerApp` arranca, compone Document + CommandHistory + motor + UI, ofrece
Nuevo/Abrir/Guardar/Guardar como/Deshacer/Rehacer/Ajustes de audio y tiene `--smoke-test`.

## Contexto mínimo (lee SOLO esto)
- include/app/AppServices.h, include/ui/Localisation.h
- include/model/Document.h, include/model/ProjectFile.h
- include/commands/CommandHistory.h, include/engine/AudioEngine.h
- docs/specs/ux/F0-ventana-principal.md (de T8)
- src/ui/MainComponent.h (de T7: solo su constructor)
- JUCE 9.0.x `BREAKING_CHANGES.md` § cuadros de diálogo (`MessageBoxOptions::withButton` con código
  de resultado, `NativeMessageBox` renombrado), solo si un diálogo no compila

## Puedes editar
- src/app/** (Main.cpp, AppController.h/.cpp, MainWindow.h/.cpp, AudioSettingsDialog.cpp)

## No hagas
- Cambiar include/, CMake, src/ui/ o el motor. Textos literales en el código: usa `ui::tr`.
- Modificar el documento sin Commands (salvo `Document::reset` en Nuevo/Abrir).

## Criterios de aceptación (comprobables)
- [ ] `AppController` implementa `IAppActions`, posee Document, CommandHistory, IAudioEngine y
      construye `AppServices`; escucha el Document y llama a `engine.setProject(state, revision)`.
- [ ] Menús con atajos nativos: Archivo (Nuevo ⌘N, Abrir ⌘O, Guardar ⌘S, Guardar como ⇧⌘S,
      Ajustes de audio ⌘,) y Edición (Deshacer ⌘Z, Rehacer ⇧⌘Z / Ctrl+Y en Windows), con el
      texto «Deshacer: {acción}» a partir de `undoDescriptionKey()`. Barra de menú nativa en macOS.
- [ ] Título: «<nombre> — GhostProducer», con «•» si `isDirty()`; «Sin título» si es nuevo.
- [ ] Nuevo/Abrir/Cerrar con cambios sin guardar → diálogo asíncrono Guardar / No guardar /
      Cancelar (sin bucles modales: `JUCE_MODAL_LOOPS_PERMITTED=0`).
- [ ] Abrir: `loadProject` → `Document::reset` → `history.clear()`. Errores → mensaje traducido
      por `ErrorCode`, el proyecto actual no cambia. Guardar → `saveProject` + `markSaved`.
- [ ] Preferencias (`juce::PropertiesFile`): estado del dispositivo y último proyecto.
- [ ] Ajustes de audio: ventana con `juce::AudioDeviceSelectorComponent` (salidas y entradas
      MIDI) sobre `deviceManagerForSettingsUI()`.
- [ ] `--smoke-test`: arranca, abre la ventana, inicializa el motor (sin exigir dispositivo),
      ejecuta 3 comandos + undo, sale con 0 a los ~3 s; sale con 1 si `rtViolationCount > 0`.
- [ ] `cmake --build --preset dev` y `--preset asan` en macOS; la app abre desde
      `open build/dev/GhostProducerApp_artefacts/Debug/GhostProducer.app`.

## Prueba manual (para la guía del usuario)
1. Abre la app → ventana «Sin título — GhostProducer».
2. Activa el tono, cambia la frecuencia y pulsa ⌘Z → vuelve el valor anterior en un solo paso.
3. Guarda como «Prueba», cierra, vuelve a abrir → mismo tempo, tono y pistas; ⌘Z no hace nada.
