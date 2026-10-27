#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>
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
};
struct alignas(16) GpuMaterial {
    ShaderVector base{1,1,1,1},emission{},response{}; // base RGB: authored reflection tint.
    // offset,width,height,flags: alpha test/clamp U/clamp V/alpha kill/excluded.
    std::array<std::uint32_t,4> texture{};
    ShaderVector transform{1,1,0,0};
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
};
struct alignas(16) GpuSceneUniform {
    // nodes, triangles, point lights, emissive triangle count.
    std::array<std::uint32_t,4> counts{};
    ShaderVector environment{}; // Source sky RGB; w: uintBitsToFloat(emissive index offset).
};
static_assert(sizeof(GpuNode)==48 && sizeof(GpuTriangle)==144);
static_assert(sizeof(GpuMaterial)==80 && sizeof(GpuLight)==128 && sizeof(GpuSceneUniform)==32);
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
