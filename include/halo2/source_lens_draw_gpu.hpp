#pragma once
#include "halo2/source_lens_draw_geometry_tu15.hpp"
#include <vulkan/vulkan.h>
#include <memory>
namespace halo2 {
struct SourceLensImageBinding {unsigned material{};VkImageView view{};VkSampler sampler{};};
struct SourceLensDrawGpuContext {
    VkPhysicalDevice physical{};VkDevice device{};VkRenderPass render_pass{};
    std::filesystem::path shader_directory;
};
// Renderer supplies its actual resident image views and retains them through
// completion. Prepare runs after the previous submission's fence has retired.
class SourceLensDrawGpu {
public:
    explicit SourceLensDrawGpu(SourceLensDrawGpuContext);
    ~SourceLensDrawGpu();
    void prepare(std::span<const SourceLensDraw>,std::span<const SourceLensImageBinding>);
    void record(VkCommandBuffer,std::span<const SourceLensDraw>,unsigned first,VkExtent2D,VkRect2D);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
