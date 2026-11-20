#pragma once
/// @file AppController.h
/// Raíz de composición: posee Document, CommandHistory y motor; implementa IAppActions, el
/// menú y los atajos. Hilo: mensajes.

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioSettingsDialog.h"
#include "MainWindow.h"
#include "app/AppServices.h"

namespace gp::app {

class AppController final : public IAppActions,
                            public juce::MenuBarModel,
                            public juce::ApplicationCommandTarget,
                            private model::Document::Listener,
                            private commands::CommandHistory::Listener {
public:
    AppController();
    ~AppController() override;

    /// Abre el motor (sin exigir dispositivo) y la ventana.
    void start(bool smokeTest);
    /// Cierre ordenado: pregunta por cambios sin guardar y luego sale de la app.
    void requestQuit();
    /// Cierra ventanas y motor (idempotente).
    void shutdown();

    AppServices& services() { return services_; }

    // IAppActions
    void newProject() override;
    void openProject() override;
    void saveProject() override;
    void saveProjectAs() override;
    void showAudioSettings() override;

    // MenuBarModel
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int index, const juce::String& name) override;
    void menuItemSelected(int, int) override {}

    // ApplicationCommandTarget
    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& info) override;
    bool perform(const InvocationInfo& info) override;

private:
    enum CommandIds {
        cmdNew = 0x2001, cmdOpen, cmdSave, cmdSaveAs, cmdAudioSettings, cmdQuit, cmdUndo, cmdRedo
    };

    void documentChanged(const model::Document&) override;
    void historyChanged(const commands::CommandHistory&) override;

    void updateTitle();
    std::string projectName() const;
    void confirmDiscardThen(std::function<void()> proceed);
    void saveThen(std::function<void()> onSaved);
    void chooseSavePath(std::function<void()> onSaved);
    bool writeTo(const std::filesystem::path& file);
    void showError(const char* titleKey, const Status& status);

    model::Document document_;
    commands::CommandHistory history_{document_};
    std::unique_ptr<engine::IAudioEngine> engine_;
    AppServices services_;

    juce::ApplicationCommandManager commandManager_;
    std::unique_ptr<juce::PropertiesFile> prefs_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<AudioSettingsDialog> audioSettings_;
    std::unique_ptr<juce::FileChooser> chooser_;
    std::filesystem::path currentFile_;
    bool started_ = false;
    bool smoke_ = false;
};

}  // namespace gp::app
