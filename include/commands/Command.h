#pragma once
/// @file Command.h
/// Unidad de cambio del documento (ADR 0003). Todo cambio del usuario o de la IA es un
/// Command: deshacible, rehacible y serializable a JSON.
///
/// Hilo: SOLO hilo de mensajes (lo ejecuta commands::CommandHistory). La IA y cualquier
/// hilo de fondo construyen Commands y los envían al hilo de mensajes para ejecutarlos.
///
/// Contrato de implementación:
///  - apply(): valida y aplica. Si devuelve error, `state` queda EXACTAMENTE igual.
///    Guarda en el propio objeto lo necesario para revert() (valor anterior, índice…).
///  - revert(): solo se llama tras un apply() correcto y con el estado tal como lo dejó
///    ese apply(). Restaura el estado previo exacto (operator== verdadero).
///  - apply() tras revert() (redo) reproduce el mismo estado que el primer apply(), incluidos
///    los identificadores asignados (p. ej. el TrackId de una pista añadida).
///  - Sin efectos fuera de `state` (nada de disco, red, motor ni UI).

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

#include "core/Status.h"
#include "model/ProjectState.h"

namespace gp::commands {

class Command {
public:
    virtual ~Command() = default;

    /// Identificador estable de tipo, p. ej. "transport.setTempo". Forma parte del formato
    /// serializado: no se renombra nunca (si cambia la semántica, se crea un tipo nuevo).
    [[nodiscard]] virtual std::string_view typeId() const noexcept = 0;

    /// Clave i18n de la descripción para el historial ("cmd.transport.setTempo"). La UI la
    /// traduce con ui::tr(). No contiene texto visible.
    [[nodiscard]] virtual std::string descriptionKey() const = 0;

    [[nodiscard]] virtual Status apply(model::ProjectState& state) = 0;
    virtual void revert(model::ProjectState& state) = 0;

    /// Parámetros del comando (no los datos de undo), suficientes para reconstruirlo con
    /// CommandRegistry::create(). Formato: {"type": typeId, "version": 1, "params": {...}}.
    /// Si el comando ya asignó identificadores en apply(), se incluyen en params.
    [[nodiscard]] virtual nlohmann::ordered_json toJson() const = 0;

    /// Fusión de gestos continuos (arrastrar un slider). CommandHistory la invoca solo si se
    /// ejecutó `next` con MergePolicy::withPrevious y `this` es la cima de la pila de undo.
    /// Si devuelve true, `this` absorbe el valor final de `next` conservando su propio dato
    /// de undo (el valor de antes del gesto) y `next` se descarta. Por defecto: false.
    [[nodiscard]] virtual bool mergeWith(const Command& next) {
        (void)next;
        return false;
    }
};

}  // namespace gp::commands
