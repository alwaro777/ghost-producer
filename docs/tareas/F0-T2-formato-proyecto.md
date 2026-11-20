# F0-T2 — Serialización JSON, migraciones y archivo de proyecto
Dueño: arquitecto · Modelo: sonnet · Depende de: — · Paralelo con: T1, T3, T8

## Objetivo
Un `ProjectState` se guarda en una carpeta de proyecto `.gpproj` de forma atómica, se vuelve a
abrir idéntico y los archivos antiguos se migran con la cadena de `MigrationRegistry`.

## Contexto mínimo (lee SOLO esto)
- include/model/ProjectState.h, ProjectSerializer.h, Migrations.h, ProjectFile.h
- include/core/Status.h
- docs/adr/0002-formato-proyecto.md

## Puedes editar
- src/model/ProjectState.cpp (`isValid`)
- src/model/serialization/** (ProjectSerializer.cpp, Migrations.cpp, ProjectFile.cpp)
- tests/model/**

## No hagas
- Cambiar include/ ni CMake. No toques src/model/Document.cpp ni src/commands/ (T1).
- Usar JUCE (solo std::filesystem + nlohmann/json).
- Corregir valores fuera de rango en silencio.

## Criterios de aceptación (comprobables)
- [ ] Ida y vuelta: estado con 3 pistas, tempo 97.5, 7/8, tono 1234.5 Hz/-12 dB →
      `toJson` → `dump(2)` → parse → `fromJson` ⇒ estado `==`. Dos `toJson` del mismo estado
      dan bytes idénticos; las claves salen en el orden del ADR 0002.
- [ ] `format` erróneo o JSON roto → `parseError`; `schemaVersion` 2 con build en 1 →
      `unsupportedVersion`; rango inválido o TrackIds duplicados → `invalidArgument`.
- [ ] Clave desconocida en "project" → carga correcta con 1 aviso en `warnings`.
- [ ] Migraciones (registro inyectado): documento v0 con una migración 0→1 de prueba se carga,
      `migrated == true`, `originalSchemaVersion == 0`; falta un paso (0→2 sin 1→2) →
      `migrationFailed`; migración que devuelve error → `migrationFailed`.
- [ ] `saveProject` crea `samples/ renders/ cache/ backups/`, escribe UTF-8 con '\n' final y no
      deja `.tmp`. Si el destino no es escribible devuelve `ioError` y el archivo previo sigue
      intacto (test con directorio de solo lectura en POSIX; se omite en Windows).
- [ ] `loadProject` de un archivo migrado copia el original a `backups/<stem>.v<N>.gpproj`.
- [ ] `isValid` detecta: ids duplicados, id 0, `nextTrackId` ≤ id máximo, cada rango de limits.
- [ ] `ctest --preset headless` verde, sin warnings.

## Prueba manual (para la guía del usuario)
1. Guarda un proyecto desde la app (T6) y abre `MiCancion.gpproj` con un editor de texto →
   deberías ver un JSON legible con `"schemaVersion": 1`.
