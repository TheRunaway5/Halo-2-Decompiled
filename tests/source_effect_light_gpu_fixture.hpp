#pragma once
#include "halo2/menu_world_native_draw.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
namespace halo2_effect_light_fixture {
using namespace halo2;
void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("effect light fixture Vulkan "+std::to_string(r));}
inline unsigned validation_errors;
VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void*) {
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<data->pMessage<<'\n';}return false;
}
struct Buffer{VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};VkDeviceSize size{};};
struct Image{VkImage handle{};VkDeviceMemory memory{};VkImageView view{};unsigned mip_levels=1;};
struct Gpu {
    VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};unsigned family{};
    VkCommandPool pool{};VkCommandBuffer command{};
    std::vector<Buffer> buffers;std::vector<Image> images;std::vector<VkRenderPass> passes;std::vector<VkFramebuffer> frames;
    std::vector<VkPipeline> pipelines;std::vector<VkShaderModule> modules;std::vector<VkSampler> samplers;
    VkDescriptorSetLayout descriptor_layout{};VkDescriptorPool descriptors{};VkPipelineLayout layout{};
    bool initialize() {
        unsigned count;check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
        if(std::none_of(layers.begin(),layers.end(),[](const auto&p){return std::string(p.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
        const char *layer="VK_LAYER_KHRONOS_validation",*extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
        VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debug.pfnUserCallback=diagnostic;
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledLayerCount=1;ci.ppEnabledLayerNames=&layer;ci.enabledExtensionCount=1;ci.ppEnabledExtensionNames=&extension;ci.pNext=&debug;
        check(vkCreateInstance(&ci,nullptr,&instance));check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
        check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        for(auto p:devices) {
            vkGetPhysicalDeviceQueueFamilyProperties(p,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(p,&count,families.data());
            for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=p;family=i;break;}if(physical)break;
        }
        require(physical,"effect light test has no graphics device");
        VkPhysicalDeviceFeatures support;vkGetPhysicalDeviceFeatures(physical,&support);
        if(!support.largePoints||!support.depthClamp||!support.textureCompressionBC)return false;
        VkPhysicalDeviceFeatures features{};features.depthClamp=features.largePoints=features.textureCompressionBC=VK_TRUE;
        if(!support.geometryShader||!support.shaderTessellationAndGeometryPointSize||!support.samplerAnisotropy)return false;
        features.geometryShader=features.shaderTessellationAndGeometryPointSize=features.samplerAnisotropy=VK_TRUE;
        VkPhysicalDeviceVulkan13Features vk13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT cube_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT};vk13.pNext=&cube_features;
        VkPhysicalDeviceFeatures2 supported{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};supported.pNext=&vk13;
        vkGetPhysicalDeviceFeatures2(physical,&supported);
        if(!vk13.shaderDemoteToHelperInvocation||!cube_features.nonSeamlessCubeMap)return false;
        vk13={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};vk13.shaderDemoteToHelperInvocation=VK_TRUE;vk13.pNext=&cube_features;
        float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
        VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.pEnabledFeatures=&features;di.pNext=&vk13;di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;
        const char* cube_extension=VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME;di.enabledExtensionCount=1;di.ppEnabledExtensionNames=&cube_extension;
        check(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(device,&pc,nullptr,&pool));
        VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ca,&command));
        VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(physical,&props);std::cout<<"Effect light GPU device: "<<props.deviceName<<'\n';return true;
    }
    ~Gpu(){if(device){vkDeviceWaitIdle(device);for(auto p:pipelines)vkDestroyPipeline(device,p,nullptr);for(auto m:modules)vkDestroyShaderModule(device,m,nullptr);for(auto f:frames)vkDestroyFramebuffer(device,f,nullptr);for(auto p:passes)vkDestroyRenderPass(device,p,nullptr);if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);if(descriptor_layout)vkDestroyDescriptorSetLayout(device,descriptor_layout,nullptr);for(auto s:samplers)vkDestroySampler(device,s,nullptr);for(auto b:buffers){if(b.mapped)vkUnmapMemory(device,b.memory);vkDestroyBuffer(device,b.handle,nullptr);vkFreeMemory(device,b.memory,nullptr);}for(auto i:images){vkDestroyImageView(device,i.view,nullptr);vkDestroyImage(device,i.handle,nullptr);vkFreeMemory(device,i.memory,nullptr);}if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);if(instance)vkDestroyInstance(instance,nullptr);}
    unsigned memory(unsigned bits,VkMemoryPropertyFlags flags){VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;throw FormatError("effect light fixture memory unavailable");}
    Buffer buffer(VkDeviceSize size,VkBufferUsageFlags usage){Buffer b;b.size=size;VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=size;ci.usage=usage;check(vkCreateBuffer(device,&ci,nullptr,&b.handle));VkMemoryRequirements r;vkGetBufferMemoryRequirements(device,b.handle,&r);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memory(r.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);check(vkAllocateMemory(device,&a,nullptr,&b.memory));check(vkBindBufferMemory(device,b.handle,b.memory,0));check(vkMapMemory(device,b.memory,0,size,0,&b.mapped));buffers.push_back(b);return b;}
    Image image(VkExtent2D e,VkFormat format,VkImageUsageFlags usage,unsigned mip_levels=1){Image i;i.mip_levels=mip_levels;VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=format;ci.extent={e.width,e.height,1};ci.mipLevels=mip_levels;ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;check(vkCreateImage(device,&ci,nullptr,&i.handle));VkMemoryRequirements r;vkGetImageMemoryRequirements(device,i.handle,&r);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memory(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);check(vkAllocateMemory(device,&a,nullptr,&i.memory));check(vkBindImageMemory(device,i.handle,i.memory,0));VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=i.handle;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={VkImageAspectFlags(format==VK_FORMAT_D24_UNORM_S8_UINT?(VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT):format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT),0,mip_levels,0,1};check(vkCreateImageView(device,&v,nullptr,&i.view));images.push_back(i);return i;}
    VkRenderPass scene_pass(bool clear,VkFormat depth_format=VK_FORMAT_D32_SFLOAT){VkAttachmentDescription a[2]{};for(unsigned i=0;i<2;++i){a[i].format=i?depth_format:VK_FORMAT_R8G8B8A8_UNORM;a[i].samples=VK_SAMPLE_COUNT_1_BIT;a[i].loadOp=clear?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_LOAD;a[i].storeOp=VK_ATTACHMENT_STORE_OP_STORE;a[i].stencilLoadOp=i&&depth_format==VK_FORMAT_D24_UNORM_S8_UINT?(clear?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_LOAD):VK_ATTACHMENT_LOAD_OP_DONT_CARE;a[i].stencilStoreOp=i&&depth_format==VK_FORMAT_D24_UNORM_S8_UINT?VK_ATTACHMENT_STORE_OP_STORE:VK_ATTACHMENT_STORE_OP_DONT_CARE;a[i].initialLayout=clear?VK_IMAGE_LAYOUT_UNDEFINED:(i?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);a[i].finalLayout=i?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;}
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;sub.pDepthStencilAttachment=&depth;
        const VkSubpassDependency deps[]{{VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT},{0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT}};
        VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ci.attachmentCount=2;ci.pAttachments=a;ci.subpassCount=1;ci.pSubpasses=&sub;ci.dependencyCount=2;ci.pDependencies=deps;VkRenderPass p;check(vkCreateRenderPass(device,&ci,nullptr,&p));passes.push_back(p);return p;}
    VkFramebuffer framebuffer(VkRenderPass pass,VkExtent2D extent,Image color,Image depth){VkImageView views[]{color.view,depth.view};VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};ci.renderPass=pass;ci.attachmentCount=2;ci.pAttachments=views;ci.width=extent.width;ci.height=extent.height;ci.layers=1;VkFramebuffer f;check(vkCreateFramebuffer(device,&ci,nullptr,&f));frames.push_back(f);return f;}
    void begin(){check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo ci{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&ci));}
    void end(){check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&submit,0));check(vkQueueWaitIdle(queue));}
    void readback(VkImage image,VkExtent2D extent,VkImageLayout layout,Buffer b){VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=layout;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(command,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,b.handle,1,&copy);barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=layout;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=layout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL?VK_ACCESS_SHADER_READ_BIT:VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);}
};
}
