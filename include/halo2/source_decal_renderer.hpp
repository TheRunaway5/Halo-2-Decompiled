#pragma once
#include "halo2/source_decal_draw.hpp"
#include <vulkan/vulkan.h>
namespace halo2 {
class SourceDecalRenderer {
public:
    SourceDecalRenderer(VkPhysicalDevice,VkDevice,VkQueue,unsigned,VkRenderPass,const std::filesystem::path&);
    ~SourceDecalRenderer();
    // Caller has completed its frame fence. Uploads copy actual written cache
    // pages; retained source storage is never a device command-buffer stand-in.
    void prepare(std::span<const SourceDecalDraw>);
    void record(VkCommandBuffer,VkViewport,VkRect2D,std::size_t first,std::size_t count,
        std::uint32_t layer_mask)const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
