#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_frustum_camera.hpp"
#include <optional>
#include <array>
#include <cstddef>

namespace halo2 {
class PhysicsLiveWorldTu15;
// Complete original255C0/255E0/256D0/25800/25940 and HS2ACC20/2ACC70 CPU bodies.
// Object, marker, script and resource results remain their actual native
// engine owners. No target allocation, rendered-view or GPU completion claim.
bool source_texture_camera_producer_tu15_owns_entry(std::uint32_t);
void source_run_texture_camera_producer_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
// The semantic ScriptVm has already resolved the actual object and SID. Raw
// REAL bits reach255E0 unchanged; the original body owns FOV limits/cleanup.
void source_texture_camera_set_object_marker_tu15(PhysicsLiveWorldTu15 &,
    std::uint32_t actual_object,std::uint32_t marker,std::uint32_t real_fov_bits);
void source_texture_camera_off_tu15(PhysicsLiveWorldTu15 &);
// Existing PC presentation outputs are explicit inputs, after the genuine
// original view constructor. Global layout/aux/widescreen context remains in
// the World and is not synthesized from this packet. Configured planes are
// supplied BEFORE original2B770 performs its collection-camera far clamp.
struct SourceTextureCameraPcPresentationTu15 {
    SourceViewProjectionInput camera;
    SourceFrustumProjectionInput lens;
    float lighting_distance_scale{};
    // Needed only for a real near==0 oblique camera. Ordinary positive-near
    // presentation preserves the constructor's unused plane words verbatim.
    std::optional<ShaderVector> oblique_near_plane;
};
// Preserve the complete original constructor record/header/tail and every
// unowned camera byte. Overlay only current admitted PC presentation fields,
// then perform the original29-word camera1→camera2 copy. This intentionally
// does NOT claim equality to an untouchedXbox229A90 output.
std::array<std::byte,0x118> source_texture_camera_pc_presented_view_tu15(
    std::span<const std::byte,0x118> original_constructor_view,
    const SourceTextureCameraPcPresentationTu15 &);
// Exact229EF0/22A000 call arguments and current original48BA28 view record.
// Explicit real local-user context is required. Complete229A90 and its native
// camera/rectangle helpers produce the owned fields; the existing tail stays
// untouched. This is not a zero-filled or current-camera substitute.
std::array<std::byte,0x118> source_texture_camera_original_view_tu15(
    PhysicsLiveWorldTu15 &,unsigned ordinal,unsigned local_view_count,
    unsigned split_mode,std::int32_t actual_local_user);
}
