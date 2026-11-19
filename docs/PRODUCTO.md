# Especificación de producto

> Fuente de verdad del QUÉ. El CÓMO está en docs/ARQUITECTURA.md y en los ADR. Fases: ver la skill daw-orquestador (references/roadmap.md).

## PRODUCTO
Vamos a construir GhostProducer, una estación de trabajo de
audio digital (DAW) de escritorio para Windows y macOS (Linux como objetivo secundario).
Su flujo de trabajo se inspira en Ableton Live 12 Suite:
  - una Vista de Sesión no lineal (rejilla de clips y escenas para improvisar y tocar en directo)
  - una Vista de Arreglo lineal (línea de tiempo para editar y producir)
  - cadenas de dispositivos con racks, macros, warping de audio, MIDI avanzado y modo directo.
Tiene tres diferenciales:
  1. MODO PRINCIPIANTE de primera clase: onboarding guiado, interfaz progresiva, explicaciones
     contextuales y protección contra errores típicos.
  2. IA INTEGRADA en el flujo: separación de stems, audio a MIDI, generación de ideas,
     asistente de mezcla, búsqueda semántica de sonidos y un copiloto conversacional que
     ejecuta acciones en el proyecto.
  3. MEJORAS DE CALIDAD DE VIDA frente a los DAW actuales: soporte CLAP, historial de versiones
     del proyecto, deshacer visual, colaboración por archivos y mejor gestión de samples.

La identidad visual, el nombre, los iconos y los nombres de los dispositivos son ORIGINALES.
No copies assets, textos, presets ni la apariencia exacta de ningún producto comercial.
En docs/REFERENCIA_LIVE12.md tienes un inventario funcional de referencia. Úsalo para conocer
el alcance, no para copiar la implementación.

## STACK TÉCNICO (decisiones cerradas salvo que justifiques un cambio por escrito)
- Lenguaje del motor: C++20. Build: CMake + Ninja. Gestión de dependencias: CPM.cmake o vcpkg.
- Framework: JUCE 8 (audio I/O, MIDI, hosting VST3/AU, UI nativa acelerada).
  Evalúa en la Fase 0 si usar Tracktion Engine (motor de DAW sobre JUCE) como base del modelo
  de edición y reproducción. Documenta la decisión (ADR) con sus implicaciones de licencia
  (GPL/AGPL frente a comercial).
- Hosting de plugins: VST3 (Steinberg SDK), AU (macOS) y CLAP (clap-juce-extensions o un host
  propio). Escaneo de plugins FUERA DE PROCESO, con lista negra automática de plugins que se
  cuelguen.
- UI: componentes JUCE con un sistema de diseño propio (tokens de color, tipografía y espaciado;
  tema oscuro por defecto y tema claro). Renderizado de formas de onda y piano roll con OpenGL o
  Metal a través de JUCE. Objetivo: 60 fps con 100 pistas visibles.
- IA local: ONNX Runtime (CPU, más CoreML/DirectML si están disponibles) para los modelos que
  corren en el equipo. IA en la nube: un cliente HTTP desacoplado detrás de una interfaz
  `IAIProvider`, para poder cambiar de proveedor (por defecto, la API de Anthropic Claude para
  el copiloto LLM). Todo lo de IA es opcional y funciona en segundo plano. NUNCA corre en el
  hilo de audio.
- Formato de proyecto: carpeta de proyecto con un archivo JSON versionado (esquema con número
  de versión y migraciones), carpeta de samples y carpeta de renders/caché. Legible y fácil de
  comparar con diff.
- Tests: Catch2 o GoogleTest para el motor, tests de render offline "golden" (comparación de
  audio por hash o por umbral de error) y tests de UI de humo. CI con GitHub Actions para
  Windows y macOS.

## PRINCIPIOS DE ARQUITECTURA (no negociables)
1. Hilo de audio en tiempo real sin bloqueos: sin reservas de memoria, sin locks, sin I/O de
   disco y sin logging en el callback. La comunicación con la UI va por colas lock-free (SPSC)
   y por snapshots inmutables del grafo.
2. Separación estricta entre MODELO (documento del proyecto, con undo/redo basado en comandos),
   MOTOR (grafo de procesamiento compilado a partir del modelo) y VISTA (UI que observa el
   modelo).
3. Todo cambio del usuario es un Comando serializable. Esto da undo/redo ilimitado, historial
   visual, macros y la API que usa el copiloto de IA. La IA solo actúa emitiendo Comandos,
   nunca tocando el estado directamente.
4. Grafo de audio con compensación automática de latencia de plugins (PDC), sidechain, envíos
   pre y post fader, grupos anidados y procesamiento multinúcleo por pistas independientes.
