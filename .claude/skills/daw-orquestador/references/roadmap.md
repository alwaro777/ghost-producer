# Roadmap por fases

Cada fase termina ejecutable, con una guía de prueba manual, y espera el OK del usuario.
"Dueños" lista quién trabaja en la fase; las tarjetas concretas las genera el arquitecto en G1.

## F0 — Cimientos
- Entregables: ADR del stack (incluida la evaluación de Tracktion Engine y la estrategia de
  licencias), repo con CMake presets, CI Windows/macOS con ASan, ventana con audio I/O y
  selección de dispositivo de audio y MIDI, medidor de CPU, sistema `Command` con undo/redo
  probado, formato de proyecto JSON versionado (guardar, abrir, migración), detector RT en
  debug, `docs/ARQUITECTURA.md` y `docs/ESTADO.md`.
- Dueños: arquitecto (lidera) · motor-audio · ui-ux · qa · revisor · documentalista
- Cierre: se abre la app, suena un tono de prueba, guardar → abrir conserva el estado y
  deshacer funciona.

## F1 — Reproducción mínima en el Arreglo
- Transporte, metrónomo, pistas de audio, importar WAV/AIFF/MP3/FLAC, mover, recortar y
  dividir clips, volumen, pan, mute, solo y exportar WAV.
- Dueños: motor-audio · ui-ux · qa
- Cierre: test golden del render; el usuario arma un loop de 4 pistas y lo exporta.

## F2 — MIDI
- Pistas MIDI, piano roll, grabación con count-in, cuantización, sampler simple, synth
  sustractivo y MIDI Learn.
- Dueños: motor-audio · dsp-dispositivos · ui-ux · qa

## F3 — Plugins
- Escáner aislado, VST3, AU y CLAP, ventanas de plugin, PDC, automatización de parámetros de
  plugin y congelar pista.
- Dueños: motor-audio · ui-ux · qa · revisor (atención a la estabilidad)

## F4 — Vista de Sesión
- Clips y escenas, modos de lanzamiento, cuantización global, follow actions, grabación de la
  sesión en el arreglo y crossfader.
- Dueños: arquitecto (modelo de sesión) · motor-audio · ui-ux · qa

## F5 — Mezcla avanzada y exportación
- Envíos y retornos, grupos anidados, sidechain, el primer lote de efectos nativos (utilidad,
  EQ, compresor, limitador, reverb, delay, saturación, filtro), medidores LUFS, stems y todos
  los formatos de exportación.
- Dueños: motor-audio · dsp-dispositivos · ui-ux · qa

## F6 — Warping y edición avanzada
- Time-stretch con varios modos, marcadores de warp, detección de tempo, comping por tomas,
  consolidar, bounce y slice to MIDI.
- Dueños: motor-audio · dsp-dispositivos · ui-ux · qa

## F7 — Racks y modulación
- Racks (instrumento, audio, MIDI y batería), 16 macros con variaciones, moduladores, groove
  pool, transformaciones y generadores MIDI, escalas globales y afinaciones microtonales.
- Dueños: arquitecto · dsp-dispositivos · ui-ux · qa

## F8 — Navegador y librería
- Etiquetas, colecciones, preescucha sincronizada, búsqueda, packs propios y gestión de
  archivos perdidos.
- Dueños: ui-ux · motor-audio · producto-ux (organización del contenido)

## F9 — Modo Principiante completo
- Niveles Descubrir/Crear/Pro, onboarding del "primer beat en 5 minutos", panel "¿Qué es
  esto?", plantillas por género, guardarraíles ("no suena nada", clipping, CPU) e historial
  visual de deshacer.
- Dueños: producto-ux (lidera las specs) · ui-ux · qa (test con un guion de usuario novato)
- Nota: producto-ux hace specs ligeras desde la F1 para que la UI nazca preparada para los
  niveles.

## F10 — IA local
- Stems, audio a MIDI, similitud y búsqueda por texto, detección de tempo y tonalidad. Todo
  en segundo plano, cancelable y con barra de progreso.
- Dueños: ia-ml · ui-ux · qa · revisor (licencias de los modelos)

## F11 — IA generativa y copiloto
- Generación y variación MIDI, asistente de mezcla, mastering asistido y copiloto LLM que
  actúa solo vía `Command` (muestra un diff y pide confirmación en lo destructivo). Opt-in
  para lo que vaya a la nube.
- Dueños: ia-ml · arquitecto (API de comandos para la IA) · ui-ux · producto-ux · qa

## F12 — Directo, accesibilidad y release
- Superficies de control, Link, MIDI Clock, rendimiento, accesibilidad (lector de pantalla y
  teclado), instaladores firmados, auto-update, recuperación tras crash y prueba de estrés
  final.
- Dueños: todos · revisor (auditoría final)
