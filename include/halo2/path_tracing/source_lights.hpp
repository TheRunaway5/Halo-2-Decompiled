#pragma once
#include "halo2/path_tracing/scene.hpp"
#include "halo2/effect_light_view.hpp"

namespace halo2 {
class PhysicsLiveWorldTu15;
struct WorldEffectLight;
struct ObjectSkyLightingDefinition;
namespace path_tracing {
// Full completed occupied source bank, including off-camera emitters. Camera
// distance/PVS/receiver fades do not change a bounce ray's light energy.
// Native spheres use the actual transformed radius. Native projectors use
// their exact six world frustum planes; their radial bound encloses the source
// far-face corners. No camera distance fade or inferred spotlight cone applies.
std::vector<Light> source_lights(PhysicsLiveWorldTu15&,
    std::span<const SourceEffectLightViewRecord>);
// The standalone/attachment producer has already admitted spherical shapes;
// radius is its authored diffuse radius and current size remains independent.
std::vector<Light> source_lights(std::span<const WorldEffectLight>);
// Exact source D1A30 incident/radiosity frame. An ambient-only sky contributes
// no invented directional emitter; its outdoor ambient remains independent.
std::optional<Light> source_sky_light(const ObjectSkyLightingDefinition&);
}
}
