#pragma once
// Per-frame upload resources shared by the cached decal and contrail renderers.
//
// Each frame those renderers upload a vertex stream and a uniform stream and
// bind one descriptor set per draw. Creating and freeing the buffers, their
// device memory and the descriptor pool every frame costs several driver calls
// into the kernel. These helpers keep them across frames instead: a buffer or
// pool is replaced only when a frame needs more room than it has.
//
// The caller rewrites a stream or resets the pool only after the GPU has
// finished the previous frame (the renderers prepare after the frame fence).
// The bytes the GPU reads, their offsets and the descriptors are unchanged.
#include "halo2/cache.hpp" // FormatError
#include <vulkan/vulkan.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace halo2::frame_upload {

inline void check(VkResult result,const char* what) {
    if(result!=VK_SUCCESS)throw FormatError(std::string(what)+" Vulkan failure: "+std::to_string(result));
}

// One host-visible, host-coherent buffer, mapped for its whole lifetime.
class Stream {
public:
    Stream(VkPhysicalDevice physical,VkDevice device,VkBufferUsageFlags usage)
        :physical_(physical),device_(device),usage_(usage){}
    Stream(const Stream&)=delete;
    Stream& operator=(const Stream&)=delete;
    ~Stream(){release();}

    // Copies bytes to offset 0. exact=true replaces the buffer with one of
    // exactly bytes.size() every call (the previous per-frame behaviour).
    void write(std::span<const std::byte> bytes,bool exact) {
        if(exact || bytes.size()>capacity_) {
            const auto capacity=exact?bytes.size():std::max(bytes.size(),2*capacity_);
            release();
            create(capacity);
        }
        std::memcpy(mapped_,bytes.data(),bytes.size());
    }
    VkBuffer handle()const {return buffer_;}
    void release() {
        if(mapped_)vkUnmapMemory(device_,memory_);
        if(buffer_)vkDestroyBuffer(device_,buffer_,nullptr);
        if(memory_)vkFreeMemory(device_,memory_,nullptr);
        mapped_=nullptr;buffer_=VK_NULL_HANDLE;memory_=VK_NULL_HANDLE;capacity_=0;
    }
private:
    void create(std::size_t bytes) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=bytes;info.usage=usage_;
        check(vkCreateBuffer(device_,&info,nullptr,&buffer_),"frame upload buffer");
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device_,buffer_,&requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocate.allocationSize=requirements.size;
        allocate.memoryTypeIndex=host_memory_type(requirements.memoryTypeBits);
        check(vkAllocateMemory(device_,&allocate,nullptr,&memory_),"frame upload memory");
        check(vkBindBufferMemory(device_,buffer_,memory_,0),"frame upload bind");
        void* mapped{};check(vkMapMemory(device_,memory_,0,VK_WHOLE_SIZE,0,&mapped),"frame upload map");
        mapped_=static_cast<std::byte*>(mapped);capacity_=bytes;
    }
    unsigned host_memory_type(unsigned mask)const {
        const VkMemoryPropertyFlags wanted=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical_,&properties);
        for(unsigned n=0;n<properties.memoryTypeCount;++n)
            if((mask&(1u<<n)) && (properties.memoryTypes[n].propertyFlags&wanted)==wanted)return n;
        throw FormatError("frame upload host-visible memory unavailable");
    }
    VkPhysicalDevice physical_;VkDevice device_;VkBufferUsageFlags usage_;
    VkBuffer buffer_{};VkDeviceMemory memory_{};std::byte* mapped_{};std::size_t capacity_{};
};

// A descriptor pool of combined-image-sampler + uniform-buffer sets, reset
// each frame and recreated only when a frame needs more sets.
class DescriptorSets {
public:
    DescriptorSets(VkDevice device,VkDescriptorSetLayout layout):device_(device),layout_(layout){}
    DescriptorSets(const DescriptorSets&)=delete;
    DescriptorSets& operator=(const DescriptorSets&)=delete;
    ~DescriptorSets(){release();}

    // count fresh sets for this frame. exact=true creates a new pool of
    // exactly count sets every call (the previous per-frame behaviour).
    std::vector<VkDescriptorSet> frame(std::size_t count,bool exact) {
        if(!exact && pool_ && count<=capacity_)check(vkResetDescriptorPool(device_,pool_,0),"frame descriptor reset");
        else {
            const auto capacity=exact?count:std::max(count,2*capacity_);
            release();
            const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,std::uint32_t(capacity)},
                                              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,std::uint32_t(capacity)}};
            VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            info.maxSets=std::uint32_t(capacity);info.poolSizeCount=2;info.pPoolSizes=sizes;
            check(vkCreateDescriptorPool(device_,&info,nullptr,&pool_),"frame descriptor pool");capacity_=capacity;
        }
        std::vector<VkDescriptorSetLayout> layouts(count,layout_);std::vector<VkDescriptorSet> sets(count);
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool=pool_;allocate.descriptorSetCount=std::uint32_t(count);allocate.pSetLayouts=layouts.data();
        check(vkAllocateDescriptorSets(device_,&allocate,sets.data()),"frame descriptor sets");
        return sets;
    }
    void release() {
        if(pool_)vkDestroyDescriptorPool(device_,pool_,nullptr);
        pool_=VK_NULL_HANDLE;capacity_=0;
    }
private:
    VkDevice device_;VkDescriptorSetLayout layout_;
    VkDescriptorPool pool_{};std::size_t capacity_{};
};

} // namespace halo2::frame_upload
