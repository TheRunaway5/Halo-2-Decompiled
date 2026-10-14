#include "halo2/cache_file_snapshot.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <system_error>
#if defined(__linux__)
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/vfs.h>
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

int snapshot_file() {
    const auto directory = backing_directory();
    if (directory.empty()) return -1;
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return -1;
    int descriptor = -1;
#ifdef O_TMPFILE
    descriptor = ::open(directory.c_str(), O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
#endif
    if (descriptor < 0) {
        auto name = (directory / "cache-XXXXXX").string();
        descriptor = ::mkstemp(name.data());
        if (descriptor < 0) return -1;
        if (::unlink(name.c_str()) || ::fcntl(descriptor, F_SETFD, FD_CLOEXEC) < 0) {
            ::close(descriptor); return -1;
        }
    }
    return descriptor;
}

bool copy_contents(int source, int destination, std::size_t size) {
    // Reflink gives an independent inode without copying file data into RAM.
    // Cross-filesystem/unsupported clones use a small streaming disk copy.
    if (::ioctl(destination, FICLONE, source) == 0) return true;
    if (::ftruncate(destination, 0) || ::lseek(destination, 0, SEEK_SET) < 0) return false;
    std::array<std::byte, 65536> buffer;
    std::size_t offset = 0, pending = 0;
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
        pending += std::size_t(count);
        if (pending >= 1024 * 1024) {
            if (::fdatasync(destination)) return false;
            pending = 0;
        }
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
    const auto size = std::size_t(info.st_size);
    return clone_file(source.fd, size, false);
#else
    (void)path;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_capture_file(int descriptor) {
#if defined(__linux__)
    struct stat info{};
    if (::fstat(descriptor, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 ||
        std::uint64_t(info.st_size) > std::numeric_limits<std::size_t>::max()) return {};
    return clone_file(descriptor, std::size_t(info.st_size), true);
#else
    (void)descriptor;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::clone_file(
    int descriptor, std::size_t size, bool disk_only) {
#if defined(__linux__)
    if (!size) return std::shared_ptr<CacheFileSnapshot>(new CacheFileSnapshot(nullptr, 0));
    const File snapshot{snapshot_file()};
    if (snapshot.fd < 0) return {};
    if (disk_only) {
        struct statfs filesystem{};
        if (::fstatfs(snapshot.fd, &filesystem) || filesystem.f_type == TMPFS_MAGIC ||
            filesystem.f_type == RAMFS_MAGIC) return {};
    }
    if (!copy_contents(descriptor, snapshot.fd, size)) return {};
    struct stat copied{};
    if (::fstat(snapshot.fd, &copied) || copied.st_size < 0 ||
        std::uint64_t(copied.st_size) != size) return {};
    return map_file(snapshot.fd, size);
#else
    (void)descriptor; (void)size; (void)disk_only;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_capture(
    std::span<const std::byte> bytes, std::span<const std::byte> suffix) {
#if defined(__linux__)
    if (suffix.size() > std::numeric_limits<std::size_t>::max() - bytes.size()) return {};
    const auto size = bytes.size() + suffix.size();
    if (!size || size > std::uint64_t(std::numeric_limits<off_t>::max())) return {};
    const File snapshot{snapshot_file()};
    struct statfs filesystem{};
    if (snapshot.fd < 0 || ::fstatfs(snapshot.fd, &filesystem) ||
        filesystem.f_type == TMPFS_MAGIC || filesystem.f_type == RAMFS_MAGIC ||
        ::ftruncate(snapshot.fd, off_t(size))) return {};
    // A newly extended file contains actual zero bytes. Keep all-zero chunks
    // sparse and copy every other byte. Synchronize each bounded write batch
    // so clean file pages can be reclaimed inside the native memory budget.
    constexpr std::size_t batch = 1024 * 1024;
    std::size_t offset = 0;
    for (auto segment : {bytes, suffix}) {
        while (!segment.empty()) {
            const auto chunk = segment.first(std::min(batch, segment.size()));
            if (!std::all_of(chunk.begin(), chunk.end(), [](auto byte) { return byte == std::byte{}; })) {
                for (std::size_t written = 0; written < chunk.size();) {
                    const auto count = ::pwrite(snapshot.fd, chunk.data() + written,
                        chunk.size() - written, off_t(offset + written));
                    if (count < 0 && errno == EINTR) continue;
                    if (count <= 0) return {};
                    written += std::size_t(count);
                }
                if (::fdatasync(snapshot.fd)) return {};
            }
            offset += chunk.size();
            segment = segment.subspan(chunk.size());
        }
    }
    return map_file(snapshot.fd, size);
#else
    (void)bytes; (void)suffix;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::map_file(int descriptor, std::size_t size) {
#if defined(__linux__)
    auto *mapping = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, descriptor, 0);
    if (mapping == MAP_FAILED) return {};
    CacheFileSnapshot *result;
    try { result = new CacheFileSnapshot(mapping, size); }
    catch (...) { ::munmap(mapping, size); throw; }
    return std::shared_ptr<CacheFileSnapshot>(result);
#else
    (void)descriptor; (void)size;
    return {};
#endif
}

CacheFileSnapshot::~CacheFileSnapshot() {
#if defined(__linux__)
    if (mapping_) ::munmap(mapping_, size_);
#endif
}
std::span<const std::byte> CacheFileSnapshot::bytes() const {
    return {static_cast<const std::byte *>(mapping_), size_};
}
std::span<std::byte> CacheFileSnapshot::mutable_bytes() {
    return {static_cast<std::byte *>(mapping_), size_};
}
}
