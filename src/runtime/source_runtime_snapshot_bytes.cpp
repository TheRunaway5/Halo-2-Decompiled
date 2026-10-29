#include "halo2/source_runtime_snapshot_bytes.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
constexpr std::size_t file_threshold = 1024 * 1024;
std::shared_ptr<CacheFileSnapshot> capture(std::span<const std::byte> bytes,
                                         std::span<const std::byte> suffix = {}) {
    auto result = CacheFileSnapshot::try_capture(bytes, suffix);
    if (!result)
        throw std::runtime_error("Required independent source-memory snapshot disk storage unavailable");
    return result;
}
}
SourceRuntimeSnapshotBytes::SourceRuntimeSnapshotBytes(std::span<const std::byte> bytes)
    : size_(bytes.size()) {
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    if (size_ >= file_threshold) { mapped_ = capture(bytes); return; }
#endif
    owned_.assign(bytes.begin(), bytes.end());
}
SourceRuntimeSnapshotBytes::SourceRuntimeSnapshotBytes(const SourceRuntimeSnapshotBytes &other)
    : size_(other.size_) {
    if(other.mapped_) {
        mapped_=other.mapped_->try_private_copy(size_);
        if(mapped_)return;
    }
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    if(size_>=file_threshold){mapped_=capture(std::span<const std::byte>(other));return;}
#endif
    owned_.assign(other.begin(),other.end());
}
SourceRuntimeSnapshotBytes::SourceRuntimeSnapshotBytes(SourceRuntimeSnapshotBytes &&other) noexcept
    : owned_(std::move(other.owned_)), mapped_(std::move(other.mapped_)),
      size_(std::exchange(other.size_, 0)) {}
SourceRuntimeSnapshotBytes &SourceRuntimeSnapshotBytes::operator=(const SourceRuntimeSnapshotBytes &other) {
    if (this != &other) { SourceRuntimeSnapshotBytes copy(other); swap(copy); }
    return *this;
}
SourceRuntimeSnapshotBytes &SourceRuntimeSnapshotBytes::operator=(SourceRuntimeSnapshotBytes &&other) noexcept {
    if (this != &other) { SourceRuntimeSnapshotBytes moved(std::move(other)); swap(moved); }
    return *this;
}
SourceRuntimeSnapshotBytes SourceRuntimeSnapshotBytes::capture_file(int descriptor) {
    SourceRuntimeSnapshotBytes result;
    result.mapped_ = CacheFileSnapshot::try_capture_file(descriptor);
    if (!result.mapped_)
        throw std::runtime_error("Required independent source-file snapshot storage unavailable");
    result.size_ = result.mapped_->bytes().size();
    return result;
}
void SourceRuntimeSnapshotBytes::swap(SourceRuntimeSnapshotBytes &other) noexcept {
    owned_.swap(other.owned_); mapped_.swap(other.mapped_); std::swap(size_, other.size_);
}
std::byte *SourceRuntimeSnapshotBytes::data() noexcept {
    return mapped_ ? mapped_->mutable_bytes().data() : owned_.data();
}
const std::byte *SourceRuntimeSnapshotBytes::data() const noexcept {
    return mapped_ ? mapped_->bytes().data() : owned_.data();
}
void SourceRuntimeSnapshotBytes::clear() noexcept { SourceRuntimeSnapshotBytes{}.swap(*this); }
void SourceRuntimeSnapshotBytes::pop_back() {
    if (empty()) throw std::out_of_range("Cannot remove a byte from an empty source snapshot");
    if (!mapped_) owned_.pop_back();
    --size_;
}
void SourceRuntimeSnapshotBytes::push_back(std::byte byte) {
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    if (mapped_ || size_ >= file_threshold - 1) {
        auto replacement = capture(std::span<const std::byte>(*this), std::span(&byte, 1));
        mapped_ = std::move(replacement); std::vector<std::byte>{}.swap(owned_); ++size_; return;
    }
#endif
    owned_.push_back(byte); ++size_;
}
bool operator==(const SourceRuntimeSnapshotBytes &a, const SourceRuntimeSnapshotBytes &b) noexcept {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
} // namespace halo2
