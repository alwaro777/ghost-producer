#pragma once
/// @file ProjectSerializer.h
/// ProjectState ↔ JSON (ADR 0002). Puro: sin disco, sin JUCE. Hilo: cualquiera salvo audio.
///
/// Formato del objeto raíz (orden de claves fijo, para diffs estables):
/// @code{.json}
/// { "format": "ghostproducer.project", "schemaVersion": 1, "createdWith": "0.1.0",
///   "project": { "transport": {...}, "testTone": {...}, "tracks": [...], "nextTrackId": 1 } }
/// @endcode
/// Las claves del modelo van en camelCase y coinciden con los campos de ProjectState.

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "core/Status.h"
#include "model/Migrations.h"
#include "model/ProjectState.h"

namespace gp::model {

inline constexpr std::string_view kProjectFormatId = "ghostproducer.project";
/// Versión que escribe esta build. Subirla exige una migración en builtinMigrations().
inline constexpr int kCurrentSchemaVersion = 1;

struct LoadResult {
    Status status;                      ///< Si !isOk(), `state` es un ProjectState por defecto.
    ProjectState state;                 ///< Válido (isValid) cuando status.isOk().
    int originalSchemaVersion = 0;      ///< Versión leída del archivo antes de migrar.
    bool migrated = false;              ///< true si se aplicó al menos una migración.
    std::vector<std::string> warnings;  ///< Claves desconocidas ignoradas, etc. (inglés).
};

/// Serializa con la versión actual. `appVersion` se guarda en "createdWith" (informativo).
/// Determinista: mismo estado ⇒ mismo JSON byte a byte tras dump(2).
[[nodiscard]] nlohmann::ordered_json toJson(const ProjectState& state, std::string_view appVersion);

/// Valida "format", lee "schemaVersion", migra con `migrations` hasta `targetVersion` y
/// parsea. Errores: parseError (estructura/tipos), unsupportedVersion (archivo más nuevo
/// que targetVersion), migrationFailed, invalidArgument (rangos/invariantes de isValid).
/// Las claves desconocidas no son error: se ignoran y se anotan en `warnings`.
/// `targetVersion` solo se cambia en tests; en la app es kCurrentSchemaVersion.
[[nodiscard]] LoadResult fromJson(const nlohmann::ordered_json& document,
                                  const MigrationRegistry& migrations = builtinMigrations(),
                                  int targetVersion = kCurrentSchemaVersion);

}  // namespace gp::model
