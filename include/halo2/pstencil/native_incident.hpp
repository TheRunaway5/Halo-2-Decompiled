#pragma once
#include "halo2/object_lighting.hpp"
#include <optional>
namespace halo2 {
class PhysicsLiveWorldTu15;
struct SourceBspAdmission;
namespace pstencil {
struct NativeIncident {
    std::uint32_t owner{},cache{};
    ObjectLightmapSample sample;
};
// Publish the actual admitted BSP resources through the original cache I/O
// owner before CPU surface attribution. Supplies no geometry pointer/result.
void prepare_native_incident_geometry_tu15(PhysicsLiveWorldTu15&,const SourceBspAdmission&);
// Current-asset adaptation for the2003 distant-shadow stage. Run the complete
// native baked-light cache updater even when cinematic lighting overrides the
// visible material colors. Read its actual incident, never a clamped primary
// or projected-shadow direction. Native query/status/cache chronology survives.
std::optional<NativeIncident> native_incident_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t object,float lighting_metric);
}
}
