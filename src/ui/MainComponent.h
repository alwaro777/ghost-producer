#pragma once
/// @file MainComponent.h
/// Panel principal de F0: tiempo, tono de prueba, pistas y barra de estado. Observa el
/// Document y emite Commands vía CommandHistory; del motor solo lee deviceStatus() y
/// takeMetrics(). Hilo: mensajes.

#include <memory>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "app/AppServices.h"
#include "commands/ProjectCommands.h"
#include "model/Document.h"
#include "ui/TrackListComponent.h"

namespace gp::ui {

class MainComponent final : public juce::Component,
                            private model::Document::Listener,
                            private juce::Timer {
public:
    explicit MainComponent(app::AppServices& services);
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Texto de los ítems de menú. {0} recibe tr(cmd.*) del último cambio; vacío si no hay
    /// historial (la app deshabilita el ítem y usa menu.edit.undo / menu.edit.redo).
    [[nodiscard]] static std::string undoMenuText(const commands::CommandHistory& history);
    [[nodiscard]] static std::string redoMenuText(const commands::CommandHistory& history);

private:
    // Medidor L/R con caída; solo se repinta cuando cambia el valor mostrado.
    class Meter final : public juce::Component {
    public:
        explicit Meter(juce::String accessibleName);
        /// Devuelve true si el valor visible cambió.
        bool push(float peak);
        float levelDb() const;
        void paint(juce::Graphics& g) override;

    private:
        float level_ = 0.0f;
    };

    // Gesto de arrastre: el primer cambio abre un paso de undo, los siguientes se fusionan.
    struct Gesture {
        bool dragging = false;
        bool first = true;
        commands::MergePolicy next() {
            const auto policy = (dragging && !first) ? commands::MergePolicy::withPrevious
                                                     : commands::MergePolicy::never;
            first = false;
            return policy;
        }
        void begin() { dragging = true; first = true; }
        void end() { dragging = false; first = true; }
    };

    void documentChanged(const model::Document& document) override;
    void timerCallback() override;

    void refreshFromDocument();
    void refreshStatus(const engine::AudioDeviceStatus& device, const engine::EngineMetrics& m);
    void run(std::unique_ptr<commands::Command> command, commands::MergePolicy merge,
             juce::Label& errorLabel);
    void applyTempoText();
    void applyTimeSignature(int numerator, int denominator);
    void applyNumeratorText();
    void updateBanner();
    void paintOverChildren(juce::Graphics& g) override;
    void focusOfChildComponentChanged(FocusChangeType) override { repaint(); }

    app::AppServices& services_;
    juce::Rectangle<int> transportPanel_, tonePanel_, tracksPanel_;

    juce::Label transportTitle_, tempoName_, tempoUnit_, timeSigName_, timeSigSlash_, transportError_;
    juce::Label tempoValue_, timeSigNumerator_;
    juce::ComboBox timeSigDenominator_;

    juce::Label toneTitle_, freqName_, gainName_, toneError_;
    juce::ToggleButton toneEnable_;
    juce::Slider freqSlider_, gainSlider_;
    Gesture freqGesture_, gainGesture_;

    juce::Label tracksTitle_, tracksError_;
    juce::TextButton addTrack_;
    TrackListComponent trackList_;
    juce::Viewport trackViewport_;

    juce::Label bannerTitle_, bannerMessage_;
    juce::TextButton bannerSettings_, bannerDismiss_;
    bool bannerDismissed_ = false;
    bool bannerShown_ = false;

    juce::Label deviceLabel_, cpuLabel_, peakLabel_, midiLabel_, rtLabel_;
    Meter meterL_, meterR_;
    float peakHoldDb_ = -100.0f;
    int tick_ = 0;
    bool deviceOpen_ = false;

    juce::TooltipWindow tooltip_{this};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}  // namespace gp::ui
