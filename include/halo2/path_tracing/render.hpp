#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include "halo2/native_world_kernel.hpp"

namespace halo2::path_tracing {
struct Scene;
struct GpuSceneUniform;
struct RenderContext {
    VkDevice device{};
    VkPhysicalDevice physical{};
    VkRenderPass render_pass{};
    VkDescriptorSetLayout material_layout{};
    std::filesystem::path shaders;
    VkCullModeFlags cull_mode=VK_CULL_MODE_NONE;
    // Vulkan 1.3 device: the cull mode is dynamic pipeline state, and each
    // bind takes the one set_draw_cull_mode names (see there).
    bool dynamic_cull=false;
    // The device enabled VK_KHR_acceleration_structure, VK_KHR_ray_query and
    // buffer device addresses. Rays then use the GPU's ray-tracing hardware
    // instead of the portable software BVH; the scene must clear
    // Scene::cpu_hierarchy before update_geometry.
    bool hardware_rays=false;
};
// Owns only the optional path-tracing pipeline and its scene buffers. Set zero
// retains the renderer's material descriptors; set one contains ray geometry.
// The caller waits for the preceding frame fence before upload or destruction,
// and recreates this owner when its render pass is replaced.
class Render {
public:
    static constexpr std::size_t camera_bytes=112;
    explicit Render(RenderContext);
    ~Render();
    Render(const Render&)=delete;
    Render& operator=(const Render&)=delete;
    // Each upload advances one immutable frame serial. Temporal lighting
    // histories reset for a new source, changed static/emissive inputs, quality
    // or target extent. Animated lights and occluders invalidate only affected
    // direct visibility; presented geometry poses use current GPU guides.
    void upload(const Scene&);
    void upload(const Scene&,const GpuSceneUniform&);
    // Hardware rays: build this frame's acceleration structures. Record once
    // per frame after upload, outside any render pass and before any draw that
    // traces. A no-op for software traversal.
    void record_acceleration(VkCommandBuffer);
    bool hardware_rays()const noexcept;
    // Bounded per-draw transport budget. Both choices are 1..2; the default
    // is one sample and one secondary bounce.
    void set_quality(unsigned samples,unsigned bounces);
    // A compact RT-only environment captured with the ordinary native sky
    // pipelines. Formats match that pipeline's color/depth render pass. Each
    // face is cleared/rendered before ray draws; a clear-only cube supplies
    // the authored fallback when no sky model is selected.
    // source_available selects the authored environment as a bounded direct
    // lighting bank. A positive uploaded fallback RGB also enables that bank.
    void prepare_environment(VkFormat color,VkFormat depth,unsigned side=64,bool source_available=true);
    void begin_environment(VkCommandBuffer,unsigned face,std::array<float,4> clear);
    void end_environment(VkCommandBuffer);
    // Optional exact-W visibility producer. prepare_visibility and upload are
    // host operations performed after the preceding frame fence. Record the
    // same opaque draws/camera/material bank/viewport/scissor in this cheap
    // pass before their ray-shaded phase. It uses a private D32 attachment and
    // never replaces the ordinary source D24S8 attachment.
    void prepare_visibility(VkExtent2D);
    // The actual Vulkan viewport, including any temporal-upscaler offset.
    // Repeated same-extent calls select the next world/foreground phase
    // without changing descriptors or allocating resources while recording.
    void prepare_visibility(VkExtent2D,const VkViewport& actual_viewport);
    // Current admitted ordinary camera/raster only. This immutable per-view
    // bank stays separate from PT's per-draw quality/material push words.
    void prepare_source_depth(std::array<float,4> viewport,std::array<float,4> clip_range);
    void bind_source_depth(VkCommandBuffer,VkDescriptorSet,std::uint32_t,
        std::span<const std::byte> camera,std::uint32_t material_index,
        const NativeWorldKernelEntry&,bool visibility=false,bool lighting=false)const;
    void begin_visibility(VkCommandBuffer);
    void bind_visibility(VkCommandBuffer,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                         std::span<const std::byte> camera,std::uint32_t material_index)const;
    void end_visibility(VkCommandBuffer);
    // Replay the same visible opaque bank once into bounded HDR lighting
    // records, retaining independent world/foreground temporal histories.
    // The ordinary surface pass then resolves lighting separately
    // from its full-resolution authored albedo. Default sampling is one cell
    // per2x2 pixels; larger targets adapt to maxStorageBufferRange.
    void begin_lighting(VkCommandBuffer);
    void bind_lighting(VkCommandBuffer,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
                       std::span<const std::byte> camera,std::uint32_t material_index)const;
    void end_lighting(VkCommandBuffer);
    // A different camera/foreground/reflection phase must either produce its
    // own visibility pass or disable this guard. upload also disables it.
    void disable_visibility()noexcept;
    // Cull mode for the next bind_visibility/bind_lighting/bind only, from the
    // draw's retail material state (as the Original forward pass culls it):
    // a one-sided exterior hull must not be seen from behind through a window.
    // Each bind resets it to VK_CULL_MODE_NONE. Ignored without dynamic_cull.
    void set_draw_cull_mode(VkCullModeFlags)noexcept;
    void bind(VkCommandBuffer,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
              std::span<const std::byte> camera)const;
    // A resident BSP draw can select a shader LOD/material without rewriting
    // its immutable vertex atlas. Keep that actual draw binding authoritative.
    void bind(VkCommandBuffer,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
              std::span<const std::byte> camera,std::uint32_t material_index)const;
    // An additive self-illumination layer (forward_vertex 2, blend 3) of a
    // surface this renderer just drew: the layer's own descriptor, the
    // surface's draw, camera and material, and the layer's place among that
    // material's world_overlays_after. Depth-equal to that surface; see
    // glow_layer.frag. Honors set_draw_cull_mode like the other binds.
    // False, binding nothing, when the scene holds no program for the layer.
    bool bind_glow_layer(VkCommandBuffer,VkDescriptorSet layer,std::uint32_t dynamic_bank_offset,
                         std::span<const std::byte> camera,std::uint32_t material_index,unsigned ordinal)const;
    VkPipeline pipeline()const noexcept;
    VkPipelineLayout layout()const noexcept;
    // Actual VkDeviceMemory allocation sizes for this owner's scene buffers.
    std::size_t allocated_bytes()const noexcept;
    // CPU-owned previous shadow positions/lights used by temporal validation.
    std::size_t cpu_bytes()const noexcept;
    struct TemporalBankStats {
        std::uint32_t sampled_cells{},sampled_valid_count{},maximum_count{};
        double mean_count{};
    };
    struct TemporalStats {
        std::uint32_t frame_serial{};
        std::array<TemporalBankStats,2> banks; // world, foreground
    };
    // Screenshot diagnostics only: record the host visibility dependency
    // outside a render pass, then query after that submission's fence.
    // Reads at most64 existing mapped records per history, with no copy or
    // GPU counter and no hot-frame CPU readback.
    void record_temporal_stats(VkCommandBuffer)const;
    TemporalStats temporal_stats()const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2::path_tracing
