#include "model/ProjectFile.h"

#include <atomic>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "rt/RtGuard.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#endif

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
/// UTF-8 text of a path. The narrow-string accessor of path uses the ANSI code page on
/// Windows and can throw, so it is never used (ADR 0002).
std::string utf8(const fs::path& p) {
    const std::u8string u = p.u8string();
    return std::string(reinterpret_cast<const char*>(u.data()), u.size());
}

Status ioFail(const std::string& what, const fs::path& path, const std::error_code& ec = {}) {
    std::string msg = what + ": " + utf8(path);
    if (ec) {
        msg += " (" + ec.message() + ")";
    }
    return Status::error(ErrorCode::ioError, std::move(msg));
}

/// Unique temporary name in the same folder (same filesystem => atomic rename). Two
/// processes or two threads never share it.
fs::path uniqueTempPath(const fs::path& documentFile) {
    static std::atomic<std::uint64_t> counter{0};
#if defined(_WIN32)
    const auto pid = static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    const auto pid = static_cast<std::uint64_t>(::getpid());
#endif
    fs::path tmp = documentFile;
    tmp += "." + std::to_string(pid) + "." + std::to_string(counter.fetch_add(1)) + ".tmp";
    return tmp;
}

/// Writes `text` to a new file and forces it to stable storage (fsync / F_FULLFSYNC /
/// FlushFileBuffers). Checks every step including close.
Status writeDurably(const fs::path& path, const std::string& text) {
#if defined(_WIN32)
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return ioFail("cannot create temporary file", path,
                      std::error_code(static_cast<int>(GetLastError()), std::system_category()));
    }
    const char* data = text.data();
    std::size_t left = text.size();
    while (left > 0) {
        const DWORD chunk = left > 0x40000000u ? 0x40000000u : static_cast<DWORD>(left);
        DWORD written = 0;
        if (!WriteFile(h, data, chunk, &written, nullptr) || written == 0) {
            const auto err = static_cast<int>(GetLastError());
            CloseHandle(h);
            return ioFail("cannot write temporary file", path,
                          std::error_code(err, std::system_category()));
        }
        data += written;
        left -= written;
    }
    if (!FlushFileBuffers(h)) {
        const auto err = static_cast<int>(GetLastError());
        CloseHandle(h);
        return ioFail("cannot flush temporary file", path,
                      std::error_code(err, std::system_category()));
    }
    if (!CloseHandle(h)) {
        return ioFail("cannot close temporary file", path,
                      std::error_code(static_cast<int>(GetLastError()), std::system_category()));
    }
    return Status::ok();
#else
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (fd < 0) {
        return ioFail("cannot create temporary file", path, std::error_code(errno, std::generic_category()));
    }
    const char* data = text.data();
    std::size_t left = text.size();
    while (left > 0) {
        const ssize_t n = ::write(fd, data, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            const int err = errno;
            ::close(fd);
            return ioFail("cannot write temporary file", path, std::error_code(err, std::generic_category()));
        }
        data += n;
        left -= static_cast<std::size_t>(n);
    }
#if defined(__APPLE__)
    // fsync on macOS does not flush the drive cache; F_FULLFSYNC does.
    if (::fcntl(fd, F_FULLFSYNC) != 0 && ::fsync(fd) != 0) {
#else
    if (::fsync(fd) != 0) {
#endif
        const int err = errno;
        ::close(fd);
        return ioFail("cannot flush temporary file", path, std::error_code(err, std::generic_category()));
    }
    if (::close(fd) != 0) {
        return ioFail("cannot close temporary file", path, std::error_code(errno, std::generic_category()));
    }
    return Status::ok();
#endif
}

/// Makes the rename durable (POSIX only; Windows has no directory fsync). Best effort: the
/// data is already safe, so a failure here is not reported as a failed save.
void syncDirectory(const fs::path& dir) {
#if !defined(_WIN32)
    const int fd = ::open(dir.empty() ? "." : dir.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        (void)::fsync(fd);
        ::close(fd);
    }
#else
    (void)dir;
#endif
}
}  // namespace

Status saveProject(const ProjectState& state, const fs::path& documentFile,
                   std::string_view appVersion) {
    GP_RT_ASSERT_NOT_REALTIME();
    try {
        std::string why;
        if (!isValid(state, &why)) {
            return Status::error(ErrorCode::invalidArgument, "refusing to save invalid state: " + why);
        }
        std::string text;
        try {
            text = toJson(state, appVersion).dump(2);
        } catch (const nlohmann::json::exception& e) {
            return Status::error(ErrorCode::invalidArgument,
                                 std::string("cannot serialise: ") + e.what());
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

        const fs::path tmp = uniqueTempPath(documentFile);
        Status st = writeDurably(tmp, text);
        if (!st.isOk()) {
            std::error_code ignored;
            fs::remove(tmp, ignored);
            return st;
        }
        std::error_code ec;
        fs::rename(tmp, documentFile, ec);
        if (ec) {
            std::error_code ignored;
            fs::remove(tmp, ignored);
            return ioFail("cannot replace project file", documentFile, ec);
        }
        syncDirectory(paths.folder);
        return Status::ok();
    } catch (const std::exception& e) {
        return Status::error(ErrorCode::ioError, std::string("unexpected error saving: ") + e.what());
    }
}

static LoadResult loadProjectImpl(const fs::path& documentFile);

LoadResult loadProject(const fs::path& documentFile) {
    GP_RT_ASSERT_NOT_REALTIME();
    try {
        return loadProjectImpl(documentFile);
    } catch (const std::exception& e) {
        LoadResult failed;
        failed.status = Status::error(ErrorCode::ioError, std::string("unexpected error loading: ") + e.what());
        return failed;
    }
}

static LoadResult loadProjectImpl(const fs::path& documentFile) {
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
        fs::path backupName = documentFile.stem();
        backupName += ".v" + std::to_string(result.originalSchemaVersion);
        backupName += std::string(kProjectFileExtension);
        const fs::path backup = paths.backupsDir / backupName;
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
                                  "; original saved to " + utf8(backup));
    }
    return result;
}

}  // namespace gp::model
