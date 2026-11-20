// F0-T9: prueba genérica sobre todos los comandos de builtinCommands().
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "commands/CommandRegistry.h"
#include "commands/ProjectCommands.h"

using namespace gp;
using namespace gp::commands;
using model::ProjectState;
using model::TrackId;

namespace {
ProjectState base() {
    ProjectState s;
    s.tracks = {{TrackId{1}, "A"}, {TrackId{2}, "B"}};
    s.nextTrackId = 3;
    return s;
}

struct Sample {
    const char* typeId;
    std::function<std::unique_ptr<Command>()> make;
};

// Un ejemplo por cada typeId registrado en ProjectCommands.cpp (8 tipos). El registro no es
// enumerable: si el arquitecto añade un tipo, hay que añadirlo aquí (ver informe F0-T9).
std::vector<Sample> samples() {
    return {
        {"transport.setTempo", [] { return makeSetTempo(141.5); }},
        {"transport.setTimeSignature", [] { return makeSetTimeSignature(7, 8); }},
        {"testTone.setEnabled", [] { return makeSetTestToneEnabled(true); }},
        {"testTone.setFrequency", [] { return makeSetTestToneFrequency(880.0); }},
        {"testTone.setGain", [] { return makeSetTestToneGain(-6.0f); }},
        {"track.add", [] { return makeAddTrack("Nueva"); }},
        {"track.remove", [] { return makeRemoveTrack(TrackId{1}); }},
        {"track.rename", [] { return makeRenameTrack(TrackId{2}, "Renombrada"); }},
    };
}
}  // namespace

TEST_CASE("builtinCommands: every typeId is registered and generic properties hold",
          "[integration][commands]") {
    for (const auto& s : samples()) {
        DYNAMIC_SECTION(s.typeId) {
            REQUIRE(builtinCommands().contains(s.typeId));
            auto cmd = s.make();
            REQUIRE(std::string(cmd->typeId()) == s.typeId);
            REQUIRE_FALSE(cmd->descriptionKey().empty());

            // apply / revert / apply
            const ProjectState initial = base();
            ProjectState st = initial;
            REQUIRE(cmd->apply(st).isOk());
            const ProjectState applied = st;
            CHECK(applied != initial);
            cmd->revert(st);
            CHECK(st == initial);
            REQUIRE(cmd->apply(st).isOk());
            CHECK(st == applied);

            // JSON ida y vuelta (tras apply, para incluir ids asignados)
            const auto json = cmd->toJson();
            Status err;
            auto copy = builtinCommands().create(json, err);
            REQUIRE(copy != nullptr);
            REQUIRE(err.isOk());
            CHECK(copy->toJson() == json);
            ProjectState viaCopy = base();
            REQUIRE(copy->apply(viaCopy).isOk());
            CHECK(viaCopy == applied);
            copy->revert(viaCopy);
            CHECK(viaCopy == initial);
        }
    }
}
