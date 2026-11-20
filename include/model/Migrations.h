#pragma once
/// @file Migrations.h
/// Migraciones del esquema JSON del proyecto (ADR 0002). Cada migración transforma, en el
/// propio JSON y antes de parsearlo al modelo, un documento de la versión N a la N+1.
///
/// Hilo: cualquiera salvo audio. Las funciones de migración deben ser puras (sin I/O).

#include <functional>
#include <map>
#include <nlohmann/json_fwd.hpp>

#include "core/Status.h"

namespace gp::model {

/// Migra `document` (objeto raíz completo, incluido "schemaVersion") de `fromVersion` a
/// `fromVersion + 1` in situ. Debe actualizar "schemaVersion". Si falla, devuelve error y
/// el llamador descarta el documento (no se usa a medio migrar).
using MigrationFn = std::function<Status(nlohmann::ordered_json& document)>;

/// Cadena de migraciones. Invariante: para llegar de A a B deben existir todas las
/// migraciones A→A+1, …, B-1→B; si falta alguna, migrate() devuelve migrationFailed.
class MigrationRegistry {
public:
    /// Registra la migración fromVersion → fromVersion+1. Registrar dos veces la misma
    /// versión es un error de programación (assert en debug; la segunda se ignora).
    void add(int fromVersion, MigrationFn fn);

    /// Aplica en orden las migraciones de `fromVersion` a `toVersion` (from ≤ to).
    /// Con from == to no hace nada y devuelve ok.
    [[nodiscard]] Status migrate(nlohmann::ordered_json& document, int fromVersion,
                                 int toVersion) const;

private:
    std::map<int, MigrationFn> steps_;
};

/// Migraciones oficiales de la app. En F0 está vacía (versión actual = 1); los tests
/// inyectan registros propios para probar la maquinaria.
[[nodiscard]] const MigrationRegistry& builtinMigrations();

}  // namespace gp::model
