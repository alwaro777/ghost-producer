#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "model/ProjectFile.h"
#include "model/ProjectSerializer.h"

using namespace gp;
using namespace gp::model;
using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

namespace {
ProjectState sampleState() {
    ProjectState s;
    s.transport = {97.5, 7, 8};
    s.testTone = {true, 1234.5, -12.0f};
    s.tracks = {{{1}, "Drums"}, {{2}, "Bajo"}, {{5}, "Voz \xC3\xA1"}};
    s.nextTrackId = 6;
    return s;
}

struct TempDir {
    fs::path path;
    TempDir() {
        static int counter = 0;
        path = fs::temp_directory_path() /
               ("gp_t2_" + std::to_string(::getpid()) + "_" + std::to_string(counter++));
        fs::remove_all(path);
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

std::string slurp(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

json parsed(const ProjectState& s) {
    return json::parse(toJson(s, "0.1.0").dump(2));
}
}  // namespace

TEST_CASE("round trip and determinism", "[model][format]") {
    const auto s = sampleState();
    const std::string a = toJson(s, "0.1.0").dump(2);
    const std::string b = toJson(s, "0.1.0").dump(2);
    REQUIRE(a == b);
    const auto r = fromJson(json::parse(a));
    REQUIRE(r.status.isOk());
    REQUIRE(r.state == s);
    REQUIRE_FALSE(r.migrated);
    REQUIRE(r.warnings.empty());

    const auto j = toJson(s, "0.1.0");
    std::vector<std::string> rootKeys;
    for (auto it = j.begin(); it != j.end(); ++it) rootKeys.push_back(it.key());
    REQUIRE(rootKeys == std::vector<std::string>{"format", "schemaVersion", "createdWith", "project"});
    std::vector<std::string> projKeys;
    for (auto it = j["project"].begin(); it != j["project"].end(); ++it) projKeys.push_back(it.key());
    REQUIRE(projKeys == std::vector<std::string>{"transport", "testTone", "tracks", "nextTrackId"});
}

TEST_CASE("fromJson errors", "[model][format]") {
    SECTION("wrong format") {
        auto j = parsed(sampleState());
        j["format"] = "other";
        REQUIRE(fromJson(j).status.code == ErrorCode::parseError);
    }
    SECTION("not an object / missing key / bad type") {
        REQUIRE(fromJson(json::array()).status.code == ErrorCode::parseError);
        auto j = parsed(sampleState());
        j["project"].erase("tracks");
        REQUIRE(fromJson(j).status.code == ErrorCode::parseError);
        j = parsed(sampleState());
        j["project"]["transport"]["tempoBpm"] = "fast";
        REQUIRE(fromJson(j).status.code == ErrorCode::parseError);
    }
    SECTION("newer version") {
        auto j = parsed(sampleState());
        j["schemaVersion"] = 2;
        REQUIRE(fromJson(j).status.code == ErrorCode::unsupportedVersion);
    }
    SECTION("out of range values are rejected, not clamped") {
        auto j = parsed(sampleState());
        j["project"]["transport"]["tempoBpm"] = 5000.0;
        REQUIRE(fromJson(j).status.code == ErrorCode::invalidArgument);
        j = parsed(sampleState());
        j["project"]["transport"]["timeSigDenominator"] = 6;
        REQUIRE(fromJson(j).status.code == ErrorCode::invalidArgument);
    }
    SECTION("duplicate track ids") {
        auto j = parsed(sampleState());
        j["project"]["tracks"][1]["id"] = 1;
        REQUIRE(fromJson(j).status.code == ErrorCode::invalidArgument);
    }
}

TEST_CASE("unknown key yields one warning", "[model][format]") {
    auto j = parsed(sampleState());
    j["project"]["futureThing"] = 1;
    const auto r = fromJson(j);
    REQUIRE(r.status.isOk());
    REQUIRE(r.state == sampleState());
    REQUIRE(r.warnings.size() == 1);
}

TEST_CASE("migrations", "[model][format]") {
    auto v0 = parsed(sampleState());
    v0["schemaVersion"] = 0;

    MigrationRegistry reg;
    reg.add(0, [](json& d) {
        d["schemaVersion"] = 1;
        return Status::ok();
    });
    SECTION("0 -> 1") {
        const auto r = fromJson(v0, reg, 1);
        REQUIRE(r.status.isOk());
        REQUIRE(r.migrated);
        REQUIRE(r.originalSchemaVersion == 0);
        REQUIRE(r.state == sampleState());
    }
    SECTION("missing step") {
        REQUIRE(fromJson(v0, reg, 2).status.code == ErrorCode::migrationFailed);
    }
    SECTION("migration returns error") {
        MigrationRegistry bad;
        bad.add(0, [](json&) { return Status::error(ErrorCode::invalidState, "nope"); });
        REQUIRE(fromJson(v0, bad, 1).status.code == ErrorCode::migrationFailed);
    }
    SECTION("builtin registry cannot migrate v0") {
        REQUIRE(fromJson(v0).status.code == ErrorCode::migrationFailed);
    }
}

TEST_CASE("saveProject / loadProject", "[model][file]") {
    TempDir dir;
    const fs::path file = dir.path / "MiCancion" / "MiCancion.gpproj";
    const auto s = sampleState();
    REQUIRE(saveProject(s, file, "0.1.0").isOk());
    for (const char* sub : {"samples", "renders", "cache", "backups"}) {
        REQUIRE(fs::is_directory(file.parent_path() / sub));
    }
    REQUIRE_FALSE(fs::exists(fs::path(file).concat(".tmp")));
    const std::string text = slurp(file);
    REQUIRE(!text.empty());
    REQUIRE(text.back() == '\n');
    REQUIRE(text.find('\r') == std::string::npos);
    REQUIRE(text.find("\"schemaVersion\": 1") != std::string::npos);

    const auto r = loadProject(file);
    REQUIRE(r.status.isOk());
    REQUIRE(r.state == s);

    REQUIRE(loadProject(dir.path / "nope.gpproj").status.code == ErrorCode::ioError);
    {
        std::ofstream(dir.path / "broken.gpproj") << "{ not json";
    }
    REQUIRE(loadProject(dir.path / "broken.gpproj").status.code == ErrorCode::parseError);
}

TEST_CASE("failed save leaves the previous file intact", "[model][file]") {
    TempDir dir;
    const fs::path file = dir.path / "P" / "P.gpproj";
    REQUIRE(saveProject(sampleState(), file, "0.1.0").isOk());
    const std::string before = slurp(file);

    // Works even as root: a directory squats on the temporary file name.
    fs::create_directory(fs::path(file).concat(".tmp"));
    auto changed = sampleState();
    changed.transport.tempoBpm = 140.0;
    const auto st = saveProject(changed, file, "0.1.0");
    REQUIRE(st.code == ErrorCode::ioError);
    REQUIRE(slurp(file) == before);
}

#ifndef _WIN32
TEST_CASE("read-only folder gives ioError", "[model][file]") {
    TempDir dir;
    const fs::path file = dir.path / "P" / "P.gpproj";
    REQUIRE(saveProject(sampleState(), file, "0.1.0").isOk());
    const std::string before = slurp(file);
    fs::permissions(file.parent_path(), fs::perms::owner_read | fs::perms::owner_exec,
                    fs::perm_options::replace);
    // Root ignores permissions: skip in that case.
    const bool writable = std::ofstream(file.parent_path() / "probe").is_open();
    if (!writable) {
        auto changed = sampleState();
        changed.transport.tempoBpm = 140.0;
        REQUIRE(saveProject(changed, file, "0.1.0").code == ErrorCode::ioError);
    }
    fs::permissions(file.parent_path(), fs::perms::owner_all, fs::perm_options::replace);
    REQUIRE(slurp(file) == before);
    if (writable) SKIP("running with privileges that bypass permissions");
}
#endif

TEST_CASE("isValid detects violations", "[model]") {
    std::string why;
    REQUIRE(isValid(sampleState(), &why));
    REQUIRE(isValid(ProjectState{}));

    auto expectBad = [](auto mutate) {
        auto s = sampleState();
        mutate(s);
        std::string w;
        CHECK_FALSE(isValid(s, &w));
        CHECK_FALSE(w.empty());
    };
    expectBad([](ProjectState& s) { s.tracks[1].id.value = 1; });
    expectBad([](ProjectState& s) { s.tracks[0].id.value = 0; });
    expectBad([](ProjectState& s) { s.nextTrackId = 5; });
    expectBad([](ProjectState& s) { s.transport.tempoBpm = 19.9; });
    expectBad([](ProjectState& s) { s.transport.tempoBpm = 999.1; });
    expectBad([](ProjectState& s) { s.transport.timeSigNumerator = 0; });
    expectBad([](ProjectState& s) { s.transport.timeSigNumerator = 33; });
    expectBad([](ProjectState& s) { s.transport.timeSigDenominator = 3; });
    expectBad([](ProjectState& s) { s.transport.timeSigDenominator = 64; });
    expectBad([](ProjectState& s) { s.testTone.frequencyHz = 19.0; });
    expectBad([](ProjectState& s) { s.testTone.frequencyHz = 20001.0; });
    expectBad([](ProjectState& s) { s.testTone.gainDb = -61.0f; });
    expectBad([](ProjectState& s) { s.testTone.gainDb = 0.5f; });
    expectBad([](ProjectState& s) { s.tracks[0].name.clear(); });
    expectBad([](ProjectState& s) { s.tracks[0].name = std::string(65, 'x'); });
}
