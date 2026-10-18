#pragma once
#include "halo2/effect_light.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <functional>
#include <optional>

namespace halo2 {
// Literal current light record fields. Persistent spatial bounds and per-view
// fades have separate producers; neither is inferred from the current shape.
struct SourceEffectLightRuntime {
    std::uint32_t identity,owner;
    std::uint8_t flags; // record+2
    std::int16_t attached_node,cluster; // record+54/+48
    std::int32_t scenario_placement; // record+8; scenario+EC ordinal, NONE for attached/effect lights
    float spatial_radius,overall_fade; // record+24/+100
    Vec3 spatial_center; // record+18, distinct from the render position
    Vec3 position,forward,up; // record+84/+AC/+B8
    EffectLightState state;
};
struct SourceEffectLightRenderContext {
    std::uint8_t renderer_enabled; // (*5107E8)+8
    std::int32_t debug_gate; // 55E4D0[current debug index]
    std::uint32_t debug_flags; // (*4E9AE8)+0
    std::int16_t view_type; // 485602
    // Complete 3E9C0 result, required only when attached_node != NONE.
    std::function<std::uint8_t(std::uint32_t)> local_priority;
};
struct SourceEffectLightAdmission {
    bool eligible{};
    // C17F0 may produce a shape even when the final view/cluster gate rejects.
    // Early returns leave the caller's output shape untouched.
    std::optional<EffectLightSphere> sphere;
};
// Complete C3140 active/bound/fade/color-energy test over current state.
bool source_effect_light_live(const SourceEffectLightRuntime&);
// Complete retail 31590 with C3140,31520,C17F0 for tag-defined spheres.
// No PVS collection or history is created here. Scenario placements retain
// their separate C1980 shape owner and reject only when that branch is consumed.
SourceEffectLightAdmission source_effect_light_render_admission(
    const EffectLightDefinition&,const SourceEffectLightRuntime&,
    const SourceEffectLightRenderContext&);
struct SourceEffectLightCandidateSource {
    SourceEffectLightRuntime record;
    const EffectLightDefinition* definition;
};
struct SourceEffectLightCandidateContext {
    SourceEffectLightRenderContext admission;
    ShaderVector depth_plane;
    float pixel_scale;
    // Actual BAF80 root and current owner header+3. Missing services reject
    // only for appended candidates with an owner other than NONE.
    std::function<std::uint32_t(std::uint32_t)> root_owner;
    std::function<std::uint8_t(std::uint32_t)> owner_kind;
};
// Original 31910 through its call to 4A7C0. The source PVS queue and successful
// registration order are mandatory inputs. The output is capped at 128 while
// the original eligibility calls continue for the rest of the input queue.
// Feed this to source_shadow_select with the separate dynamic-light cache,
// preferred budget3 and acceleration.01; this does not create that history.
std::vector<SourceShadowCandidate> source_effect_light_candidates(
    std::span<const SourceEffectLightCandidateSource>,
    std::span<const std::uint32_t> visible_queue,
    const SourceEffectLightCandidateContext&);
struct SourceEffectLightVisibleQueue {
    std::uint32_t capacity;
    std::uint16_t count;
    std::span<SourceBspAdmissionRecord> storage;
};
struct SourceEffectLightVisibilityContext {
    std::uint32_t epoch; // Actual 4E0308
    std::int32_t query_mode; // Context+2A7C
    std::int8_t rejection_override; // Signed context+2A60; negative bypasses cull rejection
    const SourceClusterFrustumSet* frustums;
    Vec3 sphere_center;
    float sphere_radius;
};
// Complete 132A30 with C3140/165010/461C0/163C80/C3110. Supply the actual
// constructed cluster-entry ordinal and retained record+C visited epoch.
// Successful appends alone stamp that epoch. Spatial-list traversal and the
// separate C3950 local attached-light fallback remain outside this single visit.
bool source_effect_light_visibility_visit(const SourceEffectLightRuntime&,
    std::uint32_t& last_seen_epoch,SourceEffectLightVisibleQueue&,
    const SourceEffectLightVisibilityContext&,std::uint32_t cluster_entry,
    std::uint8_t force);
struct SourceEffectLightVisibilitySource {
    SourceEffectLightRuntime record;
    const EffectLightDefinition* definition; // Null is original tag NONE.
    std::uint32_t last_seen_epoch;
};
struct SourceEffectLightCollectionContext {
    SourceEffectLightVisibilityContext visibility;
    SourceEffectLightRenderContext owners;
    std::uint32_t flags; // Context+2A60; bit4 suppresses lights.
    std::int32_t pass_mode; // Context+2A78; nonzero suppresses lights.
    std::int16_t containing_cluster; // Context+8; NONE suppresses the fallback only.
    std::uint32_t local_player; // Actual4B9ED4 view index; NONE suppresses C3950.
};
// Complete light branches of132220, including C3950's four-record fallback.
// Sources are occupied data-array records in actual ascending pool order.
// Cluster lists are actual registration heads walked in link order, indexed by
// source cluster. The constructed entry ordinal, not cluster ID, drives cull.
// Object/instance traversal and spatial membership production stay separate.
void source_effect_light_collect_visible(
    std::span<SourceEffectLightVisibilitySource>,
    std::span<const std::vector<std::uint32_t>> cluster_lists,
    SourceEffectLightVisibleQueue&,const SourceEffectLightCollectionContext&);
}
