#pragma once
/// @file CommandHistory.h
/// Pilas de undo/redo sobre un model::Document (ADR 0003). Único punto de mutación del
/// documento. Hilo: SOLO hilo de mensajes.
///
/// Invariantes:
///  - execute() con éxito vacía la pila de redo.
///  - Cada entrada de undo es un Command o una transacción (lista de Commands aplicada y
///    deshecha como un solo paso, en orden inverso al deshacer).
///  - Tras cada cambio de estado se llama a Document::notifyChanged() una vez por paso
///    (una vez por transacción completa, no por Command interno).
///  - Undo ilimitado en F0 (sin tope); el tope por memoria es un punto de extensión.

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "commands/Command.h"
#include "core/Status.h"

namespace gp::model {
class Document;
}

namespace gp::commands {

enum class MergePolicy {
    never,         ///< Nueva entrada de undo (por defecto).
    withPrevious,  ///< Intenta fusionar con la cima (gesto continuo). Ver Command::mergeWith.
};

/// Resumen de una entrada para el historial visual.
struct HistoryEntryInfo {
    std::string descriptionKey;  ///< Del Command, o la de la transacción.
    std::size_t commandCount = 1;
};

class CommandHistory {
public:
    /// Observador: se notifica tras execute/undo/redo/clear/markSaved (hilo de mensajes).
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void historyChanged(const CommandHistory& history) = 0;
    };

    /// `document` debe sobrevivir al CommandHistory.
    explicit CommandHistory(model::Document& document);
    ~CommandHistory();
    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;

    /// Aplica `command`. Si falla, el documento no cambia, no se apila nada y se devuelve
    /// el error. Dentro de una transacción abierta, el Command se añade a ella.
    [[nodiscard]] Status execute(std::unique_ptr<Command> command,
                                 MergePolicy merge = MergePolicy::never);

    [[nodiscard]] bool canUndo() const noexcept;
    [[nodiscard]] bool canRedo() const noexcept;
    /// Devuelven false si no había nada que deshacer/rehacer. No válidos con una
    /// transacción abierta (assert en debug; devuelven false).
    bool undo();
    bool redo();

    /// Clave i18n de lo que se desharía/reharía; vacía si no hay.
    [[nodiscard]] std::string undoDescriptionKey() const;
    [[nodiscard]] std::string redoDescriptionKey() const;

    /// Transacción: agrupa varios execute() en un único paso de undo (acciones de la IA,
    /// operaciones compuestas). Sin anidamiento en F0. commit de una transacción vacía no
    /// apila nada. abort revierte, en orden inverso, lo aplicado dentro de ella.
    void beginTransaction(std::string descriptionKey);
    void commitTransaction();
    void abortTransaction();
    [[nodiscard]] bool inTransaction() const noexcept;

    /// Vacía ambas pilas (Nuevo/Abrir). Deja el estado como "guardado".
    void clear();

    /// Marca la posición actual como la guardada en disco.
    void markSaved();
    /// true si la posición actual difiere de la guardada (incluye haber deshecho más allá
    /// del punto guardado). Una fusión sobre la entrada guardada también ensucia.
    [[nodiscard]] bool isDirty() const noexcept;

    [[nodiscard]] std::vector<HistoryEntryInfo> undoEntries() const;  ///< Más antigua primero.
    [[nodiscard]] std::size_t undoCount() const noexcept;
    [[nodiscard]] std::size_t redoCount() const noexcept;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace gp::commands
