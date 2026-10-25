#pragma once
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/source_effect_replica_render_tu15.hpp"
#include "halo2/source_light_lens_tu15.hpp"

namespace halo2 {
// Replica presentation owns its request ring and actual native GPU work.
// It neither constructs a physical world nor imports a host query/result.
class NativeLensPresentationTu15 {
public:
    // Serial role changes reuse the already resident bitmap/material owner.
    // Its CPU state is rebound only at a completed, retired frame boundary.
    // The cache, bitmap owner and bound renderer outlive this borrower.
    NativeLensPresentationTu15(const Cache &,SourceLensDrawFrameTu15 &);
    ~NativeLensPresentationTu15();
    void bind(Renderer &);
    void begin(unsigned actual_local_views);
    void prepare_view(unsigned,Vec3 position,Vec3 forward,Vec3 up,float angle,
        const SourceViewProjectionBank &,std::array<std::int16_t,4> rectangle,
        unsigned width,unsigned height);
    // Closed original41970 lens branch: a genuine free projectile is its
    // own kind5 root, so whole3E9B0's local-biped predicate is false for every
    // observer mode. Marker/function/attachment producers stay with caller.
    void free_projectile(std::uint32_t full_owner,std::uint32_t lens,unsigned ordinal,
        Vec3 position,Vec3 forward,float scale,float opacity,Vec3 color);
    // Whole17C710's class1 request for a qualified nonmoving WORLD effect.
    // The imported identity/color are presentation data, never a local pool.
    void unattached_effect(const SourceEffectReplicaRenderTu15 &,std::uint32_t lens,
        unsigned ordinal,Vec3 position,Vec3 forward,float opacity);
    void nonbiped_attached_effect(const SourceEffectReplicaRenderTu15 &,std::uint32_t lens,
        unsigned ordinal,Vec3 position,Vec3 forward,float opacity);
    void standalone_light(const SourceLightLensRequestTu15 &);
    std::vector<SourceLensQueryDraw> queries();
    std::vector<SourceLensDraw> draws();
    std::span<const SourceLensQueryResult> completed_queries()const;
    void finish_view();
    void finish(bool submitted);
    // Completed-frame transition to another presentation/authority owner.
    // Retire actual GPU work; no identities or completions cross the change.
    void suspend();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    SourceLensFrameWorldTu15 execution();
};
}
