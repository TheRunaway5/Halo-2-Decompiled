#pragma once
#include "halo2/pstencil/geometry.hpp"
#include <vulkan/vulkan.h>
#include <filesystem>
#include <functional>
#include <memory>

namespace halo2::pstencil {
struct RenderContext {
    VkDevice device;VkPhysicalDevice physical;VkExtent2D extent;
    VkRenderPass continuation;VkImage original_depth;
    std::span<const VkImageView> scene_views;
    std::filesystem::path shaders;
};
struct VolumeBatch {std::span<const ShadowVertex> vertices;};
struct VolumeCamera {
    std::array<float,16> matrix;
    ShaderVector depth_row,light,control;
    ShaderVector viewport;
};
struct VolumeDraw {std::size_t batch;VolumeCamera camera;};
static_assert(sizeof(VolumeCamera)==128);
// Independent attachment/buffer owner. prepare follows the renderer's frame
// fence; record temporarily leaves and resumes its loaded continuation pass.
// Original depth/stencil contents are never used as the counter or written.
class Render {
public:
    explicit Render(RenderContext);
    ~Render();
    Render(const Render&)=delete;
    Render& operator=(const Render&)=delete;
    void prepare(std::span<const VolumeBatch>);
    void begin(VkCommandBuffer,unsigned image);
    void light(VkCommandBuffer,std::size_t batch,const VolumeCamera&,VkViewport,VkRect2D,
               const std::function<void()>& receivers);
    VkRenderPass normal_pass()const;
    VkImageView normal_view()const;
    // Target9 retains source RGBA8 normal/alpha before normal-space lighting.
    void normals(VkCommandBuffer,const std::function<void()>& receivers);
    VkRenderPass utility_pass()const;
    VkImageView utility_view()const;
    // Target10 shares this light's private depth/stencil. Complete utility
    // writes become sampled inputs before the loaded scene pass resumes.
    void utility(VkCommandBuffer,const std::function<void()>& receivers);
    // Independent layer6/7 mask and one RGB152/255 destination multiply.
    // Call before environment/emission and before all dynamic lights.
    void lightmap(VkCommandBuffer,std::span<const VolumeDraw>,VkViewport,VkRect2D);
    void end(VkCommandBuffer,VkFramebuffer original);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace halo2::pstencil
