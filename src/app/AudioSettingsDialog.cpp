// AudioSettingsDialog.cpp (dueño: ui-ux)
#include "AudioSettingsDialog.h"

#include "ui/Localisation.h"
#include "../ui/Theme.h"

namespace gp::app {

AudioSettingsDialog::AudioSettingsDialog(juce::AudioDeviceManager& manager,
                                         std::function<void()> onClosed)
    : juce::DocumentWindow(juce::String::fromUTF8(ui::tr("audio.settings.title").c_str()),
                           ui::theme::colour::background, juce::DocumentWindow::closeButton),
      onClosed_(std::move(onClosed)) {
    setUsingNativeTitleBar(true);
    auto* selector = new juce::AudioDeviceSelectorComponent(
        manager, /*minIn*/ 0, /*maxIn*/ 0, /*minOut*/ 1, /*maxOut*/ 2,
        /*showMidiInputOptions*/ true, /*showMidiOutputSelector*/ false,
        /*stereoPairs*/ true, /*hideAdvanced*/ false);
    selector->setSize(500, 480);
    setContentOwned(selector, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void AudioSettingsDialog::closeButtonPressed() {
    if (onClosed_)
        onClosed_();  // el dueño nos destruye (en diferido)
}

}  // namespace gp::app
