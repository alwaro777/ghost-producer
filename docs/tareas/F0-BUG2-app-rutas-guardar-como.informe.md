# Informe F0-BUG2 — Rutas Unicode, «Guardar como» y aviso de migración (app)
Estado: HECHO (Linux; verificación Windows pendiente en CI)

## Cambios
- `src/app/PathUtils.h` (helper único): `toPath`, `toJuceString`, `toFile`, `toUtf8`, siempre vía `std::u8string`/`u8string()`; `projectFileFor` y `migrationBackupFor`. En `src/app` ya no queda ningún `.string()`/`toStdString()` usado como ruta.
- «Guardar como» (B2): `X/Prueba` o `X/Prueba.gpproj` -> `X/Prueba/Prueba.gpproj` (se respeta si ya estás dentro de `Prueba/`). Si `X/Prueba` existe (y no es el proyecto actual) se pregunta Reemplazar/Cancelar antes de escribir (sustituye al `warnAboutOverwriting` de JUCE, que miraba otro nombre).
- Aviso de migración (I4): si `loaded.migrated`, aviso no modal en `MainComponent::showNotice` (franja sobre la barra de estado, botón «Entendido») con la ruta `backups/<stem>.v<N>.gpproj` (regla del ADR 0002, igual que `loadProject`; si BUG1 cambia el nombre de la copia, hay que alinear `migrationBackupFor`).
- `assets/i18n/es.json`: claves nuevas `dialog.overwrite.{title,message,replace,cancel}` y `notice.migrated`.
- Tests: `tests/ui/PathTests.cpp` (ida y vuelta con «José/Canción ñ 日本», estructura de `projectFileFor`, guardado real bajo `X/Prueba` con carpeta Unicode y comprobación de que no cuelgan carpetas de X).

## Verificación (build propio `build/bug2-dev`)
- Compila sin errores ni avisos; `ctest --test-dir build/bug2-dev`: todo verde (1 skip: carpeta de solo lectura como root); `xvfb-run ... --smoke-test` rc=0.
- No verificado: ejecución en Windows/macOS (el cierre de B1 en el modelo es de BUG1).

## Prueba manual
1. Guardar como «Prueba» en Documentos -> existe `Documentos/Prueba/Prueba.gpproj` y sus subcarpetas solo dentro de `Prueba/`.
2. Repetir con el mismo nombre -> pregunta antes de reemplazar.
3. Abrir un proyecto de esquema antiguo -> aviso con la ruta de la copia; «Entendido» lo cierra.
