#pragma once
#include "halo2/cache.hpp"
#include <cstdint>
#include <unordered_map>

namespace halo2::path_tracing {
// The light a shader gave off when its map's lightmaps were baked: the
// radiosity emission in the shader's runtime properties (s_shader_properties,
// written by the map compiler from the template's emissive parameters).
// Every surface that lit the bake - ceiling panels, wall strips, door lights,
// lamps - is a light source to the path tracer as well, with the colour and
// power its authors gave the bake.
struct LightmapEmission {
    Vec3 color{};             // lightmap_emissive_color
    float power{};            // lightmap_emissive_power
    Vec3 map_color{1,1,1};    // mean of lightmap_emissive_map (white without one)
};
// The surface's radiance in the path tracer's lighting units (the mean
// incoming radiance a receiver's diffuse lighting averages, which the
// renderer matches to Halo 2's 2 * lightmap). The repository's own bake
// calibration fixes the scale: the Deserted lighting pipeline
// (tools/stage_deserted_all_lights.py) reproduces a Blender emission of
// strength S in the Halo 2 bake with emissive power S / 3, and Blender's
// emission strength is radiance in these units. So radiance is
// 3 * power * colour * map.
inline constexpr float lightmap_power_radiance=3.f;
Vec3 lightmap_radiance(const LightmapEmission&);
// Every shader in the cache with a positive emissive power, by shader datum.
std::unordered_map<std::uint32_t,LightmapEmission> load_lightmap_emissions(const Cache&);
}
