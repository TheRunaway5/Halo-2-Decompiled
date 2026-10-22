#pragma once
#include "halo2/vulkan_portability.hpp"
#include "halo2/macos/vulkan_math.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

inline void require(bool condition,const std::string &message) {if(!condition)throw std::runtime_error(message);}
inline void check(VkResult result) {require(result==VK_SUCCESS,"Vulkan result "+std::to_string(result));}
inline unsigned validation_errors{};
inline VKAPI_ATTR VkBool32 VKAPI_CALL debug_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void*) {
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        if(validation_errors++<24)std::cerr<<"Validation: "<<data->pMessage<<'\n';
    }
    return VK_FALSE;
}
struct Gpu {
    VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};
    VkDevice device{};VkQueue queue{};unsigned family{};VkCommandPool pool{};VkCommandBuffer command{};
    VkPhysicalDeviceProperties properties{};bool validation{},synchronization_validation{},strict_shader_math{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};};
    struct Image {VkImage handle{};VkDeviceMemory memory{};};
    std::vector<Buffer> buffers;std::vector<Image> images;
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties p{};vkGetPhysicalDeviceMemoryProperties(physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("Required Vulkan memory type unavailable");
    }
    void initialize(bool depth_clamp=false,bool geometry=false,bool vertex_stores=false) {
        unsigned count{};check(vkEnumerateInstanceExtensionProperties(nullptr,&count,nullptr));
        std::vector<VkExtensionProperties> extensions(count);check(vkEnumerateInstanceExtensionProperties(nullptr,&count,extensions.data()));
        std::vector<const char*> enabled;const auto flags=halo2::enable_vulkan_portability_instance(extensions,enabled);
        check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
        const char *validation_layer="VK_LAYER_KHRONOS_validation";
        for(auto &layer:layers)if(std::string(layer.layerName)==validation_layer)validation=true;
        const bool debug=halo2::vulkan_extension_available(extensions,VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if(debug)enabled.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        VkDebugUtilsMessengerCreateInfoEXT debug_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug_info.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_info.pfnUserCallback=debug_message;
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Halo2 exact depth storage check";app.apiVersion=vertex_stores?VK_API_VERSION_1_3:VK_API_VERSION_1_1;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.flags=flags;create.pApplicationInfo=&app;
        create.enabledExtensionCount=enabled.size();create.ppEnabledExtensionNames=enabled.data();
        if(validation){create.enabledLayerCount=1;create.ppEnabledLayerNames=&validation_layer;}
        if(debug)create.pNext=&debug_info;
        VkValidationFeatureEnableEXT sync=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
        VkValidationFeaturesEXT validation_info{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
        if(validation) {
            enabled.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
            create.enabledExtensionCount=enabled.size();create.ppEnabledExtensionNames=enabled.data();
            validation_info.enabledValidationFeatureCount=1;validation_info.pEnabledValidationFeatures=&sync;
            validation_info.pNext=debug?&debug_info:nullptr;create.pNext=&validation_info;synchronization_validation=true;
        }
        halo2::macos::SourceShaderMath source_math;
        strict_shader_math=source_math.configure(create,extensions,enabled);
        check(vkCreateInstance(&create,nullptr,&instance));
        if(debug) {auto fn=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));require(fn,"Missing debug messenger entry");check(fn(instance,&debug_info,nullptr,&messenger));}
        check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
        for(auto candidate:devices){unsigned n{};vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,nullptr);std::vector<VkQueueFamilyProperties> families(n);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,families.data());
            for(unsigned i=0;i<n;++i)if(families[i].queueCount&&(families[i].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)){physical=candidate;family=i;break;}
            if(physical)break;}
        require(physical,"No graphics+compute device");vkGetPhysicalDeviceProperties(physical,&properties);
        check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&count,nullptr));extensions.resize(count);check(vkEnumerateDeviceExtensionProperties(physical,nullptr,&count,extensions.data()));
        enabled.clear();halo2::enable_vulkan_portability_device(extensions,enabled);
        const float priority=1;VkDeviceQueueCreateInfo qc{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qc.queueFamilyIndex=family;qc.queueCount=1;qc.pQueuePriorities=&priority;
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&qc;dc.enabledExtensionCount=enabled.size();dc.ppEnabledExtensionNames=enabled.data();
        VkPhysicalDeviceFeatures support{};vkGetPhysicalDeviceFeatures(physical,&support);
        require(!depth_clamp||support.depthClamp,"Depth clamp unsupported");require(!geometry||support.geometryShader,"Geometry shader unsupported");
        require(!vertex_stores||support.vertexPipelineStoresAndAtomics,"Vertex stores unsupported");
        VkPhysicalDeviceFeatures features{};features.depthClamp=depth_clamp;features.geometryShader=geometry;features.vertexPipelineStoresAndAtomics=vertex_stores;features.shaderTessellationAndGeometryPointSize=geometry&&vertex_stores;dc.pEnabledFeatures=&features;
        check(vkCreateDevice(physical,&dc,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;pc.queueFamilyIndex=family;check(vkCreateCommandPool(device,&pc,nullptr,&pool));
        VkCommandBufferAllocateInfo ac{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ac.commandPool=pool;ac.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ac.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ac,&command));
    }
    Buffer buffer(VkDeviceSize bytes,VkBufferUsageFlags extra_usage=0) {
        Buffer b;VkBufferCreateInfo bc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bc.size=bytes;bc.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|extra_usage;check(vkCreateBuffer(device,&bc,nullptr,&b.handle));buffers.push_back(b);auto &owned=buffers.back();
        VkMemoryRequirements m{};vkGetBufferMemoryRequirements(device,b.handle,&m);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=m.size;a.memoryTypeIndex=memory_type(m.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);check(vkAllocateMemory(device,&a,nullptr,&owned.memory));check(vkBindBufferMemory(device,b.handle,owned.memory,0));check(vkMapMemory(device,owned.memory,0,bytes,0,&owned.mapped));return owned;
    }
    Image image(VkExtent2D extent,VkFormat format,VkImageUsageFlags usage) {
        Image image;VkImageCreateInfo ic{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ic.imageType=VK_IMAGE_TYPE_2D;ic.format=format;ic.extent={extent.width,extent.height,1};ic.mipLevels=ic.arrayLayers=1;ic.samples=VK_SAMPLE_COUNT_1_BIT;ic.tiling=VK_IMAGE_TILING_OPTIMAL;ic.usage=usage;check(vkCreateImage(device,&ic,nullptr,&image.handle));images.push_back(image);auto &owned=images.back();
        VkMemoryRequirements m{};vkGetImageMemoryRequirements(device,image.handle,&m);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=m.size;a.memoryTypeIndex=memory_type(m.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);check(vkAllocateMemory(device,&a,nullptr,&owned.memory));check(vkBindImageMemory(device,image.handle,owned.memory,0));return owned;
    }
    VkShaderModule shader(const char *name) {
        std::ifstream file(std::filesystem::path(DEPTH_SHADER_DIR)/name,std::ios::binary|std::ios::ate);require(file&&file.tellg()>20,"Missing test shader");std::vector<std::uint32_t> words(std::size_t(file.tellg())/4);file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(words.data()),words.size()*4)),"Truncated test shader");VkShaderModuleCreateInfo sc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sc.codeSize=words.size()*4;sc.pCode=words.data();VkShaderModule module{};check(vkCreateShaderModule(device,&sc,nullptr,&module));return module;
    }
    void begin(){check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo bc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bc.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(vkBeginCommandBuffer(command,&bc));}
    ~Gpu(){if(device){vkDeviceWaitIdle(device);if(pool)vkDestroyCommandPool(device,pool,nullptr);for(auto b:buffers){if(b.mapped)vkUnmapMemory(device,b.memory);if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);if(b.memory)vkFreeMemory(device,b.memory,nullptr);}for(auto i:images){if(i.handle)vkDestroyImage(device,i.handle,nullptr);if(i.memory)vkFreeMemory(device,i.memory,nullptr);}vkDestroyDevice(device,nullptr);}if(messenger){auto fn=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));if(fn)fn(instance,messenger,nullptr);}if(instance)vkDestroyInstance(instance,nullptr);}
};
inline void barrier(VkCommandBuffer command,VkImage image,VkImageAspectFlags aspects,VkImageLayout before,VkImageLayout after) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.srcAccessMask=before==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;b.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;b.oldLayout=before;b.newLayout=after;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=image;b.subresourceRange={aspects,0,1,0,1};vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&b);
}
inline VkBufferImageCopy region(VkExtent2D extent,VkImageAspectFlags aspect) {VkBufferImageCopy r{};r.imageSubresource={aspect,0,0,1};r.imageExtent={extent.width,extent.height,1};return r;}
