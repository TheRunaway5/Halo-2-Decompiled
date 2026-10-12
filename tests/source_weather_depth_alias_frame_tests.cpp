#include "halo2/source_weather_depth_alias.hpp"
#include "source_weather_point_gpu_fixture.hpp"
using namespace halo2;
using namespace halo2_weather_point_fixture;
int main(){try{
    Gpu g;if(!g.initialize())return 77;
    const VkExtent2D extent{17,9};
    const auto depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    g.begin();VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.image=depth.handle;barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    const VkClearDepthStencilValue clear{1,0x5a};vkCmdClearDepthStencilImage(g.command,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&barrier.subresourceRange);
    barrier.oldLayout=barrier.newLayout;barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,1,&barrier);g.end();
    VkFence fence{};VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(g.device,&fi,nullptr,&fence));
    {
        SourceWeatherDepthAlias alias({g.physical,g.device,g.family,HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR});alias.prepare(extent);
        const auto target=alias.prepared_binding();
        const auto reject=[](auto &&action){bool failed=false;try{action();}catch(const FormatError&){failed=true;}require(failed,"invalid alias frame operation admitted");};
        reject([&]{alias.binding();});
        SourceWeatherDepthAliasRecord record{};record.source={g.device,depth.handle,VK_FORMAT_D24_UNORM_S8_UINT,extent,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_SAMPLE_COUNT_1_BIT,1,1,g.family,VK_SHARING_MODE_EXCLUSIVE};
        record.command_owner=g.device;record.command=g.command;record.completion=fence;
        record.command_queue_family=g.family;record.outside_render_pass=record.exclusive_access=true;
        const auto submit=[&]{check(vkEndCommandBuffer(g.command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&g.command;
            check(vkQueueSubmit(g.queue,1,&s,fence));check(vkWaitForFences(g.device,1,&fence,VK_TRUE,UINT64_MAX));alias.retire(fence);};
        for(unsigned pass=0;pass<2;++pass) {
            if(pass)check(vkResetFences(g.device,1,&fence));
            g.begin();alias.record(record);reject([&]{alias.discard_unsubmitted({});});
            check(vkResetCommandBuffer(g.command,0));alias.discard_unsubmitted(g.command);
            if(!pass)reject([&]{alias.binding();});else require(alias.binding().view==target.view,"discard lost prior completed target");
            g.begin();alias.record(record);const auto binding=alias.binding();require(binding.view==target.view,"prepared target changed during production");
            auto out=g.buffer(extent.width*extent.height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            VkImageMemoryBarrier read{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};read.image=binding.image;
            read.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};read.srcQueueFamilyIndex=read.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            read.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;read.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            read.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;read.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&read);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={extent.width,extent.height,1};
            vkCmdCopyImageToBuffer(g.command,binding.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,out.handle,1,&copy);
            read.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;read.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            read.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;read.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,0,nullptr,0,nullptr,1,&read);submit();
            const auto *bytes=static_cast<unsigned char*>(out.mapped);
            for(unsigned i=0;i<extent.width*extent.height;++i)require(bytes[i*4]==255&&bytes[i*4+1]==255&&bytes[i*4+2]==0x5a&&bytes[i*4+3]==255,"alias lost exact current depth/stencil bytes");
            reject([&]{alias.discard_unsubmitted(g.command);});
        }
    }
    vkDestroyFence(g.device,fence,nullptr);require(validation_errors==0,"weather alias frame Vulkan errors");
    std::cout<<"Prepared descriptors, never-submitted discard/retry, previous layout history and exact odd-extent D24/stencil bytes passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
