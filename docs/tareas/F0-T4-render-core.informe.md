# F0-T4 — Informe
Estado: HECHO
- Añadidos: src/engine/core/EngineSnapshot.cpp (buildSnapshot), src/engine/core/RenderCore.cpp (createRenderCore), tests/engine/core/RenderCoreTests.cpp.
- Diseño: seno con fase double continua; rampas lineales de 10 ms por muestra (frecuencia y ganancia; on/off = ganancia objetivo 0). Snapshot tomado al inicio de cada bloque. Picos en atómicos (máx. por CAS), sin locks ni reservas en process().
- Tests: 7 casos (conversión dB, silencio exacto, 440 Hz a 44.1/48/96 kHz con bloques 32..2048 idénticos bit a bit, sin clics, rampa 5–20 ms, determinismo, 0 violaciones RT, picos/mono). `ctest --preset headless`: 43/43 verdes (1 skip preexistente).
- Medidas: no se midió carga DSP con benchmark dedicado (coste = 1 sin() por muestra, trivial); latencia añadida: 0 muestras.
- Nota: las violaciones RT solo se detectan si el preset compila con GP_RT_CHECKS=ON; ver CMakePresets.json para headless.
