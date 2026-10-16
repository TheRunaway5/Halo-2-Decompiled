#include "halo2/renderer_debug_texture.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr VkImageUsageFlags usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
constexpr VkDeviceSize source_bytes=256*256*2;
void check(VkResult value) {
    if(value!=VK_SUCCESS)throw std::runtime_error("Native debugger texture Vulkan operation: "+std::to_string(value));
}
std::uint32_t memory_type(VkPhysicalDevice physical,std::uint32_t allowed,VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((allowed&(1u<<i))&&(properties.memoryTypes[i].propertyFlags&required)==required)return i;
    throw std::runtime_error("Native debugger texture lacks a required Vulkan memory type");
}
struct Format {
    VkFormat value;
    VkComponentMapping components;
    bool expanded;
};
Format supported_format(VkPhysicalDevice physical) {
    // Khronos VkFormat defines packed fields from high to low. The source
    // A,R,G,B nibble order therefore maps through these sampled view channels.
    constexpr std::array<Format,3> candidates{{
        {VK_FORMAT_R4G4B4A4_UNORM_PACK16,{VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A,VK_COMPONENT_SWIZZLE_R},false},
        {VK_FORMAT_B4G4R4A4_UNORM_PACK16,{VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_A,VK_COMPONENT_SWIZZLE_B},false},
        {VK_FORMAT_R8G8B8A8_UNORM,{VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY},true}
    }};
    for(const auto candidate:candidates) {
        VkFormatProperties features{};vkGetPhysicalDeviceFormatProperties(physical,candidate.value,&features);
        constexpr VkFormatFeatureFlags required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|
            VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        if((features.optimalTilingFeatures&required)!=required)continue;
        VkImageFormatProperties supported{};
        if(vkGetPhysicalDeviceImageFormatProperties(physical,candidate.value,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&supported)!=VK_SUCCESS)continue;
        if(supported.maxExtent.width>=256&&supported.maxExtent.height>=256&&(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))return candidate;
    }
    throw std::runtime_error("Native device cannot represent original1EFD0's A4R4G4B4 texture");
}
struct Transfer {
    VkDevice device;VkCommandPool pool{};VkCommandBuffer command{};VkFence fence{};
    VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};bool submitted{};
    explicit Transfer(RendererDebugTextureContext context):device(context.device){}
    void create(RendererDebugTextureContext context,VkDeviceSize bytes,VkBufferUsageFlags usage) {
        VkBufferCreateInfo buffer_create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};buffer_create.size=bytes;buffer_create.usage=usage;
        check(vkCreateBuffer(device,&buffer_create,nullptr,&buffer));VkMemoryRequirements needed;vkGetBufferMemoryRequirements(device,buffer,&needed);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=needed.size;
        allocation.memoryTypeIndex=memory_type(context.physical,needed.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&memory));check(vkBindBufferMemory(device,buffer,memory,0));
        check(vkMapMemory(device,memory,0,VK_WHOLE_SIZE,0,&mapped));
        VkCommandPoolCreateInfo pool_create{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_create.queueFamilyIndex=context.queue_family;
        check(vkCreateCommandPool(device,&pool_create,nullptr,&pool));
        VkCommandBufferAllocateInfo commands{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};commands.commandPool=pool;
        commands.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;commands.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&commands,&command));
        VkFenceCreateInfo fence_create{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&fence_create,nullptr,&fence));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&begin));
    }
    void complete(RendererDebugTextureContext context) {
        check(vkEndCommandBuffer(command));VkQueue queue{};vkGetDeviceQueue(device,context.queue_family,0,&queue);
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,fence));submitted=true;check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX));
    }
    ~Transfer() {
        if(submitted)vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX);
        if(mapped)vkUnmapMemory(device,memory);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);
        if(memory)vkFreeMemory(device,memory,nullptr);
        if(fence)vkDestroyFence(device,fence,nullptr);
        if(pool)vkDestroyCommandPool(device,pool,nullptr);
    }
};
VkImageMemoryBarrier barrier(VkImage image,VkImageLayout old_layout,VkImageLayout new_layout,VkAccessFlags source,VkAccessFlags destination) {
    VkImageMemoryBarrier result{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};result.image=image;result.oldLayout=old_layout;result.newLayout=new_layout;
    result.srcAccessMask=source;result.dstAccessMask=destination;result.srcQueueFamilyIndex=result.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    result.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};return result;
}
VkBufferImageCopy region() {
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={256,256,1};return copy;
}
}
struct RendererDebugTextureFactory::State {
    RendererDeviceLease device;VkPhysicalDevice physical;std::uint32_t queue_family;
    std::thread::id thread=std::this_thread::get_id();
};
struct RendererDebugTexture::Impl {
    RendererDeviceLease lease;RendererDeviceLease::Use owner;RendererDebugTextureContext context;Format format;
    std::thread::id thread=std::this_thread::get_id();
    VkImage image{};VkImageView view{};VkDeviceMemory memory{};RendererTargetImage record;
    bool content_initialized{};
    Impl(RendererDeviceLease lease,VkPhysicalDevice physical,std::uint32_t family,Format format)
        :lease(std::move(lease)),owner(this->lease.acquire()),context{physical,std::bit_cast<VkDevice>(owner.native_address().value),family},format(format){}
    ~Impl() {
        // Retained device ownership supports cleanup after Renderer retires.
        // The caller must discard recorded unsubmitted borrowers first.
        if(image)vkDeviceWaitIdle(context.device);
        if(view)vkDestroyImageView(context.device,view,nullptr);
        if(image)vkDestroyImage(context.device,image,nullptr);
        if(memory)vkFreeMemory(context.device,memory,nullptr);
    }
};
RendererDebugTexture::RendererDebugTexture(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
RendererDebugTexture::RendererDebugTexture(RendererDebugTexture&&) noexcept=default;
RendererDebugTexture& RendererDebugTexture::operator=(RendererDebugTexture&&) noexcept=default;
RendererDebugTexture::~RendererDebugTexture()=default;
bool RendererDebugTexture::valid()const{return impl_&&impl_->thread==std::this_thread::get_id()&&impl_->lease.valid();}
const RendererTargetImage& RendererDebugTexture::image()const {
    if(!valid())throw std::runtime_error("Native debugger texture moved, retired or belongs to another thread");
    return impl_->record;
}
VkComponentMapping RendererDebugTexture::components()const{(void)image();return impl_->format.components;}
NativeRuntimeAddress RendererDebugTexture::device_address()const{(void)image();return impl_->owner.native_address();}
RendererDebugTextureContext RendererDebugTexture::context()const{(void)image();return impl_->context;}
bool RendererDebugTexture::content_initialized()const{(void)image();return impl_->content_initialized;}
void RendererDebugTexture::upload_linear(std::span<const std::byte> source) {
    (void)image();if(source.size()!=source_bytes)throw std::invalid_argument("Native debugger producer requires all256x256 A4R4G4B4 source texels");
    const auto context=impl_->context;Transfer transfer(context);
    transfer.create(context,impl_->format.expanded?source_bytes*2:source_bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    if(impl_->format.expanded) {
        auto* bytes=static_cast<std::byte*>(transfer.mapped);
        for(std::size_t i=0;i<source_bytes/2;++i) {
            const auto word=std::to_integer<unsigned>(source[i*2])|(std::to_integer<unsigned>(source[i*2+1])<<8u);
            bytes[i*4]=std::byte(((word>>8)&15)*17);bytes[i*4+1]=std::byte(((word>>4)&15)*17);
            bytes[i*4+2]=std::byte((word&15)*17);bytes[i*4+3]=std::byte((word>>12)*17);
        }
    }else std::memcpy(transfer.mapped,source.data(),source.size());
    auto writable=barrier(impl_->image,impl_->content_initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,impl_->content_initialized?VK_ACCESS_SHADER_READ_BIT:0,VK_ACCESS_TRANSFER_WRITE_BIT);
    vkCmdPipelineBarrier(transfer.command,impl_->content_initialized?VK_PIPELINE_STAGE_ALL_COMMANDS_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&writable);
    const auto copy=region();vkCmdCopyBufferToImage(transfer.command,transfer.buffer,impl_->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    auto ready=barrier(impl_->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT);
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&ready);
    transfer.complete(context);impl_->content_initialized=true;
}
std::vector<std::byte> RendererDebugTexture::pixels()const {
    if(!content_initialized())throw std::runtime_error("Native debugger texture has no completed texel producer");
    const auto context=impl_->context;Transfer transfer(context);
    const auto count=impl_->format.expanded?source_bytes*2:source_bytes;
    transfer.create(context,count,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    auto readable=barrier(impl_->image,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT);
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&readable);
    const auto copy=region();vkCmdCopyImageToBuffer(transfer.command,impl_->image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,transfer.buffer,1,&copy);
    auto restored=barrier(impl_->image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT);
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&restored);
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.buffer=transfer.buffer;host.size=count;
    host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);transfer.complete(context);
    std::vector<std::byte> result(source_bytes);
    if(impl_->format.expanded) {
        const auto* bytes=static_cast<const std::byte*>(transfer.mapped);
        for(std::size_t i=0;i<source_bytes/2;++i) {
            for(unsigned channel=0;channel<4;++channel)
                if(std::to_integer<unsigned>(bytes[i*4+channel])%17)
                    throw std::runtime_error("Native debugger RGBA8 readback is not an exact source nibble");
            const auto word=((std::to_integer<unsigned>(bytes[i*4+3])/17)<<12)|((std::to_integer<unsigned>(bytes[i*4])/17)<<8)|
                ((std::to_integer<unsigned>(bytes[i*4+1])/17)<<4)|(std::to_integer<unsigned>(bytes[i*4+2])/17);
            result[i*2]=std::byte(word);result[i*2+1]=std::byte(word>>8);
        }
    }else std::memcpy(result.data(),transfer.mapped,result.size());return result;
}
bool RendererDebugTextureFactory::valid()const{return state_&&state_->thread==std::this_thread::get_id()&&state_->device.valid();}
NativeRuntimeAddress RendererDebugTextureFactory::device_address()const {
    if(!valid())throw std::runtime_error("Native debugger texture factory is unavailable or retired");
    return state_->device.acquire().native_address();
}
RendererDebugTextureFactory RendererDebugTextureFactory::issue(RendererDeviceLease device,VkPhysicalDevice physical,std::uint32_t family) {
    (void)device.acquire();if(!physical)throw std::invalid_argument("Native debugger texture factory lacks its physical device");
    RendererDebugTextureFactory result;result.state_=std::make_shared<State>(State{std::move(device),physical,family});return result;
}
RendererDebugTexture RendererDebugTextureFactory::create()const {
    (void)device_address();const auto format=supported_format(state_->physical);
    auto impl=std::make_unique<RendererDebugTexture::Impl>(state_->device,state_->physical,state_->queue_family,format);
    VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.imageType=VK_IMAGE_TYPE_2D;create.format=format.value;
    create.extent={256,256,1};create.mipLevels=create.arrayLayers=1;create.samples=VK_SAMPLE_COUNT_1_BIT;
    create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=usage;create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    create.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;check(vkCreateImage(impl->context.device,&create,nullptr,&impl->image));
    VkMemoryRequirements needed;vkGetImageMemoryRequirements(impl->context.device,impl->image,&needed);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=needed.size;
    allocate.memoryTypeIndex=memory_type(state_->physical,needed.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(impl->context.device,&allocate,nullptr,&impl->memory));check(vkBindImageMemory(impl->context.device,impl->image,impl->memory,0));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=impl->image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
    view.format=format.value;view.components=format.components;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    check(vkCreateImageView(impl->context.device,&view,nullptr,&impl->view));
    auto address=[](const auto* p){return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(p))};};
    impl->record={std::bit_cast<std::uint64_t>(impl->image),std::bit_cast<std::uint64_t>(impl->view),address(&impl->image),address(&impl->view),
        format.value,256,256,usage,VK_SAMPLE_COUNT_1_BIT,1,1};
    return RendererDebugTexture(std::move(impl));
}
}
