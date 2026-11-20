#include <catch2/catch_test_macros.hpp>
#include <juce_events/juce_events.h>

#include "engine/AudioEngine.h"

using namespace gp;
using namespace gp::engine;

namespace {
struct JuceInit {
    juce::ScopedJuceInitialiser_GUI init;
};
}  // namespace

TEST_CASE("estado corrupto: deviceError y el motor sigue vivo") {
    JuceInit juceInit;
    auto engine = createAudioEngine();
    const auto st = engine->initialise("<<<esto no es xml");
    CHECK(st.code == ErrorCode::deviceError);
    CHECK_FALSE(engine->deviceStatus().open);
    CHECK_FALSE(engine->deviceStatus().lastError.empty());

    model::ProjectState p;
    p.testTone.enabled = true;
    engine->setProject(p, 1);
    const auto m = engine->takeMetrics();
    CHECK(m.peakLeft == 0.0f);
    CHECK(m.rtViolationCount == 0);
    engine->shutdown();
    engine->shutdown();  // idempotente
}

TEST_CASE("tipo de dispositivo inexistente: coherente y sin abortar") {
    JuceInit juceInit;
    auto engine = createAudioEngine();
    const auto st = engine->initialise(
        "<DEVICESETUP deviceType=\"NoSuchType\" audioOutputDeviceName=\"nope\" "
        "audioInputDeviceName=\"\" audioDeviceRate=\"48000\" audioDeviceBufferSize=\"256\"/>");
    // En CI sin hardware: deviceError. Con hardware cae al dispositivo por defecto.
    CHECK(st.isOk() == engine->deviceStatus().open);
    if (!st.isOk())
        CHECK(st.code == ErrorCode::deviceError);
    engine->setProject(model::ProjectState{}, 2);
    (void)engine->takeMetrics();
    (void)engine->saveDeviceState();
    engine->shutdown();
}

TEST_CASE("initialise vacio no lanza y deja el motor utilizable") {
    JuceInit juceInit;
    auto engine = createAudioEngine();
    const auto st = engine->initialise("");
    CHECK(st.isOk() == engine->deviceStatus().open);
    engine->setProject(model::ProjectState{}, 1);
    (void)engine->takeMetrics();
    engine->shutdown();
}
