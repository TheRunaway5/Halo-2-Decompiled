#include "halo2/cache_file_snapshot.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <system_error>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#if defined(__linux__)
#include <linux/fs.h>
#include <linux/magic.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#elif defined(__APPLE__)
#include "macos/cache_file_snapshot_storage.hpp"
#elif defined(_WIN32)
#include "windows/cache_file_snapshot_storage.hpp"
#endif

namespace halo2 {
namespace {
std::atomic<std::uint64_t> capture_input_bytes{},capture_written_bytes{},independent_mapping_bytes{},retained_file_backings{};
}
CacheFileSnapshot::StorageStats CacheFileSnapshot::storage_stats() noexcept {
    return {capture_input_bytes.load(std::memory_order_relaxed),capture_written_bytes.load(std::memory_order_relaxed),
        independent_mapping_bytes.load(std::memory_order_relaxed),retained_file_backings.load(std::memory_order_relaxed)};
}
#if defined(__linux__) || defined(__APPLE__)
namespace {
struct File {
    int fd;
    explicit File(int descriptor) : fd(descriptor) {}
    File(const File &) = delete;
    File &operator=(const File &) = delete;
    ~File() { if (fd >= 0) ::close(fd); }
};

#if defined(__linux__)
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
bool disk_backed(int descriptor) {
    struct statfs filesystem{};
    return !::fstatfs(descriptor,&filesystem) && filesystem.f_type!=TMPFS_MAGIC &&
        filesystem.f_type!=RAMFS_MAGIC;
}
int snapshot_sync(int descriptor,std::size_t offset,std::size_t bytes) {
    // These private unlinked snapshots require completed, bounded writeback,
    // not crash durability. Wait on exactly this batch's data pages without
    // repeatedly flushing filesystem metadata and the device write cache.
    int result;
    do {
        result=::sync_file_range(descriptor,off_t(offset),off_t(bytes),
            SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE|SYNC_FILE_RANGE_WAIT_AFTER);
    }while(result<0 && errno==EINTR);
    if(!result)return 0;
    const int range_error=errno;
    // Unsupported kernels/filesystems retain the original durable route.
    // Also drain a failed range before unwinding, but never swallow its EIO
    // or ENOSPC merely because the wait consumed the kernel's error report.
    do{result=::fdatasync(descriptor);}while(result<0 && errno==EINTR);
    if(result)return result;
    if(range_error==ENOSYS || range_error==EINVAL || range_error==EOPNOTSUPP)return 0;
    errno=range_error;return -1;
}
#else
using cache_snapshot_detail::snapshot_file;
using cache_snapshot_detail::copy_contents;
using cache_snapshot_detail::disk_backed;
int snapshot_sync(int descriptor,std::size_t,std::size_t) {
    return cache_snapshot_detail::snapshot_sync(descriptor);
}
#endif
}
#endif

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_open(const std::filesystem::path &path) {
#if defined(__linux__) || defined(__APPLE__)
    const File source{::open(path.c_str(), O_RDONLY | O_CLOEXEC)};
    struct stat info{};
    if (source.fd < 0 || ::fstat(source.fd, &info) || !S_ISREG(info.st_mode) ||
        info.st_size <= 0 || std::uint64_t(info.st_size) > 1024ull * 1024 * 1024) return {};
    const auto size = std::size_t(info.st_size);
    return clone_file(source.fd, size, false);
#elif defined(_WIN32)
    const cache_snapshot_windows::File source{::_wopen(path.c_str(),_O_RDONLY|_O_BINARY|_O_NOINHERIT)};
    struct _stat64 info{};
    if(source.fd<0 || ::_fstat64(source.fd,&info) || !(info.st_mode&_S_IFREG) ||
        info.st_size<=0 || std::uint64_t(info.st_size)>1024ull*1024*1024)return {};
    return clone_file(source.fd,std::size_t(info.st_size),false);
#else
    (void)path;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_capture_file(int descriptor) {
#if defined(__linux__) || defined(__APPLE__)
    struct stat info{};
    if (::fstat(descriptor, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 ||
        std::uint64_t(info.st_size) > std::numeric_limits<std::size_t>::max()) return {};
    return clone_file(descriptor, std::size_t(info.st_size), true);
#elif defined(_WIN32)
    struct _stat64 info{};
    if(::_fstat64(descriptor,&info) || !(info.st_mode&_S_IFREG) || info.st_size<0 ||
        std::uint64_t(info.st_size)>std::numeric_limits<std::size_t>::max())return {};
    return clone_file(descriptor,std::size_t(info.st_size),true);
#else
    (void)descriptor;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::clone_file(
    int descriptor, std::size_t size, bool disk_only) {
#if defined(__linux__) || defined(__APPLE__)
    if (!size) return std::shared_ptr<CacheFileSnapshot>(new CacheFileSnapshot(nullptr, 0));
    const File snapshot{snapshot_file()};
    if (snapshot.fd < 0) return {};
    if (disk_only && !disk_backed(snapshot.fd)) return {};
    if (!copy_contents(descriptor, snapshot.fd, size)) return {};
    struct stat copied{};
    if (::fstat(snapshot.fd, &copied) || copied.st_size < 0 ||
        std::uint64_t(copied.st_size) != size) return {};
    return map_file(snapshot.fd, size);
#elif defined(_WIN32)
    (void)disk_only; // Windows snapshot_file admits only disk volumes.
    if(!size)return std::shared_ptr<CacheFileSnapshot>(new CacheFileSnapshot(nullptr,0));
    const cache_snapshot_windows::File snapshot{cache_snapshot_windows::snapshot_file()};
    if(snapshot.fd<0 || !cache_snapshot_windows::copy_contents(descriptor,snapshot.fd,size))return {};
    struct _stat64 copied{};
    if(::_fstat64(snapshot.fd,&copied) || copied.st_size<0 || std::uint64_t(copied.st_size)!=size)return {};
    return map_file(snapshot.fd,size);
#else
    (void)descriptor; (void)size; (void)disk_only;
    return {};
#endif
}

std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_capture(
    std::span<const std::byte> bytes, std::span<const std::byte> suffix) {
#if defined(__linux__) || defined(__APPLE__)
    if (suffix.size() > std::numeric_limits<std::size_t>::max() - bytes.size()) return {};
    const auto size = bytes.size() + suffix.size();
    if (!size || size > std::uint64_t(std::numeric_limits<off_t>::max())) return {};
    const File snapshot{snapshot_file()};
    if (snapshot.fd < 0 || !disk_backed(snapshot.fd) ||
        ::ftruncate(snapshot.fd, off_t(size))) return {};
    capture_input_bytes.fetch_add(size,std::memory_order_relaxed);
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
                    capture_written_bytes.fetch_add(std::size_t(count),std::memory_order_relaxed);
                    written += std::size_t(count);
                }
                if (snapshot_sync(snapshot.fd,offset,chunk.size())) return {};
            }
            offset += chunk.size();
            segment = segment.subspan(chunk.size());
        }
    }
    return map_file(snapshot.fd, size);
#elif defined(_WIN32)
    if(suffix.size()>std::numeric_limits<std::size_t>::max()-bytes.size())return {};
    const auto size=bytes.size()+suffix.size();
    if(!size || size>std::uint64_t(std::numeric_limits<std::int64_t>::max()))return {};
    const cache_snapshot_windows::File snapshot{cache_snapshot_windows::snapshot_file()};
    if(snapshot.fd<0 || ::_chsize_s(snapshot.fd,size))return {};
    capture_input_bytes.fetch_add(size,std::memory_order_relaxed);
    constexpr std::size_t batch=1024*1024;
    std::size_t offset=0;
    for(auto segment:{bytes,suffix}) {
        while(!segment.empty()) {
            const auto chunk=segment.first(std::min(batch,segment.size()));
            if(!std::all_of(chunk.begin(),chunk.end(),[](auto byte){return byte==std::byte{};})) {
                if(::_lseeki64(snapshot.fd,offset,SEEK_SET)<0)return {};
                for(std::size_t written=0;written<chunk.size();) {
                    const auto count=::_write(snapshot.fd,chunk.data()+written,unsigned(chunk.size()-written));
                    if(count<=0)return {};
                    capture_written_bytes.fetch_add(std::size_t(count),std::memory_order_relaxed);
                    written+=std::size_t(count);
                }
                if(::_commit(snapshot.fd))return {};
            }
            offset+=chunk.size();segment=segment.subspan(chunk.size());
        }
    }
    return map_file(snapshot.fd,size);
#else
    (void)bytes; (void)suffix;
    return {};
#endif
}

