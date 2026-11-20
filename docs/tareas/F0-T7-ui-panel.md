# F0-T7 — UI: panel principal, tema y localización
Dueño: ui-ux · Modelo: sonnet · Depende de: T8 · Paralelo con: T4, T5, T1, T2

## Objetivo
Existe `ui::MainComponent(app::AppServices&)` con controles de tono, tempo/compás, lista de
pistas, medidores y barra de estado, más `ui::tr` con el catálogo `es.json` y tokens de tema.

## Contexto mínimo (lee SOLO esto)
- include/app/AppServices.h, include/ui/Localisation.h
- include/commands/ProjectCommands.h, include/commands/CommandHistory.h
- include/model/Document.h, include/model/ProjectState.h, include/engine/AudioEngine.h
- docs/specs/ux/F0-ventana-principal.md y assets/i18n/es.json (de T8)

## Puedes editar
- src/ui/** (Localisation.cpp, Theme.h, MainComponent.h/.cpp, TrackListComponent.h)
- tests/ui/** (máximo 6 archivos en total contando el test)

## No hagas
- Cambiar include/, CMake, src/app/ ni el motor. Textos literales: todo por `tr("clave")`.
- Llamar al motor salvo `deviceStatus()` y `takeMetrics()`.
- Ejecutar comandos desde `documentChanged`.

## Criterios de aceptación (comprobables)
- [ ] `loadLanguage("es")` lee el JSON embebido (`#include <BinaryData.h>`, namespace
      `GhostAssets`, recurso `es_json`); clave ausente →
      devuelve la clave; `tr("x", {"a"})` sustituye `{0}`. Test en tests/ui.
- [ ] Tono: interruptor (`makeSetTestToneEnabled`), slider de frecuencia 20–20 000 Hz
      logarítmico y de ganancia −60–0 dB: el primer cambio de un arrastre con `never`, los
      siguientes con `withPrevious` (un arrastre = un undo).
- [ ] Tempo (20–999) y compás (numerador 1–32, denominador 1/2/4/8/16/32) con comandos; valores
      inválidos no se envían.
- [ ] Pistas: añadir, renombrar (doble clic) y eliminar con comandos; la lista se actualiza
      desde `documentChanged`.
- [ ] Timer de 30 Hz: medidores L/R con caída, CPU en %, estado del dispositivo, contador MIDI y,
      solo en debug, «Violaciones RT: N» (rojo si N > 0).
- [ ] Tema oscuro con tokens (`Theme.h`: colores, tipografía y espaciado); nada de colores
      sueltos en componentes.
- [ ] Sin dispositivo: el panel muestra el aviso de la spec y no falla.

## Prueba manual (para la guía del usuario)
1. Arrastra el slider de frecuencia de 440 a 880 → el tono sube sin clics; ⌘Z vuelve a 440.
