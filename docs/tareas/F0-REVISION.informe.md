Estado: HECHO · Vuelta 2: G5 PASA (B1, B2 e I1–I6 resueltos; 0 bloqueantes nuevos, 1 importante y 4 menores nuevos, ver «Vuelta 2»)
Alcance: git diff 959b4e7..HEAD -- ':!docs/tareas' (73 archivos, +9053)
Recuento: 2 BLOQUEANTES · 6 IMPORTANTES · 8 MENORES
Bloqueos: B1 rutas no ASCII en Windows (ui-ux + arquitecto) · B2 «Guardar como» no crea la carpeta de proyecto (ui-ux)
RT/concurrencia: sin hallazgos bloqueantes (callback, RenderCore, RtGuard, SpscQueue, SnapshotExchange correctos)
Reglas 2 y 3: se cumplen (UI → CommandHistory; motor solo vía AppServices; excepción del ADR 0004 respetada)
Licencias: THIRD_PARTY.md correcto y compatible con AGPLv3; solo hay una mejora menor (pins por SHA)
---

# F0 — Revisión de código (gate G5)

Revisor: revisor · Fecha: 2026-11-20 · Base: `959b4e7` (kit inicial) → `HEAD` (`bae97f5`)
Contrastado con: F0-PLAN.md, tarjetas F0-T1..T9, ADR 0001–0004 y checklist-tiempo-real.md.
No he repetido build ni tests (QA los validó en Linux, F0-T9-qa.informe.md).

## BLOQUEANTES

### B1 — Las rutas con caracteres no ASCII se corrompen o lanzan excepciones en Windows
- `src/app/AppController.cpp:30` construye `std::filesystem::path(juce::String::toStdString())`.
  `toStdString()` devuelve UTF-8, pero en MSVC el constructor `path(std::string)` interpreta los
  bytes en la página de códigos ANSI (CP_ACP), no en UTF-8.
- El camino inverso tiene el mismo problema: `path::string()` → `juce::String(std::string)`, que
  JUCE trata como UTF-8. Ocurre en `AppController.cpp:108` (`stem().string()`), `:171`, `:181` y `:233`.
- También en el modelo: `src/model/serialization/ProjectFile.cpp:25` (`path.string()` en el
  mensaje de error) y `:110` y `:125` (nombre de la copia de seguridad y aviso de migración).
  En MSVC, `path::string()` lanza `std::system_error` si un carácter no cabe en la página ANSI.
  Esa excepción sale de `saveProject`/`loadProject`, que tienen API sin excepciones, y llega a un
  callback de JUCE.
- Escenario: usuario de Windows «José», o proyecto «Canción»/«Música». Documentos queda como
  `C:\Users\JosÃ©\Documents`. Guardar crea una carpeta basura o falla con ioError, y abrir no
  encuentra el archivo. Si el nombre tiene caracteres fuera de cp1252 (p. ej. «Ñandú ♪», CJK o
  emoji), la app termina por una excepción no capturada. Esto rompe el criterio de cierre de F0
  (guardar → abrir) en una plataforma objetivo. La CI de Windows no lo detecta porque los tests
  usan rutas ASCII.
- Dueños: ui-ux (`src/app/**`, tarjeta T6) y arquitecto (`src/model/serialization/ProjectFile.cpp`).
- Fix: en la app, convertir `juce::File` ↔ `path` en Unicode
  (`fs::path(f.getFullPathName().toWideCharPointer())` en Windows, o
  `fs::path(std::u8string(...))`; de vuelta con `juce::String::fromUTF8(reinterpret_cast<const char*>(p.u8string().c_str()))`).
  En el modelo, usar `u8string()` en los mensajes y montar el nombre de la copia con operaciones
  de `path` (`auto b = stem(); b += ".v" + N; b += ext;`). Test: guardar y abrir en
  `tmp/"Canción ♪ Ñandú"/…` en los tres sistemas operativos (qa).

### B2 — «Guardar como» no crea la carpeta de proyecto del ADR 0002 y llena Documentos de subcarpetas
- `src/app/AppController.cpp:190` guarda en la ruta elegida tal cual (`file.withFileExtension("gpproj")`).
  `ProjectPaths::forDocument` usa como carpeta del proyecto el directorio padre
  (`ProjectFile.cpp:13-14`).