struct CacheFileSnapshot::Backing {
#if defined(__linux__) || defined(__APPLE__)
    int descriptor;
    explicit Backing(int value):descriptor(value){retained_file_backings.fetch_add(1,std::memory_order_relaxed);}
    ~Backing(){::close(descriptor);retained_file_backings.fetch_sub(1,std::memory_order_relaxed);}
#elif defined(_WIN32)
    HANDLE section;
    explicit Backing(HANDLE value):section(value){retained_file_backings.fetch_add(1,std::memory_order_relaxed);}
    ~Backing(){CloseHandle(section);retained_file_backings.fetch_sub(1,std::memory_order_relaxed);}
#endif
};
std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::map_file(int descriptor, std::size_t size) {
#if defined(__linux__) || defined(__APPLE__)
    // Every caller supplies the completed private snapshot inode. Retain that
    // immutable file, not the external source descriptor, after this call.
    const auto retained=::fcntl(descriptor,F_DUPFD_CLOEXEC,0);
    if(retained<0) {
        // Preserve the original independent snapshot under descriptor pressure;
        // only its optional no-copy successor route is unavailable.
        auto* mapping=::mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE,descriptor,0);
        if(mapping==MAP_FAILED)return {};
        CacheFileSnapshot* result;
        try{result=new CacheFileSnapshot(mapping,size);}
        catch(...){::munmap(mapping,size);throw;}
        return std::shared_ptr<CacheFileSnapshot>(result);
    }
    std::shared_ptr<const Backing> backing;
    try{backing=std::make_shared<Backing>(retained);}catch(...){::close(retained);throw;}
    return map_backing(std::move(backing),size);
