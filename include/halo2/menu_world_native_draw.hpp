#pragma once
#include "halo2/menu_world_fragment.hpp"
#include "halo2/menu_world_raster.hpp"
#include "halo2/menu_world_source_storage.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include <memory>

namespace halo2 {
struct MenuWorldNativeDrawContext {
  VkDevice device{};
  VkPhysicalDevice physical{};
  VkRenderPass render_pass{}; // Compatible color+D24S8, sample1, subpass0.
  VkDescriptorSetLayout material_layout{}; // Existing set0; binding4 dynamic.
  // Actual logical-device enabled state. Physical support alone is insufficient.
  std::uint32_t api_version{};
  bool geometry_shader_enabled{},geometry_point_size_enabled{};
  bool depth_clamp_enabled{},demote_to_helper_enabled{};
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
  // Only0(disabled),5(planar CONSTANT_COLOR),10(sphere diffuse) admitted.
  unsigned forward_blend{};
  VkDescriptorSet material_set{};
  std::uint32_t material_offset{}; // Existing set0/binding4 dynamic offset.
};

// Standalone Vulkan resource/submission seam; selects no owners, visibility,
// materials, camera, passes, texture policy or source lifecycle. The caller
// validates/completes material/texture state with validate_menu_world_fragment
// and supplies the matching existing set0 descriptor and final material UBO.
//
// prepare() may be called ONLY after all GPU/command-buffer uses of the previous
// preparation have completed (normally the renderer frame fence). It retains
// binding ownership, uploads independent per-draw3104B/448B set1 slices, and
// stores raw immutable geometry. Failure invalidates the prepared draw list.
// record() is legal inside compatible subpass0; the caller establishes a
// positive-height viewport and matching scissor. It binds both sets (only the
// material offset is dynamic), raw streams/indices, then draws indexed. It
// changes bindings: the caller MUST rebind ordinary vertex buffer/pipeline/sets
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
