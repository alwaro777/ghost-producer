# INVENTARIO DE REFERENCIA: ABLETON LIVE 12 SUITE

> Resumen funcional basado en la documentación pública de Live 12 (12.0 a 12.3). Sirve como mapa de alcance. Las funciones exactas cambian entre versiones menores, así que conviene revisar el manual oficial y las release notes al implementar cada fase.

### B.1 Ediciones
| Edición | Puntos clave |
|---|---|
| Intro | Pistas limitadas (16), pocos dispositivos, librería pequeña |
| Standard | Pistas ilimitadas, más efectos e instrumentos, warping completo, audio a MIDI |
| **Suite** | Todo: todos los instrumentos y efectos, Max for Live integrado, toda la librería de sonidos (+70 GB en packs), los modos de tuning, etc. |

### B.2 Vistas y flujo
- **Vista de Sesión**: rejilla de clips por pista y escenas por fila. Modos de lanzamiento Trigger, Gate, Toggle y Repeat. Legato, cuantización por clip, follow actions (clip y escena), escenas con tempo y compás propios, stop por pista y global, crossfader A/B, grabación de la sesión en el arreglo.
- **Vista de Arreglo**: línea de tiempo con pistas y carriles de automatización, **take lanes** para comping, locators, loop y punch, cambios de tempo y compás, edición vinculada de pistas (linked-track editing), **mezclador dentro del arreglo** (novedad en la 12) y vídeo.
- **Detail View**: Clip View (sample/notas y envolventes) y Device View (cadena de dispositivos). Se pueden ver apilados.
- **Navegador**: etiquetas (sistema de tags en la 12), filtros, colecciones, **búsqueda por similitud de sonidos** y **cambio por un sample similar** (Similar Sample Swapping), preescucha, Places/Packs. Integración con Splice (12.2+).
- **Help View / lecciones integradas**, Info View (descripción del control bajo el cursor).
- **Accesibilidad**: soporte de lector de pantalla y navegación por teclado (Live 12).
- **Keys & Scale**: escala global del proyecto, resaltado en el piano roll, "Fit to Scale" y dispositivos que siguen la escala. **Sistemas de afinación** microtonales (importación de .ascl/.scl).

### B.3 Audio
- Pistas de audio, grabación multi-pista, monitorización (In/Auto/Off), entrada externa.
- **Warping**: marcadores de warp, modos **Beats, Tones, Texture, Re-Pitch, Complex, Complex Pro**. Detección automática de tempo, envolventes de clip (ganancia, pan, transposición, envíos y parámetros).
- **Convert**: armonía, melodía o batería a nueva pista MIDI. Slice to New MIDI Track.
- Consolidate, Freeze y Flatten, **Bounce to New Track / Bounce in place** (12.2), invertir, fades, crop.
- **Separación de stems** integrada (12.3): voz, batería, bajo y otros.
- **Tempo Follower** (sigue el tempo de una entrada de audio).

### B.4 MIDI
- Piano roll con edición multi-clip, probabilidad de nota, rango de velocidad aleatorio, edición de **MPE** (pitch, slide y presión por nota), editor de batería.
- **Capture MIDI** (graba lo que tocaste aunque no estuvieras grabando), overdub, cuantización de grabación.
- **Transformaciones MIDI (12)**: Arpeggiate, Connect, Ornament, Quantize, Recombine, Span, Strum, Time Warp, Velocity Shaper.
- **Generadores MIDI (12)**: Rhythm, Seed, Shape, Stacks, Euclidean.
- Groove Pool (extraer y aplicar groove).
- MIDI Learn, Key Learn, superficies de control y scripts, MIDI Clock, **Ableton Link**.

### B.5 Instrumentos (Suite)
Wavetable, Operator (FM), Analog (sustractivo analógico virtual), **Drift** (sintetizador ligero), **Meld** (sintetizador bi-timbral macro-oscilador, 12), Sampler, Simpler, **Drum Sampler** (12.1), Impulse, Drum Rack, Collision (percusión física), Tension (cuerdas), Electric (pianos eléctricos), Granulator III (M4L, 12), Drum Synths (M4L), instrumentos de los packs Inspired by Nature (Emit, Tree Tone, Vector FM, etc.), External Instrument.

