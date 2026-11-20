# Informe F0-T7 — UI: panel principal, tema y localización
Estado: PARCIAL (código completo; sin compilar/ejecutar GUI: el contenedor es headless)

## Hecho
- `src/ui/Localisation.cpp`: `loadLanguage` (BinaryData `GhostAssets::getNamedResource("<code>_json")`, nlohmann), `tr` (clave ausente -> clave, aviso una vez en debug), `tr(key, args)` con `{N}`.
- `src/ui/Theme.h`: tokens `colour`, `space`, `font`, `motion`.
- `src/ui/MainComponent.h/.cpp`: tiempo (tempo 20–999, compás 1–32 / 1,2,4,8,16,32), tono (Sonar, frecuencia log 20–20000, volumen −60–0), pistas, medidores L/R con caída, barra de estado (timer 30 Hz; textos ~4/s), RT solo en `JUCE_DEBUG`, banner sin dispositivo.
- `src/ui/TrackListComponent.h`: añadir, renombrar (doble clic o botón), eliminar. Renombrar solo cambia texto en sitio; el cambio estructural se reconstruye con `callAsync` (la fila no se destruye dentro de su propio callback).
- Arrastre = 1 undo: el primer cambio con `never`, los siguientes con `withPrevious` (`Gesture`, vía `onDragStart/End`).
- Valores inválidos (tempo/compás por texto): no se envían, se muestra `error.invalidArgument` y el control vuelve al valor del modelo. Fallo de un comando: mismo tratamiento con su `error.*`.
- `documentChanged` solo lee y actualiza con `dontSendNotification`; no ejecuta comandos.
- Test: `tests/ui/LocalisationTests.cpp` (1 archivo).

## Respuesta a T8
Confirmado: en `menu.edit.undoWith`/`redoWith`, `{0}` recibe `tr(cmd.*)`. Lo implementan `MainComponent::undoMenuText(history)` / `redoMenuText(history)` (vacío si no hay historial; la app usa entonces `menu.edit.undo`/`redo` deshabilitados). Test incluido ("Deshacer: cambiar el tempo"). src/app debe llamar a esos helpers.

## Verificación
- Comprobación de sintaxis `g++ -fsyntax-only -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast` con cabeceras JUCE de la caché CPM: Localisation.cpp, MainComponent.cpp y el test, sin avisos.
- NO ejecutado: `ctest` de `gp_tests_gui` ni la GUI (preset `headless` no compila gp_ui). Pendiente en máquina con GUI: `cmake --preset dev && cmake --build --preset dev && ctest --preset dev`.
- Sin captura en `docs/capturas/` (sin GUI).

## Desviaciones / pendientes
- «Reaparece al intentar activar Sonar»: con el toggle deshabilitado (spec) no hay clic; el banner solo se puede descartar hasta reabrir. Decisión para producto-ux.
- Foco visible: anillo dibujado en `paintOverChildren`; falta LookAndFeel propio (T posterior).
- Atajos asignables y niveles Descubrir/Crear/Pro: fuera de F0 (la spec dice nivel único).
- Nombre accesible de los medidores usa `status.peak` con "L"/"R" (sin clave propia en es.json); pedir a producto-ux `status.meter.left/right`.
- Cache por zoom no aplica (sin formas de onda en F0).
- Nombre de pista por defecto "Pista N" con N = nº de pistas + 1 (puede repetirse tras eliminar).

## Prueba manual
1. Arrastra el slider de frecuencia de 440 a 880 -> el tono sube sin clics; Cmd/Ctrl+Z vuelve a 440 en un solo paso.
2. Escribe "5" en tempo -> aparece el error y el valor vuelve al anterior. Escribe 140 -> Edición muestra «Deshacer: cambiar el tempo».
3. Añade 3 pistas, renombra una con doble clic, elimina otra; deshaz cada paso.
4. Sin dispositivo de audio: banner visible, «Sonar» deshabilitado, resto funcional.
