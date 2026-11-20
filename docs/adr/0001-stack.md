# ADR 0001 — Stack, motor propio frente a Tracktion Engine y licencias

Estado: aceptado (F0, G1) · Fecha: 2026-11-20

## Contexto
GhostProducer es open source bajo **AGPLv3**. Plataforma principal de pruebas: macOS (Apple
Silicon, CoreAudio con la interfaz integrada); Windows en segundo lugar; Linux solo para CI.
Prioridad: estabilidad del audio > funcionalidad > estética. PRODUCTO.md fija C++20, CMake +
Ninja, JUCE 8 y pide evaluar Tracktion Engine. Las reglas no negociables exigen: modelo propio
con `Command` serializable, formato JSON versionado y un hilo de audio sin locks ni reservas.

## Decisión

### Núcleo
- **C++20, CMake ≥ 3.25 + Ninja, presets** `dev`, `asan`, `release`, `headless`, `tsan`.
- **JUCE 8.0.17-security** (commit `e548e97a`), fijado. JUCE 9.0.6 existe; ver «Preguntas al
  usuario» en F0-PLAN.md.
- Dependencias con **CPM.cmake 0.43.2** (incluido en `cmake/CPM.cmake`) por git + tag fijado; el
  commit esperado está anotado junto a cada tag. CI cachea `CPM_SOURCE_CACHE`.
- JUCE se compila **una sola vez** como librería estática `gp_juce` (los módulos JUCE son
  librerías INTERFACE con fuentes; enlazarlos desde varios targets duplica símbolos).
- Cada módulo nuestro es una librería que recoge sus `.cpp` por `GLOB CONFIGURE_DEPENDS`: las
  tarjetas añaden archivos sin editar CMake (propiedad disjunta). Modelo y comandos no dependen
  de JUCE (tests rápidos en Linux headless).
- JSON: **nlohmann/json 3.12** (`ordered_json`: orden de claves estable para diffs).
  Tests: **Catch2 3.16.1**.

### Motor: propio sobre `juce_audio_devices` / `juce_audio_processors`. Tracktion Engine NO se adopta como base.
Razones:
1. **El modelo sería el suyo.** El `Edit` de Tracktion es un `ValueTree` con su propio
   `UndoManager` y su formato XML. Nuestro `Command` serializable, la IA que solo actúa con
   Commands y el JSON versionado quedarían como capa de traducción sobre un segundo modelo que
   el propio motor también muta (grabación, plugins, automatización): doble fuente de verdad.
2. **Estabilidad del audio.** Queremos controlar y auditar cada línea del hilo de audio
   (detector RT, snapshots, colas). Heredar ~200 kLOC con su propio modelo de hilos hace esa
   auditoría inviable para el equipo.
3. **Acoplamiento de versiones.** Tracktion sigue de cerca la rama de desarrollo de JUCE; fijar
   JUCE y Tracktion a la vez nos ata a su calendario.
4. F0–F2 necesitan poco motor (tono, transporte, clips de audio y MIDI): el coste inicial de
   un motor propio es bajo; el alto (PDC, multinúcleo, warping) llega en F5–F7.

Punto de revisión: al planificar F1 y F5 se evaluará usar **solo el módulo `tracktion_graph`**
(grafo con PDC y multihilo, GPLv3) detrás de nuestra interfaz de grafo compilado
(`engine::EngineSnapshot` evoluciona a grafo). Leer Tracktion como referencia está permitido;
copiar código exige conservar su aviso de copyright y su licencia GPLv3 (compatible).

### Plugins (F3)
- **VST3**: hosting de JUCE. El SDK de VST3 que incluye JUCE 8.0.17 es **MIT** (Steinberg lo
  relicenció en la 3.8). No se soporta VST2 (SDK no disponible).
- **AU** (macOS): hosting de JUCE sobre frameworks del sistema.
- **CLAP**: JUCE 8 no aloja CLAP y `clap-juce-extensions` sirve para *crear* plugins CLAP, no
  para alojarlos. Se escribirá un host propio sobre los headers oficiales de CLAP (MIT) detrás
  de la misma interfaz de plugin que VST3/AU.
- Escaneo **fuera de proceso** con timeout y lista negra (proceso auxiliar de JUCE).

### Sincronía y time-stretch
- **Ableton Link** (F1/F2, GPLv2+): se usa bajo GPLv3, compatible. Depende de asio-standalone (BSL-1.0).
- **Time-stretch (F6)** detrás de una interfaz `ITimeStretcher`:
  **Signalsmith Stretch (MIT)** por defecto (header-only, apto para tiempo real, licencia
  permisiva) y **Rubber Band R3 (GPLv2+)** como modo «alta calidad / conserva formantes».
  La elección final del algoritmo por defecto se hace en F6 con pruebas de escucha; cambiarla
  es barato porque va detrás de la interfaz.
- **ONNX Runtime (MIT)** para IA local (F10+). Cada modelo tiene su propia licencia y se revisa
  al añadirlo.

### Estrategia de licencias
Todo el código propio es **AGPLv3**. Solo se admiten dependencias con licencia compatible:
permisivas (MIT, BSD, BSL-1.0, Apache-2.0, zlib) o copyleft GPLv3 / «GPLv2 o posterior» (GPLv3
§13 y AGPLv3 §13 permiten combinarlas). **Prohibido**: GPLv2-only, licencias no comerciales,
SDKs propietarios incompatibles (VST2). Cada dependencia se anota en `THIRD_PARTY.md`
antes de añadirla. Distribución: descarga directa firmada y notarizada (la Mac App Store no es
compatible con la familia GPL).

| Componente | Licencia | ¿Compatible con AGPLv3? |
|---|---|---|
| JUCE 8 | AGPLv3 / comercial | Sí (misma licencia) |
| VST3 SDK (incluido en JUCE 8.0.17) | MIT | Sí |
| Ableton Link | GPLv2+ / comercial | Sí (vía GPLv3) |
| Rubber Band | GPLv2+ / comercial | Sí (vía GPLv3) |
| Signalsmith Stretch | MIT | Sí |
| Tracktion Engine / tracktion_graph | GPLv3 / comercial | Sí (GPLv3 §13) — no adoptado |
| CLAP SDK | MIT | Sí |
| ONNX Runtime | MIT | Sí |
| ASIO SDK (Windows, fase posterior) | GPLv3 / propietaria (verificar al integrarlo) | Sí si se usa la GPLv3 |

## Alternativas
- **Adoptar Tracktion Engine completo**: más funcionalidad desde el principio (clips, warping,
  plugins, render), pero se pierde la propiedad del modelo y del hilo de audio. Descartado.
- **Modelo propio sincronizado con un `Edit` de Tracktion**: dos modelos, errores de
  sincronización y deshacer duplicado. Descartado.
- **vcpkg** en lugar de CPM: más pesado para JUCE (que no está en vcpkg con nuestras opciones).
- **juce::JSON / ValueTree** para el documento: acopla el modelo a JUCE y su orden de claves es
  menos controlable. Se prefiere nlohmann.

## Consecuencias
- Hay que escribir el grafo, el PDC y el render offline (F1–F5); el riesgo se controla con tests
  golden desde F1 y la revisión de `tracktion_graph`.
- El hilo de audio es completamente nuestro y auditable con el detector RT.
- Cualquier cambio de licencia futura (p. ej. doble licencia comercial) requeriría licencias
  comerciales de JUCE, Link y Rubber Band y un acuerdo con los contribuidores (ver preguntas).
