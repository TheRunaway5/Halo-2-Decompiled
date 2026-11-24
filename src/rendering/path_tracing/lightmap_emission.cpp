#include "halo2/path_tracing/lightmap_emission.hpp"
#include "halo2/path_tracing/glow.hpp"
#include "halo2/tags/directory.hpp"
#include <cmath>

namespace halo2::path_tracing {
namespace {
// s_shader_definition: runtime_properties is the tag block at +12.
// s_shader_properties (80 bytes): diffuse_map +0, lightmap_emissive_map +8
// (a tag reference: group, then datum), lightmap_emissive_color +16 (three
// floats), lightmap_emissive_power +28.
constexpr std::size_t properties_block=12,properties_size=80;
constexpr std::size_t emissive_map_datum=12,emissive_color=16,emissive_power=28;
bool finite(Vec3 value) {return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);}
}
Vec3 lightmap_radiance(const LightmapEmission& emission) {
    const float scale=lightmap_power_radiance*emission.power;
    return {scale*emission.color.x*emission.map_color.x,scale*emission.color.y*emission.map_color.y,
            scale*emission.color.z*emission.map_color.z};
}
std::unordered_map<std::uint32_t,LightmapEmission> load_lightmap_emissions(const Cache& cache) {
    std::unordered_map<std::uint32_t,LightmapEmission> result;
    // Zero-size tags are deduplicated aliases of another shader's payload,
    // which this loop reads under its own datum.
    for(const auto& tag:cache.select_tags({.group=TagGroup{"shad"},.require_address=true,.minimum_size=properties_block+8}))try {
        const Bytes shader(cache.tag_payload(tag.handle()));
        if(shader.size()<properties_block+8 || !shader.u32(properties_block))continue;
        const Bytes properties(cache.read_tag_bytes(shader.u32(properties_block+4),properties_size));
        LightmapEmission emission;
        emission.color={properties.f32(emissive_color),properties.f32(emissive_color+4),properties.f32(emissive_color+8)};
        emission.power=properties.f32(emissive_power);
        if(!std::isfinite(emission.power) || !(emission.power>0.f) || !finite(emission.color) ||
           !(emission.color.x>0.f || emission.color.y>0.f || emission.color.z>0.f))continue;
        // The emissive map shapes the emission over the surface; its mean
        // keeps the surface's total power. An unreadable map counts as white.
        if(const auto map=properties.u32(emissive_map_datum);map!=0xffffffffu)try {
            if(const auto mean=average_image_rgba(cache.load_bitmap(map,0)))
                emission.map_color={(*mean)[0],(*mean)[1],(*mean)[2]};
        } catch(const FormatError&) {}
        result.emplace(tag.datum,emission);
    } catch(const FormatError&) {
        // A shader whose properties cannot be read is simply not a light
        // source here; this optional lookup must never fail a map load.
    }
    return result;
}
}
