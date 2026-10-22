#include "halo2/source_weather_depth_alias.hpp"
#include <array>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

namespace halo2 {
namespace {
void check(VkResult r) {
    if(r!=VK_SUCCESS) throw FormatError("weather depth alias Vulkan failure: "+std::to_string(r));
}
struct Scope { VkPipelineStageFlags stage; VkAccessFlags access; };
Scope source_scope(VkImageLayout layout) {
    switch(layout) {
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_SHADER_READ_BIT};
    case VK_IMAGE_LAYOUT_GENERAL:
        return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT};
    default: throw FormatError("weather depth alias requires an initialized, supported combined depth/stencil layout");
    }
}
void image_barrier(VkCommandBuffer command,VkImage image,VkImageAspectFlags aspects,
                   VkImageLayout from,VkImageLayout to,Scope before,Scope after) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.srcAccessMask=before.access;b.dstAccessMask=after.access;b.oldLayout=from;b.newLayout=to;
    b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    b.image=image;b.subresourceRange={aspects,0,1,0,1};
    vkCmdPipelineBarrier(command,before.stage,after.stage,0,0,nullptr,0,nullptr,1,&b);
}
unsigned encoding_index(const SourceWeatherDepthAliasImage &image) {
    if(image.depth_encoding==SourceDepthStorageEncoding::native_d24 && image.format==VK_FORMAT_D24_UNORM_S8_UINT)return 0;
    if(image.depth_encoding==SourceDepthStorageEncoding::d32_integer_d24 && image.format==VK_FORMAT_D32_SFLOAT_S8_UINT)return 1;
    throw FormatError("weather depth alias physical format and source encoding disagree");
}
}
SourceWeatherDepthAliasStorage source_weather_depth_alias_storage(VkExtent2D extent) {
    const auto n=std::uint64_t(extent.width)*extent.height;
    if(!n || n>std::numeric_limits<std::uint32_t>::max())
        throw FormatError("weather depth alias requires nonempty extents with a uint32 pixel index");
    return {std::uint32_t(n),n*4,(n+3)&~std::uint64_t(3)};
}

