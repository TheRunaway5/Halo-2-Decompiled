#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Original D1850/D1E10 collision/lightmap query result, before D4080.
// Colors carry RGB in the low24 bits. D1E10 emits0x00RRGGBB; D1A30's
// diffuse fallback is literal0xFF555555. The high byte is not lighting alpha.
struct ObjectLightmapSample {
  Vec3 incident;
  std::uint32_t diffuse_rgb{}, lightmap_rgb{};
};
struct ObjectLightingVariables {
  std::uint32_t affected_objects{};
  float brightness_offset{};
  Vec3 primary_minimum, primary_maximum;
  float exclusion_degrees{};
  std::vector<std::byte> primary_scale;
  Vec3 secondary_minimum, secondary_maximum;
  Vec3 diffuse_minimum, diffuse_maximum;
  float secondary_rotation_degrees{};
  std::vector<std::byte> secondary_scale;
  Vec3 ambient_minimum, ambient_maximum;
  std::vector<std::byte> ambient_scale, shadow_scale;
};
struct ObjectLightingDefinition {
  std::string name;
  std::vector<ObjectLightingVariables> variables;
  std::shared_ptr<const ShaderFunctionTables> tables;
};
// The 20 lighting floats in 3DB00's original 84-byte record. Its final
// dword contains packed metadata (including authored scenery SH indices),
// retained separately by the source record loader. Lighting externs do not
// consume that final dword.
struct ObjectLightingState {
  Vec3 ambient, shadow_direction;
  float accuracy{}, shadow_opacity{};
  Vec3 primary_color, primary_direction, secondary_color, secondary_direction;
  // Fixture serialization: the final zero is not original packed metadata.
  std::array<float,21> source_values() const;
};
// 3DFE0's retained five-dword target/current samples and render-frame stamps.
struct ObjectLightingCache {
  ObjectLightmapSample current, target;
  ObjectLightingState lighting;
  Vec3 query_origin;
  std::int32_t last_query_frame=-1, last_rendered_frame=-1;
  bool sample_valid{}, lighting_valid{};
};
bool object_lighting_query_due(const ObjectLightingCache &, std::int32_t frame,
    Vec3 object_origin, float original_draw_distance, bool frozen);
// Query results correspond to the source attributed/resident status 0.
// A miss keeps the prior target and initial misses keep the zero cache state.
void update_object_lighting_cache(ObjectLightingCache &,
    const ObjectLightingDefinition &, unsigned object_type, std::int32_t frame,
    Vec3 object_origin, Vec3 root_velocity, bool query_attempted,
    const std::optional<ObjectLightmapSample> &query_result,
    bool interpolation_enabled=true, bool frozen=false);
ObjectLightingState object_lighting(const ObjectLightingDefinition &,
    const ObjectLightmapSample &, unsigned object_type);
// 1362B0: CPU light queries fetch one repeated nearest texel, independently
// of the material's GPU sampler. P8 images may be expanded with their exact
// authored palette; BC colors follow the original 223ED0 integer decoder.
std::optional<std::uint32_t> sample_object_texture(const Texture &, Vec2 uv,
                                                float detail = 1.f);
// 131ED0 packs RGB with the source nearest-even byte conversion and masks.
// D1E10 clamps its interpolated colors before calling this helper.
std::uint32_t pack_object_lighting_rgb(Vec3);
// 33980/19BE0 lighting externs; selected authored program stays intact.
// Applies only lighting remaps to a per-view uniform copy. Object colors,
// shader animation and fog already present in that copy are preserved.
void apply_object_lighting_bindings(PixelProgram &, ShaderConstants &,
    const MaterialPass &, const ObjectLightingState &,
    std::span<const std::int32_t> switches, float specular_brightness,
    float ambient_brightness);
void apply_object_lighting(Material &, const ObjectLightingState &);
// 2D000 classification2 selects highest-LOD layer3 dynamic implementation.
// Materials without layer3 retain implementation0 for their independent
// transparent/emissive layers (15D70). Empty/unsupported materials return false.
// Optional extern37 comes from the object's resolved parent camouflage context;
// absence does not substitute a default for a material that consumes it.
bool prepare_dynamic_material(const Cache &, Material &,
    const ObjectLightingState &, bool volume_textures=false,
    std::optional<float> camouflage = {});
} // namespace halo2
