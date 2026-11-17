#pragma once
#include "halo2/object_lighting.hpp"
#include <optional>
namespace halo2 {
class PhysicsLiveWorldTu15;
namespace pstencil {
struct NativeIncident {
    std::uint32_t owner{},cache{};
    ObjectLightmapSample sample;
};
// 001C60D0's fake lightmap-shadow family gate, adapted to the current native
// definition: only bipeds and vehicles. This does not govern point lights or
// supply the separate authored manifold / shadow-opacity checks.
bool native_lightmap_shadow_eligible_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t object);
// Current-asset adaptation for the2003 distant-shadow stage. Run the complete
// native baked-light cache updater even when cinematic lighting overrides the
// visible material colors. Read its actual incident, never a clamped primary
// or projected-shadow direction. Native query/status/cache chronology survives.
std::optional<NativeIncident> native_incident_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t object,float lighting_metric);
}
}
