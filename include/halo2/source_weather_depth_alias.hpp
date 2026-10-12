#pragma once
#include "halo2/cache.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceWeatherDepthAliasStorage {
    std::uint32_t pixels{};
    VkDeviceSize depth_bytes{}, stencil_bytes{};
};
// Checked integer sizing, including the final complete stencil SSBO word.
SourceWeatherDepthAliasStorage source_weather_depth_alias_storage(VkExtent2D);

struct SourceWeatherDepthAliasContext {
    // Actual Vulkan 1.1 (or later) device; no optional device features needed.
    VkPhysicalDevice physical{};
    VkDevice device{};
    std::uint32_t queue_family{};
    std::filesystem::path shader_directory;
};
struct SourceWeatherDepthAliasImage {
    // These are caller-provided facts about an actual image. Vulkan cannot
    // reflect an opaque image handle's creation metadata or device ownership.
    VkDevice owner{};
    VkImage image{};
    VkFormat format{};
    VkExtent2D extent{};
    VkImageUsageFlags usage{};
    VkImageLayout layout{};
    VkSampleCountFlagBits samples{};
    std::uint32_t mip_levels{}, array_layers{}, queue_family{};
    VkSharingMode sharing{};
    // Full, ordinary 2D optimal-tiling image, at mip0/layer0. Sparse, protected,
    // disjoint, subsampled and compressed creation paths are not admitted.
    VkImageType type{VK_IMAGE_TYPE_2D};
    VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
    VkImageCreateFlags create_flags{};
};
struct SourceWeatherDepthAliasRecord {
    SourceWeatherDepthAliasImage source;
    VkDevice command_owner{};
    VkCommandBuffer command{};
    // A live, caller-owned, reset/unsignaled fence. The caller MUST submit
    // this command with this fence, covering the producer and ALL consumers.
    // Keep the fence and source alive until retire() succeeds. A successful
    // record must not be submitted again. Recovery of a never-submitted,
    // successfully reset command is possible only via discard_unsubmitted().
    // No other accesses to
    // source/alias, queue-family transfers or concurrent producer calls are
    // permitted before completion. The command must be recording outside a
    // render pass, on this context's graphics+compute queue family.
    VkFence completion{};
    std::uint32_t command_queue_family{};
    bool outside_render_pass{}, exclusive_access{};
};
struct SourceWeatherDepthAliasBinding {
    VkImage image{};
    VkImageView view{};
    VkSampler sampler{};
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    VkExtent2D extent{};
    VkImageLayout layout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    std::uint32_t mips{1};
};
// Exact D24S8 current-image producer. Copies integer depth and
// stencil aspects and writes [N>>8, N, stencil, N>>16] low bytes in native row
// order. No float depth or row flip. Frame history and activation belong to
// the caller; the native renderer now supplies its actual current attachment.
// prepare() may resize only after retire(); output handles are borrowed and
// may not outlive the next prepare() or this object. record() restores the
// source layout and leaves the alias shader-readable for fragment/compute.
// The destructor waits for an outstanding completion fence; the caller must
// obey the submission/fence lifetime contract above to make that wait valid.
class SourceWeatherDepthAlias {
public:
    explicit SourceWeatherDepthAlias(SourceWeatherDepthAliasContext);
    ~SourceWeatherDepthAlias();
    SourceWeatherDepthAlias(const SourceWeatherDepthAlias &)=delete;
    SourceWeatherDepthAlias &operator=(const SourceWeatherDepthAlias &)=delete;
    void prepare(VkExtent2D);
    void record(const SourceWeatherDepthAliasRecord &);
    // Descriptor target only: not a readiness claim. It may be bound before
    // recording, but no GPU consumer may execute before record()'s producer.
    SourceWeatherDepthAliasBinding prepared_binding() const;
    SourceWeatherDepthAliasBinding binding() const;
    // Recovery only after this exact command was NEVER submitted and has
    // successfully been reset. Caller owns that execution fact; Vulkan cannot
    // reflect it from opaque handles. Restores the pre-record layout history.
    void discard_unsubmitted(VkCommandBuffer);
    // Rejects absent, different or unsignaled completion fences. The caller
    // may reset/reuse/destroy the fence only after this succeeds.
    void retire(VkFence);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
