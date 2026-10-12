#pragma once
#include "halo2/source_shadow.hpp"
#include <functional>

namespace halo2 {
// Actual current object/header fields. Full handles and literal child/sibling
// links belong to the original successful ownership/registration producer.
struct SourceShadowOwnerState {
    // B93B0 links next sibling at object+C and first child at object+10.
    std::uint32_t identity=0xffffffffu,parent=0xffffffffu,sibling=0xffffffffu,child=0xffffffffu;
    std::uint32_t object_flags{};
    std::uint16_t definition_mode{};
    std::uint8_t object_type{},header_flags{},header_kind{},lod_flags_10a{};
    std::int32_t death_tick{};
    ShadowCasterSphere sphere;
    // Required when3D7F0 actually reads unit+2B0; absent for other classes.
    std::optional<float> camouflage;
    // Actual per-view4BAF0 second output byte, including root-model, metric,
    // source force flag and current view override. This helper does not make it.
    std::optional<std::uint8_t> model_fade;
};
struct SourceShadowClock {std::int32_t tick{};float seconds_per_tick{};};
struct SourceShadowOwnerFade {bool eligible{};float fade{};bool local_shadow{};};
// Complete3D7F0 parent-root fallback and final header-kind camouflage factor,
// with explicitly supplied4BAF0 output/clock and current owner state.
SourceShadowOwnerFade source_shadow_owner_fade(std::span<const SourceShadowOwnerState>,
    std::uint32_t owner,const SourceShadowClock &);
// Literal31F50 includes the current sphere, recursively visits object+C
// (next sibling), then iterates object+10 (first child). Preserve that order.
// Missing handles and cycles are unresolved inputs, never dropped geometry.
std::vector<ShadowCasterSphere> source_shadow_owner_hierarchy(
    std::span<const SourceShadowOwnerState>,std::uint32_t root);
//3D7C0/B9CE0: checks headerbit4 or objectflagbit0 along immediate parents.
bool source_shadow_owner_blocked(std::span<const SourceShadowOwnerState>,std::uint32_t);
struct SourceShadowCollectorEntry {std::uint32_t owner{};std::uint16_t flags{};};
struct SourceShadowCandidateContext {
    // Original33400 mode0/1/2, actual current inverse-view depth row and+BC.
    unsigned mode{};ShaderVector depth_plane{};float pixel_scale{};
    SourceShadowClock clock;
    //3E9C0 actual local-player/observer producer returns a byte, not a guess
    // from native scene order.33400 calls it only for appended candidates.
    std::function<std::uint8_t(std::uint32_t)> priority;
    //33400 writes bytes0/1/2 but preserves byte3 of each admitted output slot.
    //335E0's source stack bank is uncleared; its prior contents are input.
    std::span<const std::uint8_t> retained_fourth_flag_bytes;
};
// Source547F9C collector order/flags are mandatory inputs from actual PVS and
// registration. Returns at most32 rows before4A7C0/4AC30 history/selection.
std::vector<SourceShadowCandidate> source_shadow_candidates(
    std::span<const SourceShadowOwnerState>,std::span<const SourceShadowCollectorEntry>,
    const SourceShadowCandidateContext &);
}
