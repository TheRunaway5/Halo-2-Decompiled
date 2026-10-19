#pragma once
#include "halo2/source_scene_copy.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceSceneCopyGpuContext {
    VkPhysicalDevice physical{};
    VkDevice device{};
    std::uint32_t queue_family{};
    VkFormat color_format{};
    std::filesystem::path shader_directory;
};
struct SourceSceneCopyGpuInput {
    SourceSceneCopyContext copy;
    // Source coordinate space and native image resolution remain distinct.
    // A host resolution conversion must be supplied explicitly by its owner.
    std::array<std::int32_t,2> coordinate_extent;
    VkExtent2D image_extent;
};
struct SourceSceneCopyGpuRecord {
    // Caller-provided device ownership of the actual image, command and fence,
    // followed by the actual main image's creation facts.
    VkDevice owner{};
    VkCommandBuffer command{};
    VkImage source{};
    VkFormat format{};
    VkExtent2D extent;
    VkImageUsageFlags usage{};
    VkImageLayout layout{}; // ordinary COLOR_ATTACHMENT_OPTIMAL only
    VkSampleCountFlagBits samples{};
    std::uint32_t mip_levels{},array_layers{},queue_family{};
    // A reset caller-owned fence. Submit this command exactly once with this
    // fence, covering every producer/consumer before retire() or destruction.
    VkFence completion{};
    bool outside_render_pass{},exclusive_access{};
};
struct SourceSceneCopyGpuImage {
    VkImage image{};
    VkImageView view{};
    VkFormat format{};
    VkExtent2D extent;
};
// Descriptor preparation facts only. This deliberately carries no image or
// produced-content/layout claim. Sampling requires record() first, in the
// same submitted command (or a successfully retired earlier submission).
struct SourceSceneCopyGpuBinding {
    VkImageView view{};
    VkFormat format{};
    VkExtent2D extent;
};
// Reusable native four-tap destination, including actual target19's320x240
// path. Copies the live main image before the group's body, restores that main
// image's layout and leaves the copy shader-readable. No readback or queue idle
// occurs while recording. Scheduling the group and binding extern17 stay with
// the caller; resource preparation alone is not a produced copy.
//
// prepare() is allowed only after retire(). Repeated records in one command
// share immutable camera/uniform inputs, with independent ordered copy writes.
// The returned image is borrowed until the next prepare() or destruction. While
// pending, consumers must be in the same command after record(); outside use
// requires a completed retire(). The Vulkan device/fence must outlive this
// object. Destruction waits for a pending submitted frame.
class SourceSceneCopyGpu {
public:
    explicit SourceSceneCopyGpu(SourceSceneCopyGpuContext);
    ~SourceSceneCopyGpu();
    SourceSceneCopyGpu(const SourceSceneCopyGpu &)=delete;
    SourceSceneCopyGpu &operator=(const SourceSceneCopyGpu &)=delete;
    // Allows descriptor allocation/upload before command recording. Preparing
    // this binding does not initialize the image or make it sample-ready.
    SourceSceneCopyGpuBinding prepare(const SourceSceneCopyGpuInput &);
    SourceSceneCopyGpuImage record(const SourceSceneCopyGpuRecord &);
    bool retire();
    // Only after the caller successfully resets a never-submitted command.
    // Restore the pre-record image-layout bookkeeping, without a GPU claim.
    void discard_unsubmitted(VkCommandBuffer reset_command);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
