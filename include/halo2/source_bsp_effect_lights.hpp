#pragma once
#include "halo2/effect_light_draw.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/pstencil/point_caster.hpp"

namespace halo2 {
struct SourceEffectLightGeometryFrameContext;
enum class SourceEffectLightCommandPhaseTu15 {before_seed,before_collection,collected,culled};
struct SourceEffectLightCommandObservationTu15 {
    SourceEffectLightCommandPhaseTu15 phase;
    PhysicsLiveWorldTu15& world;
    std::uint32_t light;
    const SourceDrawUnitState& state;
    const SourceBspAdmission& bsp;
    const SourceObjectAdmission& objects;
    const SourceEffectLightGeometryFrameContext& context;
    std::uint32_t main_arena_token,light_arena_token,instance_token;
    std::span<const Transform> instance_transforms;
    bool diagnostic_native_seed;
    bool native_command_join_enabled;
};
struct SourceEffectLightModelViewTu15 {
    PhysicsLiveWorldTu15& world;
    SourceObjectAdmission main;
    SourceLiveModelDrawContextTu15 context;
    std::span<const std::uint8_t> containing_fog;
    // Consume the completed main prefix before a light query can mutate its
    // shared contexts/arena. The caller copies all retained draw inputs here.
    std::function<void(const SourceDrawUnitState&)> main_draws;
    // Native renderer availability, after genuine source collection. Rejected
    // model units keep their original ordinals but submit no light pass.
    std::function<bool(const SourceCullDrawUnit&)> model_available;
    // Actual completed BSP/object queues after ordinary draw preparation,
    // PRT ordering/fade flags, with the exact main context before light queries.
    std::function<void(const SourceBspAdmission&,const SourceObjectAdmission&,const SourceLiveModelDrawContextTu15&)> main_collected;
    // Diagnostic label only. Live model views always seed the actual retained
    // native state before their genuine query reset and run complete40E80.
    bool diagnostic_native_light_seed=false;
    // Actual per-light40E80 before/after ABI, with owner=the full light datum.
    std::function<void(bool,std::uint32_t,const SourcePhysicsWorldRuntimeAbi&,
        const SourceRuntimeMemory&)> light_commands;
    // Actual ordinary view calls, retaining their genuine entry and ABI.
    std::function<void(bool,std::uint32_t,const SourcePhysicsWorldRuntimeAbi&,
        const SourceRuntimeMemory&)> view_calls;
    // Synchronous borrowed inputs, before attachment/command-bank writes.
    // Copy retained state during the call; do not retain these references.
    std::function<void(const SourceEffectLightCommandObservationTu15&)> light_command_observer;
};
// Shared BSP/model part of31C80. Immutable cache resources are loaded before
// renderer creation; tick state, selection and receiver queries come from the
// live NativeWorldEffects owner. Optional model views supply the actual live
// object world and main-view queue; both families use one command/range sort.
class SourceBspEffectLights {
public:
    SourceBspEffectLights(const Cache&,Mesh&,std::size_t bsp,
        std::span<const std::uint32_t> light_definitions,
        std::span<const std::uint32_t> render_models={});
    ~SourceBspEffectLights();
    SourceBspEffectLights(const SourceBspEffectLights&)=delete;
    SourceBspEffectLights& operator=(const SourceBspEffectLights&)=delete;
    std::vector<EffectLightRawDraw> draw(const SourceEffectLightView&,
        std::span<const WorldEffectLightReceiver>,const SourceBspAdmission& main,
        const SourceViewProjectionBank&,const BspFogSample&,
        Vec3 eye,std::array<std::int16_t,4> viewport,
        std::array<std::uint32_t,2> surface,unsigned local_players,
        SourceEffectLightModelViewTu15* models=nullptr,
        std::vector<pstencil::PointCaster>* point_casters=nullptr,std::uint32_t light_domain=0);
    const std::vector<std::string>& unavailable() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
