#pragma once
/// @file Document.h
/// Documento del proyecto abierto: dueño del ProjectState vivo y notificador de cambios.
///
/// Hilo: SOLO hilo de mensajes (todas las funciones). En debug, la implementación
/// comprueba que se llama siempre desde el hilo que construyó el Document.
///
/// Invariante: el estado solo cambia por (a) commands::CommandHistory al ejecutar,
/// deshacer o rehacer un Command, o (b) reset() al crear/abrir proyecto. Nadie más tiene
/// acceso mutable. La UI y la IA leen con state() y escriben emitiendo Commands.

#include <cstdint>
#include <memory>

#include "model/ProjectState.h"

namespace gp::commands {
class CommandHistory;
}

namespace gp::model {

class Document {
public:
    /// Observador de cambios. Se notifica de forma síncrona en el hilo de mensajes, después de
    /// que el cambio esté aplicado. No debe ejecutar Commands desde la notificación.
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void documentChanged(const Document& document) = 0;
    };

    explicit Document(ProjectState initial = {});
    ~Document();
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    /// Estado actual. La referencia es válida hasta el siguiente cambio.
    [[nodiscard]] const ProjectState& state() const noexcept;

    /// Contador monotónico: +1 en cada cambio (execute, undo, redo, reset). Sirve para
    /// etiquetar snapshots del motor y detectar estado obsoleto.
    [[nodiscard]] std::uint64_t revision() const noexcept;

    /// Sustituye todo el estado (Nuevo/Abrir). No es deshacible: quien llama debe limpiar
    /// el CommandHistory asociado. Precondición: isValid(newState).
    void reset(ProjectState newState);

    /// Los listeners no son propiedad del Document; quitar antes de destruirlos.
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    friend class gp::commands::CommandHistory;
    /// Acceso mutable reservado a CommandHistory; tras mutar debe llamar a notifyChanged().
    ProjectState& mutableState() noexcept;
    void notifyChanged();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace gp::model
