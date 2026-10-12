#pragma once
#include "menu_world_bitmap_import_support.hpp"
#include "halo2/frontend_material.hpp"
#include <vulkan/vulkan.h>
#include <atomic>
#include <bit>
#include <cmath>
#include <iomanip>
#include <limits>

// Actual Cache storage passes through the same production frontend preparation
// as live materials. Original RGBA8 images are independent expectations. The
// optional raw mode observes unchanged hardware BC as a diagnostic only.
namespace menu_bitmap_gpu {
using namespace menu_world_bitmap_import;
inline std::atomic<unsigned> validation_errors{},cleanup_errors{};
inline void check(VkResult value) {require(value==VK_SUCCESS,"bitmap decode Vulkan error "+std::to_string(value));}
inline VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void *) {
  if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<data->pMessage<<'\n';}
  return VK_FALSE;
}
inline VkFormat native_format(unsigned source) {
  if(source==11)return VK_FORMAT_R8G8B8A8_UNORM;
  if(source==14)return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
  if(source==15)return VK_FORMAT_BC2_UNORM_BLOCK;
  require(source==16,"unadmitted native texture storage format");return VK_FORMAT_BC3_UNORM_BLOCK;
}
inline const char *format_name(VkFormat format) {
  if(format==VK_FORMAT_R8G8B8A8_UNORM)return "R8G8B8A8_UNORM";
  if(format==VK_FORMAT_BC1_RGBA_UNORM_BLOCK)return "BC1_RGBA_UNORM_BLOCK";
  if(format==VK_FORMAT_BC2_UNORM_BLOCK)return "BC2_UNORM_BLOCK";
  if(format==VK_FORMAT_BC3_UNORM_BLOCK)return "BC3_UNORM_BLOCK";
  return "unsupported";
}
struct Pair {std::array<std::uint32_t,4> native,original;};
static_assert(sizeof(Pair)==32);
struct Capability {VkFormat format{};VkFormatFeatureFlags optimal{};bool image_supported{};};
class Gpu {
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};VkDeviceSize size{};};
  struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};};
  VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};
  VkDevice device{};VkQueue queue{};VkCommandPool pool{};VkCommandBuffer command{};
  VkDescriptorSetLayout set_layout{};VkDescriptorPool descriptors{};VkDescriptorSet set{};
  VkPipelineLayout pipeline_layout{};VkPipeline pipeline{};VkShaderModule shader{};VkSampler sampler{};
  Buffer staging{},output{};Image native{},original{};
  unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((bits&(1u<<i)) &&
        (properties.memoryTypes[i].propertyFlags&flags)==flags)return i;
    throw FormatError("bitmap decode required memory type unavailable");
  }
  void allocate(Buffer &buffer,VkDeviceSize size,VkBufferUsageFlags usage) {
    buffer.size=size;VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=usage;
    check(vkCreateBuffer(device,&info,nullptr,&buffer.handle));VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device,buffer.handle,&requirements);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(device,&allocation,nullptr,&buffer.memory));check(vkBindBufferMemory(device,buffer.handle,buffer.memory,0));
    check(vkMapMemory(device,buffer.memory,0,size,0,&buffer.mapped));
  }
  void destroy(Buffer &buffer) {
    if(buffer.mapped)vkUnmapMemory(device,buffer.memory);
    if(buffer.handle)vkDestroyBuffer(device,buffer.handle,nullptr);
    if(buffer.memory)vkFreeMemory(device,buffer.memory,nullptr);buffer={};
  }
  void destroy(Image &image) {
    if(image.view)vkDestroyImageView(device,image.view,nullptr);
    if(image.handle)vkDestroyImage(device,image.handle,nullptr);
    if(image.memory)vkFreeMemory(device,image.memory,nullptr);image={};
  }
  void image(Image &target,VkFormat format,unsigned width,unsigned height,unsigned layers,unsigned mips) {
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=format;
    info.extent={width,height,1};info.mipLevels=mips;info.arrayLayers=layers;info.samples=VK_SAMPLE_COUNT_1_BIT;
    info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    info.flags=layers==6?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
    check(vkCreateImage(device,&info,nullptr,&target.handle));VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device,target.handle,&requirements);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&allocation,nullptr,&target.memory));check(vkBindImageMemory(device,target.handle,target.memory,0));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=target.handle;view.viewType=VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    view.format=format;view.components={VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY};
    view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,mips,0,layers};check(vkCreateImageView(device,&view,nullptr,&target.view));
  }
  bool supported(VkPhysicalDevice candidate,std::span<const ImportedStage> textures) {
    require(!textures.empty(),"empty GPU bitmap probe");
    capabilities.clear();texture_compression_bc=false;
    VkPhysicalDeviceFeatures features;vkGetPhysicalDeviceFeatures(candidate,&features);texture_compression_bc=features.textureCompressionBC!=0;
    const bool needs_bc=std::any_of(textures.begin(),textures.end(),[](const auto &t){return t.texture.format>=14 && t.texture.format<=16;});
    if(needs_bc && !features.textureCompressionBC){unavailable_reason="textureCompressionBC feature unavailable for raw diagnostic";return false;}
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(candidate,&properties);
    if(properties.apiVersion<VK_API_VERSION_1_3){unavailable_reason="Vulkan1.3 unavailable";return false;}
    std::vector<Capability> tested;
    for(unsigned i=0;i<textures.size()*2;++i) {
      // Check every actual image shape and the independent RGBA expectation.
      const auto &t=textures[i%textures.size()].texture;const auto f=i<textures.size()?native_format(t.format):VK_FORMAT_R8G8B8A8_UNORM;
      VkFormatProperties format;vkGetPhysicalDeviceFormatProperties(candidate,f,&format);
      VkImageFormatProperties image_properties{};
      const auto result=vkGetPhysicalDeviceImageFormatProperties(candidate,f,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,
          VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,t.layers==6?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0,&image_properties);
      const unsigned levels=unsigned(t.mips.size()/t.layers);
      const bool valid=result==VK_SUCCESS && image_properties.maxExtent.width>=t.width && image_properties.maxExtent.height>=t.height &&
          image_properties.maxMipLevels>=levels && image_properties.maxArrayLayers>=t.layers;
      tested.push_back({f,format.optimalTilingFeatures,valid});capabilities=tested;
      const VkFormatFeatureFlags required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
      if(!valid || (format.optimalTilingFeatures&required)!=required) {
        capabilities=tested;unavailable_reason=std::string("native image capability unavailable: ")+format_name(f);return false;
      }
      std::size_t texels{};for(const auto &m:t.mips)texels+=std::size_t(m.width)*m.height;
      if(texels*sizeof(Pair)>properties.limits.maxStorageBufferRange){unavailable_reason="decode output exceeds maxStorageBufferRange";return false;}
    }
    capabilities=tested;return true;
  }
