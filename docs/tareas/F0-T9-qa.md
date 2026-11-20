# F0-T9 — QA: integración, matriz RT, CI y verificación de F0
Dueño: qa · Modelo: sonnet · Depende de: T1–T7 (integración: T1 + T2) · Paralelo con: —

## Objetivo
Los tests de integración del cierre de F0 existen y pasan en las tres plataformas de CI, y cada
criterio de T1–T7 está verificado.

## Contexto mínimo (lee SOLO esto)
- docs/tareas/F0-PLAN.md y los informes docs/tareas/F0-T1…T7*.informe.md
- include/ (solo los headers que usen tus tests)
- .github/workflows/ci.yml, tests/CMakeLists.txt, CMakePresets.json
- .claude/skills/daw-orquestador/references/checklist-tiempo-real.md

## Puedes editar
- tests/integration/**, tests/CMakeLists.txt
- .github/workflows/ci.yml (pasa a ser de qa)

## No hagas
- Arreglar código de producción: reporta el fallo a la tarjeta dueña.
- Cambiar CMakeLists.txt raíz, presets o include/ (propón el cambio al arquitecto).

## Criterios de aceptación (comprobables)
- [ ] Integración (headless): Document + CommandHistory + ProjectFile: ejecutar 6 comandos
      (incluida una transacción y un gesto fusionado) → guardar → cargar en un Document nuevo →
      estado `==`; deshacer en el original vuelve al estado inicial `==` al por defecto.
- [ ] Prueba genérica parametrizada para todo `builtinCommands()`: apply/revert/apply y
      JSON ida y vuelta.
- [ ] Render core con `buildSnapshot` desde un `ProjectState`: matriz de bloques {32, 64, 128,
      512, 2048} × {44.1, 48, 96} kHz con 0 violaciones RT.
- [ ] CI: macOS `dev` + `asan` + paso `--smoke-test` de la app; Linux `headless`,
      `headless+ASan/UBSan`, `tsan`; Windows `dev`. Todo verde en el PR de F0.
- [ ] Lista de verificación de criterios T1–T7 en el informe (✓/✗ y evidencia).

## Prueba manual (para la guía del usuario)
(Valida en macOS los pasos de la guía de T10 y anota el resultado en el informe.)
