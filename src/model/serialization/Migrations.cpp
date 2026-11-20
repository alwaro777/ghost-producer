#include "model/Migrations.h"

#include <cassert>
#include <nlohmann/json.hpp>
#include <string>

namespace gp::model {

void MigrationRegistry::add(int fromVersion, MigrationFn fn) {
    const bool inserted = steps_.emplace(fromVersion, std::move(fn)).second;
    assert(inserted && "migration registered twice");
    (void)inserted;
}

Status MigrationRegistry::migrate(nlohmann::ordered_json& document, int fromVersion,
                                  int toVersion) const {
    if (fromVersion > toVersion) {
        return Status::error(ErrorCode::migrationFailed, "cannot migrate to an older version");
    }
    for (int v = fromVersion; v < toVersion; ++v) {
        const auto it = steps_.find(v);
        if (it == steps_.end()) {
            return Status::error(ErrorCode::migrationFailed,
                                 "missing migration " + std::to_string(v) + " -> " +
                                     std::to_string(v + 1));
        }
        Status st = it->second(document);
        if (!st.isOk()) {
            return Status::error(ErrorCode::migrationFailed,
                                 "migration " + std::to_string(v) + " -> " +
                                     std::to_string(v + 1) + " failed: " + st.message);
        }
        if (!document.is_object() || !document.contains("schemaVersion") ||
            document["schemaVersion"] != v + 1) {
            return Status::error(ErrorCode::migrationFailed,
                                 "migration " + std::to_string(v) +
                                     " did not set schemaVersion to " + std::to_string(v + 1));
        }
    }
    return Status::ok();
}

const MigrationRegistry& builtinMigrations() {
    static const MigrationRegistry registry;  // F0: version 1 is the first; nothing to migrate.
    return registry;
}

}  // namespace gp::model
