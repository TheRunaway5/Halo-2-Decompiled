#pragma once
#include "halo2/source_shadow_plan.hpp"
#include "halo2/source_world_draw_storage.hpp"
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceShadowDrawStorageContext {
    VkDevice device{};
    VkPhysicalDevice physical{};
    VkDescriptorSetLayout texture_layout{};
    VkPipelineLayout pipeline_layout{};
    VkRenderPass generation_pass{},application_pass{};
    std::filesystem::path shader_directory;
    VkDeviceSize material_uniform_size{};
};
using SourceShadowDrawUpload=SourceWorldDrawUpload;
// Frame-fence-owned storage for the explicit Renderer shadow hook. Each
// repeated slot/root/view gets an independent descriptor and UBO slice.
// upload() is legal only after all previous uses have completed. Pipelines
// reuse the production world_prt.vert/world.frag authored program lowering.
class SourceShadowDrawStorage {
public:
    explicit SourceShadowDrawStorage(SourceShadowDrawStorageContext);
    ~SourceShadowDrawStorage();
    SourceShadowDrawStorage(const SourceShadowDrawStorage &)=delete;
    SourceShadowDrawStorage &operator=(const SourceShadowDrawStorage &)=delete;
    std::span<const VkDescriptorSet> upload(std::span<const SourceShadowDrawUpload>);
    VkPipeline pipeline(const SourceShadowMaterial &,SourceShadowPhase,SourceShadowRasterInput);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