- Escenario: es la prueba manual de T6 («Guarda como "Prueba"»). Se crea
  `Documentos/Prueba.gpproj` junto con `Documentos/samples`, `renders`, `cache` y `backups`.
  Un segundo proyecto guardado en Documentos comparte esas carpetas, y desde F1/F2 (samples,
  renders) mezclaría datos de varios proyectos. Además contradice el ADR 0002
  (`MiCancion/MiCancion.gpproj`).
- Dueño: ui-ux (T6).
- Fix: si el nombre del directorio padre no coincide con el nombre base del archivo, guardar en
  `padre/<nombre>/<nombre>.gpproj`. Test de integración de qa sobre la ruta resultante.

## IMPORTANTES (sugerencias de alta prioridad)

1. **Guardado atómico sin fsync** (`ProjectFile.cpp:68-77`): `ofstream::flush()` solo pasa los
   datos al sistema operativo. No se hace `fsync`/`F_FULLFSYNC` (macOS) ni `FlushFileBuffers`
   (Windows) del .tmp, ni `fsync` del directorio tras `rename`, y no se comprueba el resultado de
   `close()`. Si se va la luz justo después de guardar, en APFS/NTFS puede quedar un .gpproj vacío
   o truncado, y el ADR dice «un fallo nunca deja el archivo anterior corrupto». Dueño: arquitecto.
   Fix: hacer fsync del archivo antes del rename y del directorio después; actualizar el ADR 0002.
   Es obligatorio antes del autoguardado.
2. **Nombre fijo `<archivo>.tmp`** (`ProjectFile.cpp:61`) con `moreThanOneInstanceAllowed() == true`
   (`Main.cpp:17`): si dos instancias guardan el mismo proyecto, escriben a la vez en el mismo .tmp
   y se renombra un archivo mezclado. Dueño: arquitecto. Fix: nombre único (pid + contador o
   aleatorio) en la misma carpeta.
3. **`GP_RT_ASSERT_NOT_REALTIME()` no se usa en ningún sitio de `src/`**: el ADR 0004 lo exige en
   el código propio que bloquea o hace I/O. Hoy el detector solo vigila reservas de memoria; un
   lock o una lectura de disco que llegue al callback en F1 pasaría sin detectarse. Dueños:
   arquitecto (`saveProject`, `loadProject`, `Document::notifyChanged`, `CommandHistory::execute`)
   y motor-audio (`AudioEngine::setProject`, `timerCallback` y `writerLock_`).
4. **Al abrir no se avisa de la migración** (`AppController.cpp:225-235`): se ignoran
   `loaded.warnings`, así que el usuario no sabe que su proyecto se migró ni dónde está la copia de
   seguridad. Dueño: ui-ux. Fix: mostrar un aviso no modal si `loaded.migrated`.
5. **Transacciones y redo** (`CommandHistory.cpp:53`): cada `execute` dentro de una transacción
   vacía la pila de redo, y `abortTransaction` no la restaura. Una acción de la IA abortada destruye
   en silencio el redo del usuario. Además, `clear()` (`:159-167`) descarta una transacción abierta
   sin revertirla. Dueño: arquitecto. Fix: vaciar el redo solo al hacer commit de una transacción
   no vacía, y hacer que `clear()` llame a abort si hay una transacción abierta (o un assert).
6. **Tests que faltan para B1 y B2**: no hay ningún test de rutas Unicode ni de la estructura de
   carpetas al «Guardar como». Dueño: qa.

## MENORES

1. `src/ui/TrackListComponent.h:34`: si se rechaza un renombrado (más de 64 bytes), el
   `refreshFromDocument` no restaura la etiqueta porque `tracks == desired_`, y se sigue viendo el
   nombre inválido aunque el modelo no cambió. Fix: el callback de error debe llamar a
   `setShownName` de la fila. Dueño: ui-ux.
2. `src/ui/MainComponent.cpp:300-302`: `"120.5.3"` pasa el filtro y se acepta como 120.5, que es una
   corrección silenciosa. Fix: exigir como mucho un punto decimal. Dueño: ui-ux.
