#pragma once
#include "halo2/model.hpp"

namespace halo2::pstencil {
// Explicit current biped/vehicle geometry and its unmodified, interpolated
// lightmap incident sample. Never infer this from the limited retail primary
// or projected-shadow direction. Geometry is owned through the render call.
struct LightmapCaster {
    std::vector<Vertex> vertices;
    Vec3 incident;
};
// 2003 E2690 negates the normalized sample; 1CC280 multiplies that by -20000.
Vec3 lightmap_light_origin(Vec3 incident);
inline constexpr float lightmap_shadow_extrusion=2.f;
}
