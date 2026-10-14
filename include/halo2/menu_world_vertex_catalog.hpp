#pragma once
#include "halo2/menu_world_vertex_source.hpp"
#include <cstddef>
#include <span>
#include <string_view>

namespace halo2 {
// Version the complete fetch/output/UBO/epilogue contract, not a source datum.
// Changing that contract requires a new version and original-input GPU proof.
inline constexpr std::uint32_t menu_world_vertex_catalog_abi = 2;
inline constexpr unsigned menu_world_vertex_descriptor_set = 1;
inline constexpr unsigned menu_world_vertex_descriptor_binding = 0;

// Compact std140 draw/view bank. Every value is supplied by the actual caller;
// this type selects neither a camera, fog policy, owner nor clip lifecycle.
struct alignas(16) MenuWorldVertexUniforms {
  std::array<ShaderVector,192> constants{};
  ShaderVector clip_range{};
  std::array<float,2> fog_parameters{}, surface_size{};
};
static_assert(offsetof(MenuWorldVertexUniforms,constants)==0);
static_assert(offsetof(MenuWorldVertexUniforms,clip_range)==3072);
static_assert(offsetof(MenuWorldVertexUniforms,fog_parameters)==3088);
static_assert(offsetof(MenuWorldVertexUniforms,surface_size)==3096);
static_assert(sizeof(MenuWorldVertexUniforms)==3104);

struct MenuWorldVertexShaderSource {
  std::string glsl;
  std::vector<unsigned> consumed_inputs, consumed_constants;
};
// Actual class0..6 declaration ordinals determine input locations. Position/UV
// retain floating/SNORM fetches; model influences retain byte UNORM fetches;
// packed basis uses signed int32. ABI2 adds local model palette addressing.
// Missing consumed constant attributes and other semantics are explicit errors.
// The ordinary GS interface is locations0..12. No shader compiler runs here.
MenuWorldVertexShaderSource menu_world_vertex_shader_source(
    std::span<const std::byte> program,const SourceVertexDeclaration &);

// Generated immutable storage owns these spans for the binary's lifetime.
// The complete source program's SHA-256, extent and ordered semantics are the
// lookup identity; datum/name are deliberately absent. Only port GLSL and
// program identities ship in the source tree. SPIR-V is compiled at build time.
struct MenuWorldVertexCatalogEntry {
  std::uint32_t abi{}, class_index{};
  std::string_view program_sha256;
  std::size_t program_bytes{};
  std::span<const std::uint16_t> semantics;
  std::span<const std::uint32_t> spirv;
  std::span<const unsigned> consumed_inputs, consumed_constants;
  std::string_view source_sha256, spirv_sha256;
};
// Requires trusted generated catalog storage, fully checked by the build-time
// SPIR-V validator. Runtime admission checks program identity, ambiguity and
// ABI/SPIR-V header compatibility; it is not a validator for arbitrary modules.
// No subprocess, compiler, filesystem fallback or captured-identity selector.
const MenuWorldVertexCatalogEntry &menu_world_vertex_catalog_lookup(
    std::span<const MenuWorldVertexCatalogEntry>,std::span<const std::byte>,
    const SourceVertexDeclaration &);
} // namespace halo2
