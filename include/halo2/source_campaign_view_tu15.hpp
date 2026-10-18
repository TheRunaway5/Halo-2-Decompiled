#pragma once
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_bsp_draw_collector.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_campaign_environment_frame_tu15.hpp"
#include <functional>
#include <map>
#include <optional>
#include <set>

namespace halo2 {
struct BspFogSample;
class Cache;

struct SourceCampaignParticleViewTu15 {
    MuzzleEffectView camera;
    float projection_angle{},horizontal_angle{};
};

// Borrowed caller camera inputs. No viewport, containing cluster, open-door
// mask or object visibility result is inferred from the active cinematic.
struct SourceCampaignViewInputTu15 {
    SourceViewProjectionBank rendering;
    SourceFrustumCamera clipping;
    Vec3 eye,forward;
    std::array<std::int16_t,4> viewport;
    float distance_scale{},coverage_multiplier{};
    bool view_modifier{};
    // The actual submitting camera/fog/target inputs. Absence retains the
    // existing model-only diagnostic route; it cannot claim a full frame.
    std::optional<SourceCampaignParticleViewTu15> particles;
    const SourceCampaignEnvironmentFrameTu15 *environment{};
    // An actual renderer query owner supplies20700 after its once-per-frame
    //20680. Absence retains the standalone CPU collector's original gates.
    std::function<void(PhysicsLiveWorldTu15 &,unsigned)> prepare_lens_view;
};
struct SourceCampaignViewFrameTu15 {
    SourcePortalVisibilitySet visibility;
    SourceBspAdmission bsp;
    SourceObjectAdmission objects;
    SourceDrawUnitState units;
    // Owned completed main units, copied before another view/light query can
    // reuse their native contexts, palettes or geometry arena. Overrides remain
    // present even when the ordinary Vulkan consumer cannot submit them yet.
    std::vector<SourceLiveModelBaseDrawTu15> base_draws;
    // Builders include recursive children and rejected/effect-copy owners.
    // Only completed draw units establish that an owner actually submits.
    std::map<std::uint32_t,SourceObjectDrawCollector> built;
    std::set<std::uint32_t> drawn;
    std::vector<ParticleDraw> world_particles;
    std::vector<std::uint32_t> live_effect_definitions;
    unsigned source_particle_commands{};
    bool main_commands_completed{};
};
// Retain immutable authored BSP resources. Every collection rereads the real
// TU1.5 portal bank, so device updates and checkpoint restore keep one owner.
// This single-view campaign seam runs the existing original entry, fade,
// model-builder, presentation/child and draw-unit owners. GPU pass submission
// and per-part shader/override consumption remain downstream owners.
class SourceCampaignViewTu15 {
public:
    SourceCampaignViewTu15(const Cache &,std::size_t bsp);
    SourceCampaignViewFrameTu15 collect(PhysicsLiveWorldTu15 &,
        const SourceCampaignViewInputTu15 &,const BspAtmosphere &,
        const BspFogSample &);
private:
    SourcePortalBsp portals_;
    SourceBspAdmissionBsp geometry_;
    SourceWorldParticleDrawsTu15 particles_;
    std::vector<SourceBspDrawGeometry> clusters_,definitions_;
    std::vector<SourceBspDrawInstance> instances_;
    std::vector<Transform> transforms_;
    bool lightmap_matches_{};
    void prepare_bsp(PhysicsLiveWorldTu15 &,const SourceBspAdmission &);
};
}
