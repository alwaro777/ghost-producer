#pragma once
/// @file ProjectFile.h
/// Proyecto en disco (ADR 0002). Hilo: hilo de mensajes o un hilo de fondo; nunca audio.
/// Bloqueante (I/O). En F0 se llama desde el hilo de mensajes; los proyectos grandes
/// pasarán a un hilo de fondo en fases posteriores.
///
/// Estructura de carpeta (nombres fijos en inglés, independientes del idioma de la UI):
/// @code
/// MiCancion/
///   MiCancion.gpproj      ← JSON (ProjectSerializer), UTF-8, indentado a 2, '\n' final
///   samples/  renders/  cache/  backups/
/// @endcode

#include <filesystem>
#include <string_view>

#include "core/Status.h"
#include "model/ProjectSerializer.h"

namespace gp::model {

inline constexpr std::string_view kProjectFileExtension = ".gpproj";

/// Rutas derivadas de la ruta del archivo .gpproj (no toca el disco).
struct ProjectPaths {
    std::filesystem::path documentFile;  ///< …/MiCancion/MiCancion.gpproj
    std::filesystem::path folder;        ///< …/MiCancion
    std::filesystem::path samplesDir, rendersDir, cacheDir, backupsDir;

    [[nodiscard]] static ProjectPaths forDocument(const std::filesystem::path& documentFile);
};

/// Guarda de forma atómica: escribe `<archivo>.tmp`, hace flush y lo renombra sobre el
/// destino. Si falla, el archivo anterior queda intacto y no quedan .tmp. Crea la carpeta
/// y las subcarpetas si no existen. Devuelve ioError con detalle si algo falla.
[[nodiscard]] Status saveProject(const ProjectState& state,
                                 const std::filesystem::path& documentFile,
                                 std::string_view appVersion);

/// Lee y deserializa (fromJson con builtinMigrations()). Si hubo migración, copia antes el
/// archivo original a `backups/<stem>.v<N>.gpproj` (N = versión original) y lo indica en
/// warnings; el archivo original no se modifica hasta el siguiente guardado.
[[nodiscard]] LoadResult loadProject(const std::filesystem::path& documentFile);

}  // namespace gp::model
