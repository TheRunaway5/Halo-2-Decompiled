#include "halo2/cache_file_snapshot.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstdint>
#include <system_error>
#if defined(__linux__)
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace halo2 {
#if defined(__linux__)
namespace {
struct File {
    int fd;
    explicit File(int descriptor) : fd(descriptor) {}
    File(const File &) = delete;
    File &operator=(const File &) = delete;
    ~File() { if (fd >= 0) ::close(fd); }
};

std::filesystem::path backing_directory() {
    const char *cache = std::getenv("XDG_CACHE_HOME");
    std::filesystem::path root = cache && std::filesystem::path(cache).is_absolute() ? cache : "";
    if (root.empty()) {
        const char *home = std::getenv("HOME");
        if (!home || !std::filesystem::path(home).is_absolute()) return {};
        root = std::filesystem::path(home) / ".cache";
    }
    return root / "halo2-native" / "file-snapshots";
}

bool copy_contents(int source, int destination, std::size_t size) {
    // Reflink gives an independent inode without copying file data into RAM.
    // Cross-filesystem/unsupported clones use a small streaming disk copy.
    if (::ioctl(destination, FICLONE, source) == 0) return true;
    if (::ftruncate(destination, 0) || ::lseek(destination, 0, SEEK_SET) < 0) return false;
    std::array<std::byte, 65536> buffer;
    std::size_t offset = 0;
    while (offset < size) {
        const auto wanted = std::min(buffer.size(), size - offset);
        const auto count = ::pread(source, buffer.data(), wanted, offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        std::size_t written = 0;
        while (written < std::size_t(count)) {
            const auto n = ::write(destination, buffer.data() + written, count - written);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return false;
            written += std::size_t(n);
        }
        offset += std::size_t(count);
    }
    return ::fdatasync(destination) == 0;
}
}
#endif

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_open(const std::filesystem::path &path) {
#if defined(__linux__)
    const File source{::open(path.c_str(), O_RDONLY | O_CLOEXEC)};
    struct stat info{};
    if (source.fd < 0 || ::fstat(source.fd, &info) || !S_ISREG(info.st_mode) ||
        info.st_size <= 0 || std::uint64_t(info.st_size) > 1024ull * 1024 * 1024) return {};
    const auto directory = backing_directory();
    if (directory.empty()) return {};
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return {};
    int descriptor = -1;
#ifdef O_TMPFILE
    descriptor = ::open(directory.c_str(), O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
#endif
    if (descriptor < 0) {
        auto name = (directory / "cache-XXXXXX").string();
        descriptor = ::mkstemp(name.data());
        if (descriptor < 0) return {};
        if (::unlink(name.c_str()) || ::fcntl(descriptor, F_SETFD, FD_CLOEXEC) < 0) {
            ::close(descriptor); return {};
        }
    }
    const File snapshot{descriptor};
    const auto size = std::size_t(info.st_size);
    if (!copy_contents(source.fd, snapshot.fd, size)) return {};
    struct stat copied{};
    if (::fstat(snapshot.fd, &copied) || copied.st_size != info.st_size) return {};
    auto *mapping = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, snapshot.fd, 0);
    if (mapping == MAP_FAILED) return {};
    CacheFileSnapshot *result;
    try { result = new CacheFileSnapshot(mapping, size); }
    catch (...) { ::munmap(mapping, size); throw; }
    return std::shared_ptr<CacheFileSnapshot>(result);
#else
    (void)path;
    return {};
#endif
}

CacheFileSnapshot::~CacheFileSnapshot() {
#if defined(__linux__)
    ::munmap(mapping_, size_);
#endif
}
std::span<const std::byte> CacheFileSnapshot::bytes() const {
    return {static_cast<const std::byte *>(mapping_), size_};
}
std::span<std::byte> CacheFileSnapshot::mutable_bytes() {
    return {static_cast<std::byte *>(mapping_), size_};
}
}
