#pragma once
#include "halo2/path_tracing/scene.hpp"
#include "halo2/effect_light_view.hpp"
#include "halo2/transform.hpp"
#include <functional>

namespace halo2 {
class PhysicsLiveWorldTu15;
struct WorldEffectLight;
struct ObjectSkyLightingDefinition;
namespace path_tracing {
// The sampled WORLD node used by the primary model. Source attachment slots
// resolve through the native marker owner/selection before this provider is
// called. Full owner salts are retained; absent nodes leave native state exact.
using SourceLightPoseProvider=std::function<std::optional<Transform>(std::uint32_t,std::uint16_t)>;
// Move an already extracted native shape with its primary presentation frame.
// Preserve energy and all six projector half-spaces, including uniform scale.
void present_light_frame(Light&,const Transform& source,const Transform& presented);
// Full completed occupied source bank, including off-camera emitters. Camera
// distance/PVS/receiver fades do not change a bounce ray's light energy.
// Native spheres use the actual transformed radius. Native projectors use
// their exact six world frustum planes; their radial bound encloses the source
// far-face corners. No camera distance fade or inferred spotlight cone applies.
std::vector<Light> source_lights(PhysicsLiveWorldTu15&,
    std::span<const SourceEffectLightViewRecord>,const SourceLightPoseProvider& = {});
// The standalone/attachment producer has already admitted spherical shapes;
// radius is its authored diffuse radius and current size remains independent.
std::vector<Light> source_lights(std::span<const WorldEffectLight>);
// Exact source D1A30 incident/radiosity frame. An ambient-only sky contributes
// no invented directional emitter; its outdoor ambient remains independent.
std::optional<Light> source_sky_light(const ObjectSkyLightingDefinition&);
}
}
