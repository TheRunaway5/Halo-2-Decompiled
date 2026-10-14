#pragma once
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace halo2 {
// Private, file-backed initial bytes. Source edits/unlink/truncation cannot
// change this snapshot, and private metadata writes never reach the source.
// Unsupported backing returns null so the caller can keep its owned buffer.
class CacheFileSnapshot {
public:
    static std::shared_ptr<CacheFileSnapshot> try_open(const std::filesystem::path &);
    // Capture independent complete bytes on disk with bounded writeback.
    // The optional suffix supports byte-buffer growth without a RAM-sized
    // temporary. Linux requires disk-backed storage, rather than tmpfs/ramfs.
    static std::shared_ptr<CacheFileSnapshot> try_capture(
        std::span<const std::byte>, std::span<const std::byte> suffix = {});
    // Capture the actual admitted regular file without changing its cursor.
    // The descriptor remains with the caller; the resulting inode is private.
    static std::shared_ptr<CacheFileSnapshot> try_capture_file(int descriptor);
    ~CacheFileSnapshot();
    CacheFileSnapshot(const CacheFileSnapshot &) = delete;
    CacheFileSnapshot &operator=(const CacheFileSnapshot &) = delete;
    std::span<const std::byte> bytes() const;
    std::span<std::byte> mutable_bytes();
private:
    static std::shared_ptr<CacheFileSnapshot> map_file(int, std::size_t);
    static std::shared_ptr<CacheFileSnapshot> clone_file(int, std::size_t, bool disk_only);
    CacheFileSnapshot(void *mapping, std::size_t size) : mapping_(mapping), size_(size) {}
    void *mapping_;
    std::size_t size_;
};
}
