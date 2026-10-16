#include "halo2/renderer_aux_textures.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr auto format=VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkImageUsageFlags usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
constexpr std::array<std::uint32_t,3> depths{1,4,1},layers{1,1,6},source_formats{0x2210429,0x22210439,0x221042d};
constexpr std::array<VkDeviceSize,3> offsets{0,64,320},sizes{64,256,384};
void check(VkResult value) {
    if(value!=VK_SUCCESS)throw std::runtime_error("Native auxiliary texture Vulkan operation: "+std::to_string(value));
}
std::uint32_t memory_type(VkPhysicalDevice physical,std::uint32_t allowed,VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((allowed&(1u<<i))&&(properties.memoryTypes[i].propertyFlags&required)==required)return i;
    throw std::runtime_error("Native auxiliary textures lack a required Vulkan memory type");
}
// Native conversion of the reference-verified original Morton writes. X0 is
// the lowest Morton bit for both2D and XYZ volume. Thus the source alternating
// word stream becomes alternating X texels for every row/slice/cube face.
std::array<std::byte,704> source_pixels() {
    std::array<std::byte,704> result{};
    for(unsigned i=0;i<176;++i) {
        const auto word=(i&1)?0xf0f0u:0x0f00u;
        result[i*4]=std::byte(((word>>8)&15)*17);result[i*4+1]=std::byte(((word>>4)&15)*17);
        result[i*4+2]=std::byte((word&15)*17);result[i*4+3]=std::byte((word>>12)*17);
    }
    return result;
}
struct Transfer {
    VkDevice device;VkCommandPool pool{};VkCommandBuffer command{};VkFence fence{};
    VkBuffer buffer{};VkDeviceMemory memory{};void* mapped{};bool submitted{};
    explicit Transfer(RendererAuxTextureContext context):device(context.device){}
    void create(RendererAuxTextureContext context,VkDeviceSize bytes,VkBufferUsageFlags usage) {
        VkBufferCreateInfo buffer_create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};buffer_create.size=bytes;buffer_create.usage=usage;
        check(vkCreateBuffer(device,&buffer_create,nullptr,&buffer));
        VkMemoryRequirements needed;vkGetBufferMemoryRequirements(device,buffer,&needed);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=needed.size;
        allocate.memoryTypeIndex=memory_type(context.physical,needed.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&allocate,nullptr,&memory));check(vkBindBufferMemory(device,buffer,memory,0));
        check(vkMapMemory(device,memory,0,VK_WHOLE_SIZE,0,&mapped));
        VkCommandPoolCreateInfo pool_create{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool_create.queueFamilyIndex=context.queue_family;
        check(vkCreateCommandPool(device,&pool_create,nullptr,&pool));
        VkCommandBufferAllocateInfo command_allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};command_allocate.commandPool=pool;
        command_allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;command_allocate.commandBufferCount=1;
        check(vkAllocateCommandBuffers(device,&command_allocate,&command));
        VkFenceCreateInfo fence_create{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&fence_create,nullptr,&fence));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&begin));
    }
    void complete(RendererAuxTextureContext context) {
        check(vkEndCommandBuffer(command));VkQueue queue{};vkGetDeviceQueue(device,context.queue_family,0,&queue);
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        check(vkQueueSubmit(queue,1,&submit,fence));submitted=true;check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX));
    }
    ~Transfer() {
        if(submitted)vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX);
        if(mapped)vkUnmapMemory(device,memory);if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(memory)vkFreeMemory(device,memory,nullptr);
        if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);
    }
};
VkImageMemoryBarrier barrier(VkImage image,unsigned count,VkImageLayout old_layout,VkImageLayout new_layout,
    VkAccessFlags source,VkAccessFlags destination) {
    VkImageMemoryBarrier result{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};result.image=image;result.oldLayout=old_layout;result.newLayout=new_layout;
    result.srcAccessMask=source;result.dstAccessMask=destination;result.srcQueueFamilyIndex=result.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    result.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,count};return result;
}
VkBufferImageCopy copy(unsigned index,VkDeviceSize offset) {
    VkBufferImageCopy result{};result.bufferOffset=offset;result.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,layers[index]};
    result.imageExtent={4,4,depths[index]};return result;
}
}
struct RendererAuxTextureFactory::State {
    RendererDeviceLease device;VkPhysicalDevice physical;std::uint32_t queue_family;
    std::thread::id thread=std::this_thread::get_id();
};
struct RendererAuxTextures::Impl {
    RendererDeviceLease lease;RendererDeviceLease::Use owner;RendererAuxTextureContext context;
    std::thread::id thread=std::this_thread::get_id();
    std::array<VkImage,3> images{};std::array<VkImageView,3> views{};std::array<VkDeviceMemory,3> memory{};
    std::array<RendererAuxTextureImage,3> records;
    Impl(RendererDeviceLease lease,VkPhysicalDevice physical,std::uint32_t family)
        :lease(std::move(lease)),owner(this->lease.acquire()),context{physical,std::bit_cast<VkDevice>(owner.native_address().value),family}{}
    ~Impl() {
        // Keep the actual device alive through submitted work and cleanup,
        // including Renderer retirement. Discard unsubmitted borrowers first.
        if(images[0]||images[1]||images[2])vkDeviceWaitIdle(context.device);
        for(unsigned i=0;i<3;++i) {
            if(views[i])vkDestroyImageView(context.device,views[i],nullptr);
            if(images[i])vkDestroyImage(context.device,images[i],nullptr);
            if(memory[i])vkFreeMemory(context.device,memory[i],nullptr);
        }
    }
};
RendererAuxTextures::RendererAuxTextures(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
RendererAuxTextures::RendererAuxTextures(RendererAuxTextures&&) noexcept=default;
RendererAuxTextures& RendererAuxTextures::operator=(RendererAuxTextures&&) noexcept=default;
RendererAuxTextures::~RendererAuxTextures()=default;
bool RendererAuxTextures::valid()const{return impl_&&impl_->thread==std::this_thread::get_id()&&impl_->lease.valid();}
const RendererAuxTextureImage& RendererAuxTextures::image(unsigned index)const {
    if(!valid())throw std::runtime_error("Native auxiliary textures moved, retired or belong to another thread");
    if(index>=3)throw std::invalid_argument("Native auxiliary texture index is outside this ownership");return impl_->records[index];
}
NativeRuntimeAddress RendererAuxTextures::device_address()const{(void)image(0);return impl_->owner.native_address();}
RendererAuxTextureContext RendererAuxTextures::context()const{(void)image(0);return impl_->context;}
std::vector<std::byte> RendererAuxTextures::pixels(unsigned index)const {
    (void)image(index);const auto context=impl_->context;Transfer transfer(context);
    transfer.create(context,sizes[index],VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    auto readable=barrier(impl_->images[index],layers[index],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT);
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&readable);
    const auto region=copy(index,0);vkCmdCopyImageToBuffer(transfer.command,impl_->images[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,transfer.buffer,1,&region);
    auto restored=barrier(impl_->images[index],layers[index],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT);
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&restored);
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.buffer=transfer.buffer;host.size=sizes[index];
    host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);transfer.complete(context);
    std::vector<std::byte> result(sizes[index]);std::memcpy(result.data(),transfer.mapped,result.size());return result;
}
bool RendererAuxTextureFactory::valid()const{return state_&&state_->thread==std::this_thread::get_id()&&state_->device.valid();}
NativeRuntimeAddress RendererAuxTextureFactory::device_address()const {
    if(!valid())throw std::runtime_error("Native auxiliary texture factory is unavailable or retired");return state_->device.acquire().native_address();
}
RendererAuxTextureFactory RendererAuxTextureFactory::issue(RendererDeviceLease device,VkPhysicalDevice physical,std::uint32_t family) {
    (void)device.acquire();if(!physical)throw std::invalid_argument("Native auxiliary texture factory lacks its physical device");
    RendererAuxTextureFactory result;result.state_=std::make_shared<State>(State{std::move(device),physical,family});return result;
}
RendererAuxTextures RendererAuxTextureFactory::create()const {
    (void)device_address();
    for(unsigned i=0;i<3;++i) {
        VkImageFormatProperties supported;const auto type=i==1?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;
        check(vkGetPhysicalDeviceImageFormatProperties(state_->physical,format,type,VK_IMAGE_TILING_OPTIMAL,usage,i==2?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0,&supported));
        if(supported.maxExtent.width<4||supported.maxExtent.height<4||supported.maxExtent.depth<depths[i]||supported.maxArrayLayers<layers[i]||!(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))
            throw std::runtime_error("Native device cannot own original1D0C0's dimensionality/format");
    }
    auto impl=std::make_unique<RendererAuxTextures::Impl>(state_->device,state_->physical,state_->queue_family);
    for(unsigned i=0;i<3;++i) {
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.imageType=i==1?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;
        create.flags=i==2?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;create.format=format;create.extent={4,4,depths[i]};create.mipLevels=1;create.arrayLayers=layers[i];
        create.samples=VK_SAMPLE_COUNT_1_BIT;create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=usage;create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        create.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;check(vkCreateImage(impl->context.device,&create,nullptr,&impl->images[i]));
        VkMemoryRequirements needed;vkGetImageMemoryRequirements(impl->context.device,impl->images[i],&needed);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=needed.size;
        allocate.memoryTypeIndex=memory_type(state_->physical,needed.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(impl->context.device,&allocate,nullptr,&impl->memory[i]));check(vkBindImageMemory(impl->context.device,impl->images[i],impl->memory[i],0));
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=impl->images[i];view.viewType=i==2?VK_IMAGE_VIEW_TYPE_CUBE:i==1?VK_IMAGE_VIEW_TYPE_3D:VK_IMAGE_VIEW_TYPE_2D;
        view.format=format;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,layers[i]};check(vkCreateImageView(impl->context.device,&view,nullptr,&impl->views[i]));
        auto address=[](const auto* p){return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(p))};};
        impl->records[i]={{std::bit_cast<std::uint64_t>(impl->images[i]),std::bit_cast<std::uint64_t>(impl->views[i]),address(&impl->images[i]),address(&impl->views[i]),
            format,4,4,usage,VK_SAMPLE_COUNT_1_BIT,1,layers[i]},depths[i],source_formats[i],i==2};
    }
    auto context=impl->context;Transfer transfer(context);transfer.create(context,704,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    const auto pixels=source_pixels();std::memcpy(transfer.mapped,pixels.data(),pixels.size());
    for(unsigned i=0;i<3;++i) {
        auto writable=barrier(impl->images[i],layers[i],VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT);
        vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&writable);
        const auto region=copy(i,offsets[i]);vkCmdCopyBufferToImage(transfer.command,transfer.buffer,impl->images[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
        auto ready=barrier(impl->images[i],layers[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT);
        vkCmdPipelineBarrier(transfer.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&ready);
    }
    transfer.complete(context);return RendererAuxTextures(std::move(impl));
}
}
