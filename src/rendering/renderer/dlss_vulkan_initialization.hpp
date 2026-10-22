#pragma once
#include <vulkan/vulkan.h>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

namespace halo2::detail {
// The signed Linux 310.5.3 M/L paths create four input-sized RG16 working
// images, but leave two in UNDEFINED at their first shader use. Initialize
// that private image group before evaluation. The SDK binary and its model
// selection remain unchanged; validation stays enabled for every command.
class DlssVulkanInitialization {
    struct Image { DlssVulkanInitialization* owner;bool pending=true; };
    using Key=std::pair<VkDevice,VkImage>;
    inline static std::mutex mutex_;
    inline static std::map<Key,Image> images_;
    inline static thread_local DlssVulkanInitialization* capture_{};
    VkDevice device_;
    VkExtent2D input_;

    bool matches(VkDevice device,const VkImageCreateInfo& image) const {
        constexpr auto usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT;
        return device==device_ && image.imageType==VK_IMAGE_TYPE_2D && image.tiling==VK_IMAGE_TILING_OPTIMAL &&
            image.initialLayout==VK_IMAGE_LAYOUT_UNDEFINED && image.format==VK_FORMAT_R16G16_SFLOAT &&
            image.extent.width==input_.width && image.extent.height==input_.height && image.extent.depth==1 &&
            image.mipLevels==1 && image.arrayLayers==1 && image.samples==VK_SAMPLE_COUNT_1_BIT &&
            (image.usage&usage)==usage;
    }
    static VKAPI_ATTR VkResult VKAPI_CALL create_image(VkDevice device,const VkImageCreateInfo* info,
        const VkAllocationCallbacks* allocator,VkImage* image) {
        const auto create=reinterpret_cast<PFN_vkCreateImage>(vkGetDeviceProcAddr(device,"vkCreateImage"));
        const auto result=create(device,info,allocator,image);
        if(result==VK_SUCCESS && capture_ && capture_->matches(device,*info)) {
            try {
                std::lock_guard lock(mutex_);images_.emplace(Key{device,*image},Image{capture_});
            } catch(...) {
                const auto destroy=reinterpret_cast<PFN_vkDestroyImage>(vkGetDeviceProcAddr(device,"vkDestroyImage"));
                destroy(device,*image,allocator);*image=VK_NULL_HANDLE;return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
        }
        return result;
    }
    static VKAPI_ATTR void VKAPI_CALL destroy_image(VkDevice device,VkImage image,const VkAllocationCallbacks* allocator) {
        {std::lock_guard lock(mutex_);images_.erase(Key{device,image});}
        const auto destroy=reinterpret_cast<PFN_vkDestroyImage>(vkGetDeviceProcAddr(device,"vkDestroyImage"));
        destroy(device,image,allocator);
    }
public:
    // Capture ownership only while this feature calls NGX. This also keeps
    // simultaneous prepared/active features on one device independent.
    class Capture {
        DlssVulkanInitialization* previous_;
    public:
        explicit Capture(DlssVulkanInitialization* owner):previous_(capture_){capture_=owner;}
        ~Capture(){capture_=previous_;}
        Capture(const Capture&)=delete;
        Capture& operator=(const Capture&)=delete;
    };
    DlssVulkanInitialization(VkDevice device,VkExtent2D input):device_(device),input_(input){}
    ~DlssVulkanInitialization() {
        std::lock_guard lock(mutex_);
        for(auto i=images_.begin();i!=images_.end();) {
            if(i->second.owner==this)i=images_.erase(i);else ++i;
        }
    }
    DlssVulkanInitialization(const DlssVulkanInitialization&)=delete;
    DlssVulkanInitialization& operator=(const DlssVulkanInitialization&)=delete;

    static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL device_proc(VkDevice device,const char* name) {
        if(!std::strcmp(name,"vkCreateImage"))return reinterpret_cast<PFN_vkVoidFunction>(create_image);
        if(!std::strcmp(name,"vkDestroyImage"))return reinterpret_cast<PFN_vkVoidFunction>(destroy_image);
        return vkGetDeviceProcAddr(device,name);
    }
    static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL instance_proc(VkInstance instance,const char* name) {
        if(!std::strcmp(name,"vkGetDeviceProcAddr"))return reinterpret_cast<PFN_vkVoidFunction>(device_proc);
        if(!std::strcmp(name,"vkCreateImage"))return reinterpret_cast<PFN_vkVoidFunction>(create_image);
        if(!std::strcmp(name,"vkDestroyImage"))return reinterpret_cast<PFN_vkVoidFunction>(destroy_image);
        return vkGetInstanceProcAddr(instance,name);
    }
    void prepare(VkCommandBuffer command) {
        std::vector<VkImageMemoryBarrier> barriers;
        {
            std::lock_guard lock(mutex_);
            for(auto& [key,image]:images_)if(image.owner==this && image.pending) {
                VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                barrier.image=key.second;barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
                barriers.push_back(barrier);image.pending=false;
            }
        }
        if(barriers.empty())return;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,0,nullptr,0,nullptr,unsigned(barriers.size()),barriers.data());
        const VkClearColorValue zero{};
        for(auto& barrier:barriers) {
            vkCmdClearColorImage(command,barrier.image,VK_IMAGE_LAYOUT_GENERAL,&zero,1,&barrier.subresourceRange);
            barrier.oldLayout=VK_IMAGE_LAYOUT_GENERAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
        }
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,0,nullptr,0,nullptr,unsigned(barriers.size()),barriers.data());
    }
};
}