3. `src/app/AppController.cpp:205-237`: dentro del callback del FileChooser de «Guardar»
   (Guardar → Abrir), `onSaved` sustituye `chooser_` y destruye el FileChooser cuyo callback se
   está ejecutando. Hoy funciona por cómo JUCE implementa `finished()`, pero es frágil. Fix: soltar
   `chooser_` con `callAsync` o usar un chooser por operación. Dueño: ui-ux.
4. `include/rt/RtGuard.h:5` dice «presets dev/asan/tsan», pero `tsan` tiene `GP_RT_CHECKS=OFF`
   y `headless` lo tiene activado (lo correcto según el ADR 0004). Hay que corregir el comentario.
   Dueño: arquitecto.
5. `src/engine/rt/RtGuard.cpp:107`: en macOS, el primer acceso a un `thread_local` en el hilo de
   audio lo reserva dyld con `malloc`, sin pasar por el hook. Es una sola vez por hilo y es
   aceptable, pero conviene documentarlo o tocar las variables TLS en `audioDeviceAboutToStart`.
   Dueño: motor-audio.
6. `src/engine/core/RenderCore.cpp`: con la ganancia a 0, la rampa de frecuencia no avanza
   (rama de silencio). Al activar el tono hay un glissando de 10 ms desde la frecuencia antigua.
   No produce clic, pero no está documentado. Dueño: motor-audio.
7. `CMakeLists.txt:38,45,53`: los pins usan `GIT_TAG <tag>` y el SHA va solo en un comentario.
   Las etiquetas son mutables y THIRD_PARTY.md dice que la versión está fijada por SHA. Fix:
   `GIT_TAG <sha>`. Dueño: arquitecto.
8. El estado del dispositivo solo se guarda en `AppController::shutdown`: si la app se cierra de
   golpe, se pierde lo elegido en Ajustes de audio. Fix: guardarlo al cerrar el diálogo. Dueño: ui-ux.

## Verificado sin hallazgos
- **Callback** (`AudioEngine.cpp`): `ScopedRealtimeSection` y `ScopedNoDenormals`, sin reservas de
  memoria, locks, I/O ni logs. `writerLock_` solo se usa en hilos no RT. El bloque se trocea a
  `maxBlock_` y se limpian los canales sobrantes. El MIDI solo hace un contador atómico.
- **RenderCore**: `noexcept`, rampas de 10 ms en ganancia y frecuencia con fase continua,
  sobrescribe la salida y genera 0.0f exacto en silencio. Los picos usan CAS atómico y el mismo
  núcleo sirve para tiempo real y render offline.
- **SpscQueue**: índices monótonos con acquire/release correctos, sin false sharing y sin
  reservas de memoria.
- **SnapshotExchange**: cada puntero tiene siempre un único dueño (`exchange` sobre `pending_`).
  El audio nunca destruye una instantánea y, si la cola de retiro está llena, conserva la actual.
- **RtGuard**: cubre todas las variantes reemplazables de new/delete, el TLS es `constinit`
  trivial y no hay recursión desde el handler.
- **Regla 2**: toda edición de la UI pasa por `history.execute`. `Document::reset` solo se usa en
  Nuevo/Abrir, como permite la tarjeta. Undo, redo y fusión funcionan: un gesto produce un solo
  paso de undo y la primera muestra del gesto usa `never`.
- **Regla 3**: la UI usa el motor solo a través de `takeMetrics`/`deviceStatus`, más la excepción
  documentada `deviceManagerForSettingsUI`.
- **Formato y migraciones**: validación estricta, migración sobre una copia, `unsupportedVersion`,
  copia de seguridad antes de migrar y comprobación de `schemaVersion` tras cada paso.
- **Licencias**: JUCE (AGPLv3), VST3 SDK (MIT), nlohmann (MIT), Catch2 (BSL-1.0) y CPM (MIT), todas
  compatibles y documentadas.

---

## Vuelta 2 (2026-11-20) — diff `9b5d358..HEAD` (F0-BUG1, BUG2 y BUG3)

