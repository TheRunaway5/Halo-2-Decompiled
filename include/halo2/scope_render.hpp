#pragma once
#include "halo2/scope_copy.hpp"
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct ScopeRenderContext {
    VkDevice device{};
    VkPhysicalDevice physical{};
    VkExtent2D extent{};
    VkFormat scene_format{};
    VkRenderPass continuation_pass{};
    std::filesystem::path shader_directory;
    VkImageLayout scene_layout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};
// Original target16 and 2B000 copy. The single 64x64 target is reused in
// submission order: each view copies its world before drawing first person.
class ScopeRender {
public:
    explicit ScopeRender(ScopeRenderContext);
    ~ScopeRender();
    ScopeRender(const ScopeRender &)=delete;
    ScopeRender &operator=(const ScopeRender &)=delete;
    VkImageView target_view() const;
    void prepare(std::span<const std::optional<ScopeCopyFrame>>);
    // Ends the active scene pass and resumes its LOAD continuation. Caller
    // restores its viewport, scissor, camera and material bindings afterwards.
    void record(unsigned view,VkCommandBuffer,VkImage scene,VkFramebuffer continuation);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
