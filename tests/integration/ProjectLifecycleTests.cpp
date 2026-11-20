// F0-T9: integración Document + CommandHistory + ProjectFile (headless).
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "commands/CommandHistory.h"
#include "commands/ProjectCommands.h"
#include "model/Document.h"
#include "model/ProjectFile.h"

using namespace gp;
using namespace gp::commands;
namespace fs = std::filesystem;

namespace {
struct TempDir {
    fs::path path;
    TempDir() {
        static std::atomic<int> n{0};
        const auto t = std::chrono::steady_clock::now().time_since_epoch().count();
        path = fs::temp_directory_path() / ("gp_it_" + std::to_string(t) + "_" + std::to_string(n++));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};
}  // namespace

TEST_CASE("integration: 6 commands -> save -> load -> equal; undo returns to default",
          "[integration]") {
    model::Document doc;
    CommandHistory history(doc);
    const model::ProjectState initial = doc.state();
    REQUIRE(initial == model::ProjectState{});

    // 1) comando suelto
    REQUIRE(history.execute(makeSetTempo(130.0)).isOk());
    // 2) gesto fusionado (arrastre): 2 execute -> 1 entrada de undo
    REQUIRE(history.execute(makeSetTestToneFrequency(500.0)).isOk());
    REQUIRE(history.execute(makeSetTestToneFrequency(660.0), MergePolicy::withPrevious).isOk());
    REQUIRE(history.undoCount() == 2);
    // 3) transacción de 3 comandos
    history.beginTransaction("cmd.test.transaction");
    REQUIRE(history.execute(makeAddTrack("Bajo")).isOk());
    REQUIRE(history.execute(makeAddTrack("Lead")).isOk());
    REQUIRE(history.execute(makeRenameTrack(model::TrackId{1}, "Bajo sub")).isOk());
    history.commitTransaction();
    REQUIRE(history.undoCount() == 3);
    // 4-6) resto
    REQUIRE(history.execute(makeSetTestToneEnabled(true)).isOk());
    REQUIRE(history.execute(makeSetTimeSignature(7, 8)).isOk());
    REQUIRE(history.execute(makeRemoveTrack(model::TrackId{2})).isOk());
    REQUIRE(history.undoCount() == 6);

    const model::ProjectState expected = doc.state();
    REQUIRE(expected != initial);
    REQUIRE(expected.tracks.size() == 1);
    REQUIRE(expected.tracks[0].name == "Bajo sub");
    REQUIRE(expected.testTone.frequencyHz == 660.0);

    TempDir dir;
    const auto file = dir.path / "Cancion" / "Cancion.gpproj";
    REQUIRE(model::saveProject(doc.state(), file, "test").isOk());
    REQUIRE(fs::exists(file));
    REQUIRE_FALSE(fs::exists(file.string() + ".tmp"));

    const auto loaded = model::loadProject(file);
    REQUIRE(loaded.status.isOk());
    model::Document doc2;
    doc2.reset(loaded.state);
    CHECK(doc2.state() == expected);

    // deshacer todo en el original -> estado por defecto
    std::size_t steps = 0;
    while (history.canUndo()) {
        REQUIRE(history.undo());
        ++steps;
    }
    CHECK(steps == 6);
    CHECK(doc.state() == initial);

    // y rehacer todo -> vuelve a expected
    while (history.canRedo()) REQUIRE(history.redo());
    CHECK(doc.state() == expected);
}
