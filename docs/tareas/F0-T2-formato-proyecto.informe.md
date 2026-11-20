# Informe F0-T2 — formato de proyecto
Estado: HECHO (con una nota sobre un test ajeno)

- Implementado: `src/model/ProjectState.cpp` (isValid), `src/model/serialization/{ProjectSerializer,Migrations,ProjectFile}.cpp`, `tests/model/ProjectFormatTests.cpp`.
- `cmake --preset headless && cmake --build --preset headless`: sin warnings (-Werror). `ctest --preset headless`: todos los tests de T2 pasan.
- Test "read-only folder" se omite (SKIP) al ejecutarse como root; el caso de fallo de guardado se cubre igualmente con un directorio ocupando `<archivo>.tmp` (válido también como root).
- Fallo ajeno: `transactions` (Subprocess aborted) pertenece a src/commands (T1), no a esta tarjeta.
- Decisiones: `migrate()` exige que cada migración deje `schemaVersion` = v+1 (si no, migrationFailed); claves desconocidas avisan en cualquier objeto; ids negativos o >= 2^53 -> invalidArgument; si la copia a backups/ falla, loadProject devuelve ioError; saveProject rechaza estados inválidos (invalidArgument).
- No se tocó include/ ni CMake. Sin commit.
