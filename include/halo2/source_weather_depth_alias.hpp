#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_depth_storage.hpp"
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
    // Additional caller fact for the D32S8 carrier: every depth word was
    // produced by source_d24_carrier_word / its GPU equivalent. Unconverted
    // D32 values and ordinary floating clear/write paths are not admitted.
    SourceDepthStorageEncoding depth_encoding=SourceDepthStorageEncoding::native_d24;
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
// Original texture color-format fields used by TU1.5 145E0 at 515E48/515E4C.
// Both are unsigned normalized 8-bit channels over the same D24S8 bytes.
enum class SourceWeatherDepthAliasFormat : std::uint32_t {
    linear_a8r8g8b8 = 0x12,
    linear_a8b8g8r8 = 0x3f,
};
// Exact source-D24S8 current-image producer. Accepts native D24S8 or the
// explicitly declared canonical D32S8 integer carrier. Copies integer depth and
// stencil aspects and writes [N>>8, N, stencil, N>>16] low bytes in native row
// order. No float interpolation/quantization or row flip. Frame history and activation belong to
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
    // Actual device/queue facts for the retained producer, useful to native
    // command owners. Does not expose a submission or readiness claim.
    SourceWeatherDepthAliasContext context() const;
    bool has_pending_submission() const;
    void record(const SourceWeatherDepthAliasRecord &);
    // Color interpretation of the same source bytes (TU1.5 surface515E50).
    // begin_color() produces the current alias and leaves it as a color
    // attachment. The caller records color operations outside this module,
    // then calls end_color() after ending its render pass. Source depth/stencil
    // MUST remain unchanged between these calls. Each color phase writes its
    // exact final bytes back before later depth or sampled-alias consumers.
    // This does not admit simultaneous depth writes or depth/color feedback
    // within a draw. Those source consumers require their own validation.
    // Source requires TRANSFER_DST in addition to record()'s ordinary facts.
    // One exact command/fence covers production, color writes and writeback;
    // discard_unsubmitted() is the only recovery from an unfinished phase.
    SourceWeatherDepthAliasBinding begin_color(const SourceWeatherDepthAliasRecord &);
    void end_color(VkCommandBuffer);
    // Descriptor target only: not a readiness claim. It may be bound before
    // recording, but no GPU consumer may execute before record()'s producer.
    SourceWeatherDepthAliasBinding prepared_binding() const;
    SourceWeatherDepthAliasBinding binding() const;
    // These views share one production and completion fence. The default
    // remains A8R8G8B8; A8B8G8R8 samples [stencil, N, N>>8, N>>16]. View
    // swizzling affects sampling only, not image transfer/readback bytes.
    SourceWeatherDepthAliasBinding prepared_binding(SourceWeatherDepthAliasFormat) const;
    SourceWeatherDepthAliasBinding binding(SourceWeatherDepthAliasFormat) const;
    // Recovery only after this exact command was NEVER submitted and has
    // successfully been reset. Caller owns that execution fact; Vulkan cannot
    // reflect it from opaque handles. Restores the pre-record layout history.
    void discard_unsubmitted(VkCommandBuffer);
    // Rejects absent, different or unsignaled completion fences. The caller
    // may reset/reuse/destroy the fence only after completion. Noncanonical
    // D32 carrier words invalidate the production and throw here; consumers
    // from such a failed submission must not be accepted as a source result.
    void retire(VkFence);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
