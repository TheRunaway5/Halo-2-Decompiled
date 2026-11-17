#pragma once
#include "halo2/model.hpp"
#include "halo2/object_sky_lighting.hpp"
namespace halo2 {
struct BspAtmosphere;
}

namespace halo2::pstencil {
// The ordinary source view's far distance; an unobstructed sun volume ends here.
inline constexpr float sun_shadow_far_distance=1024.f;
// Explicit current biped/vehicle geometry and its unmodified, interpolated
// lightmap incident sample. Never infer this from the limited retail primary
// or projected-shadow direction. Geometry is owned through the render call.
struct LightmapCaster {
    std::vector<Vertex> vertices;
    Vec3 incident;
    bool sun{};
    // Sun volumes only: world distance each caster vertex is carried away
    // from the sun. sun_shadow_extrusion bounds it at the first visible
    // world surface so the volume cannot darken rooms behind that surface.
    float extrusion=sun_shadow_far_distance;
    // 2003 A7E00 shadow alpha: the owner's opacity selects one of the 17
    // stipple patterns. A camouflaged owner casts a proportionally thinner
    // shadow and a fully camouflaged one casts none.
    std::uint8_t opacity=255;
};
// Opacity byte for an owner with the given active-camouflage amount (0..1).
constexpr std::uint8_t camouflaged_shadow_opacity(float camouflage) {
    if(!(camouflage>0))return 255;
    if(camouflage>=1)return 0;
    return std::uint8_t((1.f-camouflage)*255.f+.5f);
}
std::optional<Vec3> sun_shadow_direction(const BspAtmosphere&,const BspSurfaceLighting&,
    std::span<const std::optional<ObjectSkyLightingDefinition>>,Vec3 origin);
// Length of a sun volume for a caster inside the sphere (center,radius).
// Five rays leave the sphere's sun-facing tangent plane away from the sun:
// one through the centre and four at the lateral radius. The centre contact
// plus 1.5x the lateral excess covers every caster vertex over a planar
// receiver. Sky contacts (material NONE) and misses receive no shadow; with
// no visible centre contact the unobstructed far distance is retained.
float sun_shadow_extrusion(const BspSurfaceLighting&,Vec3 incident,Vec3 center,float radius);
// Same, with the radius taken from the caster's actual posed vertices.
float sun_shadow_extrusion(const BspSurfaceLighting&,Vec3 incident,Vec3 center,
    std::span<const Vertex> vertices);
// 2003 E2690 negates the normalized sample; 1CC280 multiplies that by -20000.
Vec3 lightmap_light_origin(Vec3 incident);
inline constexpr float lightmap_shadow_extrusion=2.f;
}
