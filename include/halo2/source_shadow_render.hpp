#pragma once
#include "halo2/source_shadow.hpp"
#include <functional>
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceShadowRenderContext {
    VkDevice device{};
    VkPhysicalDevice physical{};
    VkExtent2D scene_extent{};
    // A compatible LOAD pass retaining both the scene color and world depth.
    VkRenderPass continuation_pass{};
    std::filesystem::path shader_directory;
};
struct SourceShadowContinuation {
    VkFramebuffer framebuffer{};
    VkViewport viewport{};
    VkRect2D scissor{};
};
struct SourceShadowDraw {
    const SourceShadowMaterial *authored{};
    // Caller binds the authored program/images, the actual camera bank and
    // this root's c31..36/c40..48, then submits its original ordered geometry.
    // Generate pipelines use generation_pass(); Apply uses continuation_pass.
    std::function<void(VkCommandBuffer,const SourceShadowUniforms &)> bind_and_draw;
};
struct SourceShadowSubmission {
    SourceShadowUniforms uniforms;
    std::span<const SourceShadowDraw> casters,receivers;
    // False only isolates the unfiltered generation stage in a diagnostic
    // fixture. The actual fixed color path always executes222A0's two passes.
    bool filter=true;
};
// Fixed 47FDA0/32350 feature-mask0x14 target10 plus222A0 scratch11. This module does not
// produce caster hierarchy, PVS/subparts, owner selection/history or materials.
// Callback pipeline contracts: Generate has no depth attachment/test/write,
// no blending and RGBA writes; Apply preserves world depth with EQUAL/no
// write, DST_COLOR/ZERO blending and RGB writes. Both retain source culling
// and depth-clamp policy. Original texture extern8 binds target_view(), using
// authored linear BORDER sampling (white RGB, zero alpha) and pixel UV scale.
class SourceShadowRender {
public:
    explicit SourceShadowRender(SourceShadowRenderContext);
    ~SourceShadowRender();
    SourceShadowRender(const SourceShadowRender &)=delete;
    SourceShadowRender &operator=(const SourceShadowRender &)=delete;
    static constexpr unsigned target_index=10, target_width=128,target_height=128;
    static constexpr VkFormat target_format=VK_FORMAT_B8G8R8A8_UNORM;
    VkImageView target_view()const;
    VkImage target_image()const;
    VkRenderPass generation_pass()const;
    // Called inside an active scene pass. Admitted roots execute sequentially
    // Generate→Filter(10→11→10)→Apply, then leave the LOAD scene pass open.
    // Empty caster/receiver groups do not interrupt the active scene pass.
    // Uniform/descriptor lifetimes and command-buffer/fence ownership belong
    // to the caller; records must remain valid until record() returns.
    void record(VkCommandBuffer,const SourceShadowContinuation &,
                std::span<const SourceShadowSubmission>);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