public:
  std::string device_name,unavailable_reason;bool texture_compression_bc{},compression_feature_enabled{};std::vector<Capability> capabilities;
  ~Gpu() {
    if(device) {
      if(vkDeviceWaitIdle(device)!=VK_SUCCESS){++cleanup_errors;std::cerr<<"bitmap decode wait failed during cleanup\n";}
      if(pool)vkDestroyCommandPool(device,pool,nullptr);
      if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
      if(shader)vkDestroyShaderModule(device,shader,nullptr);
      if(pipeline_layout)vkDestroyPipelineLayout(device,pipeline_layout,nullptr);
      if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
      if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);
      if(sampler)vkDestroySampler(device,sampler,nullptr);
      destroy(native);destroy(original);destroy(staging);destroy(output);vkDestroyDevice(device,nullptr);
    }
    if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
    if(instance)vkDestroyInstance(instance,nullptr);
  }
  bool initialize(const fs::path &shader_path,std::span<const ImportedStage> textures) {
    unsigned count{};check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);
    check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
    if(std::none_of(layers.begin(),layers.end(),[](const auto &l){return std::string(l.layerName)=="VK_LAYER_KHRONOS_validation";})) {
      unavailable_reason="Khronos validation layer unavailable";return false;
    }
    const char *layer="VK_LAYER_KHRONOS_validation";
    const char *extensions[]{VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback=diagnostic;
    VkValidationFeatureEnableEXT synchronization=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validation{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};validation.enabledValidationFeatureCount=1;validation.pEnabledValidationFeatures=&synchronization;validation.pNext=&debug;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&app;info.pNext=&validation;
    info.enabledLayerCount=1;info.ppEnabledLayerNames=&layer;info.enabledExtensionCount=2;info.ppEnabledExtensionNames=extensions;
    check(vkCreateInstance(&info,nullptr,&instance));
    check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
    check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
    unsigned family{};
    for(auto candidate:devices) {
      VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(candidate,&properties);device_name=properties.deviceName;
      if(!supported(candidate,textures))continue;
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,queues.data());
      for(unsigned i=0;i<count;++i)if(queues[i].queueFlags&VK_QUEUE_COMPUTE_BIT){physical=candidate;family=i;break;}
      if(physical)break;unavailable_reason="compute queue unavailable";
    }
    if(!physical){if(unavailable_reason.empty())unavailable_reason="no physical device";return false;}
    unavailable_reason.clear();const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;VkPhysicalDeviceFeatures enabled{};compression_feature_enabled=std::any_of(textures.begin(),textures.end(),[](const auto &t){return t.texture.format>=14 && t.texture.format<=16;});
    enabled.textureCompressionBC=compression_feature_enabled?VK_TRUE:VK_FALSE;
    VkDeviceCreateInfo create{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};create.queueCreateInfoCount=1;create.pQueueCreateInfos=&q;create.pEnabledFeatures=&enabled;
    check(vkCreateDevice(physical,&create,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    const std::array<VkDescriptorSetLayoutBinding,3> bindings{{
      {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},
      {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},
      {2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}}};
    VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set_info.bindingCount=bindings.size();set_info.pBindings=bindings.data();
    check(vkCreateDescriptorSetLayout(device,&set_info,nullptr,&set_layout));
    const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};
    VkDescriptorPoolCreateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};descriptor_info.maxSets=1;descriptor_info.poolSizeCount=2;descriptor_info.pPoolSizes=sizes;
    check(vkCreateDescriptorPool(device,&descriptor_info,nullptr,&descriptors));
    VkDescriptorSetAllocateInfo allocate_set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate_set.descriptorPool=descriptors;allocate_set.descriptorSetCount=1;allocate_set.pSetLayouts=&set_layout;
    check(vkAllocateDescriptorSets(device,&allocate_set,&set));
    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,20};VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount=1;layout.pSetLayouts=&set_layout;layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&push;
    check(vkCreatePipelineLayout(device,&layout,nullptr,&pipeline_layout));
    std::ifstream file(shader_path,std::ios::binary|std::ios::ate);require(bool(file),"compiled bitmap decode shader missing");const auto bytes=file.tellg();
    require(bytes>=20 && bytes<1024*1024 && bytes%4==0,"invalid bitmap decode SPIR-V extent");std::vector<std::uint32_t> code(static_cast<std::size_t>(bytes)/4);file.seekg(0);
    require(bool(file.read(reinterpret_cast<char *>(code.data()),bytes)) && code[0]==0x07230203,"invalid bitmap decode SPIR-V");
    VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};module.codeSize=static_cast<std::size_t>(bytes);module.pCode=code.data();
    check(vkCreateShaderModule(device,&module,nullptr,&shader));VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.layout=pipeline_layout;
    p.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};
    check(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&pipeline));
    VkSamplerCreateInfo sample{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sample.magFilter=sample.minFilter=VK_FILTER_NEAREST;sample.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sample.addressModeU=sample.addressModeV=sample.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;sample.maxLod=VK_LOD_CLAMP_NONE;
    check(vkCreateSampler(device,&sample,nullptr,&sampler));
    VkCommandPoolCreateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};command_info.queueFamilyIndex=family;command_info.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device,&command_info,nullptr,&pool));VkCommandBufferAllocateInfo commands{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commands.commandPool=pool;commands.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;commands.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&commands,&command));return true;
  }
  std::vector<Pair> run(const Texture &texture,const Stage &expected) {
    destroy(native);destroy(original);destroy(staging);destroy(output);
    const unsigned levels=unsigned(texture.mips.size()/texture.layers);std::size_t texels{},original_bytes{};
    for(const auto &m:expected.expected){texels+=std::size_t(m.width)*m.height;original_bytes+=m.sampled.size();}
    require(texture.pixels.size()%4==0,"native bitmap staging offset is unaligned");
    allocate(staging,texture.pixels.size()+original_bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);allocate(output,texels*sizeof(Pair),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    std::memcpy(staging.mapped,texture.pixels.data(),texture.pixels.size());std::memset(output.mapped,0xcd,texels*sizeof(Pair));
    image(native,native_format(texture.format),texture.width,texture.height,texture.layers,levels);
    image(original,VK_FORMAT_R8G8B8A8_UNORM,texture.width,texture.height,texture.layers,levels);
    std::vector<VkBufferImageCopy> native_copies,original_copies;std::size_t original_offset=texture.pixels.size();
    for(unsigned i=0;i<texture.mips.size();++i) {
      const auto &m=texture.mips[i];const auto &e=expected.expected[i];require(m.depth==1,"native 2D/cube mip has unsupported depth");require(m.offset<=texture.pixels.size() && m.size<=texture.pixels.size()-m.offset,"native mip payload escapes storage");
      const unsigned alignment=texture.format==14?8:((texture.format==15 || texture.format==16)?16:4);require(m.offset%alignment==0,"native mip offset violates actual block alignment");
      VkBufferImageCopy copy{};copy.bufferOffset=m.offset;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i%levels,i/levels,1};copy.imageExtent={m.width,m.height,1};native_copies.push_back(copy);
      copy.bufferOffset=original_offset;original_copies.push_back(copy);std::memcpy(static_cast<std::byte *>(staging.mapped)+original_offset,e.sampled.data(),e.sampled.size());original_offset+=e.sampled.size();
    }
    const VkDescriptorImageInfo images[]{{sampler,native.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampler,original.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    const VkDescriptorBufferInfo buffer{output.handle,0,output.size};VkWriteDescriptorSet writes[3]{};
    for(unsigned i=0;i<3;++i){writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[i].dstSet=set;writes[i].dstBinding=i;writes[i].descriptorCount=1;
      writes[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      if(i==2)writes[i].pBufferInfo=&buffer;else writes[i].pImageInfo=&images[i];}
    vkUpdateDescriptorSets(device,3,writes,0,nullptr);check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
    std::array<VkImageMemoryBarrier,2> before{};for(unsigned i=0;i<2;++i){auto &b=before[i];b={VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
      b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
      b.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.image=i?original.handle:native.handle;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,levels,0,texture.layers};}
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,before.data());
    vkCmdCopyBufferToImage(command,staging.handle,native.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,native_copies.size(),native_copies.data());
    vkCmdCopyBufferToImage(command,staging.handle,original.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,original_copies.size(),original_copies.data());
    for(auto &b:before){b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;}
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,2,before.data());
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_layout,0,1,&set,0,nullptr);
    unsigned base{};for(unsigned level=0;level<levels;++level) {
      if(level) {
        // Disjoint shader indices are explicit in GLSL, but synchronization
        // validation correctly treats the storage descriptor as one resource.
        VkMemoryBarrier next{VK_STRUCTURE_TYPE_MEMORY_BARRIER};next.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;next.dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&next,0,nullptr,0,nullptr);
      }
      const auto &m=texture.mips[level];const std::array<unsigned,5> push{m.width,m.height,texture.layers,level,base};
      vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push.data());vkCmdDispatch(command,(m.width+7)/8,(m.height+7)/8,texture.layers);
      base+=m.width*m.height*texture.layers;
    }
    require(base==texels,"native decoded result coverage changed");VkMemoryBarrier result{VK_STRUCTURE_TYPE_MEMORY_BARRIER};result.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;result.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&result,0,nullptr,0,nullptr);check(vkEndCommandBuffer(command));
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));
    std::vector<Pair> values(texels);std::memcpy(values.data(),output.mapped,texels*sizeof(Pair));return values;
  }
};
} // namespace menu_bitmap_gpu
