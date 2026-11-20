#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "commands/CommandHistory.h"
#include "commands/CommandRegistry.h"
#include "commands/ProjectCommands.h"
#include "model/Document.h"

using namespace gp;
using namespace gp::commands;
using model::ProjectState;
using model::TrackId;

namespace {
ProjectState withTracks() {
    ProjectState s;
    s.tracks = {{TrackId{1}, "A"}, {TrackId{2}, "B"}};
    s.nextTrackId = 3;
    return s;
}

struct Counter : model::Document::Listener {
    int n = 0;
    void documentChanged(const model::Document&) override { ++n; }
};
}  // namespace

TEST_CASE("every command: apply/revert/apply round trip", "[commands]") {
    std::vector<std::unique_ptr<Command>> cmds;
    cmds.push_back(makeSetTempo(140.0));
    cmds.push_back(makeSetTimeSignature(7, 8));
    cmds.push_back(makeSetTestToneEnabled(true));
    cmds.push_back(makeSetTestToneFrequency(880.0));
    cmds.push_back(makeSetTestToneGain(-6.0f));
    cmds.push_back(makeAddTrack("New"));
    cmds.push_back(makeAddTrack("Mid", 1));
    cmds.push_back(makeRemoveTrack(TrackId{1}));
    cmds.push_back(makeRenameTrack(TrackId{2}, "Renamed"));
    for (auto& c : cmds) {
        INFO(std::string(c->typeId()));
        const ProjectState initial = withTracks();
        ProjectState s = initial;
        REQUIRE(c->apply(s).isOk());
        const ProjectState first = s;
        REQUIRE(first != initial);
        c->revert(s);
        REQUIRE(s == initial);
        REQUIRE(c->apply(s).isOk());
        REQUIRE(s == first);
    }
}

TEST_CASE("add track keeps its id across redo", "[commands]") {
    ProjectState s = withTracks();
    auto c = makeAddTrack("X");
    REQUIRE(c->apply(s).isOk());
    CHECK(s.tracks.back().id.value == 3);
    CHECK(s.nextTrackId == 4);
    c->revert(s);
    CHECK(s.nextTrackId == 3);
}

TEST_CASE("out-of-range arguments are rejected without changing state", "[commands]") {
    const std::string long65(65, 'x');
    std::vector<std::unique_ptr<Command>> cmds;
    cmds.push_back(makeSetTempo(19.9));
    cmds.push_back(makeSetTempo(1000.0));
    cmds.push_back(makeSetTimeSignature(4, 3));
    cmds.push_back(makeSetTimeSignature(0, 4));
    cmds.push_back(makeSetTestToneFrequency(10.0));
    cmds.push_back(makeSetTestToneGain(1.0f));
    cmds.push_back(makeAddTrack(""));
    cmds.push_back(makeAddTrack(long65));
    cmds.push_back(makeAddTrack("ok", 3));
    cmds.push_back(makeRemoveTrack(TrackId{99}));
    cmds.push_back(makeRenameTrack(TrackId{99}, "x"));
    cmds.push_back(makeRenameTrack(TrackId{1}, ""));
    cmds.push_back(makeRenameTrack(TrackId{1}, long65));
    for (auto& c : cmds) {
        INFO(std::string(c->typeId()));
        ProjectState s = withTracks();
        const ProjectState before = s;
        CHECK(c->apply(s).code == ErrorCode::invalidArgument);
        CHECK(s == before);
    }
}

TEST_CASE("json round trip through the registry", "[commands]") {
    std::vector<std::unique_ptr<Command>> cmds;
    cmds.push_back(makeSetTempo(133.5));
    cmds.push_back(makeSetTimeSignature(3, 4));
    cmds.push_back(makeSetTestToneEnabled(true));
    cmds.push_back(makeSetTestToneFrequency(1000.0));
    cmds.push_back(makeSetTestToneGain(-12.0f));
    cmds.push_back(makeAddTrack("T", 0));
    cmds.push_back(makeRemoveTrack(TrackId{2}));
    cmds.push_back(makeRenameTrack(TrackId{1}, "Z"));
    for (auto& c : cmds) {
        INFO(std::string(c->typeId()));
        Status err;
        auto copy = builtinCommands().create(c->toJson(), err);
        REQUIRE(copy);
        REQUIRE(err.isOk());
        ProjectState a = withTracks(), b = withTracks();
        REQUIRE(c->apply(a).isOk());
        REQUIRE(copy->apply(b).isOk());
        CHECK(a == b);
        CHECK(copy->toJson() == c->toJson());
    }
}

TEST_CASE("registry errors", "[commands]") {
    Status err;
    CHECK(builtinCommands().create({{"type", "nope"}, {"version", 1}, {"params", nlohmann::json::object()}}, err) == nullptr);
    CHECK(err.code == ErrorCode::unknownCommand);
    CHECK(builtinCommands().create({{"type", "transport.setTempo"}, {"version", 1}}, err) == nullptr);
    CHECK(err.code == ErrorCode::parseError);
    CHECK(builtinCommands().create({{"type", "transport.setTempo"}, {"version", 1}, {"params", {{"value", "x"}}}}, err) == nullptr);
    CHECK(err.code == ErrorCode::invalidArgument);
    CHECK(builtinCommands().contains("track.add"));
}

