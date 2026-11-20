// Localisation.cpp — catálogo i18n embebido (dueño: ui-ux). Hilo: solo hilo de mensajes.
#include "ui/Localisation.h"

#include <BinaryData.h>

#include <cstdio>
#include <set>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace gp::ui {
namespace {

std::unordered_map<std::string, std::string>& catalog() {
    static std::unordered_map<std::string, std::string> instance;
    return instance;
}

void warnMissingOnce([[maybe_unused]] const std::string& key) {
#ifndef NDEBUG
    static std::set<std::string> warned;
    if (warned.insert(key).second)
        std::fprintf(stderr, "[i18n] missing key: %s\n", key.c_str());
#endif
}

}  // namespace

bool loadLanguage(std::string_view languageCode) {
    const std::string resourceName = std::string(languageCode) + "_json";
    int size = 0;
    const char* data = GhostAssets::getNamedResource(resourceName.c_str(), size);
    if (data == nullptr || size <= 0)
        return false;

    auto parsed = nlohmann::json::parse(data, data + size, nullptr, /*allow_exceptions*/ false);
    if (parsed.is_discarded() || !parsed.is_object())
        return false;

    std::unordered_map<std::string, std::string> loaded;
    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        if (!it.value().is_string())
            return false;
        loaded.emplace(it.key(), it.value().get<std::string>());
    }
    catalog() = std::move(loaded);
    return true;
}

std::string tr(std::string_view key) {
    const auto it = catalog().find(std::string(key));
    if (it == catalog().end()) {
        warnMissingOnce(std::string(key));
        return std::string(key);
    }
    return it->second;
}

std::string tr(std::string_view key, const std::vector<std::string>& args) {
    const std::string text = tr(key);
    std::string out;
    out.reserve(text.size() + 16);
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '{') {
            std::size_t j = i + 1;
            std::size_t index = 0;
            bool digits = false;
            while (j < text.size() && text[j] >= '0' && text[j] <= '9') {
                index = index * 10 + static_cast<std::size_t>(text[j] - '0');
                digits = true;
                ++j;
            }
            if (digits && j < text.size() && text[j] == '}' && index < args.size()) {
                out += args[index];
                i = j;
                continue;
            }
        }
        out += text[i];
    }
    return out;
}

}  // namespace gp::ui