5. Motor de render offline determinista, el mismo que en tiempo real, para exportar, congelar,
   hacer bounce y testear.
6. Precisión: procesamiento interno en float de 32 bits (mezcla en 64 bits opcional),
   automatización con precisión de muestra, MIDI con timestamps de muestra y tempo y compás
   con mapa de tempo.
7. Robustez: autoguardado cada N minutos, recuperación tras un crash, aislamiento de plugins
   opcional (sandbox por proceso) y gestión de archivos perdidos con búsqueda automática.
8. Extensibilidad: SDK interno de dispositivos (instrumentos, efectos de audio, efectos MIDI y
   moduladores) con la misma API para los nativos y para los futuros scripts de usuario
   (Lua o JS) que hagan el papel que en Live cumple Max for Live.

## ALCANCE FUNCIONAL (paridad de flujo con Live 12 Suite; ver la referencia para el detalle)
## Núcleo
- Transporte: play, stop, grabación, loop, punch in/out, metrónomo con count-in, tap tempo,
  cuantización global de lanzamiento, cuantización de grabación, grabación retroactiva de MIDI
  (equivalente a "Capture MIDI"), seguimiento de tempo por entrada de audio, sincronización por
  MIDI Clock y Ableton Link (SDK de Link, licencia GPL o comercial: documentar).
- Pistas: audio, MIDI, grupo (anidables), retorno, master. Pistas de automatización, carriles
  de toma (take lanes) para comping y carril de vídeo.
- Vista de Sesión: rejilla de slots y escenas. Modos de lanzamiento (Trigger, Gate, Toggle,
  Repeat), legato, follow actions de clip y de escena, escenas con tempo y compás propios,
  botones de stop, grabación de la sesión en el arreglo y crossfader A/B.
- Vista de Arreglo: edición no destructiva (dividir, recortar, consolidar, duplicar, invertir,
  fades), marcadores y locators, cambios de tempo y de compás, edición vinculada entre pistas,
  comping por tomas y mezclador visible dentro del arreglo.
- Clips de audio: warping con marcadores y varios algoritmos (beats/transitorios, tonal,
  textura, re-pitch y alta calidad formant-preserving). Ganancia, transposición, envolventes de
  clip, slice to MIDI y extracción a MIDI (armonía, melodía y batería).
- Clips MIDI: piano roll con escala/tonalidad visible y "snap to scale", sistemas de afinación
  microtonal (importar .scl/.ascl), edición MPE (pitch bend, slide y presión por nota),
  probabilidad y variación de velocidad por nota, editor de batería por pads y edición de
  varios clips a la vez.
- Transformaciones y generadores MIDI (herramientas de clip): arpegiar, conectar, ornamentar
  (flams/grace notes), cuantizar, recombinar, span, strum, time-warp y velocity shaper.
  Generadores de ritmo, semilla aleatoria, forma, acordes apilados y euclídeo.
- Dispositivos: cadena por pista con drag & drop. Racks de instrumento, audio, MIDI y batería
  con cadenas paralelas, zonas de tecla y velocidad, 16 macros, variaciones de macro y
  aleatorización.
- Modulación: automatización y modulación relativa sobre cualquier parámetro, moduladores
  (LFO, envolvente, seguidor de envolvente, shaper) asignables a cualquier parámetro, incluidos
  los de plugins.
- Groove pool: extraer groove de audio o MIDI, aplicar swing y timing.
- Mezclador: volumen, panorama (estéreo y split), envíos, I/O routing, sidechain, delay de
  pista, solo/cue con preescucha por auriculares, medidores (peak, RMS y LUFS en el master),
  congelar/aplanar pista y bounce a nueva pista o in situ.
- Navegador: biblioteca con etiquetas, colecciones de colores, búsqueda por texto, filtros,
  preescucha sincronizada al tempo, "sonidos similares" e intercambio de sample por uno
  similar, carpetas de usuario y gestión de paquetes de contenido.
- Exportación: audio en WAV, AIFF, FLAC, MP3, OGG y opus. Sample rate de 44,1 a 192 kHz, 16,
  24 o 32 bits float, dithering (TPDF y con noise shaping), normalización a pico o LUFS.
  Exportación de stems (todas las pistas o una selección), render de rango o loop, exportar
  MIDI (.mid), exportar vídeo con audio y "recopilar y guardar" (proyecto autocontenido).
  Importar audio, MIDI, vídeo y proyectos propios.
- Control: MIDI Learn y Key Learn para cualquier parámetro, scripts de superficies de control
  (empezar con un controlador genérico de pads y knobs), soporte MIDI 2.0 en la hoja de ruta.

