#include "model/ProjectSerializer.h"

#include <cmath>
#include <initializer_list>
#include <nlohmann/json.hpp>

namespace gp::model {

namespace {
using json = nlohmann::ordered_json;

struct ParseFailure {
    Status status;
};

[[noreturn]] void bad(const std::string& msg) {
    throw ParseFailure{Status::error(ErrorCode::parseError, msg)};
}

const json& need(const json& obj, const char* key, const char* where) {
    const auto it = obj.find(key);
    if (it == obj.end()) {
        bad("missing key '" + std::string(key) + "' in " + where);
    }
    return *it;
}

const json& needObject(const json& obj, const char* key, const char* where) {
    const json& v = need(obj, key, where);
    if (!v.is_object()) {
        bad("'" + std::string(key) + "' in " + where + " must be an object");
    }
    return v;
}

double asDouble(const json& v, const std::string& what) {
    if (!v.is_number()) {
        bad(what + " must be a number");
    }
    return v.get<double>();
}

int asInt(const json& v, const std::string& what) {
    if (!v.is_number_integer()) {
        bad(what + " must be an integer");
    }
    const auto n = v.get<std::int64_t>();
    if (n < -1'000'000'000 || n > 1'000'000'000) {
        throw ParseFailure{Status::error(ErrorCode::invalidArgument, what + " out of range")};
    }
    return static_cast<int>(n);
}

std::uint64_t asId(const json& v, const std::string& what) {
    if (!v.is_number_integer()) {
        bad(what + " must be an integer");
    }
    if (v.is_number_unsigned()) {
        return v.get<std::uint64_t>();
    }
    throw ParseFailure{Status::error(ErrorCode::invalidArgument, what + " must not be negative")};
}

void warnUnknown(const json& obj, std::initializer_list<const char*> known, const std::string& where,
                 std::vector<std::string>& warnings) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        bool found = false;
        for (const char* k : known) {
            if (it.key() == k) {
                found = true;
                break;
            }
        }
        if (!found) {
            warnings.push_back("unknown key '" + it.key() + "' in " + where + " ignored");
        }
    }
}

ProjectState parseProject(const json& p, std::vector<std::string>& warnings) {
    ProjectState s;
    warnUnknown(p, {"transport", "testTone", "tracks", "nextTrackId"}, "project", warnings);

    const json& tr = needObject(p, "transport", "project");
    warnUnknown(tr, {"tempoBpm", "timeSigNumerator", "timeSigDenominator"}, "transport", warnings);
    s.transport.tempoBpm = asDouble(need(tr, "tempoBpm", "transport"), "transport.tempoBpm");
    s.transport.timeSigNumerator =
        asInt(need(tr, "timeSigNumerator", "transport"), "transport.timeSigNumerator");
    s.transport.timeSigDenominator =
        asInt(need(tr, "timeSigDenominator", "transport"), "transport.timeSigDenominator");

    const json& tt = needObject(p, "testTone", "project");
    warnUnknown(tt, {"enabled", "frequencyHz", "gainDb"}, "testTone", warnings);
    const json& en = need(tt, "enabled", "testTone");
    if (!en.is_boolean()) {
        bad("testTone.enabled must be a boolean");
    }
    s.testTone.enabled = en.get<bool>();
    s.testTone.frequencyHz = asDouble(need(tt, "frequencyHz", "testTone"), "testTone.frequencyHz");
    s.testTone.gainDb =
        static_cast<float>(asDouble(need(tt, "gainDb", "testTone"), "testTone.gainDb"));

    const json& tracks = need(p, "tracks", "project");
    if (!tracks.is_array()) {
        bad("'tracks' must be an array");
    }
    for (const json& t : tracks) {
        if (!t.is_object()) {
            bad("track must be an object");
        }
        warnUnknown(t, {"id", "name"}, "track", warnings);
        TrackState ts;
        ts.id.value = asId(need(t, "id", "track"), "track.id");
        const json& name = need(t, "name", "track");
        if (!name.is_string()) {
            bad("track.name must be a string");
        }
        ts.name = name.get<std::string>();
        s.tracks.push_back(std::move(ts));
    }
    s.nextTrackId = asId(need(p, "nextTrackId", "project"), "nextTrackId");
    return s;
}
}  // namespace

