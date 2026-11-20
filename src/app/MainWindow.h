#pragma once
/// Ventana principal: aloja ui::MainComponent. Hilo: mensajes.

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "app/AppServices.h"
#include "../ui/MainComponent.h"
#include "../ui/Theme.h"

namespace gp::app {

class MainWindow final : public juce::DocumentWindow {
public:
    MainWindow(AppServices& services, std::function<void()> onCloseRequested)
        : juce::DocumentWindow({}, ui::theme::colour::background, juce::DocumentWindow::allButtons),
          onCloseRequested_(std::move(onCloseRequested)) {
        setUsingNativeTitleBar(true);
        setContentOwned(new ui::MainComponent(services), false);
        setResizable(true, false);
        setResizeLimits(720, 560, 4000, 3000);
        centreWithSize(920, 700);
        setVisible(true);
    }

    /// Aviso no modal (p. ej. proyecto migrado).
    void showNotice(const juce::String& text) {
        if (auto* main = dynamic_cast<ui::MainComponent*>(getContentComponent()))
            main->showNotice(text);
    }

    void closeButtonPressed() override {
        if (onCloseRequested_)
            onCloseRequested_();
    }

private:
    std::function<void()> onCloseRequested_;
};

}  // namespace gp::app
