#pragma once
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/source_sound_cache_read_tu15.hpp"
#include <memory>
#include <optional>
namespace halo2 {
class Cache;
class MccOriginalSoundRuntimeDeliveryTu15;
class MccSoundDatResourcesTu15;
struct MccSoundRuntimePlan;
struct SourceRuntimeMccPreparation;
class SourceRuntimeResourcePreparation;
class SourceRuntimeResources {
public:
    using Digest=std::array<std::byte,32>;
    struct Identity {
        Digest resident;
        std::array<std::optional<Digest>,3> shared;
        bool operator==(const Identity &) const = default;
    };
    // Bind actual private resident bytes (including balance writes) and pinned
    // original shared-resource snapshots. For MCC, resident also authenticates
    // owned English facial-resource sectors and scenario sound DAT snapshots.
    // An explicit or provenance-admitted
    // original-sound preparation binds its sealed metadata, provenance and sectors into
    // resident, preserving this identity's serialized shape. Paths stay local.
    // Cache must outlive this owner and remain immutable after construction.
    explicit SourceRuntimeResources(const Cache &);
    // Canonical runtime descriptor addresses can differ from the Cache's
    // donor-normalized metadata addresses. Translate only that typed field
    // domain; encoded file/resource offsets retain their serialized meaning.
    using DescriptorAddress=std::function<std::uint32_t(std::uint32_t,std::size_t)>;
    SourceRuntimeResources(const Cache &,DescriptorAddress);
    ~SourceRuntimeResources();
    const Identity &identity() const noexcept;
    // Require the same immutable original owner as canonical sound metadata
    // before publication. Construction fixes the resource identity; no later
    // registration or replacement of original banks is supported.
    void require_sound_plan(const MccSoundRuntimePlan&)const;
    void require_sound_plan(const Cache& expected_owner,const MccSoundRuntimePlan&)const;
    std::shared_ptr<const MccOriginalSoundRuntimeDeliveryTu15> original_sound_owner()const;
    std::shared_ptr<const MccSoundDatResourcesTu15> sound_dat_owner()const;
    // Direct World loading keeps catalog metadata paired with this resource
    // owner. A prepared match continues to use its existing registry handoff.
    // An owned pair is consumed once, at its exact canonical layout.
    std::unique_ptr<SourceRuntimeMccPreparation> take_mcc_world_metadata(std::uint32_t base,std::uint32_t budget);
    std::vector<std::byte> load(std::uint32_t encoded_offset,std::uint32_t bytes) const;
    // Exact original sound descriptors select retained original sectors.
    // MCC prefix requests use their retained DAT snapshots.
    std::vector<std::byte> sound_cache(const SourceSoundCacheReadTu15&)const;
    SourceRuntimeCacheAnimationImage animation(std::uint32_t descriptor) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    SourceRuntimeResources(const Cache&,DescriptorAddress,const SourceRuntimeMccPreparation&);
    friend class SourceRuntimeResourcePreparation;
};
}