json toJson(const ProjectState& state, std::string_view appVersion) {
    json tracks = json::array();
    for (const auto& t : state.tracks) {
        json jt = json::object();
        jt["id"] = t.id.value;
        jt["name"] = t.name;
        tracks.push_back(std::move(jt));
    }
    json project = json::object();
    project["transport"] = {{"tempoBpm", state.transport.tempoBpm},
                            {"timeSigNumerator", state.transport.timeSigNumerator},
                            {"timeSigDenominator", state.transport.timeSigDenominator}};
    project["testTone"] = {{"enabled", state.testTone.enabled},
                           {"frequencyHz", state.testTone.frequencyHz},
                           {"gainDb", static_cast<double>(state.testTone.gainDb)}};
    project["tracks"] = std::move(tracks);
    project["nextTrackId"] = state.nextTrackId;

    json root = json::object();
    root["format"] = std::string(kProjectFormatId);
    root["schemaVersion"] = kCurrentSchemaVersion;
    root["createdWith"] = std::string(appVersion);
    root["project"] = std::move(project);
    return root;
}

LoadResult fromJson(const json& document, const MigrationRegistry& migrations, int targetVersion) {
    LoadResult result;
    auto failWith = [&result](Status st) {
        result.status = std::move(st);
        result.state = ProjectState{};
        result.migrated = false;
        return result;
    };

    if (!document.is_object()) {
        return failWith(Status::error(ErrorCode::parseError, "root must be an object"));
    }
    const auto fmt = document.find("format");
    if (fmt == document.end() || !fmt->is_string() || fmt->get<std::string>() != kProjectFormatId) {
        return failWith(Status::error(ErrorCode::parseError, "unrecognised project format"));
    }
    const auto ver = document.find("schemaVersion");
    if (ver == document.end() || !ver->is_number_integer()) {
        return failWith(Status::error(ErrorCode::parseError, "schemaVersion missing or not an integer"));
    }
    const auto fileVersion = ver->get<std::int64_t>();
    if (fileVersion < 0) {
        return failWith(Status::error(ErrorCode::parseError, "schemaVersion must not be negative"));
    }
    if (fileVersion > targetVersion) {
        return failWith(Status::error(ErrorCode::unsupportedVersion,
                                      "file schemaVersion " + std::to_string(fileVersion) +
                                          " is newer than supported " + std::to_string(targetVersion)));
    }
    result.originalSchemaVersion = static_cast<int>(fileVersion);

    json doc = document;  // migrations work on a copy
    if (fileVersion < targetVersion) {
        try {
            Status st = migrations.migrate(doc, static_cast<int>(fileVersion), targetVersion);
            if (!st.isOk()) {
                return failWith(std::move(st));
            }
        } catch (const std::exception& e) {
            return failWith(Status::error(ErrorCode::migrationFailed, e.what()));
        }
        result.migrated = true;
    }

    try {
        warnUnknown(doc, {"format", "schemaVersion", "createdWith", "project"}, "root", result.warnings);
        ProjectState s = parseProject(needObject(doc, "project", "root"), result.warnings);
        std::string why;
        if (!isValid(s, &why)) {
            return failWith(Status::error(ErrorCode::invalidArgument, why));
        }
        result.state = std::move(s);
        result.status = Status::ok();
        return result;
    } catch (const ParseFailure& f) {
        return failWith(f.status);
    } catch (const nlohmann::json::exception& e) {
        return failWith(Status::error(ErrorCode::parseError, e.what()));
    }
}

}  // namespace gp::model
