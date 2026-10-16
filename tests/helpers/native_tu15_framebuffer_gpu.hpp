#pragma once
#include "halo2/native_tu15_framebuffer_startup.hpp"
#include "halo2/source_weather_depth_alias.hpp"
#include <bit>

namespace halo2_test {
// Real recorded producer ownership, deliberately never submitted. Cleanup
// must leave publication intact until the exact reset command is discarded.
inline void framebuffer_pending_cleanup_probe(halo2::NativeTu15FramebufferStartup& publication,
    halo2::NativeTu15FramebufferAliases& aliases) {
    auto& producer=aliases.depth_producer();const auto context=producer.context();
    const auto& depth=aliases.targets().depth_stencil;
    auto check=[](VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("Framebuffer cleanup probe Vulkan "+std::to_string(result));};
    struct Commands {
        VkDevice device;VkCommandPool pool{};VkFence fence{};
        ~Commands(){if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
    } commands{context.device};
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;pool.queueFamilyIndex=context.queue_family;
    check(vkCreateCommandPool(context.device,&pool,nullptr,&commands.pool));
    VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate.commandPool=commands.pool;allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=1;
    VkCommandBuffer command{};check(vkAllocateCommandBuffers(context.device,&allocate,&command));
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(context.device,&fence,nullptr,&commands.fence));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    halo2::SourceWeatherDepthAliasRecord record{};
    record.source={context.device,std::bit_cast<VkImage>(depth.image),VkFormat(depth.format),{depth.width,depth.height},
        depth.usage,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VkSampleCountFlagBits(depth.samples),
        depth.mips,depth.layers,context.queue_family,VK_SHARING_MODE_EXCLUSIVE};
    record.command_owner=context.device;record.command=command;record.completion=commands.fence;
    record.command_queue_family=context.queue_family;record.outside_render_pass=record.exclusive_access=true;
    producer.record(record);
    try {
        bool rejected=false;try{publication.release();}catch(const std::runtime_error&){rejected=true;}
        if(!rejected||!publication.initialized()||!producer.has_pending_submission())
            throw std::runtime_error("Framebuffer cleanup consumed an outstanding producer's publication");
    } catch(...) {check(vkResetCommandBuffer(command,0));producer.discard_unsubmitted(command);throw;}
    check(vkResetCommandBuffer(command,0));producer.discard_unsubmitted(command);
    if(producer.has_pending_submission())throw std::runtime_error("Discarded framebuffer probe retained pending ownership");
}
}
