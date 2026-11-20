#pragma once
/// @file AppServices.h
/// Raíz de composición que la app (src/app) entrega a la UI (src/ui). La UI no crea
/// servicios: los recibe por aquí. Hilo: hilo de mensajes. Las referencias viven mientras
/// exista la ventana principal.

#include "commands/CommandHistory.h"
#include "engine/AudioEngine.h"
#include "model/Document.h"

namespace gp::app {

/// Acciones de nivel de app que la UI puede pedir (menús, atajos, botones). Las implementa
/// src/app. Ninguna se llama desde el hilo de audio.
class IAppActions {
public:
    virtual ~IAppActions() = default;
    virtual void newProject() = 0;   ///< Pregunta si hay cambios sin guardar.
    virtual void openProject() = 0;  ///< Diálogo de archivo asíncrono.
    virtual void saveProject() = 0;  ///< "Guardar como" si aún no tiene ruta.
    virtual void saveProjectAs() = 0;
    virtual void showAudioSettings() = 0;
};

struct AppServices {
    model::Document& document;          ///< Solo lectura para la UI (state(), listeners).
    commands::CommandHistory& history;  ///< Única vía de escritura: execute/undo/redo.
    engine::IAudioEngine& engine;       ///< Solo deviceStatus/takeMetrics/ajustes.
    IAppActions& actions;
};

}  // namespace gp::app
