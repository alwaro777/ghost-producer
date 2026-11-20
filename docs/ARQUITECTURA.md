# Arquitectura de GhostProducer

> El CÓMO. El QUÉ está en `docs/PRODUCTO.md`. Las decisiones caras están en `docs/adr/`.
> Estado: F0 (Cimientos). Dueño: arquitecto. Máximo 2 páginas.

## Capas y flujo de datos

```
                 ┌──────────────────────── hilo de mensajes ────────────────────────┐
  usuario ──► ┌──────────┐  Command   ┌────────────────┐  muta  ┌──────────────────┐
  IA (F10+) ─►│ src/ui   │──────────► │ CommandHistory │──────► │ model::Document  │
              │ (vistas) │ undo/redo  │ src/commands   │        │ ProjectState     │
              └────▲─────┘            └────────────────┘        └───┬──────────┬───┘
                   │ lee state() + listeners                       │          │ guardar/abrir
                   └───────────────────────────────────────────────┤          ▼
                                                                    │   ProjectFile (JSON
                   métricas (30 Hz, atómicos)        buildSnapshot  │   versionado + migr.)
                   ┌──────────────────────┐         ┌───────────────▼────┐
                   │ IAudioEngine         │◄────────│ EngineSnapshot     │ inmutable
                   │ src/engine/device    │ publish └────────────────────┘
                   └──────────┬───────────┘
 ═════════════════════════════│═══════════ frontera RT (sin locks, sin new, sin I/O) ═════
                   ┌──────────▼───────────┐  SnapshotExchange (puntero atómico + SPSC de retiro)
   driver ───────► │ IRenderCore::process │──► salida de audio      picos ──► atómicos
 (CoreAudio/WASAPI)│ src/engine/core      │   (mismo código en render offline y tests golden)
                   └──────────────────────┘
```

Reglas: la UI nunca toca el motor salvo `IAudioEngine` (métricas, estado del dispositivo y el
selector de dispositivos, ADR 0004). El motor nunca ve Commands, solo instantáneas compiladas.
El modelo y los comandos son C++ puro, sin JUCE.

## Módulos (targets CMake) y dueños

| Target | Carpeta | Depende de | Dueño |
|---|---|---|---|
| `gp_contracts` | `include/` | nlohmann_json | arquitecto |
| `gp_model` | `src/model/` | contracts | arquitecto |
| `gp_commands` | `src/commands/` | gp_model | arquitecto |
| `gp_rt` | `src/engine/rt/` | contracts | motor-audio |
| `gp_engine_core` | `src/engine/core/` | gp_rt, gp_model, gp_juce | motor-audio |
| `gp_engine_device` | `src/engine/device/` | gp_engine_core | motor-audio |
| `gp_ui` | `src/ui/` | gp_commands, gp_juce, gp_assets | ui-ux |
| `GhostProducerApp` | `src/app/` | todo lo anterior | ui-ux (composición) |
| `gp_juce` | (JUCE 8, una sola compilación) | — | arquitecto |
| `gp_tests_core` / `gp_tests_gui` | `tests/` | — | qa |

Cada módulo recoge sus `.cpp` por glob: añadir archivos no exige tocar CMake. `GP_HEADLESS=ON`
compila solo modelo, comandos y núcleo del motor (CI Linux).

## Contratos de F0 (`include/`)
- `core/Status.h` — errores sin excepciones en la API pública.
- `model/ProjectState.h`, `Document.h` — estado y documento (hilo de mensajes).
- `model/ProjectSerializer.h`, `Migrations.h`, `ProjectFile.h` — formato (ADR 0002).
- `commands/Command.h`, `CommandHistory.h`, `CommandRegistry.h`, `ProjectCommands.h` (ADR 0003).
- `rt/RtGuard.h`, `SpscQueue.h`, `SnapshotExchange.h` — primitivas RT (ADR 0004).
- `engine/EngineSnapshot.h`, `RenderCore.h`, `AudioEngine.h` — motor.
- `app/AppServices.h` — lo que la app entrega a la UI. `ui/Localisation.h` — textos por clave.

## Hilos (resumen de ADR 0004)
Mensajes: UI, documento, comandos, E/S de proyecto. Audio: solo `process()`. MIDI: contador
atómico en F0. Fondo: reservado (plugins, disco, IA). Detector RT activo en `dev`/`asan`.

## Textos e idioma
Código e identificadores en inglés. Textos visibles por clave con `ui::tr("clave")`; catálogo
`assets/i18n/es.json` (producto-ux), embebido como BinaryData. Los Commands y los errores
exponen claves o códigos, nunca texto.

## Puntos de extensión previstos
- F1: `EngineSnapshot` → grafo compilado (nodos, buffers preasignados); transporte y clips en
  `ProjectState` (con subida de `schemaVersion`). Evaluar `tracktion_graph` (ADR 0001).
- F3: interfaz de plugin común VST3/AU/CLAP, escáner fuera de proceso.
- F6: `ITimeStretcher` (Signalsmith / Rubber Band).
- Notificación de cambios por ámbito; guardado en hilo de fondo; autoguardado.
