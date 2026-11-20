# F0-T5 — Motor con dispositivos: audio, MIDI y CPU
Dueño: motor-audio · Modelo: sonnet · Depende de: T4 · Paralelo con: T7, T1, T2

## Objetivo
`createAudioEngine()` abre CoreAudio/WASAPI con `juce::AudioDeviceManager`, procesa con el
`IRenderCore`, cuenta MIDI de las entradas activas y expone estado y métricas.

## Contexto mínimo (lee SOLO esto)
- include/engine/AudioEngine.h, include/engine/RenderCore.h, include/engine/EngineSnapshot.h
- include/rt/RtGuard.h, include/core/Status.h
- docs/adr/0004-hilos-y-tiempo-real.md
- .claude/skills/daw-orquestador/references/checklist-tiempo-real.md

## Puedes editar
- src/engine/device/**
- tests/engine/device/**

## No hagas
- Cambiar include/ ni src/engine/core/ (si el núcleo falla, repórtalo a T4).
- Lanzar excepciones o abortar si no hay dispositivo.

## Criterios de aceptación (comprobables)
- [ ] Callback: `ScopedRealtimeSection` + `juce::ScopedNoDenormals`; llama a
      `IRenderCore::process`; entradas ignoradas; canales de salida extra a cero.
- [ ] `audioDeviceAboutToStart` llama a `prepare` con sample rate/bloque reales y republica la
      última instantánea; `audioDeviceStopped` llama a `release`.
- [ ] `cpuLoad` = tiempo del callback / duración del bloque, suavizado (p. ej.
      `juce::AudioProcessLoadMeasurer`), sin reservas en el callback.
- [ ] `xrunCount` de `AudioIODevice::getXRunCount()` si es ≥ 0.
- [ ] MIDI: callback en todas las entradas activas → contador atómico (`midiEventCount`).
- [ ] `collectGarbage` del render core en un `juce::Timer` a ≥ 10 Hz en el hilo de mensajes.
- [ ] `saveDeviceState` / `initialise(estado)` restauran dispositivo, sample rate, buffer y
      entradas MIDI. `initialise("")` → dispositivo por defecto, 2 salidas, 0 entradas.
- [ ] Sin dispositivo (o con XML corrupto): devuelve `deviceError`, `deviceStatus().open ==
      false`, y `setProject`/`takeMetrics`/`shutdown` siguen funcionando. Test automático con
      `initialise` sobre un tipo de dispositivo inexistente.
- [ ] `rtViolationCount` refleja `rt::violationCount()`.
- [ ] Compila en macOS con `--preset dev` y `--preset asan`; tests de esta carpeta verdes.

## Prueba manual (para la guía del usuario)
1. En Ajustes de audio, elige la salida integrada a 48 kHz y 256 muestras → el estado muestra
   «CoreAudio · 48 000 Hz · 256».