## Dispositivos nativos (nombres propios; ver la referencia para su equivalente)
- Instrumentos, por prioridad: Sampler simple (one-shot y slicing), Sampler multi-zona,
  Drum Sampler con pads, Synth sustractivo analógico virtual, Synth de wavetable, Synth FM,
  Instrumentos de modelado físico (fase posterior), Granular, Drum synths.
- Efectos de audio, por prioridad: Utilidad (gain/pan/mono/fase), EQ paramétrico de 8 bandas,
  EQ de canal sencillo, Compresor (con sidechain), Glue/bus compressor, Limitador, Gate,
  Saturador/distorsión multi-modo, Filtro con envolvente y LFO, Delay, Reverb algorítmica,
  Reverb de convolución, Chorus/Flanger/Phaser, Multibanda, Drum bus, Bitcrusher/Redux,
  Beat repeat/stutter, Vocoder, Pitch shifter/autotune, Afinador, Analizador de espectro,
  Looper, efectos espectrales (fase posterior).
- Efectos MIDI: Arpegiador, Acordes, Escala, Pitch, Velocidad, Aleatorio, Longitud de nota,
  Eco de nota, Monitor MIDI, LFO/Envelope/Shaper MIDI (modulan CC), Expresión/MPE Control,
  secuenciadores de pasos.

## MODO PRINCIPIANTE (diferencial nº 1)
- Interfaz progresiva por niveles: "Descubrir" (solo lo esencial), "Crear" (lo intermedio) y
  "Pro" (todo). Se cambia con un clic, sin perder datos.
- Onboarding interactivo: un tutorial en el propio proyecto en el que haces tu primer beat en
  5 minutos, con resaltado de controles y validación de cada paso.
- "¿Qué es esto?": al pasar el ratón con una tecla modificadora aparece una explicación breve,
  ejemplos sonoros y un enlace a la lección. Glosario integrado.
- Plantillas de inicio por género (house, hip hop, lo-fi, techno, pop, ambient) con tempo,
  escala, pistas y routing preparados.
- Guardarraíles: aviso de clipping con explicación, detector de "no suena nada" (diagnostica
  pista muteada, sin entrada, monitorización apagada, volumen a cero o plugin desactivado),
  aviso de CPU alta con un botón de "congelar pistas pesadas", y aviso de latencia de interfaz
  con sugerencia de buffer.
- Teoría musical asistida: selector de tonalidad y escala global, resaltado de notas fuera de
  escala, sugerencia de acordes que encajan y "modo seguro" en el que el teclado solo toca
  notas de la escala.
- Presets con macros comentadas ("brillo", "pegada", "espacio") en vez de parámetros técnicos.
- Historial visual de deshacer (línea de tiempo de cambios con miniaturas) y "puntos de
  guardado" con nombre (snapshots del proyecto).
- Metas y progreso opcional: lecciones cortas con logros, sin gamificación intrusiva.

## IA INTEGRADA (diferencial nº 2) — cada función es un módulo con interfaz, test y toggle
1. Separación de stems (local, ONNX, modelo tipo HTDemucs o equivalente con licencia
   compatible): voz, batería, bajo y otros, a pistas nuevas.
2. Audio a MIDI (modelo tipo Basic Pitch) para melodías, acordes y batería, con un umbral
   ajustable.
3. Búsqueda semántica y por similitud de sonidos: embeddings de audio indexados localmente,
   búsqueda por texto ("kick seco y corto de 808") y por ejemplo ("algo como este sample").
4. Generación de ideas MIDI: continuar un patrón, generar variaciones, bajo a partir de los
   acordes, patrón de batería según género y humanización. Siempre en un clip nuevo y
   deshacible.
5. Asistente de mezcla: análisis de enmascaramiento de frecuencias entre pistas, sugerencias de
   EQ/compresión/niveles explicadas en lenguaje llano, y "aplicar" o "previsualizar A/B".
6. Mastering asistido: cadena de master con objetivo de loudness (Spotify, YouTube, club),
   comparación con un track de referencia y reporte de LUFS, true peak y rango dinámico.
7. Copiloto conversacional (LLM): panel de chat que entiende el estado del proyecto
   (serializado y resumido) y actúa SOLO mediante Comandos del sistema de undo. Ejemplos:
   "crea una pista de bajo en Fa menor con un sidechain al kick", "¿por qué suena saturado el
   master?", "explícame qué hace este compresor". Pide confirmación antes de acciones
   destructivas y muestra un diff de los cambios.
8. Detección automática de tempo, tonalidad y compás al importar audio.
9. Limpieza de audio: reducción de ruido, de-reverb y de-click (fase posterior).
10. Tutor IA: responde dudas sobre la propia app con la documentación como contexto.
Privacidad: el procesamiento local es el predeterminado. Lo que va a la nube requiere opt-in
explícito, muestra qué datos se envían y nunca sube audio sin permiso.

