#pragma once
#include <cstdint>

namespace halo2 {
// Original13450 record+4. This selects clip arithmetic only; far_plane also
// triggers source clear/stencil services outside this helper's scope.
enum class MenuWorldDepthBoundary : std::uint32_t {
  none = 0, near_plane = 1, far_plane = 2
};
struct MenuWorldDepthClipInput {
  // Current globals485AD4/485AD8, after their producer's updates. These are
  // separate from camera/global48565C, which normalizes projection W.
  float near_distance{}, far_distance{};
  MenuWorldDepthBoundary boundary = MenuWorldDepthBoundary::none;
  float boundary_distance{}; // Original13450 record+8; ignored by none.
  bool scaled_projection{}; // Original12D50 BL: multiply nearW by .15f.
};
struct MenuWorldDepthClipPlanes {
  float minimum_w{}, maximum_w{}; // Original units, before Vk depth conversion.
};
// Pure13450 clip arithmetic followed by12D50's optional near-plane scale.
// Requires finite distances, positive far with finite binary32 reciprocal,
// known boundary, and original round-to-nearest arithmetic without FTZ/DAZ.
// Negative near or near>far are supported original clamp cases. The result
// does not imply a depth format, per-view lifecycle, clear policy or admission.
MenuWorldDepthClipPlanes menu_world_depth_clip_planes(
    const MenuWorldDepthClipInput &);
} // namespace halo2
