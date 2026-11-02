#pragma once
#include "halo2/object_lighting.hpp"
namespace halo2 {
class PhysicsLiveWorldTu15;
// Original BDFF0/BE0A0 and 2AC059..2AC078 write the retained 4EABC4
// bank. Reuse the shared, original-differential-checked lighting arithmetic.
void source_campaign_primary_light_tu15(PhysicsLiveWorldTu15 &,float elevation,float azimuth,Vec3 color);
void source_campaign_secondary_light_tu15(PhysicsLiveWorldTu15 &,float elevation,float azimuth,Vec3 color);
void source_campaign_ambient_light_tu15(PhysicsLiveWorldTu15 &,Vec3 color);
// Whole BDFB0: NONE is a no-op; preserve all object bits except bit24.
void source_campaign_object_lighting_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,bool active);
// Original completed script effects:2AE380 disable latch and BE180 bit21.
// The actual retained bank supplies checkpoint/revert and renderer consumers.
void source_campaign_warthog_chaingun_light_tu15(PhysicsLiveWorldTu15 &,bool enabled);
void source_campaign_object_cinematic_visibility_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,bool visible);
}
