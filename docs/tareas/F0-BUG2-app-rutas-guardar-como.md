# F0-BUG2 — Rutas Unicode, carpeta de proyecto en «Guardar como» y aviso de migración (app)
Dueño: ui-ux · Modelo: sonnet · Depende de: — · Paralelo con: F0-BUG1, F0-BUG3
Prioridad: P0 (B1 en la app) + P1 (B2) + importante I4

## Objetivo
La app pasa rutas Unicode al modelo sin corromperlas, «Guardar como» crea la carpeta de proyecto
del ADR 0002 y, al abrir un proyecto migrado, avisa al usuario y le dice dónde está la copia.

## Contexto mínimo (lee SOLO esto)
- docs/tareas/F0-REVISION.informe.md § B1, § B2 e IMPORTANTES 4
- docs/adr/0002-formato-proyecto.md (estructura de carpeta del proyecto)
- docs/specs/ux/F0-ventana-principal.md (estilo de avisos no modales)
- src/app/AppController.cpp

## Puedes editar
- src/app/**, src/ui/**, tests/ui/**, tests/app/** (créalo si hace falta)
- assets/i18n/es.json: SOLO para añadir las claves nuevas del aviso de migración (ampliación explícita)

## No hagas
- Tocar src/model/ ni src/commands/ (eso es F0-BUG1)

## Criterios de aceptación
- [ ] Conversión juce::File/juce::String ↔ std::filesystem::path siempre vía UTF-8 explícito
      (`std::u8string`); ninguna `.string()`/`toStdString()` como ruta en src/app (B1). Un helper
      único y test de ida y vuelta con «José/Canción ñ 日本».
- [ ] «Guardar como» con nombre «Prueba» en una carpeta X crea `X/Prueba/Prueba.gpproj` y las
      subcarpetas dentro de `X/Prueba/`; si `X/Prueba` ya existe y no es un proyecto, se pregunta
      antes de sobrescribir (B2). Test de regresión de la estructura.
- [ ] Si `loaded.migrated`, aviso no modal en español con la ruta de la copia de seguridad (I4).
- [ ] `cmake --build --preset dev`, `ctest --preset dev` verdes y `xvfb-run ... --smoke-test` rc=0.
