#pragma once
#include "halo2/world_effects.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/pstencil/attached_lights.hpp"

namespace halo2::pstencil {
struct PointLightOwner {
    std::uint32_t object,definition;
    Transform root;
    std::span<const Transform> nodes;
    std::span<const std::uint8_t> permutations;
    // Present even for zero native attachments. No snapshot means only
    // constant, untimed attachments can use the existing fallback producer.
    std::optional<std::span<const AttachedLight>> native_lights;
};
// Direct ligh attachments supplement the existing effe light producer only
// in pStencil. Current authored colors/radii and actual object WORLD poses
// feed the same source sphere, spatial query and receiver collectors.
class PointLights {
public:
    PointLights(const Cache&,unsigned bsp);
    ~PointLights();
    class Checkpoint {
    public:
        bool valid()const noexcept;
        std::optional<std::uint64_t> tick()const noexcept;
    private:
        friend class PointLights;
        struct Data;std::shared_ptr<const Data> data_;
    };
    // Exact owning local state: salted datum/spatial allocators, registrations,
    // query queues/epoch, four selection caches and attachment lifetimes.
    // Resident Cache/BSP/terrain/definition banks remain in this same owner.
    // An untouched owner has a valid checkpoint with no synchronization tick.
    Checkpoint checkpoint()const;
    bool owns_checkpoint(const Checkpoint&)const noexcept;
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore&&)noexcept;
        PreparedRestore& operator=(PreparedRestore&&)noexcept;
    private:
        friend class PointLights;
        PreparedRestore();struct Data;std::unique_ptr<Data> data_;
    };
    PreparedRestore prepare_restore(const Checkpoint&)const;
    bool restore_ready(const PreparedRestore&)const noexcept;
    // No sampler, attachment, allocator, query or receiver is reconstructed.
    // Keep the consumed token until the enclosing publication has ended; it
    // retains the retired containers. Existing view pointers reference the
    // unchanged resident definition bank, not the swapped mutable storage.
    bool commit_restore(PreparedRestore&)noexcept;
    bool has(std::uint32_t definition)const;
    void synchronize(std::span<const PointLightOwner>,std::uint64_t tick);
    std::vector<std::uint32_t> definitions()const;
    std::vector<std::string> unavailable()const;
    SourceEffectLightView view(Vec3,const SourceEffectLightCollectionContext&,
        const SourceEffectLightCandidateContext&,std::int32_t frame);
    std::vector<WorldEffectLightReceiver> receivers(const SourceEffectLightView&,
        const SourceBspAdmissionBsp&,std::span<const std::uint8_t>,const SourceBspAdmission*);
    std::vector<WorldEffectLight> lights()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
