#pragma once
#include "halo2/render_program.hpp"
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceWorldDrawStorageContext {
    VkDevice device{};
    VkPhysicalDevice physical{};
    VkDescriptorSetLayout texture_layout{};
    VkDeviceSize material_uniform_size{};
};
// Complete dimensional bank for one already-owned selected image page.
// Inactive dimensions retain the caller's actual resident fallback views.
struct SourceWorldDrawStageImages {
    VkSampler sampler{};
    VkImageView two_dimensional{},cube{},volume{};
};
struct SourceWorldDrawUpload {
    VkDescriptorSet resident{};
    std::span<const std::byte> material_uniform;
    PackedVertexProgramBank vertex_bank;
    // Optional completed stage0 external render target binding.
    VkImageView target_view{};
    VkSampler target_sampler{};
    std::array<std::optional<SourceWorldDrawStageImages>,4> stage_images;
};
// Draw-specific copies of resident image bindings and completed source banks.
// Each draw/view receives independent UBO slices; callers must finish the
// previous frame's fence before upload. Pipeline/admission/order stay outside.
class SourceWorldDrawStorage {
public:
    explicit SourceWorldDrawStorage(SourceWorldDrawStorageContext);
    ~SourceWorldDrawStorage();
    SourceWorldDrawStorage(const SourceWorldDrawStorage &)=delete;
    SourceWorldDrawStorage &operator=(const SourceWorldDrawStorage &)=delete;
    std::span<const VkDescriptorSet> upload(std::span<const SourceWorldDrawUpload>);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
