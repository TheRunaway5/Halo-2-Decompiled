#pragma once
#include "halo2/vertex_program.hpp"
#include "halo2/source_vertex_declaration.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace halo2 {
// Shared definition of the existing native set0/binding4 Material upload.
// Keep this exact prefix for world.vert/world.frag and the raw menu adapter;
// no caller, pass, constants or texture state is chosen by this storage type.
struct WorldMaterialUniform {
    std::array<std::uint32_t,60> program{};
    std::array<ShaderVector,3> transforms{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,8> forward_transforms{};
    ShaderVector diffuse{}, specular{};
    std::array<VertexInstruction,136> vertex_instructions{};
    std::array<ShaderVector,16> vertex_constants{}, vertex_attributes{};
    // xyz: authored alpha test; w: original unwritten texture-W output mask.
    std::array<std::uint32_t,4> alpha_test{};
    std::array<std::array<std::uint32_t,4>,4> texture_effects{};
    std::array<ShaderVector,6> fog_constants{};
    std::array<ShaderVector,4> bump_matrices{};
    std::array<ShaderVector,7> source_camera_world{},source_camera_reflection{};
    std::array<ShaderVector,4> source_camera_foreground{};
    std::array<std::uint32_t,4> source_camera_control{};
    PackedSourceVertexDeclaration vertex_declaration=legacy_source_vertex_declaration;
};
static_assert(std::is_standard_layout_v<WorldMaterialUniform>);
static_assert(offsetof(WorldMaterialUniform,program)==0);
static_assert(offsetof(WorldMaterialUniform,transforms)==240);
static_assert(offsetof(WorldMaterialUniform,flags)==288);
static_assert(offsetof(WorldMaterialUniform,forward_transforms)==304);
static_assert(offsetof(WorldMaterialUniform,vertex_instructions)==464);
static_assert(offsetof(WorldMaterialUniform,alpha_test)==3152);
static_assert(offsetof(WorldMaterialUniform,texture_effects)==3168);
static_assert(offsetof(WorldMaterialUniform,fog_constants)==3232);
static_assert(offsetof(WorldMaterialUniform,bump_matrices)==3328);
static_assert(offsetof(WorldMaterialUniform,source_camera_world)==3392);
static_assert(offsetof(WorldMaterialUniform,source_camera_control)==3680);
static_assert(offsetof(WorldMaterialUniform,vertex_declaration)==3696);
static_assert(sizeof(WorldMaterialUniform)==3712);
} // namespace halo2
