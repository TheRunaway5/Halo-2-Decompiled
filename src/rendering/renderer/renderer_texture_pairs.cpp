#include "halo2/renderer_texture_pairs.hpp"
#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr auto format=VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkImageUsageFlags usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
void check(VkResult value) {
    if(value!=VK_SUCCESS)throw std::runtime_error("Native texture-pair Vulkan allocation: "+std::to_string(value));
}
}
struct RendererTexturePairFactory::State {
    RendererDeviceLease device;
    VkPhysicalDevice physical;
    std::uint32_t queue_family;
    std::thread::id thread=std::this_thread::get_id();
};
struct RendererTexturePairs::Impl {
    RendererDeviceLease lease;
    RendererDeviceLease::Use owner;
    VkPhysicalDevice physical;
    VkDevice device;
    std::uint32_t family;
    std::thread::id thread=std::this_thread::get_id();
    std::array<VkImage,2> images{};
    std::array<VkImageView,2> views{};
    std::array<VkDeviceMemory,2> memory{};
    std::array<RendererTargetImage,2> records;
    Impl(RendererDeviceLease lease,VkPhysicalDevice physical,std::uint32_t family)
        :lease(std::move(lease)),owner(this->lease.acquire()),
         physical(physical),device(std::bit_cast<VkDevice>(owner.native_address().value)),family(family){}
    ~Impl() {
        // A retained native use keeps the actual device alive through cleanup,
        // including after Renderer retirement. Callers must discard recorded,
        // unsubmitted commands first; submitted GPU uses finish before free.
        if(images[0]||images[1])vkDeviceWaitIdle(device);
        for(unsigned i=0;i<2;++i) {
            if(views[i])vkDestroyImageView(device,views[i],nullptr);
            if(images[i])vkDestroyImage(device,images[i],nullptr);
            if(memory[i])vkFreeMemory(device,memory[i],nullptr);
        }
    }
};
RendererTexturePairs::RendererTexturePairs(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
RendererTexturePairs::RendererTexturePairs(RendererTexturePairs&&) noexcept=default;
RendererTexturePairs& RendererTexturePairs::operator=(RendererTexturePairs&&) noexcept=default;
RendererTexturePairs::~RendererTexturePairs()=default;
bool RendererTexturePairs::valid()const {
    return impl_&&impl_->thread==std::this_thread::get_id()&&impl_->lease.valid();
}
const RendererTargetImage& RendererTexturePairs::image(unsigned index)const {
    if(!valid())throw std::runtime_error("Native texture-pair owner has moved, retired or belongs to another thread");
    if(index>=2)throw std::invalid_argument("Native texture-pair image index is outside the owned pair");
    return impl_->records[index];
}
NativeRuntimeAddress RendererTexturePairs::device_address()const{(void)image(0);return impl_->owner.native_address();}
RendererTexturePairContext RendererTexturePairs::context()const{(void)image(0);return {impl_->physical,impl_->device,impl_->family};}
bool RendererTexturePairFactory::valid()const {
    return state_&&state_->thread==std::this_thread::get_id()&&state_->device.valid();
}
NativeRuntimeAddress RendererTexturePairFactory::device_address()const {
    if(!valid())throw std::runtime_error("Native texture-pair factory is unavailable or retired");
    return state_->device.acquire().native_address();
}
RendererTexturePairFactory RendererTexturePairFactory::issue(RendererDeviceLease device,VkPhysicalDevice physical,std::uint32_t family) {
    (void)device.acquire();
    if(!physical)throw std::invalid_argument("Native texture-pair factory lacks its physical device");
    RendererTexturePairFactory result;result.state_=std::make_shared<State>(State{std::move(device),physical,family});return result;
}
RendererTexturePairs RendererTexturePairFactory::create()const {
    (void)device_address();
    VkImageFormatProperties supported;
    check(vkGetPhysicalDeviceImageFormatProperties(state_->physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&supported));
    if(supported.maxExtent.width<64||supported.maxExtent.height<64||!(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))
        throw std::runtime_error("Native device cannot own original14830's texture format/extent");
    auto impl=std::make_unique<RendererTexturePairs::Impl>(state_->device,state_->physical,state_->queue_family);
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(state_->physical,&properties);
    for(unsigned i=0;i<2;++i) {
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};create.imageType=VK_IMAGE_TYPE_2D;
        create.format=format;create.extent={64,64,1};create.mipLevels=create.arrayLayers=1;
        create.samples=VK_SAMPLE_COUNT_1_BIT;create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=usage;
        create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;create.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        check(vkCreateImage(impl->device,&create,nullptr,&impl->images[i]));
        VkMemoryRequirements needed;vkGetImageMemoryRequirements(impl->device,impl->images[i],&needed);
        unsigned type=properties.memoryTypeCount;
        for(unsigned j=0;j<properties.memoryTypeCount;++j)if((needed.memoryTypeBits&(1u<<j))&&
            (properties.memoryTypes[j].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){type=j;break;}
        if(type==properties.memoryTypeCount)throw std::runtime_error("Native texture pair lacks device-local memory");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=needed.size;allocation.memoryTypeIndex=type;
        check(vkAllocateMemory(impl->device,&allocation,nullptr,&impl->memory[i]));check(vkBindImageMemory(impl->device,impl->images[i],impl->memory[i],0));
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=impl->images[i];view.viewType=VK_IMAGE_VIEW_TYPE_2D;
        view.format=format;view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(impl->device,&view,nullptr,&impl->views[i]));
        auto address=[](const auto* p){return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(p))};};
        impl->records[i]={std::bit_cast<std::uint64_t>(impl->images[i]),std::bit_cast<std::uint64_t>(impl->views[i]),
            address(&impl->images[i]),address(&impl->views[i]),format,64,64,usage,VK_SAMPLE_COUNT_1_BIT,1,1};
    }
    return RendererTexturePairs(std::move(impl));
}
}
