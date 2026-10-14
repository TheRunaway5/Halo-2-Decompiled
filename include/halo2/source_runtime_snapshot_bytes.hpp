#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <span>
#include <vector>

namespace halo2 {
class CacheFileSnapshot;
// Value-owned complete checkpoint bytes. Copies remain independent even when
// a caller already holds mutable data. Large Linux buffers use the shared
// disk-snapshot backend; storage policy never changes the logical byte extent.
class SourceRuntimeSnapshotBytes {
public:
    using value_type = std::byte;
    using iterator = std::byte *;
    using const_iterator = const std::byte *;
    SourceRuntimeSnapshotBytes() = default;
    SourceRuntimeSnapshotBytes(std::span<const std::byte>);
    SourceRuntimeSnapshotBytes(const std::vector<std::byte> &bytes)
        : SourceRuntimeSnapshotBytes(std::span(bytes)) {}
    SourceRuntimeSnapshotBytes(std::initializer_list<std::byte> bytes)
        : SourceRuntimeSnapshotBytes(std::span(bytes.begin(), bytes.size())) {}
    template<std::contiguous_iterator Iterator>
    SourceRuntimeSnapshotBytes(Iterator first, Iterator last)
        : SourceRuntimeSnapshotBytes(std::span<const std::byte>(first, last)) {}
    SourceRuntimeSnapshotBytes(const SourceRuntimeSnapshotBytes &);
    SourceRuntimeSnapshotBytes(SourceRuntimeSnapshotBytes &&) noexcept;
    SourceRuntimeSnapshotBytes &operator=(const SourceRuntimeSnapshotBytes &);
    SourceRuntimeSnapshotBytes &operator=(SourceRuntimeSnapshotBytes &&) noexcept;
    static SourceRuntimeSnapshotBytes capture_file(int descriptor);
    void swap(SourceRuntimeSnapshotBytes &) noexcept;
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return !size_; }
    std::byte *data() noexcept;
    const std::byte *data() const noexcept;
    iterator begin() noexcept { return data(); }
    const_iterator begin() const noexcept { return data(); }
    iterator end() noexcept { return empty() ? data() : data() + size_; }
    const_iterator end() const noexcept { return empty() ? data() : data() + size_; }
    std::byte &operator[](std::size_t i) noexcept { return data()[i]; }
    const std::byte &operator[](std::size_t i) const noexcept { return data()[i]; }
    void clear() noexcept;
    void pop_back();
    void push_back(std::byte);
    friend bool operator==(const SourceRuntimeSnapshotBytes &,
                           const SourceRuntimeSnapshotBytes &) noexcept;
private:
    friend class SourceRuntimeMemory;
    std::vector<std::byte> owned_;
    std::shared_ptr<CacheFileSnapshot> mapped_;
    std::size_t size_ = 0;
};
} // namespace halo2
