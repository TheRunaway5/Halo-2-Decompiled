#pragma once
#include "halo2/effect_light.hpp"
#include "halo2/effect_light_spatial.hpp"
#include "halo2/source_sphere_position.hpp"

namespace halo2 {
// C2D00's ordinary standalone record fields. Attachment owners/nodes areNONE;
// animation shapes andC0840's segment-bound refresh retain separate producers.
struct SourceEffectLightRegistration {
    std::uint32_t identity=0xffffffffu,reference_head=0xffffffffu;
    std::uint16_t flags{};
    Vec3 spatial_center;float spatial_radius{}; //18/24
    Vec3 cull_center;float cull_radius{}; //28/34
    Vec3 location_center; //38
    SourceObjectLocation location; //44
    Vec3 position; //84
    float extension_radius{};Vec3 extension_center; //9C/A0
};
// Complete standalone sphericalC2D00 withC28B0,16A8E0,11BED0 and1CAC60.
// Terrain contacts are generated only when the original initial correction
// gate consumes16A440. Location and memberships use the real BSP banks.
// OriginalC2D00 does not erase old links; callers ownC3220 chronology.
void source_effect_light_register(const EffectLightDefinition&,
    SourceEffectLightRegistration&,const BspAtmosphere&,std::int16_t selected_bsp,
    const ObjectLightSpatialBsp&,SourceEffectLightSpatialPool&,
    const RawCollisionBsp& terrain);
// Separate fixture boundary over an already-produced16A440 contact bank.
void source_effect_light_register_from_contacts(const EffectLightDefinition&,
    SourceEffectLightRegistration&,const BspAtmosphere&,std::int16_t selected_bsp,
    const ObjectLightSpatialBsp&,SourceEffectLightSpatialPool&,
    const SourceSphereContactBank* initial_contacts=nullptr);
// C3220 preserves the cached location and bounds; inactive records do nothing.
void source_effect_light_unregister(SourceEffectLightRegistration&,SourceEffectLightSpatialPool&);
}
