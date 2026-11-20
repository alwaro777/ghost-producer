#pragma once
/// @file Status.h
/// Resultado de operaciones que pueden fallar sin excepciones en la API pública.
/// Hilo: tipo de valor, sin estado compartido. No usar en el hilo de audio
/// (`message` puede reservar memoria).

#include <string>

namespace gp {

/// Categoría estable del error. La UI la traduce a un texto en español; `message`
/// es solo detalle técnico (inglés) para logs e informes.
enum class ErrorCode {
    none,
    invalidArgument,     ///< Parámetro fuera de rango o referencia inexistente (p. ej. TrackId).
    invalidState,        ///< La operación no aplica en el estado actual.
    ioError,             ///< Error de disco (permisos, espacio, ruta).
    parseError,          ///< JSON mal formado o con tipos incorrectos.
    unsupportedVersion,  ///< schemaVersion más nueva que la que entiende esta build.
    migrationFailed,     ///< Una migración de esquema devolvió error.
    unknownCommand,      ///< Tipo de Command no registrado.
    deviceError,         ///< No se pudo abrir o configurar el dispositivo de audio/MIDI.
};

/// Éxito o error con detalle. Invariante: `code == ErrorCode::none` ⇔ éxito.
struct Status {
    ErrorCode code = ErrorCode::none;
    std::string message;  ///< Vacío si es éxito.

    [[nodiscard]] bool isOk() const noexcept { return code == ErrorCode::none; }
    [[nodiscard]] static Status ok() { return {}; }
    [[nodiscard]] static Status error(ErrorCode c, std::string msg) { return {c, std::move(msg)}; }
};

}  // namespace gp
