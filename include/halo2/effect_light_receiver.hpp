#pragma once
#include "halo2/effect_light_admission.hpp"
#include "halo2/source_shadow_view.hpp"
#include "halo2/source_object_location.hpp"
#include <functional>
#include <optional>

namespace halo2 {
struct SourceObjectAdmission;
struct SourceEffectLightFlagPolicy31650 {
    std::uint32_t definition_flags{},owner=0xffffffffu,root=0xffffffffu;
};
struct SourceEffectLightFlagChanges31650 {
    std::uint32_t rows{},changed{},owner_none_passes{},root_passes{};
};
//31650 follows admission and precedes the optional31C10 parent append.
// Preserve duplicates, every unrelated flag bit and all other row columns.
SourceEffectLightFlagChanges31650 source_effect_light_bsp_flags_31650(SourceBspAdmission&,std::uint32_t definition_flags);
SourceEffectLightFlagChanges31650 source_effect_light_object_flags_31650(SourceObjectAdmission&,const SourceEffectLightFlagPolicy31650&);
bool source_effect_light_full_f8_31650(std::uint32_t definition_flags,std::uint16_t cluster_count);
struct SourceEffectLightFlagReference31650 {
    std::uint32_t light{},owner=0xffffffffu,root=0xffffffffu;
    const SourceBspAdmission& bsp_before;
    const SourceBspAdmission& bsp_after;
    const SourceObjectAdmission& objects_before;
    SourceObjectAdmission& objects_after;
    bool full_f8{};
};
struct SourceEffectLightParentBipedInput {
    std::uint32_t definition_flags{},query_flags{},capacity{},owner=0xffffffffu;
    std::int16_t attached_node=-1;
    std::uint16_t shape{};
};
// Sphere31E22 forces shadow0. Append after ordinary admission, preserve
// duplicates and the final reserved slot, and stop at the first biped.
bool source_effect_light_append_parent_biped(SourceObjectAdmission&,
    const SourceEffectLightParentBipedInput&,const std::function<std::uint32_t(std::uint32_t)>& parent);
struct SourceEffectLightReceiverView {
    SourcePortalVisibilitySet visibility;
    SourceBspAdmission bsp;
    // Finite original-code reference mode needs the pre31650 input only
    // when bit16 actually edits BSP rows. Normal rendering retains no copy.
    std::optional<SourceBspAdmission> bsp_before_flags;
    std::uint32_t flags{},pass_mode=1,query_mode=1;
    SourceObjectLocation location;
};
struct SourceEffectLightReceiverWorld {
    const ObjectLightSpatialBsp& spatial;
    const SourceBspAdmissionBsp& geometry;
    const BspAtmosphere& locations;
    std::int16_t selected_bsp;
    const SourcePortalBsp* portals{};
    std::span<const std::uint32_t> open_portals;
};
//135837's tail after the non-light mode2 branch. Spheres supply six cameras;
// no cameras select mode1 before the radius/flag tests. NaN selects mode0.
std::uint32_t source_effect_light_query_mode_135837(std::size_t cameras,float radius,std::uint32_t flags);
// Spherical31C70→134C00 preparation.
// The shape supplies six C1D90 cameras; persistent corrected bounds supply
// sphere membership/admission. The auxiliary queues and fog membership are
// the current main view's actual inputs, not a query at the light position.
// Small bounds use135870 sphere membership. At radius>=3 (or NaN),175F90
// consumes the six cameras, actual portal BSP and this view's live door mask.
//135410's auxiliary intersection records have no downstream reader.
// Definition bit22 contributes the literal0x400 object-query restriction;
// BSP admission ignores that bit. Dynamic objects, parent append, draw-unit/
// prefix/mask production and submission are separate.
SourceEffectLightReceiverView source_effect_light_receiver_view(
    const SourceEffectLightReceiverWorld&,
    std::int16_t containing_cluster,Vec3 spatial_center,float spatial_radius,
    const EffectLightSphere&,std::span<const std::uint8_t> containing_fog,
    const SourceBspAdmission* auxiliary,std::uint32_t definition_flags);
}
