#pragma once
#include "halo2/source_shadow.hpp"
#include "halo2/source_shadow_raw_draw.hpp"
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
struct SourceShadowRawCallback {
    const SourceShadowRawDraw *source{};
    // Bind this exact raw binding/program, its actual main camera, owned
    // palette, caller raster/clip controls and external8 target10. Generate
    // uses generation_pass(); Apply uses the actual continuation pass.
    std::function<void(VkCommandBuffer,const SourceShadowUniforms &)> bind_and_draw;
};
struct SourceShadowRawSubmission {
    std::uint32_t root_owner=0xffffffffu;
    SourceShadowUniforms uniforms;
    std::span<const SourceShadowRawCallback> casters,receivers;
    // Actual completed bank header[2], Generate then Apply. Original32917 /
    //3292F gate phases on these masks, before3BAA0 can suppress every draw.
    std::array<std::uint32_t,2> command_masks{};
    bool filter=true;
};
// Fixed 47FDA0/32350 feature-mask0x14 target10 plus222A0 scratch11. This module does not
// produce caster hierarchy, PVS/subparts, owner selection/history or materials.
// Callback pipeline contracts: Generate has no depth attachment/test/write,
// no blending and RGBA writes; Apply preserves world depth with EQUAL/no
// write, DST_COLOR/ZERO blending and RGB writes. Both retain source culling
// and depth-clamp policy. Original texture extern8 binds target_view(), using
// authored linear BORDER sampling (white RGB, zero alpha) and pixel UV scale.
enum class SourceShadowTargetPhase { Generate,FilterTo11,FilterTo10,Apply,Complete };
struct SourceShadowTargetObservation {
    SourceShadowTargetPhase phase{};
    // Borrowed only immediately after the actual BeginRenderPass. Complete
    // has no begin packet; zero transitions explicitly means unchanged target.
    const VkRenderPassBeginInfo* begin{};
    std::size_t transitions{};
};
using SourceShadowTargetObserver=std::function<void(const SourceShadowTargetObservation&)>;
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
                std::span<const SourceShadowSubmission>,const SourceShadowTargetObserver& = {});
    // Separate admission for unchanged source geometry/class0..6 programs.
    // Sorted descriptors and ranges retain original order. This overload
    // shares only the target/filter lifecycle with the packed-world route;
    // it never admits raw geometry through that route's world-class guards.
    // Both actual command masks must contain their phase bit. Empty sorted
    // draw groups still execute the original clear/filter/continuation path.
    void record(VkCommandBuffer,const SourceShadowContinuation &,
                std::span<const SourceShadowRawSubmission>,const SourceShadowTargetObserver& = {});
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
