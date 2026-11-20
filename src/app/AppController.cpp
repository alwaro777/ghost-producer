// AppController.cpp — composición, menús, archivos (dueño: ui-ux). Hilo: mensajes.
#include "AppController.h"

#include "PathUtils.h"

#include "commands/CommandHistory.h"
#include "model/ProjectFile.h"
#include "ui/Localisation.h"
#include "../ui/MainComponent.h"

namespace gp::app {
namespace {

juce::String t(std::string_view key) { return juce::String::fromUTF8(ui::tr(key).c_str()); }

const char* errorKey(ErrorCode code) {
    switch (code) {
        case ErrorCode::invalidState: return "error.invalidState";
        case ErrorCode::ioError: return "error.ioError";
        case ErrorCode::parseError: return "error.parseError";
        case ErrorCode::unsupportedVersion: return "error.unsupportedVersion";
        case ErrorCode::migrationFailed: return "error.migrationFailed";
        case ErrorCode::unknownCommand: return "error.unknownCommand";
        case ErrorCode::deviceError: return "error.deviceError";
        case ErrorCode::invalidArgument:
        case ErrorCode::none: break;
    }
    return "error.invalidArgument";
}


}  // namespace

AppController::AppController()
    : engine_(engine::createAudioEngine()),
      services_{document_, history_, *engine_, *this} {
    juce::PropertiesFile::Options options;
    options.applicationName = "GhostProducer";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "GhostProducer";
    prefs_ = std::make_unique<juce::PropertiesFile>(options);
    document_.addListener(this);
    history_.addListener(this);
}

AppController::~AppController() {
    shutdown();
    history_.removeListener(this);
    document_.removeListener(this);
}

void AppController::start(bool smokeTest) {
    smoke_ = smokeTest;
    // Sin dispositivo el motor sigue vivo (status.open=false): no se exige éxito.
    (void)engine_->initialise(smokeTest ? std::string{}
                                        : prefs_->getValue("audioDeviceState").toStdString());
    engine_->setProject(document_.state(), document_.revision());
    started_ = true;

    commandManager_.registerAllCommandsForTarget(this);
    commandManager_.setFirstCommandTarget(this);

    window_ = std::make_unique<MainWindow>(services_, [this] { requestQuit(); });
    window_->addKeyListener(commandManager_.getKeyMappings());
#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(this);
#else
    window_->setMenuBar(this);
#endif
    updateTitle();
    window_->toFront(true);
}

void AppController::shutdown() {
    if (!started_)
        return;
    started_ = false;
#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr);
#else
    if (window_ != nullptr)
        window_->setMenuBar(nullptr);
#endif
    audioSettings_.reset();
    window_.reset();  // destruye MainComponent (deja de observar el Document)
    if (!smoke_) {
        prefs_->setValue("audioDeviceState", juce::String(engine_->saveDeviceState()));
        prefs_->saveIfNeeded();
    }
    engine_->shutdown();
}

// --- Documento y título ---------------------------------------------------------------------
void AppController::documentChanged(const model::Document& doc) {
    engine_->setProject(doc.state(), doc.revision());
    updateTitle();
}

void AppController::historyChanged(const commands::CommandHistory&) {
    updateTitle();
    menuItemsChanged();
    commandManager_.commandStatusChanged();
}

std::string AppController::projectName() const {
    return currentFile_.empty() ? ui::tr("window.untitled") : paths::toUtf8(currentFile_.stem());
}

void AppController::updateTitle() {
    if (window_ == nullptr)
        return;
    const auto key = history_.isDirty() ? "window.title.modified" : "window.title";
    window_->setName(juce::String::fromUTF8(ui::tr(key, {projectName()}).c_str()));
}

// --- Diálogos asíncronos --------------------------------------------------------------------
void AppController::showError(const char* titleKey, const Status& status) {
    auto options = juce::MessageBoxOptions()
                       .withIconType(juce::MessageBoxIconType::WarningIcon)
                       .withTitle(t(titleKey))
                       .withMessage(t(errorKey(status.code)))
                       .withButton(t("error.dialog.ok"), 1);
    const bool hasDetails =
        status.code == ErrorCode::parseError || status.code == ErrorCode::migrationFailed;
    if (hasDetails)
        options = options.withButton(t("error.dialog.details"), 2);
    juce::AlertWindow::showAsync(options, [message = status.message, hasDetails](int result) {
        if (hasDetails && result == 2)
            juce::AlertWindow::showAsync(
                juce::MessageBoxOptions()
                    .withTitle(t("error.dialog.details"))
                    .withMessage(juce::String::fromUTF8(message.c_str()))
                    .withButton(t("error.dialog.ok"), 1),
                nullptr);
    });
}

void AppController::confirmDiscardThen(std::function<void()> proceed) {
    if (!history_.isDirty()) {
        proceed();
        return;
    }
    const auto options = juce::MessageBoxOptions()
                             .withIconType(juce::MessageBoxIconType::QuestionIcon)
                             .withTitle(juce::String::fromUTF8(
                                 ui::tr("dialog.unsaved.title", {projectName()}).c_str()))
                             .withMessage(t("dialog.unsaved.message"))
                             .withButton(t("dialog.unsaved.save"), 1)
                             .withButton(t("dialog.unsaved.discard"), 2)
                             .withButton(t("dialog.unsaved.cancel"), 0)
                             .withAssociatedComponent(window_.get());
    juce::AlertWindow::showAsync(options, [this, proceed = std::move(proceed)](int result) {
        if (result == 1)
            saveThen(proceed);  // si guardar falla o se cancela, no continúa
        else if (result == 2)
            proceed();
    });
}

// --- Archivo --------------------------------------------------------------------------------
bool AppController::writeTo(const std::filesystem::path& file) {
    const auto status = model::saveProject(document_.state(), file, GP_APP_VERSION);
    if (!status.isOk()) {
        showError("error.dialog.saveTitle", status);
        return false;
    }
    currentFile_ = file;
    history_.markSaved();
    prefs_->setValue("lastProject", paths::toJuceString(file));
    prefs_->saveIfNeeded();
    updateTitle();
    return true;
}

void AppController::chooseSavePath(std::function<void()> onSaved) {
    const auto base = currentFile_.empty() ? std::filesystem::path{}
                                           : currentFile_.parent_path().parent_path();
    const juce::File initial =
        currentFile_.empty()
            ? juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                  .getChildFile(juce::String::fromUTF8(projectName().c_str()))
            : paths::toFile(base / currentFile_.stem());
    chooser_ = std::make_unique<juce::FileChooser>(t("menu.file.saveAs"), initial, "*.gpproj");
    chooser_->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, onSaved = std::move(onSaved)](const juce::FileChooser& fc) {
            const auto chosen = fc.getResult();
            if (chosen == juce::File())
                return;  // cancelado
            // ADR 0002: X/Prueba/Prueba.gpproj (las subcarpetas cuelgan de X/Prueba).
            const auto target = paths::projectFileFor(paths::toPath(chosen));
            std::error_code ec;
            const bool folderExists = std::filesystem::exists(target.parent_path(), ec);
            if (folderExists && target != currentFile_) {
                const auto options =
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::WarningIcon)
                        .withTitle(juce::String::fromUTF8(
                            ui::tr("dialog.overwrite.title",
                                   {paths::toUtf8(target.parent_path().filename())})
                                .c_str()))
                        .withMessage(t("dialog.overwrite.message"))
                        .withButton(t("dialog.overwrite.replace"), 1)
                        .withButton(t("dialog.overwrite.cancel"), 0)
                        .withAssociatedComponent(window_.get());
                juce::AlertWindow::showAsync(options, [this, target, onSaved](int result) {
                    if (result == 1 && writeTo(target) && onSaved)
                        onSaved();
                });
                return;
            }
            if (writeTo(target) && onSaved)
                onSaved();
        });
}

