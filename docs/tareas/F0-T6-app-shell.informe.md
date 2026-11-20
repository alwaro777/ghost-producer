# Informe F0-T6 — App: composición, menús, archivos y smoke test
Estado: HECHO (Linux/xvfb). Pendiente en macOS: preset asan y apertura del .app.

## Hecho
- `src/app/`: Main.cpp (app JUCE + `--smoke-test`), AppController.h/.cpp (posee Document, CommandHistory, IAudioEngine; implementa IAppActions, MenuBarModel y ApplicationCommandTarget; escucha el Document -> `engine.setProject`), MainWindow.h, AudioSettingsDialog.h/.cpp.
- Menús con atajos (Nuevo/Abrir/Guardar/Guardar como/Ajustes de audio/Salir; Deshacer/Rehacer, Ctrl+Y fuera de macOS). Deshacer/Rehacer usan `MainComponent::undoMenuText/redoMenuText` («Deshacer: {tr(cmd.*)}»), deshabilitados sin historial. Barra nativa en macOS (`setMacMainMenu`).
- Título con `window.title` / `window.title.modified` / `window.untitled`.
- Nuevo/Abrir/Salir con cambios: diálogo asíncrono Guardar / No guardar / Cancelar; si guardar falla o se cancela, no continúa.
- Abrir: loadProject -> reset -> clear; error traducido por código (+ «Ver detalles» para parse/migración), proyecto intacto. Guardar -> saveProject + markSaved.
- Preferencias (PropertiesFile): estado del dispositivo y último proyecto (se guarda; no se reabre automáticamente en F0).
- Ajustes de audio: AudioDeviceSelectorComponent (salidas + entradas MIDI).
- Smoke test: t=1 s 3 comandos + undo con comprobación de estado; t=3 s sale 0, o 1 si falla algo o `rtViolationCount > 0`.

## Verificación (este contenedor)
- `cmake --preset dev && cmake --build --preset dev`: OK, sin errores.
- `ctest --preset dev`: 51/51 OK (1 skip: carpeta de solo lectura como root).
- `xvfb-run -a build/dev/GhostProducerApp_artefacts/Debug/GhostProducer --smoke-test`: rc=0 (~5 s). Salida: un `jassertfalse` de JUCE por no haber `/dev/snd/seq` (ALSA MIDI), solo entorno.
- Sin captura en docs/capturas/ (no se hizo; xvfb disponible, pendiente).

## Arreglos en T7 (src/ui, tests/ui)
- gp_ui expone `src/ui` solo como include PRIVADO: `#include "ui/MainComponent.h"` etc. no compilaba. Ahora src/ui usa includes locales; src/app incluye `"../ui/MainComponent.h"` y el test `"../../src/ui/MainComponent.h"`. Propuesta al arquitecto: exponer `src/` como include PUBLIC de gp_ui para volver a `ui/...`.
- Entorno: instalé `libxi-dev` (faltaba `X11/extensions/XInput2.h`; JUCE lo exige).

## Pendientes heredados de T7 (sin resolver)
- «Sonar» deshabilitado sin dispositivo no puede reabrir el banner (spec).
- LookAndFeel propio para foco visible; claves de accesibilidad de medidores L/R en es.json; atajos asignables.
- Título usa `window.title.modified` («(sin guardar)») en vez de «•» (la clave es de T8/es.json).

## Prueba manual
1. Abre la app -> «Sin título — GhostProducer».
2. Activa el tono, cambia frecuencia y pulsa Ctrl/Cmd+Z -> vuelve el valor anterior en un solo paso; Edición muestra «Deshacer: …».
3. Guarda como «Prueba», cierra, reabre -> mismo tempo, tono y pistas; Deshacer deshabilitado.
4. Con cambios, Nuevo/Salir -> diálogo Guardar / No guardar / Cancelar.