struct SourceWeatherDepthAlias::Impl {
    SourceWeatherDepthAliasContext context;
    VkPhysicalDeviceProperties properties{};
    VkDescriptorSetLayout descriptor_layout{};
    VkPipelineLayout pipeline_layout{};
    VkShaderModule shader{};
    std::array<VkPipeline,2> pipeline{};
    std::array<VkShaderModule,2> color_shader{};
    std::array<VkPipeline,2> color_pipeline{};
    VkSampler sampler{};
    VkFence pending{};
    VkCommandBuffer pending_command{};
    bool previous_recorded{};
    bool color_open{};
    SourceWeatherDepthAliasImage color_source{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};VkDeviceSize size{};};
    struct Resources {
        VkDevice device{};
        VkExtent2D extent{};
        std::array<Buffer,2> buffers{};
        Buffer validation{};void *validation_mapped{};
        VkImage image{};VkDeviceMemory memory{};
        VkImageView sampled{},alternate_sampled{},storage{};
        VkDescriptorPool pool{};VkDescriptorSet descriptor{};
        bool recorded{};
        ~Resources() {
            if(validation_mapped)vkUnmapMemory(device,validation.memory);
            if(validation.handle)vkDestroyBuffer(device,validation.handle,nullptr);
            if(validation.memory)vkFreeMemory(device,validation.memory,nullptr);
            if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
            if(storage)vkDestroyImageView(device,storage,nullptr);
            if(alternate_sampled)vkDestroyImageView(device,alternate_sampled,nullptr);
            if(sampled)vkDestroyImageView(device,sampled,nullptr);
            if(image)vkDestroyImage(device,image,nullptr);
            if(memory)vkFreeMemory(device,memory,nullptr);
            for(auto b:buffers){if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);if(b.memory)vkFreeMemory(device,b.memory,nullptr);}
        }
    };
    std::unique_ptr<Resources> resources;
    explicit Impl(SourceWeatherDepthAliasContext c):context(std::move(c)) {
        try {
            if(!context.physical||!context.device)throw FormatError("weather depth alias requires actual physical/device handles");
            vkGetPhysicalDeviceProperties(context.physical,&properties);
            if(properties.apiVersion<VK_API_VERSION_1_1)throw FormatError("weather depth alias requires Vulkan 1.1 format/view support");
            unsigned count=0;vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,nullptr);
            std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,families.data());
            if(context.queue_family>=count || !families[context.queue_family].queueCount ||
               (families[context.queue_family].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))!=(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))
                throw FormatError("weather depth alias requires an actual graphics+compute queue family");
            const auto &limits=properties.limits;
            if(limits.maxComputeWorkGroupInvocations<64 || limits.maxComputeWorkGroupSize[0]<8 || limits.maxComputeWorkGroupSize[1]<8 || limits.maxPushConstantsSize<8 || limits.maxPerStageDescriptorStorageBuffers<3 || limits.maxPerStageDescriptorStorageImages<1)
                throw FormatError("weather depth alias compute limits unsupported");
            const auto format=[&](VkFormat f,VkFormatFeatureFlags required){VkFormatProperties p;vkGetPhysicalDeviceFormatProperties(context.physical,f,&p);if((p.optimalTilingFeatures&required)!=required)throw FormatError("weather depth alias native image format features unsupported");};
            format(VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
            format(VK_FORMAT_R8G8B8A8_UINT,VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
            std::array<VkDescriptorSetLayoutBinding,4> bindings{};
            for(unsigned i=0;i<4;++i){bindings[i].binding=i;bindings[i].descriptorCount=1;bindings[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;}
            VkDescriptorSetLayoutCreateInfo dc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dc.bindingCount=bindings.size();dc.pBindings=bindings.data();check(vkCreateDescriptorSetLayout(context.device,&dc,nullptr,&descriptor_layout));
            const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,8};
            VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.setLayoutCount=1;lc.pSetLayouts=&descriptor_layout;lc.pushConstantRangeCount=1;lc.pPushConstantRanges=&push;check(vkCreatePipelineLayout(context.device,&lc,nullptr,&pipeline_layout));
            std::ifstream file(context.shader_directory/"source_weather_depth_alias.comp.spv",std::ios::binary|std::ios::ate);
            if(!file || file.tellg()<20 || std::uint64_t(file.tellg())%4)throw FormatError("weather depth alias SPIR-V missing or incomplete");
            std::vector<std::uint32_t> words(std::size_t(file.tellg())/4);file.seekg(0);
            if(!file.read(reinterpret_cast<char*>(words.data()),words.size()*4)||words[0]!=0x07230203)throw FormatError("weather depth alias SPIR-V invalid");
            VkShaderModuleCreateInfo sc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sc.codeSize=words.size()*4;sc.pCode=words.data();check(vkCreateShaderModule(context.device,&sc,nullptr,&shader));
            VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=pipeline_layout;pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader;pc.stage.pName="main";
            const VkSpecializationMapEntry entry{0,0,4};
            for(std::uint32_t i=0;i<2;++i){const VkSpecializationInfo info{1,&entry,4,&i};pc.stage.pSpecializationInfo=&info;check(vkCreateComputePipelines(context.device,{},1,&pc,nullptr,&pipeline[i]));}
            VkSamplerCreateInfo ss{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ss.minFilter=ss.magFilter=VK_FILTER_NEAREST;ss.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;ss.addressModeU=ss.addressModeV=ss.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;ss.minLod=ss.maxLod=0;check(vkCreateSampler(context.device,&ss,nullptr,&sampler));
        } catch(...) {cleanup();throw;}
    }
    void cleanup() {
        resources.reset();
        if(sampler)vkDestroySampler(context.device,sampler,nullptr);
        for(auto p:pipeline)if(p)vkDestroyPipeline(context.device,p,nullptr);
        for(auto p:color_pipeline)if(p)vkDestroyPipeline(context.device,p,nullptr);
        for(auto s:color_shader)if(s)vkDestroyShaderModule(context.device,s,nullptr);
        if(shader)vkDestroyShaderModule(context.device,shader,nullptr);
        if(pipeline_layout)vkDestroyPipelineLayout(context.device,pipeline_layout,nullptr);
        if(descriptor_layout)vkDestroyDescriptorSetLayout(context.device,descriptor_layout,nullptr);
    }
    ~Impl(){if(pending)vkWaitForFences(context.device,1,&pending,VK_TRUE,std::numeric_limits<std::uint64_t>::max());cleanup();}
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(context.physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("weather depth alias requested memory type unavailable");
    }
    void allocate(VkMemoryRequirements r,VkDeviceMemory &memory,VkMemoryPropertyFlags flags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memory_type(r.memoryTypeBits,flags);check(vkAllocateMemory(context.device,&a,nullptr,&memory));
    }
    void prepare(VkExtent2D extent) {
        const auto size=source_weather_depth_alias_storage(extent);
        const auto &limits=properties.limits;
        if(extent.width>limits.maxImageDimension2D||extent.height>limits.maxImageDimension2D || size.depth_bytes>limits.maxStorageBufferRange||size.stencil_bytes>limits.maxStorageBufferRange || (std::uint64_t(extent.width)+7)/8>limits.maxComputeWorkGroupCount[0]||(std::uint64_t(extent.height)+7)/8>limits.maxComputeWorkGroupCount[1])
            throw FormatError("weather depth alias extent exceeds actual device limits");
        if(pending)throw FormatError("weather depth alias resources still owned by an unretired submission");
        if(resources&&resources->extent.width==extent.width&&resources->extent.height==extent.height)return;
        VkImageFormatProperties supported{};
        check(vkGetPhysicalDeviceImageFormatProperties(context.physical,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,&supported));
        if(extent.width>supported.maxExtent.width||extent.height>supported.maxExtent.height || !(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))throw FormatError("weather depth alias image extent unsupported");
        auto r=std::make_unique<Resources>();r->device=context.device;r->extent=extent;
        for(unsigned i=0;i<2;++i){auto &b=r->buffers[i];b.size=i?size.stencil_bytes:size.depth_bytes;VkBufferCreateInfo bc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bc.size=b.size;bc.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;check(vkCreateBuffer(context.device,&bc,nullptr,&b.handle));VkMemoryRequirements m;vkGetBufferMemoryRequirements(context.device,b.handle,&m);allocate(m,b.memory);check(vkBindBufferMemory(context.device,b.handle,b.memory,0));}
        {auto &b=r->validation;b.size=4;VkBufferCreateInfo bc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bc.size=4;bc.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;check(vkCreateBuffer(context.device,&bc,nullptr,&b.handle));VkMemoryRequirements m;vkGetBufferMemoryRequirements(context.device,b.handle,&m);allocate(m,b.memory,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);check(vkBindBufferMemory(context.device,b.handle,b.memory,0));check(vkMapMemory(context.device,b.memory,0,4,0,&r->validation_mapped));}
        VkImageCreateInfo ic{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ic.flags=VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;ic.imageType=VK_IMAGE_TYPE_2D;ic.format=VK_FORMAT_R8G8B8A8_UNORM;ic.extent={extent.width,extent.height,1};ic.mipLevels=ic.arrayLayers=1;ic.samples=VK_SAMPLE_COUNT_1_BIT;ic.tiling=VK_IMAGE_TILING_OPTIMAL;ic.usage=VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;check(vkCreateImage(context.device,&ic,nullptr,&r->image));VkMemoryRequirements m;vkGetImageMemoryRequirements(context.device,r->image,&m);allocate(m,r->memory);check(vkBindImageMemory(context.device,r->image,r->memory,0));
        VkImageViewUsageCreateInfo view_usage{VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};view_usage.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        VkImageViewCreateInfo vc{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vc.pNext=&view_usage;vc.image=r->image;vc.viewType=VK_IMAGE_VIEW_TYPE_2D;vc.format=VK_FORMAT_R8G8B8A8_UNORM;vc.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(context.device,&vc,nullptr,&r->sampled));
        // The existing producer stores A8R8G8B8's sampled RGBA order. The
        // original A8B8G8R8 header interprets the same bytes with R/B swapped.
        view_usage.usage=VK_IMAGE_USAGE_SAMPLED_BIT;
        vc.components={VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_A};
        check(vkCreateImageView(context.device,&vc,nullptr,&r->alternate_sampled));
        vc.components={};vc.format=VK_FORMAT_R8G8B8A8_UINT;view_usage.usage=VK_IMAGE_USAGE_STORAGE_BIT;check(vkCreateImageView(context.device,&vc,nullptr,&r->storage));
        const VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,3},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=2;pool.pPoolSizes=pool_sizes;check(vkCreateDescriptorPool(context.device,&pool,nullptr,&r->pool));
        VkDescriptorSetAllocateInfo ac{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ac.descriptorPool=r->pool;ac.descriptorSetCount=1;ac.pSetLayouts=&descriptor_layout;check(vkAllocateDescriptorSets(context.device,&ac,&r->descriptor));
        std::array<VkDescriptorBufferInfo,3> buffer_info{};std::array<VkWriteDescriptorSet,4> writes{};
        const VkDescriptorImageInfo image_info{{},r->storage,VK_IMAGE_LAYOUT_GENERAL};
        for(unsigned i=0;i<3;++i){writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[i].dstSet=r->descriptor;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;if(i<2){buffer_info[i]={r->buffers[i].handle,0,r->buffers[i].size};writes[i].pBufferInfo=&buffer_info[i];}else writes[i].pImageInfo=&image_info;}
        buffer_info[2]={r->validation.handle,0,4};writes[3]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[3].dstSet=r->descriptor;writes[3].dstBinding=3;writes[3].descriptorCount=1;writes[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[3].pBufferInfo=&buffer_info[2];
        vkUpdateDescriptorSets(context.device,writes.size(),writes.data(),0,nullptr);
        resources=std::move(r);
    }
    void record(const SourceWeatherDepthAliasRecord &record) {
        const auto &s=record.source;
        const auto encoding=encoding_index(s);
        if(!resources||!record.command||!record.completion||!s.image)throw FormatError("weather depth alias requires prepared resources and actual command/image/fence handles");
        if(pending)throw FormatError("weather depth alias submission must be retired before reuse");
        if(record.command_owner!=context.device||s.owner!=context.device || record.command_queue_family!=context.queue_family||s.queue_family!=context.queue_family || s.sharing!=VK_SHARING_MODE_EXCLUSIVE || !record.exclusive_access||!record.outside_render_pass)
            throw FormatError("weather depth alias requires exclusive same-device/same-family ownership outside a render pass");
        if(s.samples!=VK_SAMPLE_COUNT_1_BIT || s.mip_levels!=1||s.array_layers!=1 || s.type!=VK_IMAGE_TYPE_2D||s.tiling!=VK_IMAGE_TILING_OPTIMAL||s.create_flags || (s.usage&(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT))!=(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT) || s.extent.width!=resources->extent.width||s.extent.height!=resources->extent.height || s.image==resources->image)
            throw FormatError("weather depth alias requires a matching actual single-subresource D24S8 transfer-source depth attachment");
        VkImageFormatProperties supported{};check(vkGetPhysicalDeviceImageFormatProperties(context.physical,s.format,s.type,s.tiling,s.usage,s.create_flags,&supported));
        if(s.extent.width>supported.maxExtent.width||s.extent.height>supported.maxExtent.height||!(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))throw FormatError("weather depth alias source image usage/extent unsupported");
        const auto scope=source_scope(s.layout);
        const auto status=vkGetFenceStatus(context.device,record.completion);
        if(status==VK_SUCCESS)throw FormatError("weather depth alias completion fence must be reset/unsignaled");
        if(status!=VK_NOT_READY)check(status);
        auto &r=*resources;const auto command=record.command;
        VkBufferMemoryBarrier validation{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};validation.srcAccessMask=VK_ACCESS_HOST_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;validation.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;validation.srcQueueFamilyIndex=validation.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;validation.buffer=r.validation.handle;validation.size=4;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,1,&validation,0,nullptr);vkCmdFillBuffer(command,r.validation.handle,0,4,0);
        validation.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;validation.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,1,&validation,0,nullptr);
        const Scope transfer_read{VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT},transfer_write{VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT},compute_read{VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT},compute_write{VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_WRITE_BIT},sample_read{VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT};
        std::array<VkBufferMemoryBarrier,2> buffers{};
        for(unsigned i=0;i<2;++i){buffers[i]={VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};buffers[i].srcAccessMask=r.recorded?VK_ACCESS_SHADER_READ_BIT:0;buffers[i].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;buffers[i].srcQueueFamilyIndex=buffers[i].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;buffers[i].buffer=r.buffers[i].handle;buffers[i].size=r.buffers[i].size;}
        vkCmdPipelineBarrier(command,r.recorded?VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,buffers.size(),buffers.data(),0,nullptr);
        image_barrier(command,s.image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,s.layout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,scope,transfer_read);
        // Allocate and initialize the complete final uint so odd extents never
        // cause a shader read past the stencil descriptor or undefined tail.
        vkCmdFillBuffer(command,r.buffers[1].handle,0,r.buffers[1].size,0);
        auto fill=buffers[1];fill.srcAccessMask=fill.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,1,&fill,0,nullptr);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={s.extent.width,s.extent.height,1};
        vkCmdCopyImageToBuffer(command,s.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r.buffers[0].handle,1,&copy);
        copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyImageToBuffer(command,s.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r.buffers[1].handle,1,&copy);
        image_barrier(command,s.image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,s.layout,transfer_read,scope);
        for(auto &b:buffers){b.srcAccessMask=transfer_write.access;b.dstAccessMask=compute_read.access;}
        vkCmdPipelineBarrier(command,transfer_write.stage,compute_read.stage,0,0,nullptr,buffers.size(),buffers.data(),0,nullptr);
        image_barrier(command,r.image,VK_IMAGE_ASPECT_COLOR_BIT,r.recorded?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,r.recorded?sample_read:Scope{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,0},compute_write);
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline[encoding]);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_layout,0,1,&r.descriptor,0,nullptr);
        const std::array<std::uint32_t,2> dimensions{s.extent.width,s.extent.height};vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(dimensions),dimensions.data());
        vkCmdDispatch(command,(s.extent.width+7)/8,(s.extent.height+7)/8,1);
        validation.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT|VK_ACCESS_SHADER_WRITE_BIT;validation.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&validation,0,nullptr);
        image_barrier(command,r.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,compute_write,sample_read);
        previous_recorded=r.recorded;r.recorded=true;pending=record.completion;pending_command=record.command;
    }
    void prepare_color_pipeline(const SourceWeatherDepthAliasImage &source) {
        const auto encoding=encoding_index(source);
        if(color_pipeline[encoding])return;
        VkFormatProperties format{};vkGetPhysicalDeviceFormatProperties(context.physical,source.format,&format);
        if(!(format.optimalTilingFeatures&VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
            throw FormatError("depth color alias requires exact D24S8 transfer-destination support");
        std::ifstream file(context.shader_directory/"source_depth_color_writeback.comp.spv",std::ios::binary|std::ios::ate);
        if(!file||file.tellg()<20||std::uint64_t(file.tellg())%4)
            throw FormatError("depth color alias writeback SPIR-V missing or incomplete");
        std::vector<std::uint32_t> words(std::size_t(file.tellg())/4);file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(words.data()),words.size()*4)||words[0]!=0x07230203)
            throw FormatError("depth color alias writeback SPIR-V invalid");
        VkShaderModule module{};VkShaderModuleCreateInfo sc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sc.codeSize=words.size()*4;sc.pCode=words.data();check(vkCreateShaderModule(context.device,&sc,nullptr,&module));
        VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=pipeline_layout;pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=module;pc.stage.pName="main";
        const VkSpecializationMapEntry entry{0,0,4};const VkSpecializationInfo info{1,&entry,4,&encoding};pc.stage.pSpecializationInfo=&info;
        const auto result=vkCreateComputePipelines(context.device,{},1,&pc,nullptr,&color_pipeline[encoding]);
        if(result!=VK_SUCCESS){if(color_pipeline[encoding])vkDestroyPipeline(context.device,color_pipeline[encoding],nullptr);color_pipeline[encoding]={};vkDestroyShaderModule(context.device,module,nullptr);check(result);}
        color_shader[encoding]=module;
    }
    SourceWeatherDepthAliasBinding begin_color(const SourceWeatherDepthAliasRecord &input) {
        if(!(input.source.usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT))
            throw FormatError("depth color alias source must also support transfer-destination writes");
        const auto pixels=source_weather_depth_alias_storage(input.source.extent).pixels;
        if((std::uint64_t(pixels)+255)/256>properties.limits.maxComputeWorkGroupCount[0])
            throw FormatError("depth color alias writeback extent exceeds actual dispatch limits");
        prepare_color_pipeline(input.source);
        record(input);
        image_barrier(input.command,resources->image,VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT},
            {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT});
        color_source=input.source;color_open=true;
        return {resources->image,resources->sampled,sampler,VK_FORMAT_R8G8B8A8_UNORM,resources->extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,1};
    }
    void end_color(VkCommandBuffer command) {
        if(!color_open||!command||command!=pending_command)
            throw FormatError("depth color alias writeback requires its open color phase and exact command");
        auto &r=*resources;
        const Scope compute_read{VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT},
            transfer_write{VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT},
            sample_read{VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_SHADER_READ_BIT};
        image_barrier(command,r.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL,
            {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},compute_read);
        std::array<VkBufferMemoryBarrier,2> buffers{};
        for(unsigned i=0;i<2;++i){auto &b=buffers[i];b={VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};b.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;b.dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.buffer=r.buffers[i].handle;b.size=r.buffers[i].size;}
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,buffers.size(),buffers.data(),0,nullptr);
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,color_pipeline[encoding_index(color_source)]);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_layout,0,1,&r.descriptor,0,nullptr);
        const std::array<std::uint32_t,2> dimensions{r.extent.width,r.extent.height};vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(dimensions),dimensions.data());
        const auto count=source_weather_depth_alias_storage(r.extent).pixels;
        vkCmdDispatch(command,std::uint32_t(((std::uint64_t(count)+3)/4+63)/64),1,1);
        for(auto &b:buffers){b.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;}
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,buffers.size(),buffers.data(),0,nullptr);
        const auto source_access=source_scope(color_source.layout);
        image_barrier(command,color_source.image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,color_source.layout,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,source_access,transfer_write);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={r.extent.width,r.extent.height,1};
        vkCmdCopyBufferToImage(command,r.buffers[0].handle,color_source.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;vkCmdCopyBufferToImage(command,r.buffers[1].handle,color_source.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        image_barrier(command,color_source.image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,color_source.layout,transfer_write,source_access);
        image_barrier(command,r.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,compute_read,sample_read);
        color_open=false;color_source={};
    }
};
SourceWeatherDepthAlias::SourceWeatherDepthAlias(SourceWeatherDepthAliasContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceWeatherDepthAlias::~SourceWeatherDepthAlias()=default;
void SourceWeatherDepthAlias::prepare(VkExtent2D e){impl_->prepare(e);}
SourceWeatherDepthAliasContext SourceWeatherDepthAlias::context()const{return impl_->context;}
bool SourceWeatherDepthAlias::has_pending_submission()const{return impl_->pending!=VK_NULL_HANDLE;}
void SourceWeatherDepthAlias::record(const SourceWeatherDepthAliasRecord &r){impl_->record(r);}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::begin_color(const SourceWeatherDepthAliasRecord &r){return impl_->begin_color(r);}
void SourceWeatherDepthAlias::end_color(VkCommandBuffer command){impl_->end_color(command);}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::binding()const {
    return binding(SourceWeatherDepthAliasFormat::linear_a8r8g8b8);
}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::binding(SourceWeatherDepthAliasFormat format)const {
    if(!impl_->resources||!impl_->resources->recorded||impl_->color_open)throw FormatError("weather depth alias has no completed recorded production");
    return prepared_binding(format);
}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::prepared_binding()const {
    return prepared_binding(SourceWeatherDepthAliasFormat::linear_a8r8g8b8);
}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::prepared_binding(SourceWeatherDepthAliasFormat format)const {
    if(!impl_->resources)throw FormatError("weather depth alias has no prepared target");
    const auto &r=*impl_->resources;
    VkImageView view{};
    switch(format) {
    case SourceWeatherDepthAliasFormat::linear_a8r8g8b8:view=r.sampled;break;
    case SourceWeatherDepthAliasFormat::linear_a8b8g8r8:view=r.alternate_sampled;break;
    default:throw FormatError("weather depth alias requires original A8R8G8B8 or A8B8G8R8 texture format");
    }
    return {r.image,view,impl_->sampler,VK_FORMAT_R8G8B8A8_UNORM,r.extent,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,1};
}
void SourceWeatherDepthAlias::discard_unsubmitted(VkCommandBuffer command) {
    if(!impl_->pending||!command||command!=impl_->pending_command)
        throw FormatError("weather depth alias discard requires its exact never-submitted reset command");
    const auto status=vkGetFenceStatus(impl_->context.device,impl_->pending);
    if(status!=VK_NOT_READY){if(status!=VK_SUCCESS)check(status);throw FormatError("weather depth alias cannot discard a completed submission");}
    impl_->resources->recorded=impl_->previous_recorded;impl_->pending={};impl_->pending_command={};impl_->color_open=false;impl_->color_source={};
}
void SourceWeatherDepthAlias::retire(VkFence fence) {
    if(!fence||fence!=impl_->pending)throw FormatError("weather depth alias requires its outstanding submission fence");
    if(impl_->color_open)throw FormatError("depth color alias cannot retire an unfinished writeback phase");
    const auto status=vkGetFenceStatus(impl_->context.device,fence);
    if(status!=VK_SUCCESS){if(status!=VK_NOT_READY)check(status);throw FormatError("weather depth alias submission has not completed");}
    impl_->pending={};impl_->pending_command={};
    if(*static_cast<const std::uint32_t*>(impl_->resources->validation_mapped)) {
        impl_->resources->recorded=false;
        throw FormatError("source D24 carrier contains a noncanonical physical depth word");
    }
}
}