#elif defined(_WIN32)
    const auto section=CreateFileMappingW(cache_snapshot_windows::handle(descriptor),nullptr,PAGE_WRITECOPY,0,0,nullptr);
    if(!section)return {};
    std::shared_ptr<const Backing> backing;
    try{backing=std::make_shared<Backing>(section);}catch(...){CloseHandle(section);throw;}
    return map_backing(std::move(backing),size);
#else
    (void)descriptor; (void)size;
    return {};
#endif
}
std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::map_backing(
    std::shared_ptr<const Backing> backing,std::size_t size) {
#if defined(__linux__) || defined(__APPLE__)
    auto *mapping = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, backing->descriptor, 0);
    if (mapping == MAP_FAILED) return {};
    CacheFileSnapshot *result;
    try { result = new CacheFileSnapshot(mapping, size,std::move(backing)); }
    catch (...) { ::munmap(mapping, size); throw; }
    return std::shared_ptr<CacheFileSnapshot>(result);
#elif defined(_WIN32)
    auto *mapping=MapViewOfFile(backing->section,FILE_MAP_COPY,0,0,size);
    if(!mapping)return {};
    CacheFileSnapshot *result;
    try{result=new CacheFileSnapshot(mapping,size,std::move(backing));}
    catch(...){UnmapViewOfFile(mapping);throw;}
    return std::shared_ptr<CacheFileSnapshot>(result);
#else
    (void)backing; (void)size;
    return {};
#endif
}

CacheFileSnapshot::~CacheFileSnapshot() {
#if defined(__linux__) || defined(__APPLE__)
    if (mapping_) ::munmap(mapping_, size_);
#elif defined(_WIN32)
    if(mapping_)UnmapViewOfFile(mapping_);
#endif
}
std::span<const std::byte> CacheFileSnapshot::bytes() const {
    return {static_cast<const std::byte *>(mapping_), size_};
}
std::span<std::byte> CacheFileSnapshot::mutable_bytes() {
    mutable_exposed_.store(true,std::memory_order_release);
    return {static_cast<std::byte *>(mapping_), size_};
}
std::shared_ptr<CacheFileSnapshot> CacheFileSnapshot::try_private_copy(std::size_t prefix) const {
    if(!prefix || prefix>size_ || !backing_ || mutable_exposed_.load(std::memory_order_acquire))return {};
    auto copy=map_backing(backing_,prefix);
    if(copy)independent_mapping_bytes.fetch_add(prefix,std::memory_order_relaxed);
    return copy;
}
}
