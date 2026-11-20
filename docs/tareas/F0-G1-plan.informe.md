Estado: HECHO
Archivos: docs/adr/0001–0004, docs/ARQUITECTURA.md, THIRD_PARTY.md, LICENSE.md, CLAUDE.md (§ Comandos), include/** (18 headers), CMakeLists.txt, CMakePresets.json, cmake/{CPM,GhostHelpers}.cmake, tests/CMakeLists.txt, .github/workflows/ci.yml, .clang-format, .gitignore, docs/tareas/F0-PLAN.md + F0-T1…T10
Criterios: ADRs 4/4 ✓ · contratos compilan aislados (GCC 13, Clang 18, -Werror) ✓ · preset headless configura/compila/enlaza un test Catch2 de prueba (fuera del repo) ✓ · app GUI no verificable en Linux (la valida T6 en macOS)
Bloqueos: —
Decisiones locales: Tracktion Engine no se adopta (motor propio; tracktion_graph a revisar en F1/F5). JUCE 9.0.6 fijado (decisión del usuario; antes 8.0.17). Headless usa JUCE_MODULES_ONLY (juceaide exige X11 en Linux). nlohmann/json en modelo y Commands (sin JUCE).
Deuda/TODO: verificar en macOS que gp_juce estático enlaza la app (riesgo 1 del plan). Las 4 preguntas ya están decididas (F0-PLAN.md).
---
Los archivos de GitHub de tarballs (archive/*.tar.gz) devuelven 403 en este proxy; por eso las dependencias se obtienen por git + tag (commit anotado).