### Estado de la vuelta 1
| Id | Estado | Comprobación |
|---|---|---|
| B1 rutas Unicode | RESUELTO | `src/app/PathUtils.h` convierte siempre por `u8string`/`toRawUTF8`. En `src/` ya no queda ningún `path::string()` ni ningún `path(std::string)`. `ProjectFile.cpp` usa `utf8()` y monta el nombre de la copia con operaciones de `path` (`:245-248`). Además, `saveProject`/`loadProject` capturan `std::exception` y ya no la propagan. Hay tests con «José/Canción ñ 日本» en el modelo y en la app. |
| B2 carpeta de proyecto | RESUELTO | `paths::projectFileFor` convierte `X/Prueba` en `X/Prueba/Prueba.gpproj`. Si la carpeta ya existe, la app pide confirmación con su propio diálogo, que sustituye a `warnAboutOverwriting`. Lo cubre un test de estructura. |
| I1 fsync | RESUELTO | `writeDurably` (`ProjectFile.cpp:75-141`) hace `F_FULLFSYNC` con `fsync` de respaldo, `FlushFileBuffers` en Windows y comprueba `close`. Tras el rename hace fsync de la carpeta en POSIX. El ADR 0002 está actualizado. |
| I2 .tmp único | RESUELTO | `<archivo>.<pid>.<n>.tmp` se crea con `O_EXCL`/`CREATE_NEW` y hay un test con 4 hilos. |
| I3 GP_RT_ASSERT_NOT_REALTIME | RESUELTO | Está en el modelo (save, load, notifyChanged), en `execute` y en el motor (setProject, timer, aboutToStart, stopped, publish, collectGarbage). Cada caso tiene su test. |
| I4 aviso de migración | RESUELTO | `MainComponent::showNotice` muestra un aviso no modal con la ruta de la copia. Los textos están en `es.json`. |
| I5 redo y transacciones | RESUELTO | El redo solo se vacía al hacer commit de una transacción no vacía (`CommandHistory.cpp:137`) y `clear()` revierte la transacción abierta y notifica el cambio. Hay tests. |
| I6 tests | RESUELTO | `tests/ui/PathTests.cpp` y los casos Unicode y concurrentes de `ProjectFormatTests.cpp`. |

Los menores de la vuelta 1 siguen abiertos (no estaban en el alcance de las tarjetas BUG) y no bloquean.

### Hallazgos nuevos
No hay bloqueantes nuevos. Las rutas de tiempo real no han cambiado: solo se han añadido asserts en funciones que no son RT.

**IMPORTANTE**
- N1 — `CMakeLists.txt:98-99`: `gp_model` y `gp_commands` llaman ahora a `gp::rt::isInRealtimeSection`/`reportViolation` (vía `GP_RT_ASSERT_NOT_REALTIME`), pero no declaran `DEPS gp_rt`. Hoy enlazan solo porque los consumidores (la app y las suites de tests) ponen `gp_rt` detrás en la línea de enlace. El primer ejecutable o herramienta que enlace `gp_model` sin `gp_rt` fallará con `GP_RT_CHECKS=ON`, que es el caso de dev, asan y headless. Dueño: arquitecto. Fix: `gp_add_module(gp_model src/model DEPS gp_rt)`.

**MENORES**
- N2 — `src/app/PathUtils.h:56-62`: `migrationBackupFor` duplica la regla de nombres de `loadProject` (`ProjectFile.cpp:245-248`). Si una de las dos cambia, el aviso mostrará una ruta falsa. Fix: añadir `backupFile` a `LoadResult`, un cambio de contrato que corresponde al arquitecto, y que la app lo use.
- N3 — `src/app/PathUtils.h:45,50`: las comparaciones de la extensión y del nombre de carpeta distinguen mayúsculas. «Prueba.GPPROJ» da `Prueba.GPPROJ/Prueba.GPPROJ.gpproj`, y elegir `x/prueba/Prueba.gpproj` en un sistema de archivos insensible a mayúsculas crea `x/prueba/Prueba/`. Fix: comparar sin distinguir mayúsculas. Dueño: ui-ux.
- N4 — `ProjectFile.cpp:196`: en Windows, `fs::rename` (MoveFileExW sin `MOVEFILE_WRITE_THROUGH`) no garantiza que el rename sea duradero. Los datos ya están a salvo, así que el riesgo es bajo. Fix: usar `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` o documentarlo en el ADR. Dueño: arquitecto.
- N5 — `src/app/PathUtils.h:44-46`: con un nombre «.gpproj» (solo la extensión), `extension()` sale vacía y la ruta resultante es `.gpproj/.gpproj.gpproj`, una carpeta oculta en POSIX. Fix: rechazar nombres vacíos o que empiecen por punto. Dueño: ui-ux.