void AppController::saveThen(std::function<void()> onSaved) {
    if (currentFile_.empty()) {
        chooseSavePath(std::move(onSaved));
        return;
    }
    if (writeTo(currentFile_) && onSaved)
        onSaved();
}

void AppController::newProject() {
    confirmDiscardThen([this] {
        document_.reset(model::ProjectState{});
        history_.clear();
        currentFile_.clear();
        updateTitle();
    });
}

void AppController::openProject() {
    confirmDiscardThen([this] {
        chooser_ = std::make_unique<juce::FileChooser>(
            t("menu.file.open"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.gpproj");
        chooser_->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                const auto file = fc.getResult();
                if (file == juce::File())
                    return;
                auto loaded = model::loadProject(paths::toPath(file));
                if (!loaded.status.isOk()) {
                    showError("error.dialog.openTitle", loaded.status);  // el proyecto no cambia
                    return;
                }
                const bool migrated = loaded.migrated;
                const int originalVersion = loaded.originalSchemaVersion;
                document_.reset(std::move(loaded.state));
                history_.clear();
                currentFile_ = paths::toPath(file);
                prefs_->setValue("lastProject", paths::toJuceString(currentFile_));
                prefs_->saveIfNeeded();
                updateTitle();
                if (migrated && window_ != nullptr)
                    window_->showNotice(juce::String::fromUTF8(
                        ui::tr("notice.migrated",
                               {paths::toUtf8(paths::migrationBackupFor(currentFile_, originalVersion))})
                            .c_str()));
            });
    });
}

