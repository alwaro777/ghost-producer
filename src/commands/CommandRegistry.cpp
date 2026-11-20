#include "commands/CommandRegistry.h"

#include <cassert>
#include <nlohmann/json.hpp>

#include "ProjectCommandsInternal.h"

namespace gp::commands {

void CommandRegistry::add(std::string typeId, CommandFactory factory) {
    const bool inserted = factories_.emplace(std::move(typeId), std::move(factory)).second;
    assert(inserted && "typeId registrado dos veces");
    (void)inserted;
}

bool CommandRegistry::contains(std::string_view typeId) const {
    return factories_.find(typeId) != factories_.end();
}

std::unique_ptr<Command> CommandRegistry::create(const nlohmann::ordered_json& json,
                                                 Status& error) const {
    error = Status::ok();
    if (!json.is_object() || !json.contains("type") || !json["type"].is_string() ||
        !json.contains("version") || !json["version"].is_number_integer() ||
        !json.contains("params") || !json["params"].is_object()) {
        error = Status::error(ErrorCode::parseError,
                              "command JSON needs string 'type', integer 'version', object 'params'");
        return nullptr;
    }
    const auto& type = json["type"].get_ref<const std::string&>();
    const auto it = factories_.find(type);
    if (it == factories_.end()) {
        error = Status::error(ErrorCode::unknownCommand, "unknown command type: " + type);
        return nullptr;
    }
    if (json["version"].get<long long>() != 1) {
        error = Status::error(ErrorCode::unknownCommand, "unsupported version for " + type);
        return nullptr;
    }
    Status factoryError;
    auto cmd = it->second(json["params"], factoryError);
    if (!cmd) {
        error = factoryError.isOk()
                    ? Status::error(ErrorCode::invalidArgument, "factory failed for " + type)
                    : factoryError;
        return nullptr;
    }
    return cmd;
}

const CommandRegistry& builtinCommands() {
    static const CommandRegistry registry = [] {
        CommandRegistry r;
        registerProjectCommands(r);
        return r;
    }();
    return registry;
}

}  // namespace gp::commands
