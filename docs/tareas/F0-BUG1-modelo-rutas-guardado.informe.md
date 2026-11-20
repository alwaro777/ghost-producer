# Informe F0-BUG1
Estado: HECHO
- B1: ProjectFile.cpp usa u8string() para mensajes/avisos y monta el nombre de la copia con operaciones de path. Test Unicode (Jose/Cancion n 日本) guardar -> abrir.
- I1: .tmp con fsync (F_FULLFSYNC macOS, FlushFileBuffers Windows), close comprobado, fsync de carpeta en POSIX. I2: .tmp unico `<archivo>.<pid>.<n>.tmp`; test con 4 hilos.
- I3: GP_RT_ASSERT_NOT_REALTIME en saveProject, loadProject, Document::notifyChanged, CommandHistory::execute.
- I5: redo solo se vacia al commit de transaccion no vacia; abort conserva redo; clear() revierte la transaccion abierta (tests en tests/commands/RedoTransactionTests.cpp).
- Nombre de la copia de migracion SIN cambios: `backups/<stem>.v<N>.gpproj` (compatible con migrationBackupFor de src/app).
- Sustituido el test del .tmp fijo (ya no aplica) por uno de destino-directorio; el test de solo lectura sigue omitido como root. ADR 0002 actualizado. include/ y CMake sin tocar.
- headless 52/52 y dev 52+13 verdes (build/bug1-h, build/bug1-dev).
