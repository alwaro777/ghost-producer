# Estado de GhostProducer

**Fase actual:** F0 — Cimientos (versión 0.1.0). Gates G1, G4 (QA) y G5 (revisión, vuelta 2) aprobados.
**Pendiente:** gate G6 (documentación) y prueba manual del usuario en macOS con
`docs/guias/prueba-F0.md`.

## Qué funciona (verificado en Linux: headless, xvfb y ASan/UBSan)
- Documento y comandos con deshacer/rehacer, transacciones y fusión de gestos (un arrastre = un paso).
- Proyecto `.gpproj` (JSON versionado): guardado atómico, abrir, migración con copia de seguridad,
  rutas con acentos y caracteres no ASCII.
- «Guardar como» crea la carpeta del proyecto (`X/Nombre/Nombre.gpproj`) con sus subcarpetas dentro.
- Motor: tono de prueba sin clics, medidor de picos, carga de CPU, selección de salida y entrada MIDI.
  Detector de tiempo real activo en debug: 0 violaciones en la matriz de render.
- App: menús Archivo/Edición, paneles Tiempo/Tono/Pistas, medidores, barra de estado, Ajustes de audio,
  aviso no modal de migración.
- Tests en verde en dev, headless, headless+ASan/UBSan y tsan. Algunos se omiten por diseño
  (detector RT en tsan) o como root (carpeta de solo lectura).

## Sin verificar
- macOS, Windows y dispositivo de audio real: no hay en el contenedor de desarrollo. Pendientes:
  preset `asan` y apertura del `.app` en macOS; job Windows de CI para rutas Unicode.
- Carga DSP: no medida (F0 no tiene benchmark).
- Callback con salida real; desconectar la salida durante la reproducción (no probado).
- Prueba manual de F0 por el usuario: pendiente.

## Deuda conocida (sin tarjeta todavía)
- ~~N1~~ resuelto al cierre: `gp_model` y `gp_commands` declaran `DEPS gp_rt`; `gp_ui` expone `src/` como include PUBLIC.
- **Menores de la revisión, vuelta 1:**
  1. Renombrar rechazado no restaura la etiqueta de la pista (ui-ux).
  2. `120.5.3` se acepta como 120.5 en el tempo (ui-ux).
  3. Callback de «Guardar» destruye su propio FileChooser (ui-ux).
  4. Comentario de `RtGuard.h` sobre los presets (arquitecto).
  5. Primer `thread_local` en el hilo de audio de macOS reserva memoria una vez (motor-audio).
  6. Con ganancia 0 la rampa de frecuencia no avanza: glissando de 10 ms al activar (motor-audio).
  7. Pins de CMake por etiqueta, no por SHA (arquitecto).
  8. El dispositivo elegido se guarda solo al cerrar la app (ui-ux).
- **Menores de la revisión, vuelta 2:** N2 duplicación de la regla de la copia de seguridad;
  N3 extensión y carpeta sensibles a mayúsculas; N4 rename en Windows sin write-through;
  N5 nombre «.gpproj» vacío (ui-ux / arquitecto).
- **Pendientes de UI (T6/T7):** «Sonar» deshabilitado sin dispositivo no vuelve a mostrar el banner;
  foco visible sin LookAndFeel propio; claves de accesibilidad de los medidores L/R; el título usa
  «(sin guardar)» y la spec dice «•»; atajos asignables (fuera de F0).
- **CI:** job Linux con GUI propuesto, no aplicado. T3: tsan no ejecuta los tests del detector (aceptado).

## Preguntas de F0-PLAN
Las cuatro decisiones del usuario están cerradas (JUCE 9.0.6, AGPLv3 + DCO, macOS 12+, WASAPI).
Queda diferida la integración de ASIO: hay que revisar la licencia del SDK (ADR 0001, § Pendientes).

## Próximos pasos
1. El usuario prueba en macOS con `docs/guias/prueba-F0.md` y envía el feedback con la plantilla.
2. Según el feedback: tarjetas F0-BUG{n} para lo P0/P1.
3. Tras aprobar F0, plan de la siguiente fase (lo hace el arquitecto).
