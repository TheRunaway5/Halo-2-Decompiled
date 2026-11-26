#pragma once

#include "halo2/cache.hpp"
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <vulkan/vulkan.h>

namespace halo2 {

struct ObjectMotionBlurVertex {
    Vec3 current, previous;
};

// Reprojects consecutive object poses in the current camera and filters the
// copied scene color only where the current geometry matches the scene depth.
class ObjectMotionBlurPass {
public:
    ObjectMotionBlurPass(VkPhysicalDevice, VkDevice, VkPipelineCache, VkRenderPass,
        const std::filesystem::path& shader_directory, VkImage depth,
        VkImageView depth_view, VkImageView copied_color);
    ~ObjectMotionBlurPass();

    // The renderer waits for the previous frame before reusing upload slots.
    void begin_frame();
    // Outside a render pass, with the scene copy ready to sample and D24S8
    // in its attachment layout. The scene and depth layouts are restored.
    void record(VkCommandBuffer, VkFramebuffer, VkViewport, VkRect2D,
        const glm::mat4& matrix, glm::vec2 jitter, bool w_depth,
        float shutter_fraction, std::span<const ObjectMotionBlurVertex>);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
