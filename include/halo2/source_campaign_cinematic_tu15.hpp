#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original cinematic interface constructor/reset/start/stop and LOD
// bodies. Source object iteration, retirement and storage remain actual owners.
bool source_campaign_cinematic_tu15_owns_entry(std::uint32_t);
void source_run_campaign_cinematic_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
void source_campaign_cinematic_control_tu15(PhysicsLiveWorldTu15 &,bool active);
// Exact zero-argument cinematic_lightmap_shadow_enable/disable HS stores.
// Uses the existing checked/journaled writer on the same retained source bank.
void source_campaign_lightmap_shadow_tu15(PhysicsLiveWorldTu15 &,bool enabled);
void source_campaign_cinematic_lod_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,bool active);
// Native callers of the original HS actions on the same retained world.
// Source routines own binding, resources, event channels, parenting and poses.
bool source_campaign_cinematic_animation_tu15(PhysicsLiveWorldTu15 &,
    std::uint32_t object,std::uint32_t graph,std::uint32_t name,
    std::uint32_t anchor,bool unit,bool loop,bool interpolate);
void source_campaign_cinematic_attach_tu15(PhysicsLiveWorldTu15 &,
    std::uint32_t parent,std::uint32_t parent_marker,
    std::uint32_t child,std::uint32_t child_marker);
std::int16_t source_campaign_cinematic_animation_time_tu15(
    PhysicsLiveWorldTu15 &,std::uint32_t object,bool unit);
// Original2AD170's post-query float32/x87-nearest/evaluator-short projection.
std::int16_t source_campaign_unit_animation_ticks_tu15(float seconds);
}
