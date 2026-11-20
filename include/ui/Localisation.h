#pragma once
/// @file Localisation.h
/// Textos de UI por clave (i18n preparado). Catálogo: assets/i18n/<idioma>.json, objeto
/// plano {"clave": "texto"} en UTF-8, propiedad de producto-ux. F0 solo carga "es".
/// Hilo: solo hilo de mensajes. No usar en el hilo de audio.
///
/// Convención de claves: "<área>.<elemento>[.<variante>]", p. ej. "menu.file.save",
/// "cmd.transport.setTempo", "error.deviceError". Los textos admiten marcadores {0}, {1}.

#include <string>
#include <string_view>
#include <vector>

namespace gp::ui {

/// Carga el catálogo de `languageCode` (embebido en el binario). Llamar una vez al arrancar.
/// Si falla, tr() devuelve las claves.
bool loadLanguage(std::string_view languageCode);

/// Texto de `key`; si no existe, devuelve la propia clave (y en debug lo avisa una vez).
[[nodiscard]] std::string tr(std::string_view key);

/// tr() sustituyendo {0}, {1}… por `args` en orden.
[[nodiscard]] std::string tr(std::string_view key, const std::vector<std::string>& args);

}  // namespace gp::ui
