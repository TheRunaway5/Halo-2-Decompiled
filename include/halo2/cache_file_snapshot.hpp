#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <filesystem>
#include <memory>
#include <span>
#include <utility>

namespace halo2 {
// Private, file-backed initial bytes. Source edits/unlink/truncation cannot
// change this snapshot, and private metadata writes never reach the source.
// Unsupported backing returns null so the caller can keep its owned buffer.
class CacheFileSnapshot {
public:
    struct StorageStats {
        std::uint64_t capture_input_bytes{},capture_written_bytes{},independent_mapping_bytes{},retained_file_backings{};
    };
    // Process counters for actual memory captures/writes and independent
    // private views. File-to-file captures have their separate clone policy.
    static StorageStats storage_stats() noexcept;
    static std::shared_ptr<CacheFileSnapshot> try_open(const std::filesystem::path &);
    // Capture independent complete bytes on disk with bounded writeback.
    // The optional suffix supports byte-buffer growth without a RAM-sized
    // temporary. Linux/macOS reject tmpfs/ramfs backing.
    static std::shared_ptr<CacheFileSnapshot> try_capture(
        std::span<const std::byte>, std::span<const std::byte> suffix = {});
    // Capture the actual admitted regular file without changing its cursor.
    // The descriptor remains with the caller; the resulting inode is private.
    static std::shared_ptr<CacheFileSnapshot> try_capture_file(int descriptor);
    // A new private view of the immutable private file, never this mapping.
    // A previously exposed mutable pointer permanently disables this route.
    // Failure returns null so value owners retain their full-capture fallback.
    std::shared_ptr<CacheFileSnapshot> try_private_copy(std::size_t prefix) const;
    ~CacheFileSnapshot();
    CacheFileSnapshot(const CacheFileSnapshot &) = delete;
    CacheFileSnapshot &operator=(const CacheFileSnapshot &) = delete;
    std::span<const std::byte> bytes() const;
    std::span<std::byte> mutable_bytes();
private:
    struct Backing;
    static std::shared_ptr<CacheFileSnapshot> map_file(int, std::size_t);
    static std::shared_ptr<CacheFileSnapshot> map_backing(std::shared_ptr<const Backing>,std::size_t);
    static std::shared_ptr<CacheFileSnapshot> clone_file(int, std::size_t, bool disk_only);
    CacheFileSnapshot(void *mapping, std::size_t size,std::shared_ptr<const Backing> backing={})
        : mapping_(mapping),size_(size),backing_(std::move(backing)) {}
    void *mapping_;
    std::size_t size_;
    std::shared_ptr<const Backing> backing_;
    std::atomic<bool> mutable_exposed_{false};
};
}
