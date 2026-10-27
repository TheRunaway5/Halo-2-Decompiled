#pragma once
#include "halo2/source_shadow.hpp"
#include "halo2/source_shadow_view.hpp"
#include "halo2/source_live_shadow_ordered_draws_tu15.hpp"
#include "halo2/source_draw_unit.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <functional>
namespace halo2 {
class PhysicsLiveWorldTu15;
class SourceRuntimeMemory;
struct SourceBspAdmission;
struct SourceObjectAdmission;
struct SourceLiveModelDrawContextTu15;
struct SourceLiveShadowSelectionTu15 {
    unsigned view{},frame{},queue_capacity{};
    std::uint32_t queue{},history_address{};
    bool lightmap_enabled{},cinematic_enabled{},fixed_object_shadows_enabled{};
    SourceShadowSelectionCache history_before,history_after;
    std::vector<SourceShadowCandidate> selected;
};
struct SourceLiveShadowQueuesTu15 {
    std::uint32_t context{},visibility_bank{},pass_mode{},query_mode{};
    std::vector<SourceBspAdmissionRecord> clusters,objects,instances;
};
struct SourceLiveShadowDrawsTu15 {
    SourceDrawUnitState state;
    SourceBspRangeArena main_arena;
    std::uint32_t main_arena_address=0x557074u,secondary_arena_address=0x559b54u;
};
struct SourceLiveShadowCommandBankTu15 {
    std::uint32_t address{};
    std::array<std::uint32_t,6> header{};
    std::vector<std::array<std::uint32_t,6>> commands;
};
enum class SourceLiveShadowRootRouteTu15 {ineligible,parent,disabled,blocked,local_light,no_lighting,opacity,projected};
struct SourceLiveShadowRootTu15 {
    std::uint32_t owner{},lighting_address{};
    SourceLiveShadowRootRouteTu15 route=SourceLiveShadowRootRouteTu15::ineligible;
    bool first_person{};
    float owner_fade{},history_fade{},cached_opacity{};
    Vec2 extents{};
    std::optional<SourceShadowFrame> frame;
    std::optional<SourceShadowViewVolume> view_volume;
    std::optional<SourceShadowSecondaryProjection> projection;
    std::optional<SourceShadowSecondaryCamera> secondary_camera;
    std::optional<SourceLiveShadowQueuesTu15> secondary_queues;
    std::optional<SourceLiveShadowDrawsTu15> collected;
    std::optional<std::array<SourceLiveShadowCommandBankTu15,2>> commands;
    std::optional<std::array<SourceLiveShadowOrderedDrawsTu15,2>> ordered_draws;
    //48ED0 reads the retained bank mask before40E80 replaces commands.
    //This is a completed typed math result, not a GPU upload/completion flag.
    bool shadow_depth_feature{};
    std::optional<SourceShadowUniforms> uniforms;
    float query_radius{};
};
// Original root eligibility/lighting/hierarchy owners feed the projection tail.
// Every selected root retains its route; local-light rendering is separate.
// Consume each completed projected root synchronously, before another root
// or light query can reuse its live model contexts/palette arena. Copy any
// needed data during this call; references into root/world cannot be retained.
using SourceShadowRootCompletedTu15=std::function<void(PhysicsLiveWorldTu15&,
    const SourceLiveShadowRootTu15&)>;
// Runs one root's whole40E80 around the given translated call; the first
// argument is the call's EAX layer selector. Empty runs the translated call.
using SourceShadowCommandsTu15=std::function<void(std::uint32_t,const std::function<void()>&)>;
std::vector<SourceLiveShadowRootTu15> source_live_shadow_root_frames_tu15(
    PhysicsLiveWorldTu15&,std::span<const SourceShadowCandidate>,const SourceShadowAdmissionObserverTu15& = {},
    const SourceShadowRootCompletedTu15& = {},const SourceShadowCommandsTu15& = {});
// Publish the actual completed ordinary queues before main44710/40E80.
// Selection subsequently validates those columns without republishing them.
void source_live_shadow_main_queues_tu15(PhysicsLiveWorldTu15&,
    const SourceBspAdmission&,const SourceObjectAdmission&);
// Read-only original333F0 entry/return over the actual same native world.
using SourceShadowSelectionObserverTu15=std::function<void(bool,
    const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory&)>;
// Completed main queue only, after original PRT ordering/fade flag writes.
// Publish the actual source-camera inputs, then execute whole TU1.5 candidate,
// fade, priority and history owners. Missing native callees remain errors.
// This selects CPU roots; it does not submit shadow graphics or receivers.
SourceLiveShadowSelectionTu15 source_live_shadow_selection_tu15(
    PhysicsLiveWorldTu15&,const SourceBspAdmission&,const SourceObjectAdmission&,const SourceLiveModelDrawContextTu15&,
    std::int32_t actual_local_user,const SourceShadowSelectionObserverTu15& = {});
}
