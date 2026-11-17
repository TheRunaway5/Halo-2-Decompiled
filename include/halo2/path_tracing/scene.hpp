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
    std::array<std::uint32_t,4> color_guide{}; // Source kind, sampler, border, qualified diffuse-energy decode.
    ShaderVector glancing_tint{1,1,1,1}; // Current authored glancing reflection RGB, independent of facing.
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
};
static_assert(sizeof(GpuNode)==48 && sizeof(GpuTriangle)==160);
static_assert(sizeof(GpuMaterial)==112 && sizeof(GpuLight)==128 && sizeof(GpuSceneUniform)==32);
struct MaterialAliasKey {
    std::uint64_t owner{};
    std::uint32_t material=0xffffffffu,lightmap=0xffffffffu;
    auto operator<=>(const MaterialAliasKey&) const = default;
};
struct MaterialAliasImage {
    std::array<std::uint64_t,2> key{};
    std::array<std::uint32_t,3> image{}; // Absolute atlas offset, width, height.
};
struct Scene {
    std::vector<GpuNode> nodes;
    std::vector<GpuTriangle> triangles;
    std::vector<GpuMaterial> materials;
    std::vector<GpuLight> lights;
    // Source RGBA8 albedo texels, followed by emissive triangle indices.
    std::vector<std::uint32_t> pixels;
    std::uint32_t texture_texel_count{},emissive_count{};
    // Immutable image identities and reusable atlas allocations. These live
    // only with the optional active scene and never own source asset bytes.
    std::vector<std::array<std::uint64_t,2>> material_image_keys;
    std::vector<std::uint32_t> material_texel_capacities;
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
    GpuSceneUniform uniform(Vec3 source_environment={}) const;
};
// Frame-local source geometry must already be posed into WORLD coordinates.
// Sky, water and independent transparent/effect passes are supplied separately
// by the ordinary renderer. Invalid/mixed-material triangles are rejected.
Scene build_scene(std::span<const Vertex>,std::span<const Material>,std::span<const Light> = {});
// Retain source material images across frame-local BVH/pose/light updates.
void update_geometry(Scene &,std::span<const Vertex>,std::span<const Light> = {});
void refresh_materials(Scene &,std::span<const Material>);
bool traces_material(const Material &);
GpuMaterial material_response(const Material &);
// CPU intersection shares the shader's alpha any-hit policy. Intended for
// component validation and probes; production traversal runs on the GPU.
struct Hit {float distance{};std::uint32_t triangle{};Vec3 barycentric{};};
bool intersect(const Scene &,Vec3 origin,Vec3 direction,float maximum,Hit &);
}
