#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include <functional>

namespace halo2 {
struct MccSoundRuntimePlan;
// Serialized logical ranges only. Native backing is owned separately; a host
// pointer must never be narrowed into either address field.
// A zero-byte entry preserves an explicitly declared empty tag location. It
// owns no readable payload and cannot satisfy any nonempty pointer extent.
struct MccRuntimeRelocation {
    std::uint32_t source{}, target{}, bytes{};
};
std::uint32_t relocate_mcc_runtime_address(std::span<const MccRuntimeRelocation>,
                                         std::uint32_t source, std::size_t bytes);
struct CanonicalRuntimeCacheIndex {
    SourceRuntimeCacheTagImage::Region region;
    std::uint32_t table{}, count{}, scenario{}, globals{};
};
// The supplied rows must already contain logical target addresses. This
// builds the existing 32-byte index/group/table format, not metadata closure.
CanonicalRuntimeCacheIndex build_canonical_runtime_cache_index(
    std::span<const TagGroupDefinition>, std::span<const Tag>,
    TagHandle scenario, TagHandle globals, std::uint32_t index_address);

struct MccRuntimeDeliveryIssue {
    TagHandle tag;
    TagGroup group;
    std::optional<std::uint32_t> field_offset;
    std::string field, reason;
};
class MccRuntimeCacheDelivery final {
public:
    static MccRuntimeCacheDelivery plan(const Cache&, std::uint32_t logical_index,
                                       std::uint32_t available_bytes);
    // Transfers sound route/format ownership only after complete metadata
    // admission. Failed or incomplete deliveries never invoke the callback.
    static MccRuntimeCacheDelivery plan(const Cache&, std::uint32_t logical_index,
        std::uint32_t available_bytes,
        const std::function<void(MccSoundRuntimePlan&&)>& capture_sound);
    const CanonicalRuntimeCacheIndex& index() const { return index_; }
    std::uint32_t reservation_bytes() const { return reservation_bytes_; }
    std::span<const MccRuntimeRelocation> relocations() const { return relocations_; }
    std::span<const MccRuntimeDeliveryIssue> issues() const { return issues_; }
    // Publication is atomic and unavailable while any schema/ownership issue
    // remains. Callers cannot receive a partly relocated original-ABI bank.
    SourceRuntimeCacheTagImage tag_image() const;
    SourceRuntimeCacheBspImage bsp_image(std::size_t) const;
    std::uint32_t runtime_address(std::uint32_t source, std::size_t bytes) const;
    std::uint32_t source_address(std::uint32_t logical, std::size_t bytes) const;
private:
    MccRuntimeCacheDelivery()=default;
    CanonicalRuntimeCacheIndex index_;
    SourceRuntimeCacheTagImage::Region metadata_;
    std::vector<SourceRuntimeCacheBspImage> bsps_;
    std::vector<MccRuntimeRelocation> relocations_;
    std::vector<MccRuntimeDeliveryIssue> issues_;
    std::uint32_t reservation_bytes_{};
    void require_complete() const;
};
} // namespace halo2
