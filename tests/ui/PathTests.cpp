// Rutas Unicode y estructura de «Guardar como» (F0-BUG2). Dueño: ui-ux.
#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "../../src/app/PathUtils.h"
#include "model/ProjectFile.h"

namespace fs = std::filesystem;
using namespace gp::app;

TEST_CASE("rutas Unicode: ida y vuelta juce <-> filesystem por UTF-8", "[ui][paths]") {
    const juce::String text = juce::String::fromUTF8("/tmp/Jos\xc3\xa9/Canci\xc3\xb3n \xc3\xb1 \xe6\x97\xa5\xe6\x9c\xac");
    const auto path = paths::toPath(text);
    CHECK(paths::toJuceString(path) == text);
    CHECK(paths::toJuceString(paths::toPath(paths::toFile(path))) == text);
    CHECK(paths::toUtf8(path.filename()) == "Canci\xc3\xb3n \xc3\xb1 \xe6\x97\xa5\xe6\x9c\xac");
}

TEST_CASE("projectFileFor: X/Prueba -> X/Prueba/Prueba.gpproj", "[ui][paths]") {
    CHECK(paths::projectFileFor("/x/Prueba") == fs::path("/x/Prueba/Prueba.gpproj"));
    CHECK(paths::projectFileFor("/x/Prueba.gpproj") == fs::path("/x/Prueba/Prueba.gpproj"));
    // Ya dentro de la carpeta del proyecto: se respeta.
    CHECK(paths::projectFileFor("/x/Prueba/Prueba.gpproj") == fs::path("/x/Prueba/Prueba.gpproj"));
    // Nombres con punto no pierden el sufijo.
    CHECK(paths::projectFileFor("/x/Mix v2") == fs::path("/x/Mix v2/Mix v2.gpproj"));
}

TEST_CASE("migrationBackupFor sigue el ADR 0002", "[ui][paths]") {
    CHECK(paths::migrationBackupFor("/x/P/P.gpproj", 1) == fs::path("/x/P/backups/P.v1.gpproj"));
}

TEST_CASE("Guardar como crea la estructura del proyecto bajo X/Prueba", "[ui][paths]") {
    const auto root = fs::temp_directory_path() / paths::toPath(juce::String::fromUTF8("gp_bug2_Jos\xc3\xa9 \xe6\x97\xa5"));
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root);

    const auto target = paths::projectFileFor(root / "Prueba");
    REQUIRE(gp::model::saveProject(gp::model::ProjectState{}, target, "test").isOk());

    CHECK(fs::exists(root / "Prueba" / "Prueba.gpproj"));
    for (const char* dir : {"samples", "renders", "cache", "backups"})
        CHECK(fs::is_directory(root / "Prueba" / dir));
    // Nada de subcarpetas colgando directamente de X.
    CHECK_FALSE(fs::exists(root / "samples"));
    CHECK_FALSE(fs::exists(root / "backups"));
    CHECK(gp::model::loadProject(target).status.isOk());

    fs::remove_all(root, ec);
}
