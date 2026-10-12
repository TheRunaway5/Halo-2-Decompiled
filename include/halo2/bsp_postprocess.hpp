#pragma once
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_bloom_context.hpp"
#include <functional>
#include <memory>
#include <vulkan/vulkan.h>
namespace halo2 {
struct BspPostprocessContext {
    VkDevice device{};
    VkPhysicalDevice physical{};
    VkExtent2D extent{};
    VkFormat color_format{},depth_format{};
    VkRenderPass color_pass{},continuation_pass{};
    VkImage depth{};
    VkImageView depth_view{};
    std::filesystem::path shader_directory;
    // Rendering consumes the original shared unsynchronized LCG. A caller
    // with a captured runtime seed can supply it for deterministic replay.
    std::uint32_t random_seed=0x27aa0u;
    VkImageLayout scene_layout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};
struct BspPostprocessView {
    Camera camera;
    BspFogSample atmosphere;
    VkRect2D rectangle{};
    // Normal player callers supply source_player_bloom_screen_context;
    // a missing source screen context preserves 13870's null-pointer branch.
    std::optional<SourceBloomScreenContext> bloom_screen_context;
};
struct BspPostprocessTarget {
    VkCommandBuffer command{};
    VkImage color{};
    VkFramebuffer color_framebuffer{},continuation_framebuffer{};
};
// Owns per-player motion, shared RNG, noise/bloom surfaces and GPU bindings.
// Recreate for a new swapchain context. Texture upload ownership stays with
// the caller; setup uploads every authored noise frame once.
class BspPostprocess {
public:
    BspPostprocess(BspPostprocessContext,const BspAtmosphere &,
        const std::function<VkImageView(const Texture &)> &upload);
    ~BspPostprocess();
    BspPostprocess(const BspPostprocess &)=delete;
    BspPostprocess &operator=(const BspPostprocess &)=delete;
    // Call after the previous submission's fence; no command buffer recording.
    void prepare(std::span<const BspPostprocessView>,float time);
    // Preserve CPU motion/RNG when GPU surfaces are recreated on resize.
    BspPostprocessState persistent_state() const;
    void restore_state(const BspPostprocessState &);
    // Original 2BCD0 schedules patchy before first person, bloom afterwards.
    // Each method ends the active main pass and returns with its LOAD
    // continuation active; caller rebinds geometry state afterwards.
    // An inactive patchy view leaves the active render pass untouched.
    void record_patchy(std::size_t view,const BspPostprocessTarget &);
    void record_bloom(std::size_t view,const BspPostprocessTarget &);
    // Combined form for callers whose view has no foreground geometry.
    void record(std::size_t view,const BspPostprocessTarget &);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
