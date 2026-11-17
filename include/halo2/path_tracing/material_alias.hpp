#pragma once
#include "halo2/path_tracing/scene.hpp"
#include "halo2/world_material_uniform.hpp"

namespace halo2::path_tracing {
inline constexpr std::uint32_t material_alias_record_words=68;
struct MaterialAliasInput {
    MaterialAliasKey key;
    const WorldMaterialUniform* primary{};
    const WorldMaterialUniform* environment{};
    const Material* environment_source{};
    const WorldMaterialUniform* ambient{};
    // Borrow the actual completed descriptor's selected images. Null retains
    // the source root's current frame; no Material or source byte copy occurs.
    std::array<const Texture*,4> surface_images{};
    const Texture* ambient_image{};
};
struct MaterialAliasBinding {
    MaterialAliasKey key;
    std::uint32_t material{};
};
// Call before refreshing ordinary root materials; stable live key slots and
// shared image allocations remain owned by this view's Scene.
void clear_material_aliases(Scene&,std::size_t root_count);
// Call after root residency is refreshed, before remapping geometry and
// update_geometry. Returned bindings are sorted by key, not submission order.
std::vector<MaterialAliasBinding> refresh_material_aliases(Scene&,std::span<const Material>,
    std::span<const MaterialAliasInput>);
std::uint32_t source_material(const Scene&,std::uint32_t material);
std::size_t material_alias_cpu_bytes(const Scene&);
}
