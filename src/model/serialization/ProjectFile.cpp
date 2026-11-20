#include "model/ProjectFile.h"

#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <sstream>

namespace gp::model {

namespace fs = std::filesystem;

ProjectPaths ProjectPaths::forDocument(const fs::path& documentFile) {
    ProjectPaths p;
    p.documentFile = documentFile;
    p.folder = documentFile.parent_path();
    p.samplesDir = p.folder / "samples";
    p.rendersDir = p.folder / "renders";
    p.cacheDir = p.folder / "cache";
    p.backupsDir = p.folder / "backups";
    return p;
}

namespace {
Status ioFail(const std::string& what, const fs::path& path, const std::error_code& ec = {}) {
    std::string msg = what + ": " + path.string();
    if (ec) {
        msg += " (" + ec.message() + ")";
    }
    return Status::error(ErrorCode::ioError, std::move(msg));
}
}  // namespace

Status saveProject(const ProjectState& state, const fs::path& documentFile,
                   std::string_view appVersion) {
    std::string why;
    if (!isValid(state, &why)) {
        return Status::error(ErrorCode::invalidArgument, "refusing to save invalid state: " + why);
    }
    std::string text;
    try {
        text = toJson(state, appVersion).dump(2);
    } catch (const nlohmann::json::exception& e) {
        return Status::error(ErrorCode::invalidArgument, std::string("cannot serialise: ") + e.what());
    }
    text.push_back('\n');

    const ProjectPaths paths = ProjectPaths::forDocument(documentFile);
    for (const fs::path* dir : {&paths.folder, &paths.samplesDir, &paths.rendersDir,
                                &paths.cacheDir, &paths.backupsDir}) {
        if (dir->empty()) {
            continue;
        }
        std::error_code ec;
        fs::create_directories(*dir, ec);
        if (ec) {
            return ioFail("cannot create directory", *dir, ec);
        }
    }

    fs::path tmp = documentFile;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return ioFail("cannot open temporary file", tmp);
        }
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        if (!out.good()) {
            out.close();
            std::error_code ignored;
            fs::remove(tmp, ignored);
            return ioFail("cannot write temporary file", tmp);
        }
    }
    std::error_code ec;
    fs::rename(tmp, documentFile, ec);
    if (ec) {
        std::error_code ignored;
        fs::remove(tmp, ignored);
        return ioFail("cannot replace project file", documentFile, ec);
    }
    return Status::ok();
}

LoadResult loadProject(const fs::path& documentFile) {
    LoadResult result;
    std::ifstream in(documentFile, std::ios::binary);
    if (!in) {
        result.status = ioFail("cannot open project file", documentFile);
        return result;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) {
        result.status = ioFail("cannot read project file", documentFile);
        return result;
    }
    nlohmann::ordered_json doc;
    try {
        doc = nlohmann::ordered_json::parse(text);
    } catch (const nlohmann::json::exception& e) {
        result.status = Status::error(ErrorCode::parseError, e.what());
        return result;
    }

    result = fromJson(doc, builtinMigrations());
    if (result.status.isOk() && result.migrated) {
        const ProjectPaths paths = ProjectPaths::forDocument(documentFile);
        const fs::path backup =
            paths.backupsDir / (documentFile.stem().string() + ".v" +
                                std::to_string(result.originalSchemaVersion) +
                                std::string(kProjectFileExtension));
        std::error_code ec;
        fs::create_directories(paths.backupsDir, ec);
        if (!ec) {
            fs::copy_file(documentFile, backup, fs::copy_options::overwrite_existing, ec);
        }
        if (ec) {
            LoadResult failed;
            failed.status = ioFail("cannot back up original before migration", backup, ec);
            return failed;
        }
        result.warnings.push_back("project migrated from schema " +
                                  std::to_string(result.originalSchemaVersion) +
                                  "; original saved to " + backup.string());
    }
    return result;
}

}  // namespace gp::model
