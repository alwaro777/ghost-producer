#pragma once
/// Ventana de ajustes de audio (AudioDeviceSelectorComponent sobre el AudioDeviceManager del
/// motor; excepción documentada en ADR 0004). Hilo: mensajes.

#include <functional>

#include <juce_audio_utils/juce_audio_utils.h>

namespace gp::app {

class AudioSettingsDialog final : public juce::DocumentWindow {
public:
    AudioSettingsDialog(juce::AudioDeviceManager& manager, std::function<void()> onClosed);
    void closeButtonPressed() override;

private:
    std::function<void()> onClosed_;
};

}  // namespace gp::app
