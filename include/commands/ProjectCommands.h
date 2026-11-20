#pragma once
/// @file ProjectCommands.h
/// Comandos de F0. Cada fábrica devuelve un Command no aplicado; la validación de rangos
/// (model::limits) ocurre en apply(). Hilo: construir en cualquier hilo salvo audio;
/// ejecutar solo vía CommandHistory (hilo de mensajes).
///
/// typeId (estable) | descriptionKey             | mergeWith
/// transport.setTempo         | cmd.transport.setTempo         | sí (mismo tipo)
/// transport.setTimeSignature | cmd.transport.setTimeSignature | no
/// testTone.setEnabled        | cmd.testTone.setEnabled        | no
/// testTone.setFrequency      | cmd.testTone.setFrequency      | sí (mismo tipo)
/// testTone.setGain           | cmd.testTone.setGain           | sí (mismo tipo)
/// track.add                  | cmd.track.add                  | no
/// track.remove               | cmd.track.remove               | no
/// track.rename               | cmd.track.rename               | sí (misma pista)

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "commands/Command.h"
#include "model/ProjectState.h"

namespace gp::commands {

[[nodiscard]] std::unique_ptr<Command> makeSetTempo(double bpm);
[[nodiscard]] std::unique_ptr<Command> makeSetTimeSignature(int numerator, int denominator);

[[nodiscard]] std::unique_ptr<Command> makeSetTestToneEnabled(bool enabled);
[[nodiscard]] std::unique_ptr<Command> makeSetTestToneFrequency(double frequencyHz);
[[nodiscard]] std::unique_ptr<Command> makeSetTestToneGain(float gainDb);

/// Inserta una pista en `index` (por defecto, al final; index > size ⇒ invalidArgument).
/// El TrackId se toma de state.nextTrackId en el primer apply() y se reutiliza en redo.
[[nodiscard]] std::unique_ptr<Command> makeAddTrack(std::string name,
                                                    std::optional<std::size_t> index = {});
/// revert() reinserta la pista en su índice original con el mismo TrackId.
[[nodiscard]] std::unique_ptr<Command> makeRemoveTrack(model::TrackId id);
[[nodiscard]] std::unique_ptr<Command> makeRenameTrack(model::TrackId id, std::string newName);

}  // namespace gp::commands
