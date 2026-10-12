#pragma once
#include "source_weather_plate_gpu_fixture.hpp"
#include "halo2/source_sky_stencil.hpp"
#include <limits>

namespace halo2_sky_stencil_fixture {
using namespace halo2_weather_plate_fixture;
// New isolated device setup; shared weather fixture files stay unchanged.
struct SkyGpu:Gpu {
    bool initialize() {
        unsigned count;check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
        if(std::none_of(layers.begin(),layers.end(),[](auto&p){return std::string(p.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
        const char *layer="VK_LAYER_KHRONOS_validation",*extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
        VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debug.pfnUserCallback=diagnostic;
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledLayerCount=1;ci.ppEnabledLayerNames=&layer;ci.enabledExtensionCount=1;ci.ppEnabledExtensionNames=&extension;ci.pNext=&debug;check(vkCreateInstance(&ci,nullptr,&instance));check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
        check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        for(auto p:devices) {
            VkPhysicalDeviceFeatures f;vkGetPhysicalDeviceFeatures(p,&f);if(!f.geometryShader||!f.depthClamp)continue;
            VkFormatProperties fmt;vkGetPhysicalDeviceFormatProperties(p,VK_FORMAT_D24_UNORM_S8_UINT,&fmt);const auto needed=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;if((fmt.optimalTilingFeatures&needed)!=needed)continue;
            vkGetPhysicalDeviceQueueFamilyProperties(p,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(p,&count,families.data());
            for(unsigned i=0;i<count;++i)if(families[i].queueCount&&(families[i].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)){physical=p;family=i;break;}if(physical)break;
        }
        if(!physical)return false;
        VkPhysicalDeviceFeatures features{};features.geometryShader=features.depthClamp=VK_TRUE;
        float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.pEnabledFeatures=&features;di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;check(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(device,&pc,nullptr,&pool));VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ca,&command));
        VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(physical,&props);std::cout<<"Source sky stencil GPU: "<<props.deviceName<<'\n';return true;
    }
};
struct Submission {
    SkyGpu &g;VkFence fence{};bool submitted{},completed{};
    std::vector<SourceSkyStencil*> producers;
    explicit Submission(SkyGpu &gpu):g(gpu){VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(g.device,&ci,nullptr,&fence));g.begin();}
    void record(SourceSkyStencil &p){p.record({g.device,g.command,fence,g.family,true,true});producers.push_back(&p);}
    void finish(){check(vkEndCommandBuffer(g.command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&s,fence));submitted=true;check(vkWaitForFences(g.device,1,&fence,VK_TRUE,std::numeric_limits<std::uint64_t>::max()));for(auto*p:producers)p->retire(fence);completed=true;}
    ~Submission(){if(!completed&&!producers.empty()){if(!submitted){vkEndCommandBuffer(g.command);VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&g.command;if(vkQueueSubmit(g.queue,1,&s,fence)==VK_SUCCESS)submitted=true;}if(submitted&&vkWaitForFences(g.device,1,&fence,VK_TRUE,std::numeric_limits<std::uint64_t>::max())==VK_SUCCESS)for(auto*p:producers)try{p->retire(fence);}catch(...){}}if(fence)vkDestroyFence(g.device,fence,nullptr);}
};
inline SourceWeatherDepthAliasImage metadata(const SkyGpu &g,Image image,VkExtent2D extent) {
    return {g.device,image.handle,VK_FORMAT_D24_UNORM_S8_UINT,extent,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_SAMPLE_COUNT_1_BIT,1,1,g.family,VK_SHARING_MODE_EXCLUSIVE};
}
inline void transition(SkyGpu &g,Image image,VkImageLayout from,VkImageLayout to,VkAccessFlags a,VkAccessFlags b,VkPipelineStageFlags before,VkPipelineStageFlags after) {
    VkImageMemoryBarrier m{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};m.srcAccessMask=a;m.dstAccessMask=b;m.oldLayout=from;m.newLayout=to;m.srcQueueFamilyIndex=m.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;m.image=image.handle;m.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};vkCmdPipelineBarrier(g.command,before,after,0,0,nullptr,0,nullptr,1,&m);
}
constexpr auto ds_stages=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
constexpr auto ds_access=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
inline void upload_depth(SkyGpu &g,Image image,VkExtent2D extent,std::span<const std::uint32_t> source,bool initialized) {
    const auto pixels=std::size_t(extent.width)*extent.height;require(source.size()==pixels,"sky upload size mismatch");auto b=g.buffer(pixels*5,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);auto *out=static_cast<std::uint32_t*>(b.mapped);auto *stencil=static_cast<std::uint8_t*>(b.mapped)+pixels*4;
    for(std::size_t i=0;i<pixels;++i){out[i]=source[i]&0xffffff;stencil[i]=source[i]>>24;}
    transition(g,image,initialized?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,initialized?ds_access:0,VK_ACCESS_TRANSFER_WRITE_BIT,initialized?ds_stages:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy c{};c.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};c.imageExtent={extent.width,extent.height,1};vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&c);c.bufferOffset=pixels*4;c.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&c);
    transition(g,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,ds_access,VK_PIPELINE_STAGE_TRANSFER_BIT,ds_stages);
}
inline Buffer read_depth(SkyGpu &g,Image image,VkExtent2D extent) {
    const auto pixels=std::size_t(extent.width)*extent.height;auto b=g.buffer(pixels*5,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    transition(g,image,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,ds_access,VK_ACCESS_TRANSFER_READ_BIT,ds_stages,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy c{};c.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};c.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,b.handle,1,&c);c.bufferOffset=pixels*4;c.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,b.handle,1,&c);
    transition(g,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,ds_access,VK_PIPELINE_STAGE_TRANSFER_BIT,ds_stages);
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.buffer=b.handle;host.size=b.size;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);return b;
}
inline std::vector<std::uint32_t> packed(Buffer b,std::size_t pixels) {
    std::vector<std::uint32_t> out(pixels);const auto *depth=static_cast<const std::uint32_t*>(b.mapped);const auto *stencil=static_cast<const std::uint8_t*>(b.mapped)+pixels*4;
    for(std::size_t i=0;i<pixels;++i)out[i]=(depth[i]&0xffffff)|(std::uint32_t(stencil[i])<<24);return out;
}
}
