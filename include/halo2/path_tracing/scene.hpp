#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>
#include <compare>
#include <span>
#include <optional>
#include <vector>

namespace halo2::path_tracing {
// Optional native presentation, not a reconstruction of the retail lighting.
// All GPU records deliberately consist of 16-byte std430/std140 lanes.
struct alignas(16) GpuNode {
    ShaderVector minimum{},maximum{};
    // Internal: left/right in x/y, z=0. Leaf: first/count in x/y, z=1.
    std::array<std::uint32_t,4> children{};
};
struct alignas(16) GpuTriangle {
    ShaderVector p0{},p1{},p2{},n0{},n1{},n2{},uv01{},uv2{};
    std::array<std::uint32_t,4> metadata{}; // material slot, packed RGBA8 vertex colors0..2.
    ShaderVector color_guide{}; // Packed source lightmap UVs or proven RGB; w: image index/kind.
};
struct alignas(16) GpuMaterial {
    ShaderVector base{1,1,1,1},emission{},response{}; // base RGB: authored reflection tint.
    // offset,width,height,flags: alpha test/clamp U/clamp V/alpha kill/excluded.
    std::array<std::uint32_t,4> texture{};
    ShaderVector transform{1,1,0,0};
    // Source kind, sampler or bump record, border or ambient RGB, and
    // qualified diffuse-energy decode. Fixed default ambient has no texels.
    std::array<std::uint32_t,4> color_guide{};
    ShaderVector glancing_tint{1,1,1,1}; // Glancing reflection RGB and optional chamber baked-light weight.
    // x: atlas offset of the surface's normal map record (append_normal_map),
    // zero without one; the shader's own bump map for surfaces whose primary
    // pass does not bind it (models). y: atlas offset of its authored
    // reflection cube map (append_environment_map), zero without one.
    // z: UNORM8 diffuse retention (T1 normal/grazing, then T2 normal/grazing),
    // zero for identity. w: finish roughness bits for an authored surface
    // category, or zero when its reflection finish is unclassified.
    std::array<std::uint32_t,4> detail{};
};
struct alignas(16) GpuLight {
    ShaderVector position_radius{},color_intensity{};
    std::array<ShaderVector,6> planes{}; // World n.p-d <= 0; zero accepts all.
};
struct Light {
    Vec3 position{};
    float radius{}; // Actual authored influence radius, not emitter size.
    Vec3 color{};
    float intensity=1;
    std::array<ShaderVector,6> planes{};
    std::optional<Vec3> direction; // Direction TOWARD an authored distant emitter.
    std::uint32_t source_identity=0xffffffffu; // CPU-only full native light generation.
};
struct alignas(16) GpuSceneUniform {
    // nodes, triangles, point lights, emissive triangle count.
    std::array<std::uint32_t,4> counts{};
    ShaderVector environment{}; // Source sky RGB; w: uintBitsToFloat(emissive index offset).
    // Gains matching traced lighting to the map's baked lightmaps: x: distant
    // (sun) light, y: grid diffuse (sky, bounce, local lights), w: emission
    // of the surfaces that lit the bake (baked_emission_flag). Render::upload
    // replaces them with its running calibration; z is 1 when the scene
    // separates static world triangles from dynamic objects.
    ShaderVector calibration{1,1,0,1};
};
static_assert(sizeof(GpuNode)==48 && sizeof(GpuTriangle)==160);
static_assert(sizeof(GpuMaterial)==128 && sizeof(GpuLight)==128 && sizeof(GpuSceneUniform)==48);
struct MaterialAliasKey {
    std::uint64_t owner{};
    std::uint32_t material=0xffffffffu,lightmap=0xffffffffu;
    auto operator<=>(const MaterialAliasKey&) const = default;
};
struct MaterialAliasImage {
    std::array<std::uint64_t,2> key{};
    std::array<std::uint32_t,3> image{}; // Absolute atlas offset, width, height.
};
struct MaterialEnvironmentImage {
    std::array<std::uint64_t,2> key{};
    std::uint32_t offset{},words{},flags{};
};
inline constexpr std::uint32_t no_layer_programs=~0u,layer_program_words=60;
// Emitter table entry flag: the triangle's light is already in the lightmap
// bake (a triangle with baked_emission_flag and a baked colour guide).
inline constexpr std::uint32_t emitter_baked_bit=0x80000000u;
struct Scene {
    std::vector<GpuNode> nodes;
    std::vector<GpuTriangle> triangles;
    std::vector<GpuMaterial> materials;
    std::vector<GpuLight> lights;
    // Source RGBA8 albedo texels, then the emitter table: emissive triangle
    // indices (bit 31: its light is already in the lightmap bake), then the
    // running sum of their power, the importance distribution of light
    // sampling (refresh_emitters).
    std::vector<std::uint32_t> pixels;
    std::uint32_t texture_texel_count{},emissive_count{};
    // Immutable image identities and reusable atlas allocations. These live
    // only with the optional active scene and never own source asset bytes.
    std::vector<std::array<std::uint64_t,2>> material_image_keys;
    std::vector<std::uint32_t> material_texel_capacities;
    // Roots referenced by the complete current frame, including retained
    // static geometry and raster-only draws. Atlas pressure may evict others.
    std::vector<std::uint8_t> live_material_roots;
    // Average drawn colour of each glow layer (average_glow_color), or of a
    // surface's self-illumination layers, refreshed with its images; w is 1
    // when known.
    std::vector<ShaderVector> material_glow;
    // Where each root material's layer programs start inside its own atlas
    // record (GpuMaterial::texture[0]), or no_layer_programs: the complete
    // pixel programs of its world_overlays_after layers, 60 words each in
    // order. glow_layer.frag reads its combiner control words there, since a
    // layer's per-draw material bank holds only the program's colours.
    std::vector<std::uint32_t> material_layer_programs;
    // The normal map of each root material whose primary pass does not bind
    // one (models draw it in a separate lighting pass), or null. The renderer
    // fills it once from its material bank; the images stay owned there.
    std::vector<const Texture*> material_normal_maps;
    // The authored reflection cube map of each root material (Halo 2's
    // environment pass), or null; images stay owned by the material bank.
    std::vector<const Texture*> material_environment_maps;
    // Multiple source passes can bind the same immutable cube. Root and
    // owner material rows point to this shared atlas allocation.
    std::vector<MaterialEnvironmentImage> material_environment_images;
    // Radiance of each root material whose shader lit the map's lightmap
    // bake (lightmap_emission.hpp); zero for the others. The renderer fills
    // it once for its material bank; aliases inherit their root's record.
    std::vector<Vec3> material_lightmap_emission;
    // Compact per-owner metadata precedes shared source texels. These CPU
    // identities do not change GPU record layouts or own native asset bytes.
    std::uint32_t material_alias_prefix_words{};
    std::vector<MaterialAliasKey> material_alias_keys;
    std::vector<std::uint32_t> material_source_roots;
    std::vector<MaterialAliasImage> material_alias_images;
    // Source baked color guides; qualified interior primary and secondary
    // diffuse fields also retain their authored irradiance magnitude.
    // This bounded atlas exists only with the active optional RT Scene.
    std::vector<std::uint32_t> color_guide_pixels{0,0,0,0};
    std::vector<std::array<std::uint64_t,2>> color_guide_image_keys;
    // Software traversal needs the CPU-built BVH in `nodes`. A renderer that
    // traces through Vulkan acceleration structures clears this flag, and
    // update_geometry then keeps triangles in admission order without nodes.
    bool cpu_hierarchy=true;
    // Leading triangles admitted from the caller's static world geometry
    // (the resident BSP). Their acceleration structure is rebuilt only when
    // one of them actually changes.
    std::uint32_t static_triangles{};
    // The last update kept the previous static prefix unchanged.
    bool static_reused{};
    GpuSceneUniform uniform(Vec3 source_environment={}) const;
};
// Frame-local source geometry must already be posed into WORLD coordinates.
// Sky, water and independent transparent/effect passes are supplied separately
// by the ordinary renderer. Invalid/mixed-material triangles are rejected.
Scene build_scene(std::span<const Vertex>,std::span<const Material>,std::span<const Light> = {});
// Retain source material images across frame-local BVH/pose/light updates.
// The first static_vertices vertices are static world geometry; see
// Scene::static_triangles.
void update_geometry(Scene &,std::span<const Vertex>,std::span<const Light> = {},std::size_t static_vertices=0);
// Hardware traversal only: keep the static prefix admitted by the previous
// update_geometry (whose source vertices and materials are unchanged) and
// replace only the dynamic triangles that follow it.
void update_dynamic_geometry(Scene &,std::span<const Vertex> dynamic,std::span<const Light> = {});
void refresh_materials(Scene &,std::span<const Material>);
bool traces_material(const Material &);
// Additive glowing transparent world shaders (teleporter fields, gravity
// lifts, light glows). Their original pass still draws them; the ray scene
// adds them as light-only emitters that never occlude shadow rays.
bool glow_material(const Material &);
// Admitted into the ray scene: traced surfaces and light-only glows.
inline bool ray_material(const Material& material){return traces_material(material) || glow_material(material);}
GpuMaterial material_response(const Material &);
// Scene::material_normal_maps for a material bank: for each drawn material
// without a bound bump, the normal map its shader's lighting pass binds.
std::vector<const Texture*> find_normal_maps(std::span<const Material>);
// The pass holding a material's authored reflection (its environment pass,
// an environment overlay or a combined lighting pass), or null.
const Material* reflection_pass(const Material &);
// Scene::material_environment_maps: the cube map each material's reflection
// pass reflects, or null.
std::vector<const Texture*> find_environment_maps(std::span<const Material>);
// CPU intersection shares the shader's alpha any-hit policy. Intended for
// component validation and probes; production traversal runs on the GPU.
struct Hit {float distance{};std::uint32_t triangle{};Vec3 barycentric{};};
bool intersect(const Scene &,Vec3 origin,Vec3 direction,float maximum,Hit &);
}