### B.6 Efectos de audio (Suite, lista representativa)
- **Dinámica**: Compressor, Glue Compressor, Multiband Dynamics, Limiter, Gate, Drum Buss, Color Limiter / Re-Enveloper / Pitch Hack (Creative Extensions).
- **EQ y filtros**: EQ Eight, EQ Three, Channel EQ, Auto Filter (rediseñado en la 12.2), Spectral Resonator.
- **Distorsión y color**: Saturator, Overdrive, Pedal, Amp, Cabinet, Dynamic Tube, Erosion, Redux, Vinyl Distortion, **Roar** (12, saturación multi-etapa con modulación).
- **Modulación**: Chorus-Ensemble, Phaser-Flanger, Auto Pan(-Tremolo), Shifter, Frequency Shifter.
- **Delay y reverb**: Delay, Echo, Filter Delay, Grain Delay, Hybrid Reverb (convolución y algorítmica), Reverb, Spectral Time, Align Delay, Gated Delay / Vector Delay (packs).
- **Creativos**: Beat Repeat, Corpus, Resonators, Vocoder, Looper, Spectral Blur.
- **Afinación**: **Auto Shift** (12.1, corrección de tono y efectos tipo autotune), Tuner.
- **Utilidades**: Utility, Spectrum, External Audio Effect, Surround Panner (M4L), LFO / Envelope Follower / Shaper (moduladores M4L).
- **Audio Effect Rack**.

### B.7 Efectos MIDI
Arpeggiator, Chord, Note Length, Pitch, Random, Scale, Velocity, **Note Echo**, MIDI Monitor, **Expression Control**, MPE Control, Envelope MIDI, LFO MIDI, Shaper MIDI, Microtuner, Melodic Steps, Rhythmic Steps, Bouncy Notes, Step Arp, MIDI Effect Rack.

### B.8 Racks
Racks de instrumento, audio, MIDI y Drum Rack: cadenas paralelas, zonas de tecla, velocidad y chain select, **16 macros**, **variaciones de macro**, aleatorización de macros, envíos y retornos internos en Drum Rack, choke groups.

### B.9 Max for Live (Suite)
Entorno de programación visual para crear dispositivos propios (instrumentos, efectos, MIDI y moduladores) y controlar Live por API. El equivalente propuesto en nuestra app es el SDK de scripts (Lua/JS) más los moduladores nativos.

### B.10 Mezcla y routing
Volumen, pan (estéreo y split), envíos pre y post, pistas de retorno, master, grupos anidados, I/O routing por pista y sub-canal, sidechain en dinámica, track delay, solo exclusivo, cue y preescucha, medidores, compensación de latencia de plugins, CPU meter por pista.

### B.11 Plugins
VST2, VST3 y AU (macOS). Escaneo y gestión de carpetas, configuración de plugins multi-salida y parámetros configurables para automatización y macros. *Oportunidad de mejora para nuestra app: CLAP nativo, escaneo aislado y sandbox por plugin.*

### B.12 Exportación e importación
- **Export Audio/Video**: rango o loop, master o pistas individuales (**stems**), WAV, AIFF, FLAC, MP3. Sample rate, bit depth (16/24/32), dither, normalización, render como loop, convertir a mono, exportación de vídeo.
- **Export MIDI**: clip a .mid.
- **Collect All and Save**, exportar e importar packs, exportar a Ableton Cloud / Note (app móvil) para proyectos iniciados en el móvil.

### B.13 Modo directo / performance
Vista de Sesión, cuantización global, follow actions, crossfader, escenas con tempo, mapeo a controladores, integración profunda con hardware dedicado (Push 3, incluido el modo standalone), Link para sincronizar varios dispositivos y apps, MIDI Clock out, Looper.

### B.14 Librería y contenido
Librería core más packs de Suite: drum kits, instrumentos multisample (orquestales, pianos, cuerdas), loops, presets de todos los dispositivos, plantillas y lecciones. *Para nuestra app: crear contenido propio o con licencia CC0/royalty-free, y nunca reutilizar el de Ableton.*

