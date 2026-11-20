// F0-T9 re-verificación: regresión B1/B2 a nivel de modelo con rutas no ASCII.
#include <filesystem>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "model/ProjectFile.h"

namespace fs = std::filesystem;
using namespace gp::model;

namespace {
bool validUtf8(const std::string& s) {
    for (std::size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i]);
        const int n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (n == 0 || i + std::size_t(n) > s.size()) return false;
        for (int k = 1; k < n; ++k)
            if ((static_cast<unsigned char>(s[i + std::size_t(k)]) & 0xC0) != 0x80) return false;
        i += std::size_t(n);
    }
    return true;
}
}  // namespace

TEST_CASE("unicode: carpeta de proyecto completa bajo nombre no ASCII y error con mensaje UTF-8",
          "[integration][unicode]") {
    const fs::path root = fs::temp_directory_path() / fs::path(u8"gp_t9_José 日本");
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root);

    const fs::path name = fs::path(u8"Canción ñ");
    const fs::path file = root / name / (name.u8string() + u8".gpproj");
    ProjectState st;
    st.tracks = {{TrackId{1}, "Pista 日本"}};
    st.nextTrackId = 2;
    REQUIRE(saveProject(st, file, "test").isOk());

    for (const char* d : {"samples", "renders", "cache", "backups"})
        CHECK(fs::is_directory(root / name / d));
    CHECK_FALSE(fs::exists(root / "backups"));  // B2: nada fuera de la carpeta del proyecto
    CHECK(loadProject(file).state == st);

    // Archivo inexistente con ruta Unicode: error, sin excepción, mensaje UTF-8 válido.
    const auto bad = loadProject(root / fs::path(u8"nó existe 日本") / "x.gpproj");
    CHECK_FALSE(bad.status.isOk());
    CHECK(validUtf8(bad.status.message));

    fs::remove_all(root, ec);
}
