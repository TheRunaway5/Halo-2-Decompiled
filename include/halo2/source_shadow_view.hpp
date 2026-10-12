#pragma once
#include "halo2/source_frustum_camera.hpp"

namespace halo2 {
// Actual320B0 ESI input: a source transform followed by three half extents.
// This is32350's completed receiver volume, not a reconstructed model bound.
struct SourceShadowViewBox {
    Transform world_from_box;
    Vec3 half_extent;
};
static_assert(sizeof(SourceShadowViewBox)==0x40);
struct SourceShadowViewVolume {
    bool admitted{};
    //1652E0's xmin,xmax,ymin,ymax,minDepth,maxDepth. Empty output retains
    // the literal initialized bounds, including positive/negative FLT_MAX.
    std::array<float,6> projected_bounds{};
    //11FA40's twelve edges after320B0's ordered source transform. These
    // are retained diagnostic inputs, before1652E0's private near clipping.
    std::array<Vec3,24> world_endpoints{};
};
// Complete320B0 over its actual first main camera bank. It invokes1652E0
// with24 endpoints, near clipping enabled and far clipping disabled. The
// caller's camera.near_distance and retained frustum.metric are both read.
SourceShadowViewVolume source_shadow_view_volume(const SourceFrustumCamera&,
    const SourceShadowViewBox&);

// Literal completed C1A80 projection bank. No missing field gets a default.
// For the single-camera branch C1D90 reads mode0, left7..9, virtual-distance10,
// eye11..13, projection-ray15..17, dimensions22/23 and far-offset25.
struct SourceShadowSecondaryProjection {
    std::array<std::uint32_t,31> words;
};
struct SourceShadowSecondaryCamera {
    SourceFrustumCamera camera;
    bool plane_inconsistent{}; // Original163DB0 diagnostic side effect.
};
// C1D90's complete nonzero-mode single-camera branch. Fixed47FDA0 uses mode2;
// mode1 sets the source orthographic byte. Direction/up are actual light
// fields+28/+34, independently of the completed projection-ray/left bank.
// The separate mode0 six-camera branch uses source_effect_light_cameras below.
SourceShadowSecondaryCamera source_shadow_secondary_camera(
    const SourceShadowSecondaryProjection&,Vec3 light_direction,Vec3 light_up);

// C1D90's separate mode0 branch. The radius is the completed shape+44,
// including the original max(diffuse radius, specular radius) selection.
// Six cameras retain the source face order, polygon winding and zero near
// plane; visibility/cluster admission remains the caller's responsibility.
std::array<SourceShadowSecondaryCamera,6> source_effect_light_cameras(
    Vec3 position,float radius);
}
