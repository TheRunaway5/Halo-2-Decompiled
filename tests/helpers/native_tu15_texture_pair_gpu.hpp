#pragma once
#include "halo2/renderer_texture_pairs.hpp"
#include <bit>
#include <array>
#include <cstring>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>

namespace halo2_test {
inline nlohmann::json texture_pair_gpu_probe(const halo2::RendererTexturePairFactory& factory) {
    auto images=factory.create();const auto context=images.context();const auto device=context.device;
    auto check=[](VkResult result){if(result!=VK_SUCCESS)throw std::runtime_error("Native texture-pair probe Vulkan "+std::to_string(result));};
    auto require=[](bool result){if(!result)throw std::runtime_error("Native texture-pair GPU ownership/pixel proof differs");};
    auto rejected=[&](auto&& action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}require(caught);};
    const auto identity=images.image(0).image;const auto backing=images.image(0).image_backing;auto moved=std::move(images);
    rejected([&]{images.image(0);});require(moved.image(0).image==identity&&moved.image(0).image_backing==backing);
    rejected([&]{moved.image(2);});
    bool wrong_thread=false;std::thread outsider([&]{try{moved.image(0);}catch(const std::runtime_error&){wrong_thread=true;}});outsider.join();require(wrong_thread);
    struct Commands {
        VkDevice device;VkCommandPool pool{};VkFence fence{};VkRenderPass pass{};
        std::array<VkFramebuffer,2> targets{};VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};bool submitted{};
        ~Commands(){if(submitted)vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX);
            if(mapped)vkUnmapMemory(device,memory);if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(memory)vkFreeMemory(device,memory,nullptr);
            for(auto target:targets)if(target)vkDestroyFramebuffer(device,target,nullptr);
            if(pass)vkDestroyRenderPass(device,pass,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);}
    } gpu{device};
    check(vkDeviceWaitIdle(device));
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=context.queue_family;
    check(vkCreateCommandPool(device,&pool,nullptr,&gpu.pool));
    VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};allocate.commandPool=gpu.pool;
    allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=1;VkCommandBuffer command{};check(vkAllocateCommandBuffers(device,&allocate,&command));
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&fence,nullptr,&gpu.fence));
    constexpr VkDeviceSize stride=64*64*4,total=stride*2;
    VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};buffer.size=total;buffer.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    check(vkCreateBuffer(device,&buffer,nullptr,&gpu.buffer));
    VkMemoryRequirements needed;vkGetBufferMemoryRequirements(device,gpu.buffer,&needed);
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(context.physical,&properties);
    unsigned type=properties.memoryTypeCount;
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((needed.memoryTypeBits&(1u<<i))&&
        (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
    require(type<properties.memoryTypeCount);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=needed.size;allocation.memoryTypeIndex=type;
    check(vkAllocateMemory(device,&allocation,nullptr,&gpu.memory));check(vkBindBufferMemory(device,gpu.buffer,gpu.memory,0));
    check(vkMapMemory(device,gpu.memory,0,VK_WHOLE_SIZE,0,&gpu.mapped));
    VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_B8G8R8A8_UNORM;attachment.samples=VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};
    subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&reference;
    std::array<VkSubpassDependency,2> deps{};deps[0].srcSubpass=VK_SUBPASS_EXTERNAL;deps[0].dstSubpass=0;
    deps[0].srcStageMask=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;deps[0].dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;deps[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].srcSubpass=0;deps[1].dstSubpass=VK_SUBPASS_EXTERNAL;deps[1].srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[1].dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;deps[1].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;deps[1].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=1;pass.pAttachments=&attachment;
    pass.subpassCount=1;pass.pSubpasses=&subpass;pass.dependencyCount=2;pass.pDependencies=deps.data();check(vkCreateRenderPass(device,&pass,nullptr,&gpu.pass));
    const std::array<std::array<unsigned,4>,2> background{{{17,68,153,221},{203,97,31,79}}},patch{{{1,2,3,4},{251,252,253,254}}};
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    for(unsigned i=0;i<2;++i) {
        const auto& image=moved.image(i);require(image.format==VK_FORMAT_B8G8R8A8_UNORM&&image.width==64&&image.height==64&&
            image.image_backing.value>0xffffffffu&&image.view_backing.value>0xffffffffu&&image.mips==1);
        const auto view=std::bit_cast<VkImageView>(image.view);VkFramebufferCreateInfo target{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        target.renderPass=gpu.pass;target.attachmentCount=1;target.pAttachments=&view;target.width=target.height=64;target.layers=1;
        check(vkCreateFramebuffer(device,&target,nullptr,&gpu.targets[i]));
        VkClearValue clear{};for(unsigned j=0;j<4;++j)clear.color.float32[j]=float(background[i][j])/255.f;
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=gpu.pass;render.framebuffer=gpu.targets[i];
        render.renderArea.extent={64,64};render.clearValueCount=1;render.pClearValues=&clear;vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
        VkClearAttachment partial{};partial.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        for(unsigned j=0;j<4;++j)partial.clearValue.color.float32[j]=float(patch[i][j])/255.f;
        VkClearRect rect{{{7,9},{11,13}},0,1};vkCmdClearAttachments(command,1,&partial,1,&rect);vkCmdEndRenderPass(command);
        VkBufferImageCopy copy{};copy.bufferOffset=i*stride;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={64,64,1};
        vkCmdCopyImageToBuffer(command,std::bit_cast<VkImage>(image.image),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,gpu.buffer,1,&copy);
        VkImageMemoryBarrier ready{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};ready.image=std::bit_cast<VkImage>(image.image);
        ready.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;ready.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ready.srcQueueFamilyIndex=ready.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;ready.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        ready.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;ready.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&ready);
    }
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.buffer=gpu.buffer;host.size=total;
    host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);check(vkEndCommandBuffer(command));
    VkQueue queue{};vkGetDeviceQueue(device,context.queue_family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&submit,gpu.fence));gpu.submitted=true;
    check(vkWaitForFences(device,1,&gpu.fence,VK_TRUE,UINT64_MAX));
    VkMappedMemoryRange invalidate{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};invalidate.memory=gpu.memory;invalidate.size=VK_WHOLE_SIZE;
    check(vkInvalidateMappedMemoryRanges(device,1,&invalidate));
    const auto* actual=static_cast<const unsigned char*>(gpu.mapped);
    for(unsigned i=0;i<2;++i)for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x) {
        const auto& rgba=(x>=7&&x<18&&y>=9&&y<22)?patch[i]:background[i];
        const std::array<unsigned char,4> bgra{static_cast<unsigned char>(rgba[2]),static_cast<unsigned char>(rgba[1]),static_cast<unsigned char>(rgba[0]),static_cast<unsigned char>(rgba[3])};
        require(std::memcmp(actual+i*stride+(y*64+x)*4,bgra.data(),4)==0);
    }
    return {{"pixels",8192},{"partial_color_pixels",286},{"native_format","B8G8R8A8_UNORM"},
        {"independent_images",moved.image(0).image!=moved.image(1).image},{"all_raw_bgra_bytes_exact",true},
        {"moved_foreign_thread_and_unknown_index_rejected",true},{"game_sampled_consumers_verified",false}};
}
}
