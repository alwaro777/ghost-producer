# F0-T4 — Núcleo de render: tono de prueba, picos y compilación de snapshot
Dueño: motor-audio · Modelo: sonnet · Depende de: T3 · Paralelo con: T1, T2, T7

## Objetivo
Existen `engine::buildSnapshot` y `createRenderCore()`: un seno sin clics controlado por
instantáneas, con medidor de picos, idéntico en tiempo real y en render offline.

## Contexto mínimo (lee SOLO esto)
- include/engine/EngineSnapshot.h, include/engine/RenderCore.h
- include/rt/RtGuard.h, include/rt/SnapshotExchange.h
- include/model/ProjectState.h
- docs/adr/0004-hilos-y-tiempo-real.md
- .claude/skills/daw-orquestador/references/checklist-tiempo-real.md

## Puedes editar
- src/engine/core/**
- tests/engine/core/**

## No hagas
- Cambiar include/. Abrir dispositivos (eso es T5). Usar JUCE en la API pública.
- Reservar memoria, bloquear o registrar en `process()`.

## Criterios de aceptación (comprobables)
- [ ] `buildSnapshot`: dB → lineal (−18 dB ≈ 0.12589), copia frecuencia, enabled y revisión.
- [ ] Antes de publicar o con el tono desactivado: salida 0.0f exacta.
- [ ] Render offline 1 s a 48 kHz, 440 Hz, 0 dB: pico ∈ [0.99, 1.0]; frecuencia medida por
      cruces por cero ∈ 440 ± 1 Hz; mismo resultado con bloques de 32, 64, 128, 512 y 2048 y a
      44.1, 48 y 96 kHz.
- [ ] Sin clics: al cambiar 440→880 Hz, 0→−60 dB y on/off, la diferencia máxima entre
      muestras consecutivas no supera la de un seno de 880 Hz a 0 dB (+5 %); la rampa dura
      5–20 ms.
- [ ] Determinismo: dos renders con la misma secuencia de snapshots y bloques → bytes iguales.
- [ ] `process()` dentro de `ScopedRealtimeSection` con publicaciones intercaladas →
      `rt::violationCount() == 0`.
- [ ] `takePeaks()` devuelve el pico y lo pone a 0; mono → right == left.
- [ ] `ctest --preset headless` verde.

## Prueba manual (para la guía del usuario)
(Se oye en T6: activar el tono y mover la frecuencia sin clics.)
