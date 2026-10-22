#pragma once
#include "halo2/render_program.hpp"
#include <memory>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

namespace halo2 {
struct ResidentDrawUpload {
    VkDescriptorSet resident{};
    std::span<const std::byte> material;
    std::optional<PackedVertexProgramBank> vertex_bank;
};
// Native resource ownership only: share the resident authored image/sampler
// descriptors, retaining distinct material and optional vertex UBOs per draw.
// upload must follow the previous frame's fence. No gameplay capacity or
// original visibility/admission policy is imposed by this storage.
class ResidentDrawStorage {
public:
    ResidentDrawStorage(VkDevice,VkPhysicalDevice,VkDescriptorSetLayout,VkDeviceSize uniform_size);
    ~ResidentDrawStorage();
    std::span<const VkDescriptorSet> upload(std::span<const ResidentDrawUpload>);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
