#pragma once
#include "halo2/source_contrail_draw.hpp"
#include <vulkan/vulkan.h>
namespace halo2 {
class SourceContrailRenderer {
public:
    SourceContrailRenderer(VkPhysicalDevice,VkDevice,VkQueue,unsigned,VkRenderPass,const std::filesystem::path&);
    ~SourceContrailRenderer();
    // Caller has completed its frame fence. Uploads copy actual original geometry
    // output; retained source storage is never a device command-buffer stand-in.
    void prepare(std::span<const SourceContrailDraw>);
    void record(VkCommandBuffer,VkViewport,VkRect2D,std::size_t first,std::size_t count)const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
