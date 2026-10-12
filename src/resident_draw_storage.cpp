#include "resident_draw_storage.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace halo2 {
namespace {
void check(VkResult result,const char* operation) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": Vulkan result "+std::to_string(result));
}
}
struct ResidentDrawStorage::Impl {
    VkDevice device;
    VkPhysicalDevice physical;
    VkDescriptorSetLayout layout;
    VkDeviceSize uniform_size,stride;
    VkDescriptorPool pool{};
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    void *mapping{};
    std::size_t capacity{};
    std::vector<VkDescriptorSet> sets;
    Impl(VkDevice d,VkPhysicalDevice p,VkDescriptorSetLayout l,VkDeviceSize size)
        :device(d),physical(p),layout(l),uniform_size(size) {
        if(!d || !p || !l || !size)throw FormatError("incomplete resident draw storage context");
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(p,&properties);
        if(size>properties.limits.maxUniformBufferRange)throw FormatError("resident draw uniform exceeds Vulkan range");
        const auto alignment=properties.limits.minUniformBufferOffsetAlignment;
        stride=(size+alignment-1)/alignment*alignment;
    }
    ~Impl(){clear();}
    void clear() {
        if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);
        if(mapping)vkUnmapMemory(device,memory);
        if(memory)vkFreeMemory(device,memory,nullptr);
        mapping=nullptr;
        pool=VK_NULL_HANDLE;buffer=VK_NULL_HANDLE;memory=VK_NULL_HANDLE;capacity=0;sets.clear();
    }
    void reserve(std::size_t count) {
        if(count<=capacity)return;
        if(count>std::numeric_limits<std::uint32_t>::max()/12u ||
           count>std::numeric_limits<VkDeviceSize>::max()/stride)
            throw FormatError("resident draw storage exceeds Vulkan count range");
        clear();capacity=count;
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create.size=stride*count;create.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(device,&create,nullptr,&buffer),"create resident draw constants");
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device,buffer,&requirements);
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
        unsigned type=properties.memoryTypeCount;
        for(unsigned i=0;i<properties.memoryTypeCount;++i)
            if((requirements.memoryTypeBits&(1u<<i)) &&
               (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
               (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
        if(type==properties.memoryTypeCount)throw FormatError("resident draw constants lack coherent host memory");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
        check(vkAllocateMemory(device,&allocation,nullptr,&memory),"allocate resident draw constants");
        check(vkBindBufferMemory(device,buffer,memory,0),"bind resident draw constants");
        check(vkMapMemory(device,memory,0,VK_WHOLE_SIZE,0,&mapping),"map resident draw constants");
        const VkDescriptorPoolSize sizes[]{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,std::uint32_t(count*12)},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,std::uint32_t(count)},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,std::uint32_t(count)}};
        VkDescriptorPoolCreateInfo descriptors{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptors.maxSets=std::uint32_t(count);descriptors.poolSizeCount=3;descriptors.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(device,&descriptors,nullptr,&pool),"create resident draw descriptors");
        sets.resize(count);std::vector<VkDescriptorSetLayout> layouts(count,layout);
        VkDescriptorSetAllocateInfo allocation_sets{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation_sets.descriptorPool=pool;allocation_sets.descriptorSetCount=std::uint32_t(count);allocation_sets.pSetLayouts=layouts.data();
        check(vkAllocateDescriptorSets(device,&allocation_sets,sets.data()),"allocate resident draw descriptors");
    }
    std::span<const VkDescriptorSet> upload(std::span<const ResidentDrawUpload> draws) {
        for(const auto&draw:draws)if(!draw.resident || draw.material.size()!=uniform_size)
            throw FormatError("incomplete resident draw upload");
        if(draws.empty())return {};
        reserve(draws.size());
        // The caller has retired the previous frame. Retain these sets and
        // refresh all source descriptors, including live texture-frame changes.
        std::vector<VkCopyDescriptorSet> copies(draws.size()*7);
        std::vector<VkWriteDescriptorSet> writes(draws.size());
        std::vector<VkDescriptorBufferInfo> uniforms(draws.size());
        for(std::size_t i=0;i<draws.size();++i) {
            std::memcpy(static_cast<std::byte*>(mapping)+i*stride,draws[i].material.data(),uniform_size);
            // Preserve all twelve sampled images and their resident source
            // vertex bank; only binding4 changes to this owner's constants.
            auto *copy=copies.data()+i*7;
            for(unsigned j=0;j<7;++j) {
                const auto binding=j<4?j:j+1;
                copy[j]={VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
                copy[j].srcSet=draws[i].resident;copy[j].dstSet=sets[i];
                copy[j].srcBinding=copy[j].dstBinding=binding;
                copy[j].descriptorCount=(binding==5 || binding==6)?4:1;
            }
            auto &uniform=uniforms[i];uniform={buffer,i*stride,uniform_size};
            auto &write=writes[i];write={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet=sets[i];write.dstBinding=4;write.descriptorCount=1;
            write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;write.pBufferInfo=&uniform;
        }
        // Sources are the renderer's resident sets, never these owner sets.
        // Binding4 writes and the other binding copies are disjoint, so batching
        // Vulkan's writes-before-copies order preserves the original result.
        vkUpdateDescriptorSets(device,std::uint32_t(writes.size()),writes.data(),
            std::uint32_t(copies.size()),copies.data());
        return std::span<const VkDescriptorSet>{sets}.first(draws.size());
    }
};
ResidentDrawStorage::ResidentDrawStorage(VkDevice d,VkPhysicalDevice p,VkDescriptorSetLayout l,VkDeviceSize s)
    :impl_(std::make_unique<Impl>(d,p,l,s)){}
ResidentDrawStorage::~ResidentDrawStorage()=default;
std::span<const VkDescriptorSet> ResidentDrawStorage::upload(std::span<const ResidentDrawUpload> draws){return impl_->upload(draws);}
}
