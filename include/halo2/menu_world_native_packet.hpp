#pragma once
#include "halo2/menu_world_native_draw.hpp"
#include "halo2/world_material_uniform.hpp"

namespace halo2 {
// Explicit facts for the bounded ordinary, front-color, real RGBA raw
// bridge. The caller owns pass/owner/view selection and proves that material
// is the FINAL uploaded set0/binding4 value for material_set/material_offset.
// texture_states are the states actually used to create the bound samplers.
// images identify the actual committed descriptor images after animation;
// nullptr means fallback/unknown, never a synthetic white/cube/volume image.
// Image metadata may omit pixels, but must come from the actual real upload.
// Pixel coordinates require an explicit stage0 PROJECT2D binding to its real
// committed image; the shader divides by that image's actual dimensions.
// Signed/color-key images and other hidden sampling remaps remain excluded.
struct MenuWorldNativePacketInput {
  const MenuWorldSourceBinding &source;
  std::span<const MenuWorldVertexCatalogEntry> catalog;
  const WorldMaterialUniform &material;
  std::array<std::uint8_t,8> fragment_offsets{};
  ForwardRasterState forward;
  unsigned forward_blend{};
  std::array<ShaderTextureState,4> texture_states{};
  std::array<const Texture *,4> images{};
  MenuWorldFragmentSamplerLimits sampler_limits;
  MenuWorldRasterPacket raster;
  std::array<float,4> clip_range{};
  std::array<float,2> fog_parameters{},surface_size{};
  bool demote_to_helper_enabled{};
  VkDescriptorSet material_set{};
  // Only the bank offset: the descriptor already includes its material slot.
  // Actual Vulkan alignment is checked by MenuWorldNativeDraw::prepare.
  std::uint32_t material_offset{};
  std::array<MenuWorldTextureCoordinates,4> texture_coordinates{
      MenuWorldTextureCoordinates::normalized,MenuWorldTextureCoordinates::normalized,
      MenuWorldTextureCoordinates::normalized,MenuWorldTextureCoordinates::normalized};
  MenuWorldWClipMode w_clip_mode=MenuWorldWClipMode::cull;
  // Authored/material-completed XYZW coverage for the separate attribute bank.
  // A zero mask proves no values; the packet rejects every uncovered read.
  std::array<std::uint8_t,16> vertex_attribute_components{};
  bool fog_alpha_bound{};
};

// Pure packet assembly and completed-program admission; no Vulkan calls,
// compiler, pass/texture selection, defaults or lifecycle reconstruction.
// Copies the source c[192] bank without replacing owner/view constants. The
// source raster packet supplies its real inclusive/exclusive window policy;
// the current fragment bridge rejects exclusive windows rather than changing
// them. Ordinary triangles, front colors and explicitly declared real RGBA
// sampling coordinates are the bounded contract above, not inferred state.
//
// The returned source/packets are owned. Trusted immutable catalog storage,
// descriptor/UBO contents and their external Vulkan lifetimes remain the
// caller's responsibility. Fixed pipeline admission, alignment, device/render
// pass compatibility and GPU synchronization remain with the draw service.
MenuWorldNativeDrawInput menu_world_native_packet(const MenuWorldNativePacketInput &);
} // namespace halo2