TEST_CASE("history basics", "[history]") {
    model::Document doc;
    CommandHistory h(doc);
    CHECK_FALSE(h.undo());
    CHECK_FALSE(h.redo());
    REQUIRE(h.execute(makeSetTempo(100)).isOk());
    REQUIRE(h.execute(makeSetTempo(110)).isOk());
    CHECK(h.undo());
    CHECK(h.canRedo());
    CHECK(doc.state().transport.tempoBpm == 100);
    REQUIRE(h.execute(makeSetTempo(90)).isOk());
    CHECK_FALSE(h.canRedo());
    CHECK(h.execute(makeSetTempo(5)).code == ErrorCode::invalidArgument);
    CHECK(h.undoCount() == 2);
    CHECK(h.undoDescriptionKey() == "cmd.transport.setTempo");
}

TEST_CASE("gesture merging", "[history]") {
    model::Document doc;
    CommandHistory h(doc);
    for (int i = 0; i < 50; ++i)
        REQUIRE(h.execute(makeSetTestToneFrequency(100.0 + i), MergePolicy::withPrevious).isOk());
    CHECK(h.undoCount() == 1);
    CHECK(doc.state().testTone.frequencyHz == 149.0);
    h.undo();
    CHECK(doc.state().testTone.frequencyHz == 440.0);
    h.redo();
    CHECK(doc.state().testTone.frequencyHz == 149.0);

    model::Document doc2;
    CommandHistory h2(doc2);
    for (int i = 0; i < 50; ++i)
        REQUIRE(h2.execute(makeSetTestToneFrequency(100.0 + i), MergePolicy::never).isOk());
    CHECK(h2.undoCount() == 50);
}

TEST_CASE("transactions", "[history]") {
    model::Document doc;
    Counter counter;
    doc.addListener(&counter);
    CommandHistory h(doc);
    const ProjectState initial = doc.state();

    h.beginTransaction("cmd.ai.batch");
    REQUIRE(h.execute(makeSetTempo(130)).isOk());
    REQUIRE(h.execute(makeAddTrack("T")).isOk());
    REQUIRE(h.execute(makeSetTestToneEnabled(true)).isOk());
    CHECK(h.inTransaction());
    CHECK_FALSE(h.undo());
    h.commitTransaction();
    CHECK(h.undoCount() == 1);
    CHECK(counter.n == 1);
    CHECK(h.undoEntries().front().commandCount == 3);
    CHECK(h.undo());
    CHECK(doc.state() == initial);
    CHECK(counter.n == 2);

    h.beginTransaction("x");
    REQUIRE(h.execute(makeSetTempo(130)).isOk());
    REQUIRE(h.execute(makeAddTrack("T")).isOk());
    REQUIRE(h.execute(makeSetTestToneEnabled(true)).isOk());
    h.abortTransaction();
    CHECK(doc.state() == initial);

    const auto undoBefore = h.undoCount();
    h.beginTransaction("empty");
    h.commitTransaction();
    CHECK(h.undoCount() == undoBefore);
    doc.removeListener(&counter);
}

TEST_CASE("dirty tracking and revision", "[history]") {
    model::Document doc;
    CommandHistory h(doc);
    CHECK_FALSE(h.isDirty());
    REQUIRE(h.execute(makeSetTempo(100)).isOk());
    CHECK(h.isDirty());
    h.markSaved();
    CHECK_FALSE(h.isDirty());
    REQUIRE(h.execute(makeSetTempo(101)).isOk());
    CHECK(h.isDirty());
    h.undo();
    CHECK_FALSE(h.isDirty());
    h.undo();
    CHECK(h.isDirty());
    h.redo();
    CHECK_FALSE(h.isDirty());
    // Nueva rama tras deshacer más allá del guardado: nunca vuelve a limpio por posición.
    h.undo();
    REQUIRE(h.execute(makeSetTempo(55)).isOk());
    REQUIRE(h.execute(makeSetTempo(56)).isOk());
    CHECK(h.isDirty());
    // Fusión sobre la entrada guardada ensucia.
    h.markSaved();
    CHECK_FALSE(h.isDirty());
    REQUIRE(h.execute(makeSetTempo(57), MergePolicy::withPrevious).isOk());
    CHECK(h.isDirty());
    h.clear();
    CHECK_FALSE(h.isDirty());

    const auto rev = doc.revision();
    REQUIRE(h.execute(makeSetTempo(70)).isOk());
    CHECK(doc.revision() == rev + 1);
    h.undo();
    CHECK(doc.revision() == rev + 2);
    h.redo();
    CHECK(doc.revision() == rev + 3);
}