void AppController::saveProject() { saveThen({}); }

void AppController::saveProjectAs() { chooseSavePath({}); }

void AppController::showAudioSettings() {
    if (audioSettings_ != nullptr) {
        audioSettings_->toFront(true);
        return;
    }
    audioSettings_ = std::make_unique<AudioSettingsDialog>(
        engine_->deviceManagerForSettingsUI(), [this] {
            juce::MessageManager::callAsync([safe = juce::Component::SafePointer<juce::Component>(
                                                 audioSettings_.get()),
                                             this] {
                if (safe != nullptr)
                    audioSettings_.reset();
            });
        });
}

void AppController::requestQuit() {
    confirmDiscardThen([] { juce::JUCEApplicationBase::quit(); });
}

// --- Menús y atajos -------------------------------------------------------------------------
juce::StringArray AppController::getMenuBarNames() {
    return {t("menu.file"), t("menu.edit")};
}

juce::PopupMenu AppController::getMenuForIndex(int index, const juce::String&) {
    juce::PopupMenu menu;
    if (index == 0) {
        for (auto id : {cmdNew, cmdOpen, cmdSave, cmdSaveAs})
            menu.addCommandItem(&commandManager_, id);
        menu.addSeparator();
        menu.addCommandItem(&commandManager_, cmdAudioSettings);
#if !JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem(&commandManager_, cmdQuit);
#endif
    } else {
        menu.addCommandItem(&commandManager_, cmdUndo);
        menu.addCommandItem(&commandManager_, cmdRedo);
    }
    return menu;
}

void AppController::getAllCommands(juce::Array<juce::CommandID>& commands) {
    commands.addArray({cmdNew, cmdOpen, cmdSave, cmdSaveAs, cmdAudioSettings, cmdQuit, cmdUndo, cmdRedo});
}

void AppController::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& info) {
    constexpr auto cmd = juce::ModifierKeys::commandModifier;
    constexpr auto shift = juce::ModifierKeys::shiftModifier;
    const auto group = t("menu.file");
    switch (id) {
        case cmdNew:
            info.setInfo(t("menu.file.new"), t("menu.file.new"), group, 0);
            info.addDefaultKeypress('N', cmd);
            break;
        case cmdOpen:
            info.setInfo(t("menu.file.open"), t("menu.file.open"), group, 0);
            info.addDefaultKeypress('O', cmd);
            break;
        case cmdSave:
            info.setInfo(t("menu.file.save"), t("menu.file.save"), group, 0);
            info.addDefaultKeypress('S', cmd);
            break;
        case cmdSaveAs:
            info.setInfo(t("menu.file.saveAs"), t("menu.file.saveAs"), group, 0);
            info.addDefaultKeypress('S', cmd | shift);
            break;
        case cmdAudioSettings:
            info.setInfo(t("menu.file.audioSettings"), t("menu.file.audioSettings"), group, 0);
            info.addDefaultKeypress(',', cmd);
            break;
        case cmdQuit:
            info.setInfo(t("menu.file.quit"), t("menu.file.quit"), group, 0);
            info.addDefaultKeypress('Q', cmd);
            break;
        case cmdUndo: {
            const auto text = ui::MainComponent::undoMenuText(history_);
            const auto label = text.empty() ? t("menu.edit.undo") : juce::String::fromUTF8(text.c_str());
            info.setInfo(label, label, t("menu.edit"), 0);
            info.setActive(history_.canUndo());
            info.addDefaultKeypress('Z', cmd);
            break;
        }
        case cmdRedo: {
            const auto text = ui::MainComponent::redoMenuText(history_);
            const auto label = text.empty() ? t("menu.edit.redo") : juce::String::fromUTF8(text.c_str());
            info.setInfo(label, label, t("menu.edit"), 0);
            info.setActive(history_.canRedo());
            info.addDefaultKeypress('Z', cmd | shift);
#if !JUCE_MAC
            info.addDefaultKeypress('Y', cmd);
#endif
            break;
        }
        default: break;
    }
}

bool AppController::perform(const InvocationInfo& info) {
    switch (info.commandID) {
        case cmdNew: newProject(); return true;
        case cmdOpen: openProject(); return true;
        case cmdSave: saveProject(); return true;
        case cmdSaveAs: saveProjectAs(); return true;
        case cmdAudioSettings: showAudioSettings(); return true;
        case cmdQuit: requestQuit(); return true;
        case cmdUndo: history_.undo(); return true;
        case cmdRedo: history_.redo(); return true;
        default: return false;
    }
}

}  // namespace gp::app
