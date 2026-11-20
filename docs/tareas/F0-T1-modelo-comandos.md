# F0-T1 — Documento, CommandHistory y comandos de F0
Dueño: arquitecto · Modelo: sonnet · Depende de: — · Paralelo con: T2, T3, T8

## Objetivo
Existen `model::Document`, `commands::CommandHistory` (undo/redo, transacciones, fusión de
gestos, estado sucio), los 8 comandos de `ProjectCommands.h` y `builtinCommands()`.

## Contexto mínimo (lee SOLO esto)
- include/model/ProjectState.h, include/model/Document.h
- include/commands/Command.h, CommandHistory.h, CommandRegistry.h, ProjectCommands.h
- include/core/Status.h
- docs/adr/0003-comandos-undo.md

## Puedes editar
- src/model/Document.cpp
- src/commands/** (CommandHistory.cpp, ProjectCommands.cpp, CommandRegistry.cpp)
- tests/commands/**

## No hagas
- Cambiar include/ ni CMake (los .cpp se recogen solos).
- Implementar o llamar a `model::isValid` ni a nada de serialización de proyecto (es de T2);
  en los tests compara estados con `operator==`.
- Dependencias de JUCE en estos módulos.

## Criterios de aceptación (comprobables)
- [ ] Para cada comando: apply → revert deja el estado `==` al inicial; apply → revert → apply
      deja el estado `==` al del primer apply (incluido el TrackId de `track.add`).
- [ ] Valores fuera de `model::limits` (tempo 19.9, denominador 3, nombre vacío o de 65 bytes,
      TrackId inexistente, índice > size) → `invalidArgument` y el estado no cambia.
- [ ] `toJson()` → `builtinCommands().create()` → comando equivalente (mismo efecto); tipo
      desconocido → `unknownCommand`; JSON sin "params" → `parseError`.
- [ ] `execute` vacía redo; `undo`/`redo` devuelven false sin nada que hacer.
- [ ] 50 `makeSetTestToneFrequency` con `MergePolicy::withPrevious` = 1 paso de undo que vuelve
      al valor anterior al gesto; con `never` = 50 pasos.
- [ ] Transacción con 3 comandos = 1 paso; abort revierte los 3; commit vacío no apila.
- [ ] `isDirty`: false tras `markSaved`; true tras execute; false otra vez tras undo hasta el
      punto guardado; true si se deshace más allá; una fusión sobre la entrada guardada ensucia.
- [ ] `Document::revision()` sube 1 por paso; listeners notificados 1 vez por paso (también
      transacción).
- [ ] `ctest --preset headless` verde, sin warnings (`-Werror`).

## Prueba manual (para la guía del usuario)
(Se prueba a través de la app en T6: Cmd+Z / Cmd+Shift+Z.)
