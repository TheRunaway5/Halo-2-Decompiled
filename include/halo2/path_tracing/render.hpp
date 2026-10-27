#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

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
    // histories reset for a new source, changed light/emissive energy, quality
    // or target extent; ordinary geometry poses are rejected by GPU guides.
    void upload(const Scene&);
    void upload(const Scene&,const GpuSceneUniform&);
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
    void bind(VkCommandBuffer,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
              std::span<const std::byte> camera)const;
    // A resident BSP draw can select a shader LOD/material without rewriting
    // its immutable vertex atlas. Keep that actual draw binding authoritative.
    void bind(VkCommandBuffer,VkDescriptorSet material,std::uint32_t dynamic_bank_offset,
              std::span<const std::byte> camera,std::uint32_t material_index)const;
    VkPipeline pipeline()const noexcept;
    VkPipelineLayout layout()const noexcept;
    // Actual VkDeviceMemory allocation sizes for this owner's scene buffers.
    std::size_t allocated_bytes()const noexcept;
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
