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
    VkPipeline pipeline{};
    VkSampler sampler{};
    VkFence pending{};
    VkCommandBuffer pending_command{};
    bool previous_recorded{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};VkDeviceSize size{};};
    struct Resources {
        VkDevice device{};
        VkExtent2D extent{};
        std::array<Buffer,2> buffers{};
        VkImage image{};VkDeviceMemory memory{};
        VkImageView sampled{},storage{};
        VkDescriptorPool pool{};VkDescriptorSet descriptor{};
        bool recorded{};
        ~Resources() {
            if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
            if(storage)vkDestroyImageView(device,storage,nullptr);
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
            if(limits.maxComputeWorkGroupInvocations<64 || limits.maxComputeWorkGroupSize[0]<8 || limits.maxComputeWorkGroupSize[1]<8 || limits.maxPushConstantsSize<8 || limits.maxPerStageDescriptorStorageBuffers<2 || limits.maxPerStageDescriptorStorageImages<1)
                throw FormatError("weather depth alias compute limits unsupported");
            const auto format=[&](VkFormat f,VkFormatFeatureFlags required){VkFormatProperties p;vkGetPhysicalDeviceFormatProperties(context.physical,f,&p);if((p.optimalTilingFeatures&required)!=required)throw FormatError("weather depth alias native image format features unsupported");};
            format(VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
            format(VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
            format(VK_FORMAT_R8G8B8A8_UINT,VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
            std::array<VkDescriptorSetLayoutBinding,3> bindings{};
            for(unsigned i=0;i<3;++i){bindings[i].binding=i;bindings[i].descriptorCount=1;bindings[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;}
            VkDescriptorSetLayoutCreateInfo dc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dc.bindingCount=bindings.size();dc.pBindings=bindings.data();check(vkCreateDescriptorSetLayout(context.device,&dc,nullptr,&descriptor_layout));
            const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,8};
            VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.setLayoutCount=1;lc.pSetLayouts=&descriptor_layout;lc.pushConstantRangeCount=1;lc.pPushConstantRanges=&push;check(vkCreatePipelineLayout(context.device,&lc,nullptr,&pipeline_layout));
            std::ifstream file(context.shader_directory/"source_weather_depth_alias.comp.spv",std::ios::binary|std::ios::ate);
            if(!file || file.tellg()<20 || std::uint64_t(file.tellg())%4)throw FormatError("weather depth alias SPIR-V missing or incomplete");
            std::vector<std::uint32_t> words(std::size_t(file.tellg())/4);file.seekg(0);
            if(!file.read(reinterpret_cast<char*>(words.data()),words.size()*4)||words[0]!=0x07230203)throw FormatError("weather depth alias SPIR-V invalid");
            VkShaderModuleCreateInfo sc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sc.codeSize=words.size()*4;sc.pCode=words.data();check(vkCreateShaderModule(context.device,&sc,nullptr,&shader));
            VkComputePipelineCreateInfo pc{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pc.layout=pipeline_layout;pc.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};pc.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;pc.stage.module=shader;pc.stage.pName="main";check(vkCreateComputePipelines(context.device,{},1,&pc,nullptr,&pipeline));
            VkSamplerCreateInfo ss{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ss.minFilter=ss.magFilter=VK_FILTER_NEAREST;ss.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;ss.addressModeU=ss.addressModeV=ss.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;ss.minLod=ss.maxLod=0;check(vkCreateSampler(context.device,&ss,nullptr,&sampler));
        } catch(...) {cleanup();throw;}
    }
    void cleanup() {
        resources.reset();
        if(sampler)vkDestroySampler(context.device,sampler,nullptr);
        if(pipeline)vkDestroyPipeline(context.device,pipeline,nullptr);
        if(shader)vkDestroyShaderModule(context.device,shader,nullptr);
        if(pipeline_layout)vkDestroyPipelineLayout(context.device,pipeline_layout,nullptr);
        if(descriptor_layout)vkDestroyDescriptorSetLayout(context.device,descriptor_layout,nullptr);
    }
    ~Impl(){if(pending)vkWaitForFences(context.device,1,&pending,VK_TRUE,std::numeric_limits<std::uint64_t>::max());cleanup();}
    unsigned memory_type(unsigned bits) {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(context.physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))return i;
        throw FormatError("weather depth alias device-local memory unavailable");
    }
    void allocate(VkMemoryRequirements r,VkDeviceMemory &memory) {
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memory_type(r.memoryTypeBits);check(vkAllocateMemory(context.device,&a,nullptr,&memory));
    }
    void prepare(VkExtent2D extent) {
        const auto size=source_weather_depth_alias_storage(extent);
        const auto &limits=properties.limits;
        if(extent.width>limits.maxImageDimension2D||extent.height>limits.maxImageDimension2D || size.depth_bytes>limits.maxStorageBufferRange||size.stencil_bytes>limits.maxStorageBufferRange || (std::uint64_t(extent.width)+7)/8>limits.maxComputeWorkGroupCount[0]||(std::uint64_t(extent.height)+7)/8>limits.maxComputeWorkGroupCount[1])
            throw FormatError("weather depth alias extent exceeds actual device limits");
        if(pending)throw FormatError("weather depth alias resources still owned by an unretired submission");
        if(resources&&resources->extent.width==extent.width&&resources->extent.height==extent.height)return;
        VkImageFormatProperties supported{};
        check(vkGetPhysicalDeviceImageFormatProperties(context.physical,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,&supported));
        if(extent.width>supported.maxExtent.width||extent.height>supported.maxExtent.height || !(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))throw FormatError("weather depth alias image extent unsupported");
        auto r=std::make_unique<Resources>();r->device=context.device;r->extent=extent;
        for(unsigned i=0;i<2;++i){auto &b=r->buffers[i];b.size=i?size.stencil_bytes:size.depth_bytes;VkBufferCreateInfo bc{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bc.size=b.size;bc.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;check(vkCreateBuffer(context.device,&bc,nullptr,&b.handle));VkMemoryRequirements m;vkGetBufferMemoryRequirements(context.device,b.handle,&m);allocate(m,b.memory);check(vkBindBufferMemory(context.device,b.handle,b.memory,0));}
        VkImageCreateInfo ic{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ic.flags=VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;ic.imageType=VK_IMAGE_TYPE_2D;ic.format=VK_FORMAT_R8G8B8A8_UNORM;ic.extent={extent.width,extent.height,1};ic.mipLevels=ic.arrayLayers=1;ic.samples=VK_SAMPLE_COUNT_1_BIT;ic.tiling=VK_IMAGE_TILING_OPTIMAL;ic.usage=VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;check(vkCreateImage(context.device,&ic,nullptr,&r->image));VkMemoryRequirements m;vkGetImageMemoryRequirements(context.device,r->image,&m);allocate(m,r->memory);check(vkBindImageMemory(context.device,r->image,r->memory,0));
        VkImageViewUsageCreateInfo view_usage{VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};view_usage.usage=VK_IMAGE_USAGE_SAMPLED_BIT;
        VkImageViewCreateInfo vc{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vc.pNext=&view_usage;vc.image=r->image;vc.viewType=VK_IMAGE_VIEW_TYPE_2D;vc.format=VK_FORMAT_R8G8B8A8_UNORM;vc.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(context.device,&vc,nullptr,&r->sampled));vc.format=VK_FORMAT_R8G8B8A8_UINT;view_usage.usage=VK_IMAGE_USAGE_STORAGE_BIT;check(vkCreateImageView(context.device,&vc,nullptr,&r->storage));
        const VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=2;pool.pPoolSizes=pool_sizes;check(vkCreateDescriptorPool(context.device,&pool,nullptr,&r->pool));
        VkDescriptorSetAllocateInfo ac{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ac.descriptorPool=r->pool;ac.descriptorSetCount=1;ac.pSetLayouts=&descriptor_layout;check(vkAllocateDescriptorSets(context.device,&ac,&r->descriptor));
        std::array<VkDescriptorBufferInfo,2> buffer_info{};std::array<VkWriteDescriptorSet,3> writes{};
        const VkDescriptorImageInfo image_info{{},r->storage,VK_IMAGE_LAYOUT_GENERAL};
        for(unsigned i=0;i<3;++i){writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[i].dstSet=r->descriptor;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;if(i<2){buffer_info[i]={r->buffers[i].handle,0,r->buffers[i].size};writes[i].pBufferInfo=&buffer_info[i];}else writes[i].pImageInfo=&image_info;}
        vkUpdateDescriptorSets(context.device,writes.size(),writes.data(),0,nullptr);
        resources=std::move(r);
    }
    void record(const SourceWeatherDepthAliasRecord &record) {
        const auto &s=record.source;
        if(!resources||!record.command||!record.completion||!s.image)throw FormatError("weather depth alias requires prepared resources and actual command/image/fence handles");
        if(pending)throw FormatError("weather depth alias submission must be retired before reuse");
        if(record.command_owner!=context.device||s.owner!=context.device || record.command_queue_family!=context.queue_family||s.queue_family!=context.queue_family || s.sharing!=VK_SHARING_MODE_EXCLUSIVE || !record.exclusive_access||!record.outside_render_pass)
            throw FormatError("weather depth alias requires exclusive same-device/same-family ownership outside a render pass");
        if(s.format!=VK_FORMAT_D24_UNORM_S8_UINT||s.samples!=VK_SAMPLE_COUNT_1_BIT || s.mip_levels!=1||s.array_layers!=1 || s.type!=VK_IMAGE_TYPE_2D||s.tiling!=VK_IMAGE_TILING_OPTIMAL||s.create_flags || (s.usage&(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT))!=(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT) || s.extent.width!=resources->extent.width||s.extent.height!=resources->extent.height || s.image==resources->image)
            throw FormatError("weather depth alias requires a matching actual single-subresource D24S8 transfer-source depth attachment");
        const auto scope=source_scope(s.layout);
        const auto status=vkGetFenceStatus(context.device,record.completion);
        if(status==VK_SUCCESS)throw FormatError("weather depth alias completion fence must be reset/unsignaled");
        if(status!=VK_NOT_READY)check(status);
        auto &r=*resources;const auto command=record.command;
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
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
        vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_layout,0,1,&r.descriptor,0,nullptr);
        const std::array<std::uint32_t,2> dimensions{s.extent.width,s.extent.height};vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(dimensions),dimensions.data());
        vkCmdDispatch(command,(s.extent.width+7)/8,(s.extent.height+7)/8,1);
        image_barrier(command,r.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,compute_write,sample_read);
        previous_recorded=r.recorded;r.recorded=true;pending=record.completion;pending_command=record.command;
    }
};
SourceWeatherDepthAlias::SourceWeatherDepthAlias(SourceWeatherDepthAliasContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceWeatherDepthAlias::~SourceWeatherDepthAlias()=default;
void SourceWeatherDepthAlias::prepare(VkExtent2D e){impl_->prepare(e);}
void SourceWeatherDepthAlias::record(const SourceWeatherDepthAliasRecord &r){impl_->record(r);}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::binding()const {
    if(!impl_->resources||!impl_->resources->recorded)throw FormatError("weather depth alias has no recorded production");
    return prepared_binding();
}
SourceWeatherDepthAliasBinding SourceWeatherDepthAlias::prepared_binding()const {
    if(!impl_->resources)throw FormatError("weather depth alias has no prepared target");
    const auto &r=*impl_->resources;return {r.image,r.sampled,impl_->sampler,VK_FORMAT_R8G8B8A8_UNORM,r.extent,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,1};
}
void SourceWeatherDepthAlias::discard_unsubmitted(VkCommandBuffer command) {
    if(!impl_->pending||!command||command!=impl_->pending_command)
        throw FormatError("weather depth alias discard requires its exact never-submitted reset command");
    const auto status=vkGetFenceStatus(impl_->context.device,impl_->pending);
    if(status!=VK_NOT_READY){if(status!=VK_SUCCESS)check(status);throw FormatError("weather depth alias cannot discard a completed submission");}
    impl_->resources->recorded=impl_->previous_recorded;impl_->pending={};impl_->pending_command={};
}
void SourceWeatherDepthAlias::retire(VkFence fence) {
    if(!fence||fence!=impl_->pending)throw FormatError("weather depth alias requires its outstanding submission fence");
    const auto status=vkGetFenceStatus(impl_->context.device,fence);
    if(status!=VK_SUCCESS){if(status!=VK_NOT_READY)check(status);throw FormatError("weather depth alias submission has not completed");}
    impl_->pending={};impl_->pending_command={};
}
}
