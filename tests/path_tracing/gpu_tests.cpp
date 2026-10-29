#include "halo2/path_tracing/render.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include "halo2/path_tracing/geometry.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "halo2/world_material_uniform.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
constexpr unsigned width=64,height=64;
struct Skip:std::runtime_error {using std::runtime_error::runtime_error;};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void progress(const char* phase) {
    static const auto start=std::chrono::steady_clock::now();
    std::cout<<"Path Tracing GPU +"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
        <<"s: "<<phase<<std::endl;
}
void check(VkResult result,const char* message) {
    if(result!=VK_SUCCESS)throw FormatError(std::string(message)+": "+std::to_string(result));
}
struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void* mapped{};};
struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};};
struct Camera {
    std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    ShaderVector forward{0,0,1,0},eye{0,0,-2,1},viewport{width,height,0,0};
};
static_assert(sizeof(Camera)==pt::Render::camera_bytes);
struct Fixture {
    VkExtent2D target_extent{width,height};
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    VkDebugUtilsMessengerEXT messenger{};
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger{};
    std::atomic<unsigned> validation_errors{};
    VkPhysicalDeviceMemoryProperties memory{};
    unsigned family{};
    VkCommandPool commands{};VkCommandBuffer command{};
    VkFence completion{};bool fence_submissions{},synchronization_validation{};
    VkRenderPass pass{};VkFramebuffer framebuffer{};
    VkDescriptorSetLayout material_layout{};VkDescriptorPool descriptors{};VkDescriptorSet material{};
    VkSampler sampler{};
    Image color,depth;std::array<Image,4> primary_images;
    Buffer readback,vertices,uniform,texture_upload;
    std::unique_ptr<pt::Render> renderer;
    std::string device_name;
    ~Fixture() {
        if(device) {
            vkDeviceWaitIdle(device);renderer.reset();
            if(completion)vkDestroyFence(device,completion,nullptr);
            if(commands)vkDestroyCommandPool(device,commands,nullptr);
            if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
            if(pass)vkDestroyRenderPass(device,pass,nullptr);
            if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
            if(material_layout)vkDestroyDescriptorSetLayout(device,material_layout,nullptr);
            if(sampler)vkDestroySampler(device,sampler,nullptr);
            for(auto* image:{&color,&depth,&primary_images[0],&primary_images[1],&primary_images[2],&primary_images[3]}) {
                if(image->view)vkDestroyImageView(device,image->view,nullptr);
                if(image->handle)vkDestroyImage(device,image->handle,nullptr);
                if(image->memory)vkFreeMemory(device,image->memory,nullptr);
            }
            for(auto* buffer:{&readback,&vertices,&uniform,&texture_upload}) {
                if(buffer->mapped)vkUnmapMemory(device,buffer->memory);
                if(buffer->handle)vkDestroyBuffer(device,buffer->handle,nullptr);
                if(buffer->memory)vkFreeMemory(device,buffer->memory,nullptr);
            }
            vkDestroyDevice(device,nullptr);
        }
        if(messenger && destroy_messenger)destroy_messenger(instance,messenger,nullptr);
        if(instance)vkDestroyInstance(instance,nullptr);
    }
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT* data,void* context) {
        if(severity&(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
            ++static_cast<Fixture*>(context)->validation_errors;
            std::cerr<<"Vulkan validation: "<<(data->pMessage?data->pMessage:"")<<'\n';
        }
        return VK_FALSE;
    }
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags)const {
        for(unsigned i=0;i<memory.memoryTypeCount;++i)
            if((bits&(1u<<i)) && (memory.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("GPU fixture lacks required Vulkan memory");
    }
    void buffer(Buffer& output,VkDeviceSize bytes,VkBufferUsageFlags usage) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=bytes;info.usage=usage;
        check(vkCreateBuffer(device,&info,nullptr,&output.handle),"create fixture buffer");
        VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(device,output.handle,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&output.memory),"allocate fixture buffer");
        check(vkBindBufferMemory(device,output.handle,output.memory,0),"bind fixture buffer");
        check(vkMapMemory(device,output.memory,0,bytes,0,&output.mapped),"map fixture buffer");
    }
    void image(Image& output,VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect,unsigned w,unsigned h) {
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=format;
        info.extent={w,h,1};info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;
        info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;
        check(vkCreateImage(device,&info,nullptr,&output.handle),"create fixture image");
        VkMemoryRequirements requirements{};vkGetImageMemoryRequirements(device,output.handle,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&output.memory),"allocate fixture image");
        check(vkBindImageMemory(device,output.handle,output.memory,0),"bind fixture image");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=output.handle;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
        view.format=format;view.subresourceRange={aspect,0,1,0,1};
        check(vkCreateImageView(device,&view,nullptr,&output.view),"view fixture image");
    }
    void begin() {
        check(vkResetCommandBuffer(command,0),"reset fixture command");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        check(vkBeginCommandBuffer(command,&begin),"begin fixture command");
    }
    void submit() {
        check(vkEndCommandBuffer(command),"end fixture command");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        if(fence_submissions) {
            if(!completion) {
                VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
                check(vkCreateFence(device,&info,nullptr,&completion),"create fixture completion fence");
            }
            check(vkResetFences(device,1,&completion),"reset fixture completion fence");
            check(vkQueueSubmit(queue,1,&submit,completion),"submit fixture fenced command");
            check(vkWaitForFences(device,1,&completion,VK_TRUE,UINT64_MAX),"wait fixture completion fence");
        }else {
            check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE),"submit fixture command");
            check(vkQueueWaitIdle(queue),"wait fixture command");
        }
    }
    void initialize(const std::filesystem::path& shaders,VkExtent2D target={width,height},bool enable_sync_validation=false) {
        require(target.width>0 && target.height>0,"GPU fixture requires a nonempty target");
        target_extent=target;
        progress("initializing Vulkan fixture");
        unsigned count{};check(vkEnumerateInstanceLayerProperties(&count,nullptr),"enumerate layers");
        std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()),"read layers");
        const bool validation=std::any_of(layers.begin(),layers.end(),[](const auto& layer){return std::strcmp(layer.layerName,"VK_LAYER_KHRONOS_validation")==0;});
        check(vkEnumerateInstanceExtensionProperties(nullptr,&count,nullptr),"enumerate extensions");
        std::vector<VkExtensionProperties> extensions(count);check(vkEnumerateInstanceExtensionProperties(nullptr,&count,extensions.data()),"read extensions");
        const bool debug_utils=validation && std::any_of(extensions.begin(),extensions.end(),[](const auto& extension){return std::strcmp(extension.extensionName,VK_EXT_DEBUG_UTILS_EXTENSION_NAME)==0;});
        if(enable_sync_validation && !debug_utils)
            throw Skip("Native fence fixture requires an observable Vulkan validation callback");
        const char* layer="VK_LAYER_KHRONOS_validation";
        std::vector<const char*> enabled_extensions;
        if(debug_utils)enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
        const VkValidationFeatureEnableEXT synchronization=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
        if(enable_sync_validation) {
            if(!validation)throw Skip("Native fence fixture requires Vulkan validation");
            unsigned layer_count{};
            check(vkEnumerateInstanceExtensionProperties(layer,&layer_count,nullptr),"enumerate validation extensions");
            std::vector<VkExtensionProperties> layer_extensions(layer_count);
            check(vkEnumerateInstanceExtensionProperties(layer,&layer_count,layer_extensions.data()),"read validation extensions");
            const auto supports_features=[](const auto& extension) {
                return std::strcmp(extension.extensionName,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)==0;
            };
            if(!std::any_of(extensions.begin(),extensions.end(),supports_features) &&
               !std::any_of(layer_extensions.begin(),layer_extensions.end(),supports_features))
                throw Skip("Native fence fixture requires synchronization validation features");
            enabled_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
            validation_features.enabledValidationFeatureCount=1;
            validation_features.pEnabledValidationFeatures=&synchronization;
            synchronization_validation=true;
        }
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
        if(validation){create.enabledLayerCount=1;create.ppEnabledLayerNames=&layer;}
        create.enabledExtensionCount=enabled_extensions.size();create.ppEnabledExtensionNames=enabled_extensions.data();
        if(enable_sync_validation)create.pNext=&validation_features;
        const auto instance_result=vkCreateInstance(&create,nullptr,&instance);
        if(instance_result==VK_ERROR_INCOMPATIBLE_DRIVER)throw Skip("No compatible Vulkan driver");
        check(instance_result,"create fixture instance");
        if(debug_utils) {
            const auto create_debug=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
            destroy_messenger=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));
            VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
            info.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            info.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
            info.pfnUserCallback=debug;info.pUserData=this;
            check(create_debug(instance,&info,nullptr,&messenger),"create validation messenger");
        }
        check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"enumerate GPUs");
        std::vector<VkPhysicalDevice> candidates(count);check(vkEnumeratePhysicalDevices(instance,&count,candidates.data()),"read GPUs");
        for(auto candidate:candidates) {
            VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(candidate,&properties);
            VkPhysicalDeviceFeatures features{};vkGetPhysicalDeviceFeatures(candidate,&features);
            if(properties.limits.maxPerStageDescriptorStorageBuffers<6 ||
               properties.limits.maxDescriptorSetStorageBuffers<6 || !features.fragmentStoresAndAtomics)continue;
            unsigned queues{};vkGetPhysicalDeviceQueueFamilyProperties(candidate,&queues,nullptr);
            std::vector<VkQueueFamilyProperties> families(queues);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&queues,families.data());
            for(unsigned i=0;i<queues;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) {
                physical=candidate;family=i;device_name=properties.deviceName;break;
            }
            if(physical)break;
        }
        if(!physical)throw Skip("No Vulkan graphics device with six fragment storage descriptors");
        vkGetPhysicalDeviceMemoryProperties(physical,&memory);
        const float priority=1;VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex=family;queue_info.queueCount=1;queue_info.pQueuePriorities=&priority;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device_info.queueCreateInfoCount=1;device_info.pQueueCreateInfos=&queue_info;
        VkPhysicalDeviceFeatures features{};features.fragmentStoresAndAtomics=VK_TRUE;device_info.pEnabledFeatures=&features;
        check(vkCreateDevice(physical,&device_info,nullptr,&device),"create fixture device");vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;pool.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        check(vkCreateCommandPool(device,&pool,nullptr,&commands),"create fixture command pool");
        VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};command_info.commandPool=commands;
        command_info.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;command_info.commandBufferCount=1;
        check(vkAllocateCommandBuffers(device,&command_info,&command),"allocate fixture command");
        image(color,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,target.width,target.height);
        image(depth,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,target.width,target.height);
        for(auto& primary:primary_images)image(primary,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_IMAGE_ASPECT_COLOR_BIT,width,height);
        const VkAttachmentDescription attachments[]{
            {0,VK_FORMAT_R8G8B8A8_UNORM,VK_SAMPLE_COUNT_1_BIT,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,VK_ATTACHMENT_STORE_OP_DONT_CARE,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
            {0,VK_FORMAT_D24_UNORM_S8_UINT,VK_SAMPLE_COUNT_1_BIT,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_DONT_CARE,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}};
        const VkAttachmentReference color_reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth_reference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color_reference;subpass.pDepthStencilAttachment=&depth_reference;
        const VkSubpassDependency dependencies[]{
            {VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,0},
            {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0}};
        VkRenderPassCreateInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass_info.attachmentCount=2;pass_info.pAttachments=attachments;
        pass_info.subpassCount=1;pass_info.pSubpasses=&subpass;pass_info.dependencyCount=2;pass_info.pDependencies=dependencies;
        check(vkCreateRenderPass(device,&pass_info,nullptr,&pass),"create fixture render pass");
        const VkImageView views[]{color.view,depth.view};VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass=pass;framebuffer_info.attachmentCount=2;framebuffer_info.pAttachments=views;
        framebuffer_info.width=target.width;framebuffer_info.height=target.height;framebuffer_info.layers=1;
        check(vkCreateFramebuffer(device,&framebuffer_info,nullptr,&framebuffer),"create fixture framebuffer");
        buffer(readback,VkDeviceSize(target.width)*target.height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        buffer(vertices,24*sizeof(Vertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        buffer(texture_upload,width*height*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        buffer(uniform,sizeof(WorldMaterialUniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        std::memset(uniform.mapped,0,sizeof(WorldMaterialUniform));
        const VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layout.bindingCount=5;layout.pBindings=bindings;
        check(vkCreateDescriptorSetLayout(device,&layout,nullptr,&material_layout),"create fixture material layout");
        const VkDescriptorPoolSize descriptor_sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1}};
        VkDescriptorPoolCreateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};descriptor_info.maxSets=1;
        descriptor_info.poolSizeCount=2;descriptor_info.pPoolSizes=descriptor_sizes;
        check(vkCreateDescriptorPool(device,&descriptor_info,nullptr,&descriptors),"create fixture material pool");
        VkDescriptorSetAllocateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};set_info.descriptorPool=descriptors;
        set_info.descriptorSetCount=1;set_info.pSetLayouts=&material_layout;
        check(vkAllocateDescriptorSets(device,&set_info,&material),"allocate fixture material set");
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler_info.magFilter=sampler_info.minFilter=VK_FILTER_NEAREST;
        sampler_info.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;sampler_info.addressModeU=sampler_info.addressModeV=sampler_info.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
        check(vkCreateSampler(device,&sampler_info,nullptr,&sampler),"create fixture sampler");
        std::array<VkDescriptorImageInfo,4> sampled{};
        const VkDescriptorBufferInfo bank{uniform.handle,0,sizeof(WorldMaterialUniform)};
        std::array<VkWriteDescriptorSet,5> writes{};
        for(unsigned stage=0;stage<5;++stage) {
            auto& write=writes[stage];write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet=material;write.dstBinding=stage;write.descriptorCount=1;
            if(stage<4) {
                sampled[stage]={sampler,primary_images[stage].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&sampled[stage];
            } else {write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;write.pBufferInfo=&bank;}
        }
        vkUpdateDescriptorSets(device,writes.size(),writes.data(),0,nullptr);
        begin();
        for(auto& primary:primary_images) {
            VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=primary.handle;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
            const VkClearColorValue white_color{{1,1,1,1}};vkCmdClearColorImage(command,primary.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&white_color,1,&barrier.subresourceRange);
            barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        }
        submit();
        progress("creating primary ray pipeline");
        renderer=std::make_unique<pt::Render>(pt::RenderContext{device,physical,pass,material_layout,shaders});
        progress("primary ray pipeline ready");
    }
    void primary_color(ShaderVector color,unsigned stage=0) {
        begin();VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image=primary_images[stage].handle;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);
        VkClearColorValue clear{};std::copy(color.begin(),color.end(),clear.float32);
        vkCmdClearColorImage(command,primary_images[stage].handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&barrier.subresourceRange);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);submit();
    }
    void primary_texture(std::span<const std::uint8_t> rgba,unsigned stage=0) {
        require(rgba.size()==width*height*4,"fixture primary texture dimensions changed");
        std::memcpy(texture_upload.mapped,rgba.data(),rgba.size());begin();
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=primary_images[stage].handle;
        barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);
        const VkBufferImageCopy copy{0,0,0,{VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},{0,0,0},{width,height,1}};
        vkCmdCopyBufferToImage(command,texture_upload.handle,primary_images[stage].handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr,1,&barrier);submit();
    }
    std::vector<std::uint8_t> draw(const pt::Scene& scene,std::span<const Vertex> primary,
        Vec3 environment={},const Camera& camera=Camera{},std::uint32_t material_override=~0u,bool visibility=false,
        bool filtered=false,const std::array<Vec3,6>* sky_faces=nullptr,bool advance_frame=true,
        bool collect_temporal_stats=false,bool capture_frame=true,const VkViewport* actual_viewport=nullptr) {
        require(primary.size()<=24,"fixture primary vertex bank overflow");
        std::memcpy(vertices.mapped,primary.data(),primary.size_bytes());
        visibility=visibility || filtered;
        const VkViewport viewport=actual_viewport?*actual_viewport:
            VkViewport{0,0,float(target_extent.width),float(target_extent.height),0,1};
        if(visibility)renderer->prepare_visibility(target_extent,viewport);
        renderer->prepare_environment(VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_D24_UNORM_S8_UINT,1,
            sky_faces!=nullptr || environment.x>0 || environment.y>0 || environment.z>0);
        if(advance_frame)renderer->upload(scene,scene.uniform(environment));
        begin();
        for(unsigned face=0;face<6;++face) {
            const auto c=sky_faces?(*sky_faces)[face]:environment;
            renderer->begin_environment(command,face,{c.x,c.y,c.z,1});renderer->end_environment(command);
        }
        const VkRect2D scissor{{0,0},target_extent};
        const VkDeviceSize offset{};
        if(visibility) {
            renderer->begin_visibility(command);
            vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
            vkCmdBindVertexBuffers(command,0,1,&vertices.handle,&offset);
            for(std::size_t first=0;first<primary.size();first+=3) {
                const auto selected=material_override==~0u?primary[first].material:material_override;
                renderer->bind_visibility(command,material,0,std::as_bytes(std::span(&camera,1)),selected);
                vkCmdDraw(command,3,1,unsigned(first),0);
            }
            renderer->end_visibility(command);
        }
        if(filtered) {
            renderer->begin_lighting(command);
            vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
            vkCmdBindVertexBuffers(command,0,1,&vertices.handle,&offset);
            for(std::size_t first=0;first<primary.size();first+=3) {
                const auto selected=material_override==~0u?primary[first].material:material_override;
                renderer->bind_lighting(command,material,0,std::as_bytes(std::span(&camera,1)),selected);
                vkCmdDraw(command,3,1,unsigned(first),0);
            }
            renderer->end_lighting(command);
        }
        std::array<VkClearValue,2> clear{};clear[0].color={{0,0,0,1}};clear[1].depthStencil={1,0};
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=framebuffer;
        render.renderArea={{0,0},target_extent};render.clearValueCount=2;render.pClearValues=clear.data();
        vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
        vkCmdBindVertexBuffers(command,0,1,&vertices.handle,&offset);
        if(material_override==~0u)renderer->bind(command,material,0,std::as_bytes(std::span(&camera,1)));
        else renderer->bind(command,material,0,std::as_bytes(std::span(&camera,1)),material_override);
        vkCmdDraw(command,primary.size(),1,0,0);vkCmdEndRenderPass(command);
        if(collect_temporal_stats)renderer->record_temporal_stats(command);
        if(!capture_frame){submit();return {};}
        const VkBufferImageCopy copy{0,0,0,{VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},{0,0,0},{target_extent.width,target_extent.height,1}};
        vkCmdCopyImageToBuffer(command,color.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.handle,1,&copy);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);submit();
        const auto* bytes=static_cast<const std::uint8_t*>(readback.mapped);
        return {bytes,bytes+std::size_t(target_extent.width)*target_extent.height*4};
    }
};
void quad(std::vector<Vertex>& vertices,float left,float bottom,float right,float top,float z,Vec3 normal,unsigned material) {
    for(const auto p:std::array<Vec3,6>{{{left,bottom,z},{right,bottom,z},{left,top,z},
        {left,top,z},{right,bottom,z},{right,top,z}}}) {
        Vertex vertex;vertex.position=p;vertex.normal=normal;vertex.color={1,1,1};vertex.uv={(p.x-left)/(right-left),(p.y-bottom)/(top-bottom)};
        vertex.material=material;vertices.push_back(vertex);
    }
}
Material white_material() {
    Material material;material.diffuse={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};return material;
}
Texture solid_texture(std::array<std::uint8_t,4> rgba) {
    return {1,1,11,{std::byte(rgba[0]),std::byte(rgba[1]),std::byte(rgba[2]),std::byte(rgba[3])}};
}
template<std::size_t N> std::vector<std::byte> vertex_bytes(const std::array<std::uint32_t,N>& words) {
    std::vector<std::byte> result(N*4);
    for(std::size_t n=0;n<N;++n)for(unsigned b=0;b<4;++b)result[n*4+b]=std::byte(words[n]>>(b*8));
    return result;
}
Material native_surface(bool terrain) {
    auto material=white_material();material.forward_pass=true;material.forward_pass_index=0;
    material.forward_vertex=terrain?3:6;
    MaterialPass pass;
    pass.name=terrain?"shaders\\shader_passes\\texture_mapping\\detail_blend_detail_texture_zwrite":
        "shaders\\shader_passes\\lightmap\\lightmap_tex_detail_alpha_test_no_bump";
    pass.vertex_shader=terrain?"rasterizer\\vertex_shaders_xbox\\stencil_texture":
        "rasterizer\\vertex_shaders_xbox\\lightmap_perpixel_tex_detail";
    material.vertex_program=terrain?vertex_bytes(pt::unlit_uv_detail::texture_words):
        vertex_bytes(pt::unlit_uv_detail::perpixel_detail_words);
    pass.vertex_code=material.vertex_program;material.passes.push_back(pass);
    material.forward_program.words=terrain?pt::surface_detail::terrain:pt::surface_detail::leaf;
    material.forward_transforms[0]=material.forward_transforms[1]={1,1,0,0};
    material.forward_transforms[2]=terrain?ShaderVector{1,1,1,1}:ShaderVector{1,1,0,0};
    material.detail=material.bump=material.auxiliary=material.diffuse;
    if(!terrain) {
        material.forward_raster.alpha_test=1;material.forward_raster.alpha_function=0x201;
        material.forward_raster.alpha_reference=255;
    }
    require(pt::qualified_surface(material).has_value(),"Native surface fixture failed complete source qualification");
    return material;
}
void native_uniform(Fixture& gpu,const Material& material) {
    WorldMaterialUniform bank{};bank.program=material.forward_program.words;
    bank.forward_transforms=material.forward_transforms;
    std::memcpy(gpu.uniform.mapped,&bank,sizeof(bank));
}
float mean(const std::vector<std::uint8_t>& frame,unsigned channel,unsigned left=24,unsigned right=40,unsigned top=24,unsigned bottom=40) {
    double total{};for(unsigned y=top;y<bottom;++y)for(unsigned x=left;x<right;++x)total+=frame[(y*width+x)*4+channel];
    return float(total/((right-left)*(bottom-top)));
}
float deviation(const std::vector<std::uint8_t>& frame,unsigned channel) {
    const auto average=mean(frame,channel,8,56,8,56);double squared{};
    for(unsigned y=8;y<56;++y)for(unsigned x=8;x<56;++x) {
        const auto delta=float(frame[(y*width+x)*4+channel])-average;squared+=delta*delta;
    }
    return float(std::sqrt(squared/(48*48)));
}
struct NoiseMetrics {
    double mean{},deviation{},high_frequency{};
};
NoiseMetrics noise_metrics(std::span<const std::uint8_t> frame,VkExtent2D extent) {
    require(frame.size()==std::size_t(extent.width)*extent.height*4,"Noise metric received a partial native frame");
    require(extent.width>=16 && extent.height>=16,"Noise metric requires a bounded interior region");
    const unsigned left=extent.width/8,right=extent.width*7/8;
    const unsigned top=extent.height/8,bottom=extent.height*7/8;
    const auto luma=[&](unsigned x,unsigned y) {
        const auto at=(std::size_t(y)*extent.width+x)*4;
        return .2126*frame[at]+.7152*frame[at+1]+.0722*frame[at+2];
    };
    double sum=0,squared=0,high_frequency=0;
    const auto count=std::size_t(right-left)*(bottom-top);
    for(unsigned y=top;y<bottom;++y)for(unsigned x=left;x<right;++x) {
        const auto value=luma(x,y);sum+=value;squared+=value*value;
        // Unit-variance independent noise has a five-tap Laplacian variance
        // of20. This complements the full-region deviation: the latter also
        // catches coarse speckle or blocks that a local high-pass can miss.
        const auto residual=4*value-luma(x-1,y)-luma(x+1,y)-luma(x,y-1)-luma(x,y+1);
        high_frequency+=residual*residual;
    }
    const auto mean=sum/count;
    return {mean,std::sqrt(std::max(0.,squared/count-mean*mean)),std::sqrt(high_frequency/(20*count))};
}
void print_noise_metrics(const char* role,const NoiseMetrics& value) {
    std::cout<<'"'<<role<<"\":{\"mean\":"<<value.mean<<",\"deviation\":"<<value.deviation
        <<",\"high_frequency\":"<<value.high_frequency<<'}';
}
int native_noise(const std::filesystem::path& shaders,VkExtent2D extent,bool fence_only=false) {
    // This opt-in path is the real visibility/HDR-lighting/resolve sequence at
    // native output size. No upscaling implementation or post-process is linked
    // into the fixture. A constant receiver and distant light have no texture
    // detail or geometric gradient that could be mistaken for random noise.
    Fixture gpu;gpu.initialize(shaders,extent,fence_only);gpu.fence_submissions=fence_only;
    gpu.renderer->set_quality(1,1);
    gpu.primary_color({.1f,.1f,.1f,1});
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    auto black_emitter=white_material();black_emitter.diffuse=solid_texture({0,0,0,255});
    black_emitter.forward_vertex=2;black_emitter.forward_diffuse={1,1,1,1};
    const std::array<Material,2> materials{white_material(),black_emitter};
    auto geometry=primary;quad(geometry,3,3,4,4,-3,{0,0,1},1);
    pt::Light light;light.direction=Vec3{0,0,-1};light.color={2,2,2};
    // A native WORLD view spans tens of source units, unlike the unit quad
    // above. Use Coagulation's captured spawn origin and a128-unit receiver
    // span without changing its screen coverage, normals, albedo or lighting.
    // The fenced path therefore covers actual WORLD/source-camera magnitudes
    // rather than exercising synchronization only on a unit-space fixture.
    const float world_scale=fence_only?64.f:1.f;
    const Vec3 origin=fence_only?Vec3{74.2382f,-117.899f,.21363f}:Vec3{};
    const auto place=[&](std::vector<Vertex>& vertices) {
        for(auto& vertex:vertices)vertex.position={origin.x+vertex.position.x*world_scale,
            origin.y+vertex.position.y*world_scale,origin.z+vertex.position.z*world_scale};
    };
    place(primary);place(geometry);
    Camera base_camera;
    base_camera.matrix[0]=base_camera.matrix[5]=base_camera.matrix[10]=1.f/world_scale;
    base_camera.matrix[12]=-origin.x/world_scale;
    base_camera.matrix[13]=-origin.y/world_scale;
    base_camera.matrix[14]=-origin.z/world_scale;
    base_camera.eye[0]=origin.x;base_camera.eye[1]=origin.y;
    base_camera.eye[2]=origin.z-2.f*world_scale;
    if(fence_only) {
        WorldMaterialUniform bank{};bank.source_camera_control[0]=1;
        bank.source_camera_world[3]={0,0,16777215.f/world_scale,-origin.z*16777215.f/world_scale};
        std::memcpy(gpu.uniform.mapped,&bank,sizeof(bank));base_camera.forward[3]=1;
    }
    const auto scene=pt::build_scene(geometry,materials,std::span(&light,1));
    const auto raw=noise_metrics(gpu.draw(scene,primary,{},base_camera),extent);
    if(fence_only) {
        // Match ordinary frame completion: wait a submission fence, with no
        // queue-idle operation, color readback, history-to-host barrier or CPU
        // history reads during the first fifteen filtered frames. Only the
        // final frame captures the result and completed history diagnostics.
        const auto stationary_scene=scene;
        for(unsigned frame=0;frame<15;++frame)
            gpu.draw(stationary_scene,primary,{},base_camera,~0u,false,true,nullptr,true,false,false);
        const auto final=noise_metrics(gpu.draw(stationary_scene,primary,{},base_camera,
            ~0u,false,true,nullptr,true,true),extent);
        const auto history=gpu.renderer->temporal_stats();
        std::cout<<"Path Tracing native fence noise: {\"width\":"<<extent.width<<",\"height\":"<<extent.height
            <<",\"upscaler\":\"Off\",\"samples\":1,\"bounces\":1,\"filtered_frames\":16,"
            <<"\"intermediate_readbacks\":0,\"completion\":\"Fence\",\"synchronization_validation\":"
            <<(gpu.synchronization_validation?"true":"false")<<',';
        print_noise_metrics("raw",raw);std::cout<<',';print_noise_metrics("filtered_sixteenth",final);
        std::cout<<",\"world_history_mean\":"<<history.banks[0].mean_count
            <<",\"world_history_maximum\":"<<history.banks[0].maximum_count
            <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
        require(raw.deviation<2 && raw.high_frequency<2,
            "A black emissive bank introduced noise into deterministic native sun lighting");
        require(std::abs(raw.mean-52)<3 && std::abs(final.mean-52)<3,
            "Native fenced noise resolve changed known receiver energy");
        require(final.deviation<std::max(raw.deviation*.15,.5) &&
                final.high_frequency<std::max(raw.high_frequency*.15,.5),
            "Native fenced frames retained stochastic noise without intermediate captures");
        require(history.banks[0].mean_count>15.5 && history.banks[0].maximum_count==16,
            "Native fenced frames failed to retain lighting history without intermediate captures");
        require(gpu.validation_errors.load()==0,"Vulkan synchronization validation reported native fenced-frame hazards");
        return 0;
    }
    const auto first=noise_metrics(gpu.draw(scene,primary,{},base_camera,~0u,false,true,nullptr,true,true),extent);
    const auto bytes=gpu.renderer->allocated_bytes();
    gpu.draw(scene,primary,{},base_camera,~0u,false,true);
    const auto final=noise_metrics(gpu.draw(scene,primary,{},base_camera,~0u,false,true,nullptr,true,true),extent);
    const auto history=gpu.renderer->temporal_stats();

    // Start a separate history on the same static WORLD geometry, then move
    // the camera a quarter output pixel along the receiver per frame. Update
    // both view projection and eye: changing only the eye would not exercise
    // the actual changing WORLD positions seen at fixed raster coordinates.
    const auto moving_scene=scene;
    const auto moving_camera=[&](unsigned frame) {
        Camera camera=base_camera;
        const float displacement=float(frame)*.5f*world_scale/float(extent.width);
        camera.matrix[12]-=displacement/world_scale;camera.eye[0]+=displacement;
        return camera;
    };
    const auto moving_first=noise_metrics(gpu.draw(moving_scene,primary,{},moving_camera(0),~0u,false,true),extent);
    for(unsigned frame=1;frame<3;++frame)
        gpu.draw(moving_scene,primary,{},moving_camera(frame),~0u,false,true);
    const auto moving_final=noise_metrics(
        gpu.draw(moving_scene,primary,{},moving_camera(3),~0u,false,true,nullptr,true,true),extent);
    const auto moving_history=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing native noise: {\"width\":"<<extent.width<<",\"height\":"<<extent.height
        <<",\"upscaler\":\"Off\",\"samples\":1,\"bounces\":1,\"rendered_frames\":8,"
        <<"\"motion_pixels_per_frame\":0.25,\"world_units_per_pixel\":"<<2.f*world_scale/extent.width
        <<",\"source_world_camera\":false,";
    print_noise_metrics("raw",raw);std::cout<<',';print_noise_metrics("filtered_first",first);
    std::cout<<',';print_noise_metrics("filtered_third",final);
    std::cout<<',';print_noise_metrics("moving_first",moving_first);
    std::cout<<',';print_noise_metrics("moving_fourth",moving_final);
    std::cout<<",\"world_history_mean\":"<<history.banks[0].mean_count
        <<",\"world_history_maximum\":"<<history.banks[0].maximum_count
        <<",\"moving_history_mean\":"<<moving_history.banks[0].mean_count
        <<",\"moving_history_maximum\":"<<moving_history.banks[0].maximum_count
        <<",\"gpu_bytes\":"<<gpu.renderer->allocated_bytes()
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(raw.deviation<2 && raw.high_frequency<2,
        "A black emissive bank introduced noise into deterministic native sun lighting");
    // The exact unfiltered expectation is two times the quantized primary
    // color (26/255), hence52 in the RGBA8 output. These checks cannot pass by
    // suppressing light energy or displaying a constant black image.
    require(std::abs(raw.mean-52)<3 && std::abs(first.mean-52)<3 && std::abs(final.mean-52)<3,
        "Native lighting noise reduction changed the known receiver energy");
    require(first.deviation<std::max(raw.deviation*.6,.5) &&
            first.high_frequency<std::max(raw.high_frequency*.6,.5),
        "Native-resolution HDR resolve did not reduce measured stochastic noise");
    require(final.deviation<std::max(first.deviation*.85,.5) &&
            final.high_frequency<std::max(first.high_frequency*.85,.5),
        "Native-resolution temporal lighting did not reduce stationary noise across frames");
    require(history.banks[0].mean_count>2.8 && history.banks[0].maximum_count==3,
        "Native stationary fixture did not retain three actual temporal samples");
    require(std::abs(moving_first.mean-52)<3 && std::abs(moving_final.mean-52)<3,
        "Native camera-motion noise reduction changed the known receiver energy");
    require(moving_final.deviation<std::max(moving_first.deviation*.85,.5) &&
            moving_final.high_frequency<std::max(moving_first.high_frequency*.85,.5),
        "Native quarter-pixel camera motion retained stochastic noise instead of accumulating lighting");
    require(moving_history.banks[0].mean_count>3.7 && moving_history.banks[0].maximum_count==4,
        "Native quarter-pixel camera motion discarded valid planar lighting history");
    require(gpu.renderer->allocated_bytes()==bytes,"Native temporal history grew across identical frames");
    require(gpu.validation_errors.load()==0,"Vulkan validation reported errors in native noise fixture");
    return 0;
}
}
int shadow_disocclusion(const std::filesystem::path& shaders,VkExtent2D extent,bool native_banks=false) {
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    auto black_emitter=white_material();black_emitter.diffuse=solid_texture({0,0,0,255});
    black_emitter.forward_vertex=2;black_emitter.forward_diffuse={1,1,1,1};
    std::array<Material,3> materials{white_material(),white_material(),black_emitter};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    auto left=primary,right=primary;
    // The blocker is behind the camera, so it changes the receiver's shadow
    // without changing primary visibility, normals, material or world position.
    quad(left,-.75f,-.75f,-.25f,.75f,-3,{0,0,1},1);
    quad(right,.25f,-.75f,.75f,.75f,-3,{0,0,1},1);
    if(native_banks)for(auto* vertices:{&left,&right})
        quad(*vertices,3,3,4,4,-3,{0,0,1},2);
    constexpr float scale=64.f;const Vec3 origin{74.2382f,-117.899f,.21363f};
    for(auto* vertices:{&primary,&left,&right})for(auto& vertex:*vertices)
        vertex.position={origin.x+vertex.position.x*scale,origin.y+vertex.position.y*scale,
                         origin.z+vertex.position.z*scale};
    Camera camera;camera.matrix[0]=camera.matrix[5]=camera.matrix[10]=1.f/scale;
    camera.matrix[12]=-origin.x/scale;camera.matrix[13]=-origin.y/scale;
    camera.matrix[14]=-origin.z/scale;
    camera.eye={origin.x,origin.y,origin.z-2.f*scale,1};camera.forward[3]=1;
    WorldMaterialUniform bank{};bank.source_camera_control[0]=1;
    bank.source_camera_world[3]={0,0,16777215.f/scale,-origin.z*16777215.f/scale};
    std::memcpy(gpu.uniform.mapped,&bank,sizeof(bank));
    pt::Light light;light.direction=Vec3{0,0,-1};light.color={.6f,.6f,.6f};
    std::array<Vec3,6> sky;sky.fill({.01f,.025f,.1f});
    if(native_banks)light.color={.6f,.3f,.15f};
    const auto scene_left=pt::build_scene(left,materials,std::span(&light,1));
    const auto scene_right=pt::build_scene(right,materials,std::span(&light,1));
    Camera panned=camera;panned.matrix[12]-=16.f/float(extent.width);
    panned.eye[0]+=16.f*scale/float(extent.width);
    const auto render=[&](const pt::Scene& scene,const Camera& at,bool capture) {
        return gpu.draw(scene,primary,{},at,~0u,false,true,native_banks?&sky:nullptr,true,capture,capture);
    };
    const auto reference=[&](const pt::Scene& scene,const Camera& at) {
        if(native_banks)for(unsigned n=0;n<15;++n)render(scene,at,false);
        return render(scene,at,true);
    };
    const auto reference_left=reference(scene_left,camera);
    const auto reference_right=reference(scene_right,camera);
    const auto camera_reference=reference(scene_left,panned);
    auto moving_scene=scene_left;
    for(unsigned frame=0;frame<15;++frame)render(moving_scene,camera,false);
    render(moving_scene,camera,true);
    const auto warm_history=gpu.renderer->temporal_stats();
    const auto camera_first=render(moving_scene,panned,true);
    render(moving_scene,panned,false);
    const auto camera_third=render(moving_scene,panned,true);
    // Return to the initial view and fill its history before changing only
    // secondary-ray geometry. Endpoints use fresh-source frames, not a
    // renderer-specific expected image or the stale history under test.
    for(unsigned frame=0;frame<16;++frame)render(moving_scene,camera,false);
    pt::update_geometry(moving_scene,right,std::span(&light,1));
    const auto shadow_first=render(moving_scene,camera,true);
    render(moving_scene,camera,false);
    const auto shadow_third=render(moving_scene,camera,true);
    const auto changed_history=gpu.renderer->temporal_stats();
    const auto region=[&](const std::vector<std::uint8_t>& frame,float x0,float x1) {
        const auto l=unsigned(float(extent.width)*x0),r=unsigned(float(extent.width)*x1);
        const auto t=extent.height*2/5,b=extent.height*3/5;
        double sum{};for(unsigned y=t;y<b;++y)for(unsigned x=l;x<r;++x)
            sum+=frame[(std::size_t(y)*extent.width+x)*4];
        return sum/double((r-l)*(b-t));
    };
    const auto error=[&](const std::vector<std::uint8_t>& frame,const std::vector<std::uint8_t>& reference) {
        double sum{};std::size_t count{};
        for(unsigned y=extent.height/5;y<extent.height*4/5;++y)
            for(unsigned x=extent.width/5;x<extent.width*4/5;++x) {
                const auto at=(std::size_t(y)*extent.width+x)*4;
                sum+=std::abs(int(frame[at])-int(reference[at]));++count;
            }
        return sum/double(count);
    };
    const auto old_dark=region(reference_left,.18f,.30f),old_lit=region(reference_right,.18f,.30f);
    const auto new_dark=region(reference_right,.70f,.82f),new_lit=region(reference_left,.70f,.82f);
    const auto contrast=std::min(old_lit-old_dark,new_lit-new_dark);
    const auto old_residue=old_lit-region(shadow_third,.18f,.30f);
    const auto new_residue=region(shadow_third,.70f,.82f)-new_dark;
    std::cout<<"Path Tracing shadow disocclusion: {\"width\":"<<extent.width<<",\"height\":"<<extent.height
        <<",\"upscaler\":\"Off\",\"samples\":1,\"bounces\":1,\"warm_frames\":16,\"stop_frames\":3"
        <<",\"native_sky_bank\":"<<(native_banks?"true":"false")
        <<",\"emissive_triangles\":"<<scene_left.emissive_count
        <<",\"reference_contrast\":"<<contrast
        <<",\"camera_first_error\":"<<error(camera_first,camera_reference)
        <<",\"camera_third_error\":"<<error(camera_third,camera_reference)
        <<",\"shadow_first_error\":"<<error(shadow_first,reference_right)
        <<",\"shadow_third_error\":"<<error(shadow_third,reference_right)
        <<",\"old_shadow_residue\":"<<old_residue<<",\"new_shadow_residue\":"<<new_residue
        <<",\"warm_history_mean\":"<<warm_history.banks[0].mean_count
        <<",\"changed_history_mean\":"<<changed_history.banks[0].mean_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(contrast>100 && old_dark<(native_banks?10:2) && new_dark<(native_banks?10:2),
        "Shadow disocclusion control did not establish distinct current visibility endpoints");
    require(warm_history.banks[0].mean_count>15.5,
        "Shadow disocclusion control did not establish a full stationary lighting history");
    require(error(camera_third,camera_reference)<(native_banks?10:2),
        "RT lighting retained the prior screen position after camera motion stopped");
    require(old_residue<contrast*.1 && new_residue<contrast*.1,
        "RT lighting kept stale shadow visibility after the opaque blocker moved");
    require(gpu.validation_errors.load()==0,"Shadow disocclusion fixture reported Vulkan errors");
    return 0;
}
int unrelated_motion_noise(const std::filesystem::path& shaders,VkExtent2D extent) {
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);gpu.primary_color({.1f,.1f,.1f,1});
    auto black_emitter=white_material();black_emitter.diffuse=solid_texture({0,0,0,255});
    black_emitter.forward_vertex=2;black_emitter.forward_diffuse={1,1,1,1};
    auto black_surface=white_material();black_surface.diffuse=solid_texture({0,0,0,255});
    const std::array<Material,3> materials{white_material(),black_emitter,black_surface};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    auto geometry=primary;quad(geometry,3,3,4,4,-3,{0,0,1},1);
    const auto far_begin=geometry.size();
    // This black, nonemissive object is far outside the camera and the
    // receiver's direct-light rays. Its ordinary pose updates cannot justify
    // discarding all receiver lighting samples every frame.
    quad(geometry,12,12,13,13,-3,{0,0,1},2);
    constexpr float scale=64.f;const Vec3 origin{74.2382f,-117.899f,.21363f};
    for(auto* vertices:{&primary,&geometry})for(auto& vertex:*vertices)
        vertex.position={origin.x+vertex.position.x*scale,origin.y+vertex.position.y*scale,
                         origin.z+vertex.position.z*scale};
    Camera camera;camera.matrix[0]=camera.matrix[5]=camera.matrix[10]=1.f/scale;
    camera.matrix[12]=-origin.x/scale;camera.matrix[13]=-origin.y/scale;
    camera.matrix[14]=-origin.z/scale;camera.forward[3]=1;
    camera.eye={origin.x,origin.y,origin.z-2.f*scale,1};
    WorldMaterialUniform bank{};bank.source_camera_control[0]=1;
    bank.source_camera_world[3]={0,0,16777215.f/scale,-origin.z*16777215.f/scale};
    std::memcpy(gpu.uniform.mapped,&bank,sizeof(bank));
    pt::Light light;light.direction=Vec3{0,0,-1};light.color={2,2,2};
    std::array<Vec3,6> sky;sky.fill({.08f,.08f,.08f});
    // Unequal cube faces keep the continuation's indirect estimate stochastic
    // even when the primary sun is sampled deterministically every frame.
    sky[0]={1.2f,1.2f,1.2f};sky[3]={.8f,.8f,.8f};
    const auto static_scene=pt::build_scene(geometry,materials,std::span(&light,1));
    const auto raw=noise_metrics(gpu.draw(static_scene,primary,{},camera,~0u,false,false,&sky),extent);
    const auto render=[&](const pt::Scene& scene,bool capture) {
        return gpu.draw(scene,primary,{},camera,~0u,false,true,&sky,true,capture,capture);
    };
    for(unsigned frame=0;frame<15;++frame)render(static_scene,false);
    const auto stationary=noise_metrics(render(static_scene,true),extent);
    const auto stationary_history=gpu.renderer->temporal_stats();
    auto moving_scene=static_scene;
    for(unsigned frame=0;frame<16;++frame) {
        auto moved=geometry;
        for(std::size_t n=far_begin;n<moved.size();++n) {
            moved[n].position.x+=float(frame)*.1f*scale;
            moved[n].position.y+=float(frame%3)*.05f*scale;
        }
        pt::update_geometry(moving_scene,moved,std::span(&light,1));
        if(frame!=15)render(moving_scene,false);
    }
    const auto animated=noise_metrics(render(moving_scene,true),extent);
    const auto moving_history=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing unrelated motion noise: {\"width\":"<<extent.width<<",\"height\":"<<extent.height
        <<",\"upscaler\":\"Off\",\"samples\":1,\"bounces\":1,\"stationary_frames\":16,\"moving_frames\":16"
        <<",\"native_sky_bank\":true,\"light_count\":"<<static_scene.lights.size()
        <<",\"emissive_triangles\":"<<static_scene.emissive_count<<',';
    print_noise_metrics("raw",raw);std::cout<<',';print_noise_metrics("stationary",stationary);
    std::cout<<',';print_noise_metrics("unrelated_motion",animated);
    std::cout<<",\"stationary_history_mean\":"<<stationary_history.banks[0].mean_count
        <<",\"moving_history_mean\":"<<moving_history.banks[0].mean_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(static_scene.emissive_count>0 && raw.mean>10 && raw.deviation>2 && raw.high_frequency>2,
        "Unrelated-motion control did not establish stochastic source/sky/emissive-bank lighting");
    require(stationary.deviation<raw.deviation*.2 && stationary.high_frequency<raw.high_frequency*.1,
        "Unrelated-motion control did not establish a denoised stationary reference");
    require(std::abs(stationary.mean-raw.mean)<3 && std::abs(animated.mean-raw.mean)<3,
        "Unrelated-motion denoising suppressed the established receiver lighting energy");
    require(std::abs(animated.mean-stationary.mean)<2,
        "Unrelated offscreen object motion changed receiver lighting energy");
    require(animated.deviation<stationary.deviation*1.35 &&
            animated.high_frequency<stationary.high_frequency*1.4+.1,
        "Unrelated offscreen geometry animation discarded denoised receiver lighting");
    require(stationary_history.banks[0].mean_count>15.5 && moving_history.banks[0].mean_count>15.5,
        "Unrelated offscreen geometry motion reset valid stationary receiver history");
    require(gpu.validation_errors.load()==0,"Unrelated-motion fixture reported Vulkan errors");
    return 0;
}
#include "lockout_color_gpu_fixture.hpp"
#include "surface_ray_gpu_fixture.hpp"
#include "grazing_normal_gpu_fixture.hpp"
#include "grazing_secondary_gpu_fixture.hpp"
#include "temporal_reprojection_gpu_fixture.hpp"
int main(int argc,char** argv)try {
    if((argc==3 || argc==5) && (std::string_view(argv[2])=="--foreground-reprojection" ||
                              std::string_view(argv[2])=="--foreground-reprojection-trace")) {
        VkExtent2D extent{512,288};
        if(argc==5) {
            const auto w=std::stoul(argv[3]),h=std::stoul(argv[4]);
            require(w>=64 && w<=4096 && h>=64 && h<=4096,
                "Foreground reprojection extent must be within64..4096 pixels per axis");
            extent={unsigned(w),unsigned(h)};
        }
        return foreground_reprojection_gpu(argv[1],extent,
            std::string_view(argv[2])=="--foreground-reprojection-trace");
    }
    if((argc==3 || argc==5) && (std::string_view(argv[2])=="--jitter-reprojection" ||
                              std::string_view(argv[2])=="--camera-reprojection")) {
        VkExtent2D extent{512,288};
        if(argc==5) {
            const auto w=std::stoul(argv[3]),h=std::stoul(argv[4]);
            require(w>=64 && w<=4096 && h>=64 && h<=4096,
                "Temporal reprojection extent must be within64..4096 pixels per axis");
            extent={unsigned(w),unsigned(h)};
        }
        return temporal_reprojection_gpu(argv[1],std::string_view(argv[2])=="--camera-reprojection",extent);
    }
    if(argc==3 && std::string_view(argv[2])=="--grazing-normal")
        return grazing_normal_gpu(argv[1]);
    if(argc==3 && std::string_view(argv[2])=="--grazing-secondary")
        return grazing_secondary_gpu(argv[1]);
    if((argc==3 || argc==4) && std::string_view(argv[2])=="--smooth-terminator")
        return smooth_terminator_gpu(argv[1],argc==4?argv[3]:nullptr);
    if(argc==6 && std::string_view(argv[2])=="--smooth-terminator") {
        const auto w=std::stoul(argv[4]),h=std::stoul(argv[5]);
        require(w>=64 && w<=4096 && h>=64 && h<=4096,"Smooth-terminator extent must be64..4096 per axis");
        return smooth_terminator_gpu(argv[1],argv[3],{unsigned(w),unsigned(h)});
    }
    if(argc==5 && std::string_view(argv[2])=="--actual-lockout-colors")
        return actual_lockout_colors(argv[1],argv[3],argv[4]);
    if((argc==3 || argc==5) && std::string_view(argv[2])=="--native-shadow-disocclusion") {
        VkExtent2D extent{512,288};
        if(argc==5) {
            const auto w=std::stoul(argv[3]),h=std::stoul(argv[4]);
            require(w>=64 && w<=4096 && h>=64 && h<=4096,
                "Native shadow disocclusion extent must be within64..4096 pixels per axis");
            extent={unsigned(w),unsigned(h)};
        }
        return shadow_disocclusion(argv[1],extent,true);
    }
    if((argc==3 || argc==5) && std::string_view(argv[2])=="--unrelated-motion-noise") {
        VkExtent2D extent{512,288};
        if(argc==5) {
            const auto w=std::stoul(argv[3]),h=std::stoul(argv[4]);
            require(w>=64 && w<=4096 && h>=64 && h<=4096,
                "Unrelated-motion extent must be within64..4096 pixels per axis");
            extent={unsigned(w),unsigned(h)};
        }
        return unrelated_motion_noise(argv[1],extent);
    }
    if((argc==3 || argc==5) && std::string_view(argv[2])=="--shadow-disocclusion") {
        VkExtent2D extent{512,288};
        if(argc==5) {
            const auto w=std::stoul(argv[3]),h=std::stoul(argv[4]);
            require(w>=64 && w<=4096 && h>=64 && h<=4096,
                "Shadow disocclusion extent must be within64..4096 pixels per axis");
            extent={unsigned(w),unsigned(h)};
        }
        return shadow_disocclusion(argv[1],extent);
    }
    if((argc==3 || argc==5) && (std::string_view(argv[2])=="--native-noise" ||
                              std::string_view(argv[2])=="--native-fence-noise")) {
        VkExtent2D extent{2560,1388};
        if(argc==5) {
            const auto parsed_width=std::stoul(argv[3]),parsed_height=std::stoul(argv[4]);
            require(parsed_width>=16 && parsed_width<=4096 && parsed_height>=16 && parsed_height<=4096,
                "Native noise extent must be within16..4096 pixels per axis");
            extent={unsigned(parsed_width),unsigned(parsed_height)};
        }
        const bool fence_only=std::string_view(argv[2])=="--native-fence-noise";
        return native_noise(argv[1],extent,fence_only);
    }
    require(argc==2,"Supply compiled path-tracing shader directory");
    Fixture gpu;gpu.initialize(argv[1]);
    // Rasterize two overlapping emissive triangles, blue first and red last.
    // Their source integer-W order deliberately reverses geometric Z; treating
    // raw c3 as normalized depth clamps both to one and wrongly shows red.
    std::vector<Material> depth_materials(2);for(auto& material:depth_materials)material=white_material();
    depth_materials[0].forward_vertex=2;depth_materials[0].forward_diffuse={1,0,0,1};
    depth_materials[1].forward_vertex=2;depth_materials[1].forward_diffuse={0,0,1,1};
    const auto depth_scene=pt::build_scene({},depth_materials);
    std::vector<Vertex> overlapping;
    for(const auto plane:std::array<std::array<float,2>,2>{{{.75f,1},{.25f,0}}})
        for(const auto p:std::array<Vec3,3>{{{-1,-1,plane[0]},{1,-1,plane[0]},{0,1,plane[0]}}}) {
            Vertex vertex;vertex.position=p;vertex.color={1,1,1};vertex.normal={0,0,-1};
            vertex.material=unsigned(plane[1]);overlapping.push_back(vertex);
        }
    Camera depth_camera;depth_camera.forward[3]=1;
    const auto geometric_depth=gpu.draw(depth_scene,overlapping,{},depth_camera);
    const auto rebound_material=gpu.draw(depth_scene,overlapping,{},depth_camera,1);
    require(mean(rebound_material,2)>200 && mean(rebound_material,0)<5,
        "Resident geometry ignored its actual draw material/LOD binding");
    require(mean(geometric_depth,0)>240 && mean(geometric_depth,2)<8,
        "Geometric W-depth control did not show the nearer red triangle");
    WorldMaterialUniform source_camera{};source_camera.source_camera_control[0]=1;
    source_camera.source_camera_world[3]={0,0,-8388608,12582912};
    source_camera.source_camera_reflection[3]={0,0,8388608,0};
    source_camera.source_camera_foreground[3]=source_camera.source_camera_reflection[3];
    std::memcpy(gpu.uniform.mapped,&source_camera,sizeof(source_camera));
    const auto source_world_depth=gpu.draw(depth_scene,overlapping,{},depth_camera);
    require(mean(source_world_depth,2)>240 && mean(source_world_depth,0)<8,
        "Path Tracing did not use the actual integer-W world camera row");
    require(gpu.draw(depth_scene,overlapping,{},depth_camera,~0u,true)==source_world_depth,
        "Visibility prepass replaced source W with hardware geometric depth");
    require(gpu.draw(depth_scene,overlapping,{},depth_camera,0,true)==
            gpu.draw(depth_scene,overlapping,{},depth_camera,0),
        "Visibility prepass ignored the actual per-draw material override");
    auto cutout_materials=depth_materials;
    cutout_materials[1].forward_raster.alpha_test=true;
    cutout_materials[1].forward_raster.alpha_function=0x204;
    cutout_materials[1].forward_raster.alpha_reference=128;
    const auto cutout_scene=pt::build_scene({},cutout_materials);
    gpu.primary_color({1,1,1,.25f});
    const auto cutout_frame=gpu.draw(cutout_scene,overlapping,{},depth_camera);
    require(mean(cutout_frame,0)>240 && mean(cutout_frame,2)<8,
        "Visibility alpha control did not expose the farther source-W surface");
    require(gpu.draw(cutout_scene,overlapping,{},depth_camera,~0u,true)==cutout_frame,
        "Visibility prepass turned alpha-tested source holes into opaque blockers");
    gpu.primary_color({1,1,1,1});
    auto stippled=overlapping;
    for(unsigned n=0;n<3;++n)stippled[n].source_stipple_level=8;
    const auto stippled_frame=gpu.draw(depth_scene,stippled,{},depth_camera);
    require(mean(stippled_frame,0)>20 && mean(stippled_frame,2)>20,
        "Visibility stipple control did not retain both source-W surfaces");
    require(gpu.draw(depth_scene,stippled,{},depth_camera,~0u,true)==stippled_frame,
        "Visibility prepass changed exact source stipple coverage");
    source_camera.source_camera_world[3]={0,0,8388608,0};
    source_camera.source_camera_reflection[3]={0,0,-8388608,12582912};
    source_camera.source_camera_foreground[3]=source_camera.source_camera_reflection[3];
    std::memcpy(gpu.uniform.mapped,&source_camera,sizeof(source_camera));
    depth_camera.eye[3]=3;
    const auto reflected_depth=gpu.draw(depth_scene,overlapping,{},depth_camera);
    require(mean(reflected_depth,2)>240 && mean(reflected_depth,0)<8,
        "Path Tracing reflection selected the world W-depth row");
    require(gpu.draw(depth_scene,overlapping,{},depth_camera,~0u,true)==reflected_depth,
        "Reflection visibility prepass used a different source W camera");
    depth_camera.eye[3]=2;depth_camera.forward[3]=.15f;
    const auto foreground_depth=gpu.draw(depth_scene,overlapping,{},depth_camera);
    require(mean(foreground_depth,2)>240 && mean(foreground_depth,0)<8,
        "Path Tracing first person scaled or replaced its actual integer-W row");
    require(gpu.draw(depth_scene,overlapping,{},depth_camera,~0u,true)==foreground_depth,
        "Foreground visibility prepass used the wrong source W scale/camera");
    std::memset(gpu.uniform.mapped,0,sizeof(WorldMaterialUniform));
    progress("source-W and coverage fixtures passed; starting bounded ray transport");
    std::vector<Material> materials(3);for(auto& material:materials)material=white_material();
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    auto geometry=primary;
    const pt::Light light{{0,0,-1},8,{1,1,1},1};
    auto scene=pt::build_scene(geometry,materials,std::span(&light,1));
    const auto lit=gpu.draw(scene,primary);
    require(gpu.draw(scene,primary,{},Camera{},~0u,true)==lit,
        "Exact-W visibility changed the visible receiver's ray-shaded pixels");
    auto baked=primary;
    for(auto &vertex:baked){vertex.color={.01f,.02f,.03f};vertex.lightmap=1;}
    auto baked_scene=pt::build_scene(baked,materials,std::span(&light,1));
    const auto baked_frame=gpu.draw(baked_scene,baked);
    require(baked_frame==lit,"Baked lightmap vertex radiance survived in Path Tracing primary or secondary shading");
    for(auto &vertex:baked){vertex.lightmap=0;vertex.bsp_cluster=0;vertex.color={.2f,.1f,.05f};}
    pt::update_geometry(baked_scene,baked,std::span(&light,1));
    require(gpu.draw(baked_scene,baked)==lit,"Baked per-vertex BSP lighting survived without a lightmap image");
    for(auto &vertex:baked)vertex.bsp_cluster=-1;
    pt::update_geometry(baked_scene,baked,std::span(&light,1));
    require(mean(gpu.draw(baked_scene,baked),0)<mean(lit,0)-20,"Actual object vertex tint was lost with baked BSP lighting");
    quad(geometry,-.4f,-.4f,.4f,.4f,0,{0,0,-1},1);
    pt::refresh_materials_for_geometry(scene,materials,geometry);pt::update_geometry(scene,geometry,std::span(&light,1));
    const auto shadow=gpu.draw(scene,primary);
    require(mean(lit,0)>mean(shadow,0)+10,"Ray shadow blocker did not darken the receiver");
    // Local full WORLD bodies can be absent from primary first-person
    // visibility while still blocking the actual light's secondary rays.
    auto local_body=std::vector<Vertex>(geometry.begin()+primary.size(),geometry.end());
    for(auto &vertex:local_body)vertex.source_section=17;
    const auto primary_before=primary;std::vector<Vertex> local_shadow_geometry;
    const std::array<pt::MaterialRange,1> body_material{{{1,1}}};
    pt::append_secondary_geometry(local_shadow_geometry,primary,materials,body_material);
    pt::append_secondary_geometry(local_shadow_geometry,local_body,materials);
    auto local_shadow_scene=pt::build_scene(local_shadow_geometry,materials,std::span(&light,1));
    require(gpu.draw(local_shadow_scene,primary)==shadow,
        "Full local WORLD caster omitted from first-person primary geometry did not produce the same ray shadow");
    const std::array<pt::SecondaryBodyOwner,1> local_body_owner{{{
        .root=0x12340007u,.definition=0xabcd0001u,.render_model=0xabcd0002u,
        .source_tick=90,.lod=4,.materials={1,1},.first_vertex=0,
        .vertex_count=static_cast<std::uint32_t>(local_body.size()),.sections={17}}}};
    const auto local_body_count=pt::secondary_body_counts(local_shadow_scene,
        local_body,local_body_owner,materials,body_material);
    require(local_body_count.size()==1 && local_body_count.front().triangles==2 &&
        local_body_count.front().eligible_triangles==2,
        "Actual ray-shadow BVH omitted its designated full-body caster receipt");
    auto weapon_only_geometry=local_shadow_geometry;
    for(auto &vertex:weapon_only_geometry)if(vertex.material==1)vertex.material=2;
    const auto weapon_only_scene=pt::build_scene(weapon_only_geometry,materials,std::span(&light,1));
    require(gpu.draw(weapon_only_scene,primary)==shadow,
        "Weapon-only negative control did not retain the same visible ray shadow");
    const auto weapon_only_count=pt::secondary_body_counts(weapon_only_scene,
        local_body,local_body_owner,materials,body_material);
    require(weapon_only_count.size()==1 && weapon_only_count.front().triangles==0 &&
        weapon_only_count.front().eligible_triangles==2,
        "A visible weapon-only ray shadow falsely acquired a full-body caster receipt");
    require(primary.size()==primary_before.size() &&
        std::memcmp(primary.data(),primary_before.data(),primary.size()*sizeof(Vertex))==0,
        "Secondary local-body shadow changed primary first-person visibility");
    auto faded_geometry=geometry;
    for(std::size_t first=primary.size();first<faded_geometry.size();first+=3)
        faded_geometry[first].source_stipple_level=0;
    const auto faded_scene=pt::build_scene(faded_geometry,materials,std::span(&light,1));
    require(gpu.draw(faded_scene,primary)==lit,"Fully faded source geometry cast an invisible ray shadow");
    auto faded_primary=primary;
    for(std::size_t first=0;first<faded_primary.size();first+=3)faded_primary[first].source_stipple_level=0;
    const auto faded_primary_scene=pt::build_scene(faded_primary,materials,std::span(&light,1));
    require(mean(gpu.draw(faded_primary_scene,faded_primary),0)<1,
        "CPU ray visibility disagreed with the actual first-vertex flat stipple raster policy");
    // Keep direct illumination occluded, then add a lit diffuse side wall.
    // The new contribution must travel light -> wall -> receiver.
    for(const auto position:std::array<Vec3,6>{{{1,-2,-2},{1,2,-2},{1,-2,.5f},
        {1,-2,.5f},{1,2,-2},{1,2,.5f}}}) {
        Vertex vertex;vertex.position=position;vertex.normal={-1,0,0};vertex.color={1,1,1};vertex.material=2;
        geometry.push_back(vertex);
    }
    pt::refresh_materials_for_geometry(scene,materials,geometry);
    pt::update_geometry(scene,geometry,std::span(&light,1));const auto bounced=gpu.draw(scene,primary);
    require(mean(bounced,0)>mean(shadow,0)+2,
        "Lit diffuse wall did not bounce indirect illumination into the ray shadow");
    pt::update_geometry(scene,primary,std::span(&light,1));
    const auto restored=gpu.draw(scene,primary);
    require(restored==lit,"Frame-local BVH update did not restore the unblocked receiver");
    pt::Light source_sky;source_sky.direction=Vec3{0,0,-1};source_sky.color={1,1,1};
    auto sky_scene=pt::build_scene(primary,materials,std::span(&source_sky,1));
    const auto directional_lit=gpu.draw(sky_scene,primary);
    geometry=primary;quad(geometry,-.4f,-.4f,.4f,.4f,-100,{0,0,-1},1);
    pt::refresh_materials_for_geometry(sky_scene,materials,geometry);
    pt::update_geometry(sky_scene,geometry,std::span(&source_sky,1));
    const auto directional_shadow=gpu.draw(sky_scene,primary);
    require(mean(directional_lit,0)>mean(directional_shadow,0)+50,
        "Actual source sky direction did not cast an offscreen distant ray shadow");
    auto reverse_sky=source_sky;reverse_sky.direction=Vec3{0,0,1};
    pt::update_geometry(sky_scene,primary,std::span(&reverse_sky,1));
    require(mean(gpu.draw(sky_scene,primary),0)<mean(directional_lit,0)-50,
        "Source sky incident vector was reversed into the surface");
    auto dark_scene=pt::build_scene(primary,materials);const auto dark=gpu.draw(dark_scene,primary);
    auto clipped_light=light;clipped_light.planes[0]={1,0,0,-2};
    pt::update_geometry(scene,primary,std::span(&clipped_light,1));
    const auto outside_cone=gpu.draw(scene,primary);
    require(outside_cone==dark,"Ray lighting escaped the authored projected-light influence planes");
    geometry=primary;quad(geometry,-1,-1,1,1,-1,{0,0,1},1);
    materials[1].forward_vertex=2;materials[1].forward_diffuse={.6f,.04f,.02f,1};
    auto emissive=pt::build_scene(geometry,materials);const auto gi=gpu.draw(emissive,primary);
    require(mean(gi,0)>mean(dark,0)+4 && mean(gi,0)>mean(gi,2)+3,
        "Emissive geometry did not produce red indirect lighting on the diffuse receiver");
    auto weak_light=light;weak_light.intensity=.1f;
    auto mixed_materials=materials;mixed_materials[1].forward_diffuse={.1f,.006666667f,.003333333f,1};
    auto mixed=pt::build_scene(geometry,mixed_materials,std::span(&weak_light,1));
    auto point_only=mixed;point_only.emissive_count=0;point_only.materials[1].emission={};
    auto mesh_only=mixed;mesh_only.lights.clear();
    const auto mixed_frame=gpu.draw(mixed,primary);
    const auto point_frame=gpu.draw(point_only,primary);
    const auto mesh_frame=gpu.draw(mesh_only,primary);
    std::cout<<"Path Tracing mixed source energy: {\"channels\":[";
    for(unsigned channel=0;channel<3;++channel) {
        if(channel)std::cout<<',';
        std::cout<<"{\"combined\":"<<mean(mixed_frame,channel,0,width,0,height)
            <<",\"point\":"<<mean(point_frame,channel,0,width,0,height)
            <<",\"mesh\":"<<mean(mesh_frame,channel,0,width,0,height)<<'}';
    }
    std::cout<<"]}"<<std::endl;
    for(unsigned channel=0;channel<3;++channel) {
        const auto expected=mean(point_frame,channel,0,width,0,height)+mean(mesh_frame,channel,0,width,0,height);
        require(std::abs(mean(mixed_frame,channel,0,width,0,height)-expected)<std::max(2.f,.15f*expected),
            "Combined source-light/emissive sampling dropped or doubled radiance");
    }
    // The reflector sits behind the camera and outside the primary clip range.
    // It can reach the visible receiver only through the traced scene.
    geometry=primary;quad(geometry,-.8f,-.8f,.8f,.8f,-3,{0,0,1},1);
    auto diffuse=pt::build_scene(geometry,materials);const auto diffuse_frame=gpu.draw(diffuse,primary);
    materials[0].name="objects\\characters\\masterchief\\shaders\\masterchief_visor";
    materials[0].environment_pass=std::make_shared<Material>();
    materials[0].environment_pass->forward_vertex=4;
    auto visor=pt::build_scene(geometry,materials);const auto visor_frame=gpu.draw(visor,primary);
    require(visor.materials[0].response[1]>0 && visor.materials[0].response[1]<=.4f &&
        diffuse.materials[0].response[1]==0 && diffuse.materials[0].response[0]>visor.materials[0].response[0],
        "Authored visor and diffuse materials lost distinct reflection responses");
    require(mean(visor_frame,0)>mean(diffuse_frame,0)+3,
        "Player visor did not reflect the offscreen emissive scene");
    auto rough=visor;rough.materials[0].response[0]=.85f;
    const auto rough_frame=gpu.draw(rough,primary);
    require(mean(visor_frame,0)>mean(rough_frame,0)+2,
        "Rough material retained the sharp visor reflection");
    progress("baseline visibility, lighting and reflection fixtures passed; starting HDR");
    // The authored sky fallback must illuminate diffuse misses even without
    // any point/emissive lights, and resolve must preserve that constant HDR.
    std::vector<Material> filter_materials{white_material(),white_material()};
    const auto sky_only=pt::build_scene(primary,filter_materials);
    const auto environment_frame=gpu.draw(sky_only,primary,{.125f,.25f,.5f},Camera{},~0u,false,true);
    require(std::abs(mean(environment_frame,0)-32)<2 && std::abs(mean(environment_frame,1)-64)<2 &&
        std::abs(mean(environment_frame,2)-128)<2,"Native sky cube fallback did not illuminate filtered diffuse misses");

    // A one-texel stripe pattern must survive the half-resolution lighting
    // grid. Its source albedo is applied after the lighting filter.
    std::vector<std::uint8_t> checker(width*height*4,255);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
        // Odd columns are the2x2 grid's representatives. Their black albedo
        // must not erase irradiance needed by neighboring colored texels.
        const std::array<std::uint8_t,3> color=x%2?std::array<std::uint8_t,3>{0,0,0}:
            std::array<std::uint8_t,3>{255,127,63};
        for(unsigned c=0;c<3;++c)checker[(y*width+x)*4+c]=color[c];
    }
    gpu.primary_texture(checker);
    const auto textured=gpu.draw(sky_only,primary,{.25f,.25f,.25f},Camera{},~0u,false,true);
    double texture_error{};
    for(unsigned y=8;y<56;++y)for(unsigned x=8;x<56;++x)for(unsigned c=0;c<3;++c)
        texture_error+=std::abs(float(textured[(y*width+x)*4+c])-.25f*checker[(y*width+x)*4+c]);
    require(texture_error/(48*48*3)<1.5,"Lighting denoise blurred or recolored full-resolution authored albedo");

    // Two unequal authored point lights keep this estimate stochastic even
    // when the primary sun no longer competes with a black emitter bank.
    // Their combined irradiance is2; filter that HDR before the display clamp
    // at the unchanged default1/1 query budget.
    gpu.primary_color({.1f,.1f,.1f,1});
    auto black_emitter=white_material();black_emitter.diffuse.pixels[0]=black_emitter.diffuse.pixels[1]=
        black_emitter.diffuse.pixels[2]=std::byte{0};black_emitter.forward_vertex=2;
    black_emitter.forward_diffuse={1,1,1,1};filter_materials[1]=black_emitter;
    auto filter_geometry=primary;quad(filter_geometry,3,3,4,4,-3,{0,0,1},1);
    pt::Light bright_sky;bright_sky.direction=Vec3{0,0,-1};bright_sky.color={2,2,2};
    std::array<pt::Light,2> unequal_lights{bright_sky,bright_sky};
    unequal_lights[0].color={1.8f,1.8f,1.8f};unequal_lights[1].color={.2f,.2f,.2f};
    const auto noisy_scene=pt::build_scene(filter_geometry,filter_materials,unequal_lights);
    const auto noisy=gpu.draw(noisy_scene,primary);
    const auto denoised=gpu.draw(noisy_scene,primary,{},Camera{},~0u,false,true);
    const auto reference_scene=pt::build_scene(primary,filter_materials,std::span(&bright_sky,1));
    const auto reference=gpu.draw(reference_scene,primary);
    require(std::abs(mean(denoised,0,8,56,8,56)-mean(reference,0,8,56,8,56))<8,
        "Filtered HDR irradiance was clamped before applying primary albedo or lost emitter-bank energy");
    require(deviation(denoised,0)<deviation(noisy,0)*.6f,
        "Bounded1/1 HDR lighting resolve did not reduce stochastic bank-selection noise");
    gpu.primary_color({1,1,1,1});
    const auto hdr_white=gpu.draw(noisy_scene,primary,{},Camera{},~0u,false,true);
    require(mean(hdr_white,0,8,56,8,56)>210,
        "Lighting resolve averaged display-clamped samples instead of linear HDR radiance");

    // Different opaque materials remain separate at their shared screen edge.
    filter_materials={white_material(),white_material()};filter_materials[1].forward_vertex=2;
    filter_materials[1].forward_diffuse={0,0,.25f,1};
    std::vector<Vertex> edge_geometry;quad(edge_geometry,-1,-1,0,1,.5f,{0,0,-1},0);
    quad(edge_geometry,0,-1,1,1,.5f,{0,0,-1},1);
    auto edge_light=bright_sky;edge_light.color={.5f,.5f,.5f};
    auto edge_scene=pt::build_scene(edge_geometry,filter_materials,std::span(&edge_light,1));
    // Isolate the primary material guide from mesh-emitter sampling variance.
    edge_scene.emissive_count=0;edge_scene.pixels.resize(edge_scene.texture_texel_count);
    const auto material_edge=gpu.draw(edge_scene,edge_geometry,{},Camera{},~0u,false,true);
    require(mean(material_edge,0,30,32)>120 && mean(material_edge,0,32,34)<3 &&
        mean(material_edge,2,32,34)>60,"Lighting resolve bled across an authored material boundary");

    // Same-material neighboring normals reflect distinct native cube faces.
    // The normal guide must retain that edge instead of smearing visor color.
    filter_materials={white_material()};auto reflection_scene=pt::build_scene({},filter_materials);
    reflection_scene.materials[0].base={1,1,1,1};reflection_scene.materials[0].response={.12f,1,1,0};
    edge_geometry.clear();quad(edge_geometry,-1,-1,0,1,.5f,{0,0,-1},0);
    quad(edge_geometry,0,-1,1,1,.5f,{.70710678f,0,-.70710678f},0);
    const std::array<Vec3,6> faces{{{0,1,0},{0,0,1},{0,0,1},{0,0,1},{0,0,1},{1,0,0}}};
    const auto reflection_edge=gpu.draw(reflection_scene,edge_geometry,{},Camera{},~0u,false,true,&faces);
    require(mean(reflection_edge,0,30,32)>160 && mean(reflection_edge,1,30,32)<30 &&
        mean(reflection_edge,1,32,34)>160 && mean(reflection_edge,0,32,34)<30,
        "Filtered specular lighting reversed cube directions or blurred across a same-material normal edge");

    // A one-pixel silhouette missing every lighting-grid representative must
    // still receive its own bounded trace rather than becoming an empty hole.
    std::vector<Vertex> thin;quad(thin,.009f,-1,.025f,1,.5f,{0,0,-1},0);
    const auto thin_scene=pt::build_scene({},filter_materials);
    const auto thin_frame=gpu.draw(thin_scene,thin,{.25f,.25f,.25f},Camera{},~0u,false,true);
    require(mean(thin_frame,0,32,33,8,56)>60,"Lighting-grid rejection left an unshaded thin silhouette");
    // Independent frame seeds converge in the demodulated HDR history with
    // exactly the same1/1 ray budget and fixed allocated storage.
    gpu.primary_color({.1f,.1f,.1f,1});
    const auto temporal_first=gpu.draw(noisy_scene,primary,{},Camera{},~0u,false,true);
    const auto temporal_bytes=gpu.renderer->allocated_bytes();
    auto temporal_final=temporal_first;
    for(unsigned frame=1;frame<32;++frame)
        temporal_final=gpu.draw(noisy_scene,primary,{},Camera{},~0u,false,true);
    require(deviation(temporal_final,0)<deviation(temporal_first,0)*.6f &&
        std::abs(mean(temporal_final,0,8,56,8,56)-mean(reference,0,8,56,8,56))<8,
        "Fixed-budget temporal HDR accumulation did not converge without losing light energy");
    require(gpu.renderer->allocated_bytes()==temporal_bytes,"Temporal history storage grew with frame count");
    gpu.primary_color({1,1,1,1});
    std::vector<Material> history_materials(2);for(auto& m:history_materials) {
        m=white_material();m.specular_brightness=1;
    }
    auto history_scene=pt::build_scene({},history_materials);
    std::array<Vec3,6> red_faces,blue_faces;red_faces.fill({1,0,0});blue_faces.fill({0,0,1});
    const auto warm_red=[&](std::span<const Vertex> bank,const Camera& camera=Camera{}) {
        for(unsigned frame=0;frame<8;++frame)
            gpu.draw(history_scene,bank,{},camera,~0u,false,true,&red_faces);
    };
    const auto current_blue=[&](std::span<const Vertex> bank,const Camera& camera=Camera{}) {
        return gpu.draw(history_scene,bank,{},camera,~0u,false,true,&blue_faces);
    };
    const auto blue_only=[&](const auto& frame,const char* message) {
        require(mean(frame,2)>240 && mean(frame,0)<3,message);
    };
    warm_red(primary);auto moved_primary=primary;
    for(auto& vertex:moved_primary)vertex.position.z+=.25f;
    blue_only(current_blue(moved_primary),"Temporal history ghosted a surface at its previous world position");
    warm_red(primary);auto changed_normal=primary;
    for(auto& vertex:changed_normal)vertex.normal={.8f,0,-.6f};
    blue_only(current_blue(changed_normal),"Temporal history crossed a changed surface normal");
    warm_red(primary);auto changed_material=primary;
    for(auto& vertex:changed_material)vertex.material=1;
    blue_only(current_blue(changed_material),"Temporal history crossed a changed material binding");
    warm_red(primary);Camera changed_view;changed_view.eye[0]=2;
    blue_only(current_blue(primary,changed_view),"Temporal reflection history retained a changed view direction");
    const std::array matte_history_materials{white_material()};
    auto matte_history_scene=pt::build_scene({},matte_history_materials);
    for(unsigned frame=0;frame<8;++frame)
        gpu.draw(matte_history_scene,primary,{},Camera{},~0u,false,true,&red_faces);
    const auto matte_view=gpu.draw(matte_history_scene,primary,{},changed_view,
        ~0u,false,true,&red_faces,true,true);
    const auto matte_history=gpu.renderer->temporal_stats();
    require(mean(matte_view,0)>240 && mean(matte_view,2)<3 &&
            matte_history.banks[0].mean_count>8.8 && matte_history.banks[0].maximum_count==9,
        "Changed view direction discarded valid view-independent matte lighting history");
    warm_red(primary);gpu.renderer->set_quality(2,2);
    blue_only(current_blue(primary),"Changing the ray budget retained stale temporal lighting");
    gpu.renderer->set_quality(1,1);
    warm_red(primary);gpu.renderer->prepare_visibility({32,32});
    blue_only(current_blue(primary),"Resizing visibility retained stale temporal lighting");
    warm_red(primary);auto replacement_source=history_scene;
    blue_only(gpu.draw(replacement_source,primary,{},Camera{},~0u,false,true,&blue_faces),
        "Replacing the source Scene retained stale temporal lighting");
    warm_red(primary);gpu.draw(history_scene,primary,{},Camera{},~0u,false,false,&blue_faces);
    blue_only(current_blue(primary),"A missing lighting frame reused nonconsecutive history");

    // Both camera phases share one upload serial, but their history banks
    // must never contribute to each other even at identical pixel positions.
    auto phase_scene=pt::build_scene({},history_materials);Camera foreground_camera;foreground_camera.eye[3]=2;
    for(unsigned frame=0;frame<8;++frame) {
        const auto world_frame=gpu.draw(phase_scene,primary,{},Camera{},~0u,false,true,&red_faces);
        const auto foreground_frame=gpu.draw(phase_scene,primary,{},foreground_camera,~0u,false,true,&blue_faces,false);
        require(mean(world_frame,0)>240 && mean(world_frame,2)<3 &&
            mean(foreground_frame,2)>240 && mean(foreground_frame,0)<3,
            "World and foreground phases shared a temporal lighting history");
    }
    auto changing_light=bright_sky;changing_light.color={1,0,0};
    auto changing_lights=pt::build_scene({},history_materials,std::span(&changing_light,1));
    for(unsigned frame=0;frame<8;++frame)gpu.draw(changing_lights,primary,{},Camera{},~0u,false,true);
    changing_light.color={0,0,1};pt::update_geometry(changing_lights,{},std::span(&changing_light,1));
    blue_only(gpu.draw(changing_lights,primary,{},Camera{},~0u,false,true),
        "Changed authored light energy retained stale temporal illumination");
    progress("HDR and temporal fixtures passed; starting native material fixtures");
    // Actual qualified Coagulation terrain combines four stages before RT
    // lighting. Dynamic native color constants stay in the real per-draw UBO.
    auto terrain_material=native_surface(true);
    const std::array<std::array<std::uint8_t,4>,4> terrain_pixels{{
        {100,120,80,64},{40,110,30,255},{130,60,25,255},{120,100,140,255}}};
    terrain_material.diffuse=solid_texture(terrain_pixels[0]);terrain_material.detail=solid_texture(terrain_pixels[1]);
    terrain_material.bump=solid_texture(terrain_pixels[2]);terrain_material.auxiliary=solid_texture(terrain_pixels[3]);
    terrain_material.forward_program.words[43]=0xffc09966;terrain_material.forward_program.words[44]=0x00030507;
    native_uniform(gpu,terrain_material);
    for(unsigned stage=0;stage<4;++stage) {
        ShaderVector color{};for(unsigned c=0;c<4;++c)color[c]=float(terrain_pixels[stage][c])/255.f;
        gpu.primary_color(color,stage);
    }
    auto source_model_primary=primary;for(auto& vertex:source_model_primary)vertex.color={0,0,0};
    auto terrain_scene=pt::build_scene(source_model_primary,std::span(&terrain_material,1));
    require((terrain_scene.materials[0].texture[3]&(pt::surface_flag|128u))==(pt::surface_flag|128u) &&
        std::bit_cast<std::uint32_t>(terrain_scene.materials[0].base[3])!=0,
        "Terrain fixture failed to admit its resident native surface and source-radiance policy");
    const auto terrain_frame=gpu.draw(terrain_scene,source_model_primary,{.25f,.25f,.25f});
    std::array<ShaderVector,4> terrain_samples{};
    for(unsigned stage=0;stage<4;++stage)for(unsigned c=0;c<4;++c)
        terrain_samples[stage][c]=float(terrain_pixels[stage][c])/255.f;
    auto terrain_expected=pt::surface_color(pt::SurfaceFamily::TerrainBlend,
        {terrain_material.forward_program.words[13],terrain_material.forward_program.words[21],
         terrain_material.forward_program.words[43],terrain_material.forward_program.words[44]},terrain_samples);
    for(unsigned c=0;c<3;++c)require(std::abs(mean(terrain_frame,c)-terrain_expected[c]*63.75f)<2,
        "Native terrain lost authored blend/detail, color constants, or treated source model radiance as tint");
    terrain_material.forward_program.words[43]=0xff40e080;terrain_material.forward_program.words[44]=0x000b0201;
    native_uniform(gpu,terrain_material);
    const auto changed_terrain=gpu.draw(terrain_scene,source_model_primary,{.25f,.25f,.25f});
    terrain_expected=pt::surface_color(pt::SurfaceFamily::TerrainBlend,
        {0,0,terrain_material.forward_program.words[43],terrain_material.forward_program.words[44]},terrain_samples);
    for(unsigned c=0;c<3;++c)require(std::abs(mean(changed_terrain,c)-terrain_expected[c]*63.75f)<2,
        "Primary native terrain read stale atlas color constants instead of its actual draw bank");
    // Foliage LESS255 compares native mux alpha: raw alpha0 is a hole;
    // raw alpha255 writes output0 and survives. Test all three raster roles.
    auto leaf_material=native_surface(false);leaf_material.detail=solid_texture({128,128,128,255});
    leaf_material.diffuse={2,1,11,{std::byte{32},std::byte{180},std::byte{64},std::byte{0},
        std::byte{32},std::byte{180},std::byte{64},std::byte{255}}};
    std::vector<std::uint8_t> leaf_pixels(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
        const unsigned at=(y*width+x)*4;leaf_pixels[at]=32;leaf_pixels[at+1]=180;
        leaf_pixels[at+2]=64;leaf_pixels[at+3]=x<width/2?0:255;
    }
    gpu.primary_texture(leaf_pixels);gpu.primary_color({128.f/255,128.f/255,128.f/255,1},1);
    native_uniform(gpu,leaf_material);const auto leaf_scene=pt::build_scene(source_model_primary,std::span(&leaf_material,1));
    require((leaf_scene.materials[0].texture[3]&pt::surface_flag)!=0 &&
        std::bit_cast<std::uint32_t>(leaf_scene.materials[0].base[3])!=0,
        "Foliage fixture failed to admit its resident native surface");
    const auto leaf_frame=gpu.draw(leaf_scene,source_model_primary,{.25f,.25f,.25f});
    require(mean(leaf_frame,1,8,24)<1 && mean(leaf_frame,1,40,56)>40,
        "Native foliage output-alpha mux inverted leaf holes and opaque coverage");
    require(gpu.draw(leaf_scene,source_model_primary,{.25f,.25f,.25f},Camera{},~0u,true)==leaf_frame,
        "Foliage native alpha disagreed between the source-W prepass and shading");
    const auto filtered_leaf=gpu.draw(leaf_scene,source_model_primary,{.25f,.25f,.25f},Camera{},~0u,false,true);
    require(mean(filtered_leaf,1,8,24)<1 && mean(filtered_leaf,1,40,56)>40,
        "Filtered lighting filled native foliage holes or discarded opaque leaf pixels");
    auto per_vertex_leaf=leaf_material;
    per_vertex_leaf.vertex_program=vertex_bytes(pt::unlit_uv_detail::pervertex_detail_words);
    per_vertex_leaf.passes[0].vertex_code=per_vertex_leaf.vertex_program;
    per_vertex_leaf.passes[0].vertex_shader="rasterizer\\vertex_shaders_xbox\\lightmap_pervertex_tex_detail";
    per_vertex_leaf.forward_program.words=pt::surface_detail::leaf_per_vertex;
    native_uniform(gpu,per_vertex_leaf);
    const auto per_vertex_leaf_scene=pt::build_scene(source_model_primary,std::span(&per_vertex_leaf,1));
    require((per_vertex_leaf_scene.materials[0].texture[3]&pt::surface_flag)!=0,
        "Per-vertex foliage fixture failed to admit its resident native surface");
    require(gpu.draw(per_vertex_leaf_scene,source_model_primary,{.25f,.25f,.25f},Camera{},~0u,true)==leaf_frame,
        "Per-vertex native foliage used the per-pixel variant's reversed alpha constant mapping");
    // The bump variants select coverage from normal-map alpha, even where
    // base alpha is fully opaque. Their strict source LESS255 qualification
    // preserves coverage while removing the baked-light factor from RGBA.
    gpu.primary_color({32.f/255,180.f/255,64.f/255,1});
    std::vector<std::uint8_t> normal_pixels(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
        const unsigned at=(y*width+x)*4;normal_pixels[at]=normal_pixels[at+1]=128;
        normal_pixels[at+2]=255;normal_pixels[at+3]=x<width/2?0:255;
    }
    gpu.primary_texture(normal_pixels,2);
    for(bool per_vertex:{false,true}) {
        auto bump_leaf=per_vertex?per_vertex_leaf:leaf_material;
        bump_leaf.passes[0].name="shaders\\shader_passes\\lightmap\\lightmap_tex_detail_alpha_test";
        bump_leaf.forward_program.words=per_vertex?pt::surface_detail::bump_leaf_per_vertex:pt::surface_detail::bump_leaf;
        bump_leaf.diffuse=solid_texture({32,180,64,255});
        bump_leaf.bump={2,1,11,{std::byte{128},std::byte{128},std::byte{255},std::byte{0},
            std::byte{128},std::byte{128},std::byte{255},std::byte{255}}};
        const auto model=pt::qualified_surface(bump_leaf);
        require(model && model->family==(per_vertex?pt::SurfaceFamily::BumpLeafCoveragePerVertex:
            pt::SurfaceFamily::BumpLeafCoverage),"Bump foliage fixture did not qualify its exact native source owner");
        native_uniform(gpu,bump_leaf);const auto bump_scene=pt::build_scene(source_model_primary,std::span(&bump_leaf,1));
        require((bump_scene.materials[0].texture[3]&pt::surface_flag)!=0,
            "Bump foliage fixture failed resident material admission");
        const auto bump_frame=gpu.draw(bump_scene,source_model_primary,{.25f,.25f,.25f});
        require(mean(bump_frame,1,8,24)<1 && mean(bump_frame,1,40,56)>40,
            "Bump foliage ignored normal-map alpha and made opaque leaf cards");
        require(gpu.draw(bump_scene,source_model_primary,{.25f,.25f,.25f},Camera{},~0u,true)==bump_frame,
            "Bump foliage source-W prepass disagreed with qualified native coverage");
    }
    // Secondary rays use the same native alpha, including exact raw-stage
    // kills. Keep the primary receiver's texture white independently.
    for(unsigned stage=0;stage<4;++stage)gpu.primary_color({1,1,1,1},stage);
    std::memset(gpu.uniform.mapped,0,sizeof(WorldMaterialUniform));
    std::array<Material,2> leaf_shadow_materials{white_material(),leaf_material};
    auto leaf_geometry=primary;quad(leaf_geometry,-.8f,-.8f,.8f,.8f,0,{0,0,-1},1);
    for(std::size_t n=primary.size();n<leaf_geometry.size();++n)leaf_geometry[n].uv={.25f,.5f};
    auto leaf_shadow_scene=pt::build_scene(leaf_geometry,leaf_shadow_materials,std::span(&light,1));
    const auto leaf_hole_shadow=gpu.draw(leaf_shadow_scene,primary);
    for(std::size_t n=primary.size();n<leaf_geometry.size();++n)leaf_geometry[n].uv={.75f,.5f};
    pt::update_geometry(leaf_shadow_scene,leaf_geometry,std::span(&light,1));
    const auto leaf_opaque_shadow=gpu.draw(leaf_shadow_scene,primary);
    require(mean(leaf_hole_shadow,0)>mean(leaf_opaque_shadow,0)+20,
        "Secondary ray alpha inverted the native foliage hole and opaque leaf mask");
    progress("native terrain and foliage fixtures passed; starting colored sky GI");
    // A sky-lit matte wall must transfer its own color at depth1. Terminal
    // environment NEE supplies that illumination without a second bounce or
    // extra per-surface shadow query. Identical black/blue walls are controls.
    gpu.renderer->set_quality(1,1);
    std::array<Material,2> canyon_materials{white_material(),white_material()};
    std::vector<Vertex> canyon_geometry=primary;
    for(const auto p:std::array<Vec3,6>{{{.65f,-3,-3},{.65f,3,-3},{.65f,-3,.5f},
        {.65f,-3,.5f},{.65f,3,-3},{.65f,3,.5f}}}) {
        Vertex vertex;vertex.position=p;vertex.normal={-1,0,0};vertex.color={1,1,1};
        vertex.material=1;canyon_geometry.push_back(vertex);
    }
    canyon_materials[1].diffuse=solid_texture({0,0,0,255});
    const auto black_canyon=pt::build_scene(canyon_geometry,canyon_materials);
    const auto black_canyon_frame=gpu.draw(black_canyon,primary,{.5f,.5f,.5f});
    canyon_materials[1].diffuse=solid_texture({255,8,4,255});
    const auto red_canyon=pt::build_scene(canyon_geometry,canyon_materials);
    const auto red_canyon_frame=gpu.draw(red_canyon,primary,{.5f,.5f,.5f});
    canyon_materials[1].diffuse=solid_texture({4,8,255,255});
    const auto blue_canyon=pt::build_scene(canyon_geometry,canyon_materials);
    const auto blue_canyon_frame=gpu.draw(blue_canyon,primary,{.5f,.5f,.5f});
    require(mean(red_canyon_frame,0)>mean(black_canyon_frame,0)+3 &&
            mean(red_canyon_frame,0)>mean(red_canyon_frame,2)+3,
        "At1bounce a sky-lit red canyon failed to bounce red illumination onto a white receiver");
    require(mean(blue_canyon_frame,2)>mean(black_canyon_frame,2)+3 &&
            mean(blue_canyon_frame,2)>mean(blue_canyon_frame,0)+3,
        "Canyon GI used a global warm tint instead of the actual wall albedo");
    progress("colored sky GI passed; checking renderer release and recreation");
    gpu.primary_color({1,1,1,1});
    require(gpu.renderer->allocated_bytes()>0,"Active Path Tracing renderer reported no GPU allocations");
    gpu.renderer.reset();
    gpu.renderer=std::make_unique<pt::Render>(pt::RenderContext{gpu.device,gpu.physical,gpu.pass,gpu.material_layout,argv[1]});
    require(gpu.renderer->allocated_bytes()==0,"Recreated Path Tracing renderer retained old scene allocations");
    const auto reenabled=gpu.draw(visor,primary);
    require(reenabled==visor_frame,"Destroying and rebuilding Path Tracing changed the rendered scene");
    require(gpu.validation_errors.load()==0,"Vulkan validation reported errors in Path Tracing draws");
    std::cout<<"PASS Path Tracing Vulkan GPU: device="<<gpu.device_name
        <<" direct="<<mean(lit,0)<<" shadow="<<mean(shadow,0)
        <<" bounce="<<mean(bounced,0)<<" GI="<<mean(gi,0)<<" diffuse="<<mean(diffuse_frame,0)
        <<" visor="<<mean(visor_frame,0)<<" rough="<<mean(rough_frame,0)
        <<" sky="<<mean(directional_lit,0)<<" sky_shadow="<<mean(directional_shadow,0)
        <<" HDR_noise="<<deviation(noisy,0)<<" filtered_noise="<<deviation(denoised,0)
        <<" temporal_initial="<<deviation(temporal_first,0)<<" temporal_final="<<deviation(temporal_final,0)
        <<" validation_errors="<<gpu.validation_errors.load()<<'\n';
    return 0;
}catch(const Skip& reason){std::cout<<"SKIP "<<reason.what()<<'\n';return 77;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
