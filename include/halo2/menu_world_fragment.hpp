#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cstddef>
#include <limits>

namespace halo2 {
inline constexpr unsigned menu_world_fragment_descriptor_set=1;
inline constexpr unsigned menu_world_fragment_descriptor_binding=1;
// Explicit admitted NV097_SET_ZMIN_MAX_CONTROL values. The shader applies
// this after reconstructed W plus polygon offset, before D24 conversion.
// Retained ordinary callers keep cull; shadow callers must supply clamp.
enum class MenuWorldWClipMode : std::uint32_t { cull=0, clamp=16 };

// Actual caller-owned ordinary-world state, not defaults selected from a
// screen, target extent or a retained capture. The matching GLSL block is
// set1/binding1, beside the independent raw-vertex bank at set1/binding0.
// W clipping is a separate explicit pipeline specialization; these original
// 448 bytes and their independently captured fixture offsets remain intact.
struct alignas(16) MenuWorldFragmentUniforms {
  std::array<float,4> clip_range;
  std::array<std::array<std::int32_t,4>,8> regions;
  std::array<float,2> depth_factor_offset;
  std::array<std::int32_t,2> surface_scale;
  // Original interleaved C0/C1 stages0..7, then final C0/C1. Missing binding
  // is deliberately nonfinite even when the completed program colors are0.
  std::array<ShaderVector,18> combiner_constants=[] {
    std::array<ShaderVector,18> result;
    for(auto &color:result)for(auto &lane:color)lane=std::numeric_limits<float>::quiet_NaN();
    return result;
  }();
};
static_assert(offsetof(MenuWorldFragmentUniforms,clip_range)==0);
static_assert(offsetof(MenuWorldFragmentUniforms,regions)==16);
static_assert(offsetof(MenuWorldFragmentUniforms,depth_factor_offset)==144);
static_assert(offsetof(MenuWorldFragmentUniforms,surface_scale)==152);
static_assert(offsetof(MenuWorldFragmentUniforms,combiner_constants)==160);
static_assert(sizeof(MenuWorldFragmentUniforms)==448);

// The caller must supply the FINAL selected owner/pass/view program after
// authored remaps, externs, animation and per-view overrides. This performs
// only exact packed-byte expansion; it does not select a pass, finish a
// template, infer live inputs, or substitute captured values. Rebind after
// any completed program change. Captured inputs belong only in diagnostics.
void bind_menu_world_fragment_completed_program(MenuWorldFragmentUniforms &,
    const PixelProgram &completed_selected_program);

enum class MenuWorldFragmentDepth { unspecified, clipped_w_d24 };
enum class MenuWorldFragmentWindow { unspecified, inclusive, exclusive };
enum class MenuWorldFragmentColors { unspecified, front, back_or_two_sided };
enum class MenuWorldFragmentPrimitive { unspecified, ordinary_triangles, other };
enum class MenuWorldTextureCoordinates { unspecified, normalized, pixel };
enum class MenuWorldTextureSemantics { unspecified, ordinary_rgba, other };
struct MenuWorldFragmentTexture {
  const Texture *image{};
  MenuWorldTextureCoordinates coordinates=MenuWorldTextureCoordinates::unspecified;
  // Caller proves ordinary decoded RGBA sampling: no shadow comparison,
  // border-size adjustment, convolution, palette/HILO or hidden coordinate
  // remap. Texture alone does not retain those original hardware facts.
  MenuWorldTextureSemantics semantics=MenuWorldTextureSemantics::unspecified;
};
struct MenuWorldFragmentSamplerLimits {
  float maximum_lod_bias{},maximum_anisotropy{};
  bool anisotropy_enabled{};
  bool custom_border_colors_enabled{};
};
struct MenuWorldFragmentInput {
  // Program is the completed selected pass after extern/remap/animation
  // binding. Its constants must match the explicitly bound float bank;
  // this API never selects or completes a template/pass.
  const PixelProgram *program{};
  const ForwardRasterState *raster{};
  const std::array<ShaderTextureState,4> *texture_states{};
  const MenuWorldFragmentSamplerLimits *sampler_limits{};
  std::array<MenuWorldFragmentTexture,4> textures{};
  const MenuWorldFragmentUniforms *uniforms{};
  MenuWorldFragmentDepth depth=MenuWorldFragmentDepth::unspecified;
  MenuWorldWClipMode w_clip_mode=MenuWorldWClipMode::cull;
  MenuWorldFragmentWindow window=MenuWorldFragmentWindow::unspecified;
  MenuWorldFragmentColors colors=MenuWorldFragmentColors::unspecified;
  MenuWorldFragmentPrimitive primitive=MenuWorldFragmentPrimitive::unspecified;
  // Enabled vkCreateDevice feature, not merely physical-device support.
  bool demote_to_helper_enabled{};
  // Explicit interpolated vertex fog alpha. Fog RGB remains unbound; the
  // caller's actual VS/fog-parameter packet owns this attenuation input.
  bool fog_alpha_bound{};
};

// Bounded admission for original PROJECT2D/PROJECT3D/CUBE/NONE families and
// texture clip planes. No ordinary stencil/blend/cull/lifecycle is selected
// here. Unsupported state fails instead of entering native texture fallback.
void validate_menu_world_fragment(const MenuWorldFragmentInput &);
} // namespace halo2
