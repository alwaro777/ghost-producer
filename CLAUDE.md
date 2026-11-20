# CLAUDE.md — DAW con IA — GhostProducer

DAW de escritorio (Windows/macOS) inspirada en el flujo Sesión + Arreglo de Live 12 Suite,
con Modo Principiante e IA integrada. Identidad, nombres de dispositivos y contenido ORIGINALES.

## Cómo se trabaja aquí
- Lo construye una red de subagentes coordinada por la sesión principal con la skill
  `daw-orquestador`. Para cualquier trabajo de fases, tareas o features, cárgala primero.
- Fuentes de verdad (léelas solo si tu tarea lo pide):
  `docs/PRODUCTO.md` (qué), `docs/ARQUITECTURA.md` + `docs/adr/` (cómo),
  `docs/ESTADO.md` (dónde estamos), `docs/REFERENCIA_LIVE12.md` (alcance de referencia),
  `docs/tareas/` (tarjetas e informes).

## Stack
C++20 · CMake + Ninja · JUCE 8 · VST3/AU/CLAP · ONNX Runtime · Catch2 · GitHub Actions.

## Mapa de propiedad (quién puede editar qué)
| Ruta | Dueño |
|---|---|
| `src/model/`, `src/commands/`, `include/` (interfaces) | arquitecto |
| `src/engine/`, `src/host/` (plugins), `src/io/` | motor-audio |
| `src/devices/` (instrumentos, efectos, MIDI fx) | dsp-dispositivos |
| `src/ui/`, `assets/ui/` | ui-ux |
| `src/ai/`, `models/` | ia-ml |
| `tests/` | qa (los implementadores añaden tests unitarios de su módulo) |
| `docs/ESTADO.md`, `CHANGELOG.md`, `docs/guias/` | documentalista |
| `docs/specs/ux/`, `assets/lecciones/`, textos de UI | producto-ux |

Una tarjeta puede ampliar esa propiedad de forma explícita. Fuera de ella, no edites: propón el
cambio en tu informe.

## Reglas que no se negocian
1. El hilo de audio no reserva memoria, no usa locks, no hace I/O y no escribe logs. La
   comunicación con él pasa por colas lock-free y snapshots inmutables.
2. Todo cambio del usuario o de la IA es un `Command` serializable con undo y redo.
3. Separación modelo / motor / vista. La UI nunca toca el motor directamente.
4. Contract-first: las interfaces de `include/` las fija el arquitecto antes de implementar.
5. No se copian assets, textos ni presets de productos comerciales. Las licencias se
   documentan en `THIRD_PARTY.md`.

## Comandos
- Configurar: `cmake --preset dev` (presets: `dev`, `asan`, `release`, `headless`, `tsan`)
- Compilar: `cmake --build --preset dev`
- Tests: `ctest --preset dev` (salida de fallos activada en el preset)
- Ejecutar (macOS): `open build/dev/GhostProducerApp_artefacts/Debug/GhostProducer.app`
- Ejecutar (Windows): `build\dev\GhostProducerApp_artefacts\Debug\GhostProducer.exe`
- En este contenedor Linux (sin GUI): `cmake --preset headless && cmake --build --preset headless && ctest --preset headless`
- Caché de dependencias: exporta `CPM_SOURCE_CACHE=$HOME/.cache/CPM` para no reclonar JUCE.
(El arquitecto mantiene esta sección al día.)

## Estilo
clang-format del repo · nombres en inglés en el código · docs y textos de UI en español
(i18n preparado) · commits pequeños: `area: qué y por qué`.
