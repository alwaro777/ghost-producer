#include <catch2/catch_test_macros.hpp>

#include "commands/CommandHistory.h"
#include "commands/ProjectCommands.h"
#include "model/Document.h"

using namespace gp::commands;

TEST_CASE("aborted transaction keeps redo", "[history][redo]") {
    gp::model::Document doc;
    CommandHistory h(doc);
    REQUIRE(h.execute(makeSetTempo(130)).isOk());
    REQUIRE(h.undo());
    REQUIRE(h.redoCount() == 1);

    h.beginTransaction("ai");
    REQUIRE(h.execute(makeSetTempo(150)).isOk());
    h.abortTransaction();
    CHECK(h.redoCount() == 1);
    CHECK(h.redo());
    CHECK(doc.state().transport.tempoBpm == 130.0);
}

TEST_CASE("empty committed transaction keeps redo; non-empty clears it", "[history][redo]") {
    gp::model::Document doc;
    CommandHistory h(doc);
    REQUIRE(h.execute(makeSetTempo(130)).isOk());
    REQUIRE(h.undo());

    h.beginTransaction("empty");
    h.commitTransaction();
    CHECK(h.redoCount() == 1);

    h.beginTransaction("real");
    REQUIRE(h.execute(makeSetTempo(150)).isOk());
    h.commitTransaction();
    CHECK(h.redoCount() == 0);
    CHECK(h.undoCount() == 1);
}

TEST_CASE("clear aborts an open transaction", "[history][redo]") {
    gp::model::Document doc;
    CommandHistory h(doc);
    const auto initial = doc.state();
    h.beginTransaction("ai");
    REQUIRE(h.execute(makeSetTempo(150)).isOk());
    REQUIRE(h.execute(makeAddTrack("T")).isOk());
    h.clear();
    CHECK_FALSE(h.inTransaction());
    CHECK(doc.state() == initial);
    CHECK(h.undoCount() == 0);
    CHECK(h.redoCount() == 0);
}
