#pragma once
#include "halo2/menu_world_fragment.hpp"
#include "halo2/menu_world_raster.hpp"
#include "halo2/menu_world_source_storage.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/source_depth_storage.hpp"
#include <memory>
#include <filesystem>

namespace halo2 {
struct MenuWorldNativeDrawContext {
  VkDevice device{};
  VkPhysicalDevice physical{};
  VkRenderPass render_pass{}; // Compatible color+declared depth storage, sample1, subpass0.
  VkDescriptorSetLayout material_layout{}; // Existing set0; binding4 dynamic.
  // Actual logical-device enabled state. Physical support alone is insufficient.
  std::uint32_t api_version{};
  bool geometry_shader_enabled{},geometry_point_size_enabled{};
  bool depth_clamp_enabled{},demote_to_helper_enabled{};
  // Empty retains the configured production directory. Explicit captures
  // carry the same private shader snapshot through the raw geometry backend.
  std::filesystem::path shader_directory;
  // Explicit alternate shader interface; all program/declaration identities and
  // source owner admission remain unchanged. Requires compiled triangle ABI1.
  bool triangle_vertices{};
  SourceDepthStorageEncoding depth_encoding=SourceDepthStorageEncoding::native_d24;
  // Actual instance policy, configured before device creation by the macOS
  // SourceShaderMath owner. Required for triangle vertices on Metal.
  bool strict_shader_math_enabled{};
};
struct MenuWorldNativeDrawInput {
  MenuWorldSourceBinding source;
  // Immutable generated catalog storage must outlive this service. Identity is
  // literal program/declaration, never the original datum or a fixture owner.
  const MenuWorldVertexCatalogEntry *vertex_shader{};
  MenuWorldVertexUniforms vertex;
  MenuWorldFragmentUniforms fragment;
  MenuWorldRasterPacket raster;
  ForwardRasterState forward;
  //0(disabled),1(hologram SRC_ALPHA/INV_SRC_ALPHA),3(projected ONE/ONE),5(planar CONSTANT_COLOR),
  //8(projected DST_ALPHA/ONE),9(shadow DST_COLOR/ZERO), and10(sphere
  //diffuse). Pass/receiver admission is caller owned.
  unsigned forward_blend{};
  VkDescriptorSet material_set{};
  std::uint32_t material_offset{}; // Existing set0/binding4 dynamic offset.
  MenuWorldWClipMode w_clip_mode=MenuWorldWClipMode::cull;
  std::array<ShaderVector,16> vertex_attributes{};
  std::array<std::uint8_t,16> vertex_attribute_components{};
  bool fog_alpha_bound{};
  // Complete CPU-side identity. GPU uniforms carry only completed colors;
  // this key selects immutable compiled fragment arithmetic.
  std::array<std::uint32_t,60> fragment_program{};
  // Native temporal raster extension, in physical pixels after the original
  // viewport conversion. Zero retains the original fragment/layout path.
  std::array<float,2> raster_offset{};
};

// Standalone Vulkan resource/submission seam; selects no owners, visibility,
// materials, camera, passes, texture policy or source lifecycle. The caller
// validates/completes material/texture state with validate_menu_world_fragment
// and supplies the matching existing set0 descriptor and final material UBO.
//
// prepare() may be called ONLY after all GPU/command-buffer uses of the previous
// preparation have completed (normally the renderer frame fence). It retains
// binding ownership, uploads independent per-draw ABI2/ABI3 and448B set1 slices, and
// stores raw immutable geometry. Failure invalidates the prepared draw list.
// record() is legal inside compatible subpass0; the caller establishes a
// positive-height viewport and matching scissor. It binds both sets (only the
// material offset is dynamic), raw streams/indices, then draws indexed. It
// Changes bindings; the optional temporal variant also writes its fragment
// push tail at112. The caller MUST rebind ordinary vertex buffer/pipeline/sets
// before the next packed CPU draw. No viewport, pass, queue, wait or submission
// is performed here. Its112B vertex push range preserves existing layout state.
//
// Destroy/recreate with the render pass/swapchain, after GPU completion. Device,
// material layout/sets/buffers and catalog entries must remain valid throughout
// their uses. No destructor wait conceals missing caller fence ownership.
class MenuWorldNativeDraw {
public:
  explicit MenuWorldNativeDraw(MenuWorldNativeDrawContext);
  ~MenuWorldNativeDraw();
  MenuWorldNativeDraw(const MenuWorldNativeDraw &)=delete;
  MenuWorldNativeDraw &operator=(const MenuWorldNativeDraw &)=delete;
  void prepare(std::span<const MenuWorldNativeDrawInput>);
  void record(VkCommandBuffer,std::size_t index) const;
  std::size_t size() const;
  VkPipelineLayout pipeline_layout() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
