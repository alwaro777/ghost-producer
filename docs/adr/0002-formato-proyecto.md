# ADR 0002 — Formato de proyecto: carpeta + JSON versionado con migraciones

Estado: aceptado (F0, G1) · Fecha: 2026-11-20 · Contratos: `include/model/ProjectSerializer.h`,
`Migrations.h`, `ProjectFile.h`

## Contexto
PRODUCTO.md pide una carpeta de proyecto con un JSON versionado, legible y fácil de comparar
con diff, con migraciones. Cambiar el formato después obliga a migrar todos los proyectos de
los usuarios: es la decisión más cara de revertir de F0.

## Decisión
- **Carpeta** `MiCancion/` con `MiCancion.gpproj` (JSON UTF-8) y subcarpetas `samples/`,
  `renders/`, `cache/`, `backups/` (nombres fijos en inglés, no dependen del idioma de la UI).
  El nombre del proyecto es el nombre del archivo; no se guarda dentro.
- **Objeto raíz**: `{"format": "ghostproducer.project", "schemaVersion": N, "createdWith":
  "<versión app>", "project": {…}}`. Claves camelCase iguales a los campos de `ProjectState`,
  orden fijo, indentación de 2 espacios y `\n` final: misma entrada ⇒ mismos bytes.
- **Versión del esquema**: entero monotónico (`kCurrentSchemaVersion`, F0 = 1). Se sube en todo
  cambio de esquema.
- **Migraciones**: funciones puras `JSON vN → JSON vN+1`, encadenadas, aplicadas sobre el JSON
  **antes** de parsear al modelo. El parser solo conoce la versión actual. Un archivo con
  versión mayor que la de la build se rechaza (`unsupportedVersion`), nunca se abre a medias.
  Al migrar se copia el original a `backups/<stem>.v<N>.gpproj`.
- **Validación estricta**: tipos y rangos (`model::limits`, `isValid`) se comprueban al cargar;
  nada se corrige en silencio. Claves desconocidas: se ignoran con aviso (compatibilidad hacia
  delante suave dentro de la misma versión).
- **Guardado atómico y duradero**: se escribe un `.tmp` de nombre único en la misma carpeta
  (`<archivo>.<pid>.<contador>.tmp`, para que dos instancias o hilos no se pisen), se hace
  `fsync` del archivo (`F_FULLFSYNC` en macOS, `FlushFileBuffers` en Windows), se comprueba el
  `close`, se renombra sobre el destino y, en POSIX, se hace `fsync` de la carpeta. Un fallo
  nunca deja el archivo anterior corrupto ni un `.tmp` huérfano.
- **Rutas Unicode**: el modelo nunca usa `path::string()` ni construye `path` desde `std::string`
  (en Windows usan la página ANSI). Los textos salen con `u8string()` y los nombres derivados se
  montan con operaciones de `path`.
- Los IDs son enteros < 2^53 (exactos en cualquier lector JSON).
- Los tests de migración inyectan su propio `MigrationRegistry` y una versión objetivo; no se
  publican formatos ficticios.

## Alternativas
- Archivo binario o comprimido (rápido, compacto): no se puede comparar con diff. Descartado.
- XML/ValueTree de JUCE: acopla el formato a JUCE; diffs más ruidosos.
- Varios JSON por pista: mejores diffs en proyectos grandes, más complejidad de atomicidad.
  Punto de extensión: si el tamaño lo exige, se migrará a varios archivos con una migración.
- SQLite: no legible ni fácil de comparar con diff.

## Consecuencias
- Cada fase que añada estado al modelo sube la versión y, si cambia un significado, añade una
  migración con su test.
- Los proyectos grandes (miles de notas) pueden tardar en serializarse: en F2 se medirá y, si
  hace falta, el guardado pasará a un hilo de fondo sobre una copia del estado.
- Autoguardado y recuperación (F posterior) reutilizan `saveProject` sobre otra ruta.

## Licencias
nlohmann/json (MIT). Sin otras dependencias.
