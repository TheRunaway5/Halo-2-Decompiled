#include "halo2/source_world_draw_storage.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
namespace halo2 {
namespace {
void check(VkResult result,const char *operation) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": Vulkan result "+std::to_string(result));
}
VkDeviceSize aligned(VkDeviceSize value,VkDeviceSize alignment) {
    return (value+alignment-1)/alignment*alignment;
}
}
struct SourceWorldDrawStorage::Impl {
    SourceWorldDrawStorageContext context;
    VkDescriptorPool pool{};
    VkBuffer materials{},banks{};
    VkDeviceMemory material_memory{},bank_memory{};
    VkDeviceSize material_stride{},bank_stride{};
    std::size_t capacity{};
    std::vector<VkDescriptorSet> sets;
    explicit Impl(SourceWorldDrawStorageContext input):context(std::move(input)) {
        if(!context.device || !context.physical || !context.texture_layout ||
           context.material_uniform_size==0)
            throw FormatError("incomplete world draw storage context");
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(context.physical,&properties);
        if(context.material_uniform_size>properties.limits.maxUniformBufferRange ||
           sizeof(PackedVertexProgramBank)>properties.limits.maxUniformBufferRange)
            throw FormatError("world draw constants exceed native uniform range");
        material_stride=aligned(context.material_uniform_size,properties.limits.minUniformBufferOffsetAlignment);
        bank_stride=aligned(sizeof(PackedVertexProgramBank),properties.limits.minUniformBufferOffsetAlignment);
    }
    ~Impl() {
        clear_uploads();
    }
    void clear_uploads() {
        if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);
        if(materials)vkDestroyBuffer(context.device,materials,nullptr);
        if(banks)vkDestroyBuffer(context.device,banks,nullptr);
        if(material_memory)vkFreeMemory(context.device,material_memory,nullptr);
        if(bank_memory)vkFreeMemory(context.device,bank_memory,nullptr);
        pool=VK_NULL_HANDLE;materials=banks=VK_NULL_HANDLE;
        material_memory=bank_memory=VK_NULL_HANDLE;capacity=0;sets.clear();
    }
    void buffer(VkDeviceSize size,VkBuffer &buffer,VkDeviceMemory &memory) {
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create.size=size;create.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(context.device,&create,nullptr,&buffer),"create queued world constants");
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(context.device,buffer,&requirements);
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(context.physical,&properties);
        unsigned type=properties.memoryTypeCount;
        for(unsigned i=0;i<properties.memoryTypeCount;++i)
            if((requirements.memoryTypeBits&(1u<<i)) &&
               (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
               (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {type=i;break;}
        if(type==properties.memoryTypeCount)throw FormatError("queued world constants lack coherent host memory");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
        check(vkAllocateMemory(context.device,&allocation,nullptr,&memory),"allocate queued world constants");
        check(vkBindBufferMemory(context.device,buffer,memory,0),"bind queued world constants");
    }
    void reserve(std::size_t requested) {
        if(requested<=capacity)return;
        if(requested>std::numeric_limits<std::uint32_t>::max()/12u ||
           requested>std::numeric_limits<VkDeviceSize>::max()/std::max(material_stride,bank_stride))
            throw FormatError("queued world storage exceeds Vulkan count range");
        clear_uploads();capacity=requested;
        buffer(material_stride*capacity,materials,material_memory);
        buffer(bank_stride*capacity,banks,bank_memory);
        const VkDescriptorPoolSize sizes[]{
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,std::uint32_t(capacity*12)},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,std::uint32_t(capacity)},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,std::uint32_t(capacity)}};
        VkDescriptorPoolCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        create.maxSets=std::uint32_t(capacity);create.poolSizeCount=3;create.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&create,nullptr,&pool),"create queued world descriptor pool");
    }
    std::span<const VkDescriptorSet> upload(std::span<const SourceWorldDrawUpload> draws) {
        for(const auto &draw:draws)
            if(!draw.resident || draw.material_uniform.size()!=context.material_uniform_size ||
               bool(draw.target_view)!=bool(draw.target_sampler) ||
               draw.vertex_bank.range[0]>192 || draw.vertex_bank.range[1]>158 ||
               draw.vertex_bank.range[1]>192-draw.vertex_bank.range[0] ||
               draw.vertex_bank.range[2]>1 || draw.vertex_bank.range[3]>1 ||
               (draw.vertex_bank.range[3] && (draw.vertex_bank.range[0]!=0 || draw.vertex_bank.range[1]<12)))
                throw FormatError("incomplete queued world draw binding");
        if(draws.empty()){sets.clear();return sets;}
        reserve(draws.size());
        check(vkResetDescriptorPool(context.device,pool,0),"reset retired world descriptors");
        sets.resize(draws.size());std::vector<VkDescriptorSetLayout> layouts(draws.size(),context.texture_layout);
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool=pool;allocation.descriptorSetCount=std::uint32_t(draws.size());allocation.pSetLayouts=layouts.data();
        check(vkAllocateDescriptorSets(context.device,&allocation,sets.data()),"allocate queued world descriptors");
        void *material_data{},*bank_data{};
        check(vkMapMemory(context.device,material_memory,0,VK_WHOLE_SIZE,0,&material_data),"map queued world materials");
        struct Mapping {VkDevice device;VkDeviceMemory memory;~Mapping(){vkUnmapMemory(device,memory);}} material_map{context.device,material_memory};
        check(vkMapMemory(context.device,bank_memory,0,VK_WHOLE_SIZE,0,&bank_data),"map queued world vertex banks");
        Mapping bank_map{context.device,bank_memory};
        for(std::size_t i=0;i<draws.size();++i) {
            const auto &draw=draws[i];
            std::memcpy(static_cast<std::byte *>(material_data)+i*material_stride,draw.material_uniform.data(),context.material_uniform_size);
            std::memcpy(static_cast<std::byte *>(bank_data)+i*bank_stride,&draw.vertex_bank,sizeof(draw.vertex_bank));
            VkCopyDescriptorSet copies[6]{};
            for(unsigned binding=0;binding<6;++binding) {
                copies[binding]={VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
                copies[binding].srcSet=draw.resident;copies[binding].dstSet=sets[i];
                copies[binding].srcBinding=copies[binding].dstBinding=binding<4?binding:binding+1;
                copies[binding].descriptorCount=binding<4?1:4;
            }
            vkUpdateDescriptorSets(context.device,0,nullptr,6,copies);
            const VkDescriptorBufferInfo material{materials,i*material_stride,context.material_uniform_size};
            const VkDescriptorBufferInfo bank{banks,i*bank_stride,sizeof(PackedVertexProgramBank)};
            const VkDescriptorImageInfo target{draw.target_sampler,draw.target_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet writes[3]{};
            for(unsigned j=0;j<3;++j) {
                writes[j]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[j].dstSet=sets[i];writes[j].descriptorCount=1;
            }
            writes[0].dstBinding=4;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;writes[0].pBufferInfo=&material;
            writes[1].dstBinding=7;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[1].pBufferInfo=&bank;
            writes[2].dstBinding=0;writes[2].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[2].pImageInfo=&target;
            vkUpdateDescriptorSets(context.device,draw.target_view?3:2,writes,0,nullptr);
        }
        return sets;
    }
};
SourceWorldDrawStorage::SourceWorldDrawStorage(SourceWorldDrawStorageContext context):impl_(std::make_unique<Impl>(std::move(context))){}
SourceWorldDrawStorage::~SourceWorldDrawStorage()=default;
std::span<const VkDescriptorSet> SourceWorldDrawStorage::upload(std::span<const SourceWorldDrawUpload> draws){return impl_->upload(draws);}
}
