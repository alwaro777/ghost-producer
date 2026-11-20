#pragma once
/// @file CommandRegistry.h
/// Reconstrucción de Commands desde JSON (macros, log de acciones, copiloto IA, futuro
/// historial persistente). Hilo: cualquiera salvo audio; registrar solo al arrancar.

#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

#include "commands/Command.h"
#include "core/Status.h"

namespace gp::commands {

/// Construye un Command desde "params". Devuelve nullptr y rellena `error` si los
/// parámetros son inválidos (tipos o rangos).
using CommandFactory =
    std::function<std::unique_ptr<Command>(const nlohmann::ordered_json& params, Status& error)>;

class CommandRegistry {
public:
    /// Registrar dos veces el mismo typeId es un error de programación (assert en debug).
    void add(std::string typeId, CommandFactory factory);

    /// Espera {"type", "version", "params"}. Errores: parseError (estructura),
    /// unknownCommand (tipo no registrado o versión no soportada), invalidArgument.
    [[nodiscard]] std::unique_ptr<Command> create(const nlohmann::ordered_json& json,
                                                  Status& error) const;

    [[nodiscard]] bool contains(std::string_view typeId) const;

private:
    std::map<std::string, CommandFactory, std::less<>> factories_;
};

/// Registro con todos los comandos de ProjectCommands.h.
[[nodiscard]] const CommandRegistry& builtinCommands();

}  // namespace gp::commands
