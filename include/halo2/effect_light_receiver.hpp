#pragma once
#include "halo2/effect_light_admission.hpp"
#include "halo2/source_shadow_view.hpp"
#include "halo2/source_object_location.hpp"

namespace halo2 {
struct SourceEffectLightReceiverView {
    SourcePortalVisibilitySet visibility;
    SourceBspAdmission bsp;
    std::uint32_t flags{},pass_mode=1,query_mode=1;
    SourceObjectLocation location;
};
struct SourceEffectLightReceiverWorld {
    const ObjectLightSpatialBsp& spatial;
    const SourceBspAdmissionBsp& geometry;
    const BspAtmosphere& locations;
    std::int16_t selected_bsp;
};
// Ordinary standalone spherical31C80→1320F0→132D60/134390 preparation.
// The shape supplies six C1D90 cameras; persistent corrected bounds supply
// sphere membership/admission. The auxiliary queues and fog membership are
// the current main view's actual inputs, not a query at the light position.
// This is the spatial-radius<3 branch (the actual frag/plasma radius is2).
// Larger bounds select a different portal/intersection owner in132B80.
// Dynamic objects, draw-unit/prefix/mask production and submission are separate.
SourceEffectLightReceiverView source_effect_light_receiver_view(
    const SourceEffectLightReceiverWorld&,
    std::int16_t containing_cluster,Vec3 spatial_center,float spatial_radius,
    const EffectLightSphere&,std::span<const std::uint8_t> containing_fog,
    const SourceBspAdmission* auxiliary);
}
