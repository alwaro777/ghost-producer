# F0 — Cimientos · Plan (gate G1)

Dueño: arquitecto · Fecha: 2026-11-20

## Objetivos
1. Repo que compila en macOS, Windows y Linux headless con presets `dev`, `asan`, `release`,
   `headless`, `tsan`, y CI que ejecuta los tests (macOS con ASan/UBSan, Linux con TSan).
2. Modelo de documento + sistema `Command` con undo/redo, transacciones y fusión de gestos.
3. Formato de proyecto JSON versionado: guardar (atómico), abrir y migrar.
4. Motor mínimo: tono de prueba sin clics, medidor de picos y CPU, selección de dispositivo
   de audio y MIDI, detector RT activo en debug con 0 violaciones.
5. Ventana con menús Archivo/Edición, panel principal en español y ajustes de audio.
6. Docs: ADR 0001–0004, `ARQUITECTURA.md`, `THIRD_PARTY.md`, `ESTADO.md`, guía de prueba.

**Cierre de F0**: la app abre, suena un tono de prueba, guardar → abrir conserva el estado y
deshacer funciona (ver prueba manual en F0-T10).

## Ya entregado en G1 (arquitecto)
- `docs/adr/0001-stack.md`, `0002-formato-proyecto.md`, `0003-comandos-undo.md`,
  `0004-hilos-y-tiempo-real.md`; `docs/ARQUITECTURA.md`; `THIRD_PARTY.md`; `LICENSE.md` (AGPLv3).
- Contratos `include/**` (18 headers; compilan aislados con GCC 13 y Clang 18, `-Werror`).
- `CMakeLists.txt`, `CMakePresets.json`, `cmake/` (CPM fijado + helpers), `tests/CMakeLists.txt`,
  `.github/workflows/ci.yml`, `.clang-format`, `.gitignore`. Verificado aquí: `headless`
  configura, compila y enlaza un test Catch2 contra `gp_juce` + nlohmann con `-Werror`.

## Tarjetas

| Id | Título | Dueño | Modelo | Depende de |
|---|---|---|---|---|
| F0-T1 | Documento, CommandHistory y comandos | arquitecto | sonnet | — |
| F0-T2 | Serialización, migraciones y archivo de proyecto | arquitecto | sonnet | — |
| F0-T3 | Primitivas RT y detector RT | motor-audio | **opus** | — |
| F0-T4 | Núcleo de render (tono, picos, snapshot) | motor-audio | sonnet | T3 |
| F0-T5 | Motor con dispositivos (audio/MIDI, CPU) | motor-audio | sonnet | T4 |
| F0-T6 | App: composición, menús, archivos, smoke test | ui-ux | sonnet | T1, T2, T5, T7 |
| F0-T7 | UI: panel principal, tema, localización | ui-ux | sonnet | T8 (contratos ya fijados) |
| F0-T8 | Spec UX de la ventana F0 + catálogo es.json | producto-ux | sonnet | — |
| F0-T9 | QA: integración, matriz RT, CI y verificación | qa | sonnet | T1–T7 |
| F0-T10 | ESTADO, CHANGELOG y guía de prueba F0 | documentalista | haiku | T9 + revisión |

## Grafo de dependencias

```
 T1 (modelo/comandos) ───────────────┐
 T2 (formato) ───────────────────────┤
 T3 (RT) ──► T4 (render) ──► T5 (dispositivos) ──┤
 T8 (spec UX) ──► T7 (panel UI) ─────────────────┴──► T6 (app) ──► T9 (QA) ──► revisor ──► T10
```

## Paralelismo (olas)
- **Ola 1** (en paralelo, sin conflictos de archivos): T1, T2, T3, T8.
- **Ola 2**: T4 (al cerrar T3) y T7 (al cerrar T8), en paralelo entre sí y con T1/T2 si siguen.
- **Ola 3**: T5 (al cerrar T4).
- **Ola 4**: T6 (necesita T1, T2, T5 y T7 para enlazar la app).
- **Ola 5**: T9 → revisión del revisor sobre todo el diff de F0 → T10.
  T9 puede empezar sus tests de integración en cuanto cierren T1 y T2.

La propiedad es disjunta: cada tarjeta tiene sus carpetas en «Puedes editar». Nadie edita CMake
raíz ni `include/` (salvo T3, por ampliación explícita, debajo de los marcadores).

## Riesgos y mitigación
| Riesgo | Impacto | Mitigación |
|---|---|---|
| JUCE en macOS con `gp_juce` estático (fuentes `.mm`, frameworks) | La app no enlaza | T6 lo valida primero; patrón documentado en ADR 0001; si falla, BLOQUEADO → arquitecto |
| El enlazador descarta el hook de `operator new` de `libgp_rt.a` | Detector RT ciego | En la misma TU que `ScopedRealtimeSection`; test que provoca una reserva y espera 1 violación |
| Runners de CI sin dispositivo de audio | Smoke test falla | `initialise` tolera no tener dispositivo; smoke test no exige audio |
| ASan + hook de `new` + JUCE en macOS arm64 | Falsos positivos o leaks no soportados | `detect_leaks=0` en macOS; leaks se comprueban en Linux |
| Clics al cambiar frecuencia/ganancia | Mala primera impresión | Rampas y fase continua como criterio de T4 con test numérico |
| Fusión de gestos mal hecha (un arrastre = 100 undos o undo erróneo) | Deshacer confuso | Criterios explícitos en T1 y prueba manual |
| JUCE 8 frente a 9 | Migración posterior | Pregunta 1 al usuario |
| Tiempos de clonado de JUCE en CI | CI lenta | Caché de `CPM_SOURCE_CACHE` |

## Fuera de alcance de F0
Transporte con reproducción, clips, pistas de audio, plugins, autoguardado, recuperación,
temas claros, Modo Principiante, IA.

## Preguntas al usuario (decisiones caras que siguen abiertas)
1. **JUCE 8 frente a JUCE 9.** Fijamos JUCE 8.0.17-security como pediste. Ya existe JUCE 9.0.6
   (misma licencia AGPLv3), con una implementación nueva de CoreAudio y correcciones de CoreAudio
   y MIDI que interesan en macOS. Cambiar ahora cuesta una línea; después de F3 (plugins) costará
   bastante más. ¿Seguimos con 8 o pasamos a 9 antes de F1?
2. **Contribuciones y licencia futura.** ¿Quieres conservar la opción de una doble licencia
   comercial más adelante? Eso exige un CLA para contribuidores externos desde el principio
   (y licencias comerciales de JUCE, Link y Rubber Band). Con AGPLv3 + DCO, la licencia queda
   AGPLv3 para siempre. Consecuencia en ambos casos: no hay Mac App Store, sino descarga directa
   firmada y notarizada.
3. **macOS mínimo y arquitecturas.** Hemos puesto macOS 12 como mínimo y compilación nativa
   (arm64 en tu Mac). ¿Las releases deben ser universales (arm64 + x86_64) y qué macOS mínimo
   quieres soportar?
4. **ASIO en Windows.** WASAPI basta en F0. ¿Quieres soporte ASIO en una fase posterior
   (añade el SDK de Steinberg; hay que verificar su licencia GPLv3 al integrarlo) o solo WASAPI?
