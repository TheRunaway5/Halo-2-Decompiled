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
    ~CacheFileSnapshot();
    CacheFileSnapshot(const CacheFileSnapshot &) = delete;
    CacheFileSnapshot &operator=(const CacheFileSnapshot &) = delete;
    std::span<const std::byte> bytes() const;
    std::span<std::byte> mutable_bytes();
private:
    CacheFileSnapshot(void *mapping, std::size_t size) : mapping_(mapping), size_(size) {}
    void *mapping_;
    std::size_t size_;
};
}
