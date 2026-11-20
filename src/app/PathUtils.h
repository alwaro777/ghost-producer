#pragma once
/// @file PathUtils.h
/// Conversión juce <-> std::filesystem::path SIEMPRE vía UTF-8 explícito (en MSVC,
/// path(std::string) y path::string() usan la página ANSI y corrompen/lanzan con Unicode), y
/// reglas de la carpeta de proyecto (ADR 0002). Hilo: cualquiera salvo audio.

#include <filesystem>
#include <string>

#include <juce_core/juce_core.h>

namespace gp::app::paths {

[[nodiscard]] inline std::filesystem::path toPath(const juce::String& text) {
    const auto* utf8 = text.toRawUTF8();
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8),
                                               static_cast<std::size_t>(text.getNumBytesAsUTF8())));
}

[[nodiscard]] inline std::filesystem::path toPath(const juce::File& file) {
    return toPath(file.getFullPathName());
}

[[nodiscard]] inline juce::String toJuceString(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return juce::String::fromUTF8(reinterpret_cast<const char*>(u8.data()),
                                  static_cast<int>(u8.size()));
}

[[nodiscard]] inline juce::File toFile(const std::filesystem::path& path) {
    return juce::File(toJuceString(path));
}

/// UTF-8 en std::string (para textos de UI, no para rutas).
[[nodiscard]] inline std::string toUtf8(const std::filesystem::path& path) {
    return toJuceString(path).toStdString();
}

/// Archivo .gpproj definitivo para lo que eligió el usuario en «Guardar como»:
/// `X/Prueba` o `X/Prueba.gpproj` -> `X/Prueba/Prueba.gpproj`. Si ya está dentro de una carpeta
/// con el mismo nombre (`X/Prueba/Prueba.gpproj`), se respeta.
[[nodiscard]] inline std::filesystem::path projectFileFor(const std::filesystem::path& chosen) {
    namespace fs = std::filesystem;
    fs::path name = chosen.filename();
    if (name.extension() == ".gpproj")
        name = name.stem();
    const fs::path parent = chosen.parent_path();
    fs::path file = name;
    file += ".gpproj";
    if (parent.filename() == name)
        return parent / file;
    return parent / name / file;
}

/// Copia de seguridad que crea loadProject al migrar (ADR 0002: backups/<stem>.v<N>.gpproj).
[[nodiscard]] inline std::filesystem::path migrationBackupFor(const std::filesystem::path& documentFile,
                                                              int originalVersion) {
    std::filesystem::path name = documentFile.stem();
    name += ".v" + std::to_string(originalVersion);
    name += ".gpproj";
    return documentFile.parent_path() / "backups" / name;
}

}  // namespace gp::app::paths
