# Informe F0-T1 — HECHO (con una salvedad de verificación)

Archivos: src/model/Document.cpp, src/commands/{CommandHistory,CommandRegistry,ProjectCommands}.cpp,
src/commands/ProjectCommandsInternal.h (interno), tests/commands/CommandsTest.cpp.

Verificación: 9 casos Catch2 / 283 aserciones en verde, compilados con ASan+UBSan y los mismos
flags de warning con -Werror (g++), enlazando solo mis fuentes. NO se pudo ejecutar
`ctest --preset headless` completo: gp_model falla por warnings -Werror=dangling-reference en
src/model/serialization/ProjectSerializer.cpp (T2, líneas 84-114: `const json& x = need(...)`
devolviendo referencia a temporal). Debe corregirlo T2.

Decisiones:
- Fábricas del registro: tipos incorrectos -> invalidArgument; rangos se validan en apply().
- track.add: toJson incluye "index" e "id" tras apply; si llega "id" desde JSON exige id >= nextTrackId.
- undo()/redo() con transacción abierta devuelven false sin assert (el test lo comprueba).
- Fusión solo si la cima se creó con el último execute (tras undo/redo no se fusiona) y es de 1 comando.
- Abort con comandos aplicados notifica una vez al Document (revision +1).
