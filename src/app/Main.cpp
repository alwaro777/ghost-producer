// Main.cpp — GhostProducerApp (dueño: ui-ux). Hilo: mensajes.
#include <cstring>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "AppController.h"
#include "commands/ProjectCommands.h"
#include "ui/Localisation.h"

namespace gp::app {

class GhostProducerApplication final : public juce::JUCEApplication, private juce::Timer {
public:
    const juce::String getApplicationName() override { return "GhostProducer"; }
    const juce::String getApplicationVersion() override { return GP_APP_VERSION; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override {
        ui::loadLanguage("es");
        smoke_ = commandLine.contains("--smoke-test");
        controller_ = std::make_unique<AppController>();
        controller_->start(smoke_);
        if (smoke_) {
            stage_ = 0;
            startTimer(1000);
        }
    }

    void shutdown() override {
        stopTimer();
        controller_.reset();
    }

    void systemRequestedQuit() override {
        if (controller_ != nullptr && !smoke_)
            controller_->requestQuit();
        else
            quit();
    }

private:
    // Smoke test: t=1 s ejecuta 3 comandos + undo; t=3 s comprueba y sale (0, o 1 si hay fallo).
    void timerCallback() override {
        auto& s = controller_->services();
        if (stage_ == 0) {
            const bool ok = s.history.execute(commands::makeSetTempo(140.0)).isOk() &&
                            s.history.execute(commands::makeSetTestToneEnabled(true)).isOk() &&
                            s.history.execute(commands::makeAddTrack("Smoke")).isOk() && s.history.undo() &&
                            s.document.state().tracks.empty() &&
                            s.document.state().transport.tempoBpm == 140.0;
            if (!ok)
                failed_ = true;
            stage_ = 1;
            startTimer(2000);
            return;
        }
        const auto metrics = s.engine.takeMetrics();
        if (metrics.rtViolationCount > 0)
            failed_ = true;
        stopTimer();
        setApplicationReturnValue(failed_ ? 1 : 0);
        quit();
    }

    std::unique_ptr<AppController> controller_;
    bool smoke_ = false;
    bool failed_ = false;
    int stage_ = 0;
};

}  // namespace gp::app

START_JUCE_APPLICATION(gp::app::GhostProducerApplication)
