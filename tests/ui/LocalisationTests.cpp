// Tests de ui::tr / loadLanguage y de los textos de deshacer (dueño: ui-ux).
#include <catch2/catch_test_macros.hpp>

#include "commands/CommandHistory.h"
#include "commands/ProjectCommands.h"
#include "model/Document.h"
#include "ui/Localisation.h"
#include "../../src/ui/MainComponent.h"

using namespace gp;

TEST_CASE("loadLanguage lee el catálogo es embebido", "[ui][i18n]") {
    REQUIRE(ui::loadLanguage("es"));
    CHECK(ui::tr("menu.file") == "Archivo");
    CHECK(ui::tr("panel.tone.enable") == "Sonar");
}

TEST_CASE("idioma inexistente falla sin romper", "[ui][i18n]") {
    CHECK_FALSE(ui::loadLanguage("xx"));
}

TEST_CASE("clave ausente devuelve la clave", "[ui][i18n]") {
    REQUIRE(ui::loadLanguage("es"));
    CHECK(ui::tr("no.existe.esta.clave") == "no.existe.esta.clave");
}

TEST_CASE("tr sustituye {0}, {1}…", "[ui][i18n]") {
    REQUIRE(ui::loadLanguage("es"));
    CHECK(ui::tr("window.title", {"Demo"}) == "Demo — GhostProducer");
    CHECK(ui::tr("status.device", {"Salida", "48000", "256"}) == "Audio: Salida · 48000 Hz · búfer 256");
    // Marcador sin argumento: se deja tal cual.
    CHECK(ui::tr("status.device", {"Salida"}).find("{1}") != std::string::npos);
}

TEST_CASE("el texto de deshacer/rehacer recibe tr(cmd.*) en {0}", "[ui][i18n]") {
    REQUIRE(ui::loadLanguage("es"));
    model::Document doc;
    commands::CommandHistory history(doc);
    CHECK(ui::MainComponent::undoMenuText(history).empty());

    REQUIRE(history.execute(commands::makeSetTempo(130.0)).isOk());
    CHECK(ui::MainComponent::undoMenuText(history) == "Deshacer: cambiar el tempo");
    CHECK(ui::MainComponent::redoMenuText(history).empty());

    REQUIRE(history.undo());
    CHECK(ui::MainComponent::redoMenuText(history) == "Rehacer: cambiar el tempo");
}
