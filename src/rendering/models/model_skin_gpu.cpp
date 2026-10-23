#include "rendering/models/model_skin_gpu.hpp"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <string_view>
#include <vector>
#if defined(__x86_64__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
void checked(VkResult result) {
    if(result!=VK_SUCCESS)throw FormatError("model skin GPU Vulkan operation failed: "+std::to_string(result));
}
class PrepareEnvironment {
    std::fenv_t fp_{};int error_=errno;
    bool available_=std::fegetenv(&fp_)==0;
public:
    ~PrepareEnvironment()noexcept {
        if(available_ && std::fesetenv(&fp_)!=0)std::terminate();
        errno=error_;
    }
    void check()const {
        if(!available_)throw FormatError("model skin GPU cannot save caller floating-point environment");
    }
};
void checked_input_allocation(VkResult result) {
    if(result==VK_ERROR_OUT_OF_HOST_MEMORY || result==VK_ERROR_OUT_OF_DEVICE_MEMORY)
        throw ModelSkinGpuPrepareRefused(ModelSkinGpuPrepareRefusalReason::input_allocation,result);
    checked(result);
}
void checked_input_mapping(VkResult result) {
    if(result==VK_ERROR_OUT_OF_HOST_MEMORY || result==VK_ERROR_MEMORY_MAP_FAILED)
        throw ModelSkinGpuPrepareRefused(ModelSkinGpuPrepareRefusalReason::input_mapping,result);
    checked(result);
}
enum class PrepareInjection {none,quota,packing,create,allocation,device_allocation,map,map_range,device_lost};
PrepareInjection prepare_injection() {
    const auto *value=std::getenv("HALO2_TEST_GPU_SKIN_PREPARE_FAILURE");
    if(!value || !*value)return PrepareInjection::none;
    const std::string_view name(value);
    if(name=="quota")return PrepareInjection::quota;
    if(name=="packing")return PrepareInjection::packing;
    if(name=="create")return PrepareInjection::create;
    if(name=="allocation")return PrepareInjection::allocation;
    if(name=="device-allocation")return PrepareInjection::device_allocation;
    if(name=="map")return PrepareInjection::map;
    if(name=="map-range")return PrepareInjection::map_range;
    if(name=="device-lost")return PrepareInjection::device_lost;
    throw FormatError("unknown model skin GPU prepare failure injection");
}
}
bool ModelSkinGpu::supported(VkPhysicalDevice physical,std::uint32_t api) {
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);
    if(api<VK_API_VERSION_1_2 || properties.apiVersion<VK_API_VERSION_1_2)return false;
    VkPhysicalDeviceFloatControlsProperties fp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
    VkPhysicalDeviceProperties2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};query.pNext=&fp;
    vkGetPhysicalDeviceProperties2(physical,&query);
    return fp.shaderRoundingModeRTEFloat32 && fp.shaderSignedZeroInfNanPreserveFloat32;
}
struct ModelSkinGpu::Impl {
    VkDevice device{};VkPhysicalDevice physical{};VkPhysicalDeviceLimits limits{};
    VkDescriptorSetLayout set_layout{};VkDescriptorPool pool{};VkDescriptorSet set{};
    VkPipelineLayout layout{};VkPipeline pipeline{};
    VkBuffer input{};VkDeviceMemory memory{};std::size_t capacity{},count{};
    void *mapped{};
    bool streamed{},records_valid{},descriptors_valid{};
    std::vector<ModelSkinGpuRecord> records;
    std::size_t palette_rows{};
    VkBuffer descriptor_input{},descriptor_destination{};
    std::size_t descriptor_input_bytes{},descriptor_destination_bytes{};
    std::uint64_t descriptor_destination_generation{};
    ModelSkinGpuPrepareStats stats{};
    struct Push {std::uint32_t count,stride,position,normal,tangent,binormal;} push{};
    ~Impl() {
        if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
        if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
        if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
        if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);
        if(mapped)vkUnmapMemory(device,memory);
        if(input)vkDestroyBuffer(device,input,nullptr);
        if(memory)vkFreeMemory(device,memory,nullptr);
    }
    bool allocate(std::size_t bytes,PrepareInjection injection,bool retain_mapping) {
        if(bytes<=capacity && injection!=PrepareInjection::create &&
            injection!=PrepareInjection::allocation && injection!=PrepareInjection::device_allocation)return false;
        // Do not retire the old input bank until the complete replacement is
        // bound. A refusal destroys only these private temporary handles.
        struct Replacement {
            VkDevice device;VkBuffer buffer{};VkDeviceMemory memory{};void *mapped{};
            ~Replacement(){if(mapped)vkUnmapMemory(device,memory);if(buffer)vkDestroyBuffer(device,buffer,nullptr);if(memory)vkFreeMemory(device,memory,nullptr);}
        } next{device};
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size=bytes;info.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        const auto created=injection==PrepareInjection::create?VK_ERROR_OUT_OF_HOST_MEMORY:
            vkCreateBuffer(device,&info,nullptr,&next.buffer);
        if(created!=VK_SUCCESS)next.buffer={};
        checked_input_allocation(created);
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device,next.buffer,&requirements);
        VkPhysicalDeviceMemoryProperties types;vkGetPhysicalDeviceMemoryProperties(physical,&types);
        unsigned type=types.memoryTypeCount;
        constexpr auto flags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for(unsigned i=0;i<types.memoryTypeCount;++i)if((requirements.memoryTypeBits&(1u<<i)) &&
            (types.memoryTypes[i].propertyFlags&flags)==flags){type=i;break;}
        if(type==types.memoryTypeCount)throw FormatError("model skin GPU has no coherent input memory");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
        VkResult allocated;
        if(injection==PrepareInjection::allocation)allocated=VK_ERROR_OUT_OF_HOST_MEMORY;
        else if(injection==PrepareInjection::device_allocation)allocated=VK_ERROR_OUT_OF_DEVICE_MEMORY;
        else {++stats.input_allocations;allocated=vkAllocateMemory(device,&allocation,nullptr,&next.memory);}
        if(allocated!=VK_SUCCESS)next.memory={};
        checked_input_allocation(allocated);
        checked(vkBindBufferMemory(device,next.buffer,next.memory,0));
        if(retain_mapping) {
            VkResult result;
            if(injection==PrepareInjection::map)result=VK_ERROR_OUT_OF_HOST_MEMORY;
            else if(injection==PrepareInjection::map_range)result=VK_ERROR_MEMORY_MAP_FAILED;
            else {++stats.input_maps;result=vkMapMemory(device,next.memory,0,bytes,0,&next.mapped);}
            if(result!=VK_SUCCESS)next.mapped=nullptr;
            checked_input_mapping(result);
        }
        // A retained replacement is not published until allocate/bind/map all
        // succeed. Refusal retires only next, leaving the old bank alive.
        if(mapped){vkUnmapMemory(device,memory);++stats.input_unmaps;mapped=nullptr;}
        if(input)vkDestroyBuffer(device,input,nullptr);
        if(memory)vkFreeMemory(device,memory,nullptr);
        input=next.buffer;memory=next.memory;mapped=next.mapped;capacity=bytes;
        next.buffer={};next.memory={};next.mapped=nullptr;
        records_valid=false;descriptors_valid=false;
        return true;
    }
};
ModelSkinGpu::ModelSkinGpu(VkDevice device,VkPhysicalDevice physical,std::uint32_t api,const std::filesystem::path &path)
    :impl_(std::make_unique<Impl>()) {
    if(!supported(physical,api))throw FormatError("model skin GPU lacks explicit float32 RTE/signed-zero controls");
    auto &p=*impl_;p.device=device;p.physical=physical;
    const auto *streamed=std::getenv("HALO2_REFERENCE_STREAMED_SKIN_INPUTS");
    p.streamed=streamed && std::string_view(streamed)=="1";
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);p.limits=properties.limits;
    VkDescriptorSetLayoutBinding bindings[2]{};
    for(unsigned i=0;i<2;++i)bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
    VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=2;sl.pBindings=bindings;
    checked(vkCreateDescriptorSetLayout(device,&sl,nullptr,&p.set_layout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&size;
    checked(vkCreateDescriptorPool(device,&pool,nullptr,&p.pool));
    VkDescriptorSetAllocateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ds.descriptorPool=p.pool;ds.descriptorSetCount=1;ds.pSetLayouts=&p.set_layout;checked(vkAllocateDescriptorSets(device,&ds,&p.set));
    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(Impl::Push)};
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount=1;pl.pSetLayouts=&p.set_layout;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;
    checked(vkCreatePipelineLayout(device,&pl,nullptr,&p.layout));
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || file.tellg()<=0 || std::size_t(file.tellg())%4)throw FormatError("model skin GPU shader is missing or malformed");
    std::vector<std::uint32_t> code(std::size_t(file.tellg())/4);file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("model skin GPU shader is truncated");
    VkShaderModule shader{};VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    sm.codeSize=code.size()*4;sm.pCode=code.data();checked(vkCreateShaderModule(device,&sm,nullptr,&shader));
    VkComputePipelineCreateInfo cp{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};cp.layout=p.layout;
    cp.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};
    const auto result=vkCreateComputePipelines(device,{},1,&cp,nullptr,&p.pipeline);
    vkDestroyShaderModule(device,shader,nullptr);checked(result);
}
ModelSkinGpu::~ModelSkinGpu()=default;
std::size_t ModelSkinGpu::vertices()const{return impl_->count;}
const ModelSkinGpuPrepareStats& ModelSkinGpu::prepare_stats()const{return impl_->stats;}
void ModelSkinGpu::prepare(std::span<const ModelSkinGpuRecord> records,
    std::span<const std::array<float,12>> palette,VkBuffer destination,std::size_t destination_vertices,
    std::uint64_t destination_generation) {
    auto &p=*impl_;p.count=0;p.stats={};
    if(records.empty())return;
    struct FailedPrepare {
        Impl &p;bool complete{};
        ~FailedPrepare(){if(!complete){p.count=0;p.records_valid=false;}}
    } failure{p};
    PrepareEnvironment environment;environment.check();
    if(!destination)throw FormatError("model skin GPU job has no destination buffer");
    // Structural validation cannot allocate: a bad allocation must never turn
    // a malformed late record into a recoverable resource refusal. Real frame
    // mappings are strictly increasing; unordered test/jobs use a bounded
    // pairwise alias check instead of allocating a destination-sized bitmap.
    bool increasing=true;
    // This pass already reads every coefficient. Summarize a bounded prefix
    // once; only the original per-reference gate rejects finite subnormals.
    // All row classification is integer-only and requires no heap allocation.
    std::array<std::uint64_t,64> normal_rows{};
    for(std::size_t row=0;row<palette.size();++row) {
        bool normal=true;
        for(auto value:palette[row]) {
            const auto bits=model_skin_admission_detail::absolute_bits(value);
            if(bits>0x49800000u)throw FormatError("model skin GPU palette leaves its finite arithmetic domain");
            normal=normal && model_skin_admission_detail::normal_bounded(bits,0x49800000u);
        }
        if(row<normal_rows.size()*64 && normal)
            normal_rows[row/64]|=std::uint64_t{1}<<(row%64);
    }
    for(std::size_t i=0;i<records.size();++i) {
        const auto &record=records[i];
        if(record.post_identity_placement>1)
            throw FormatError("model skin GPU has an unknown post-placement mode");
        if(record.destination>=destination_vertices)
            throw FormatError("model skin GPU destinations escape or alias different jobs");
        if(i && records[i-1].destination>=record.destination)increasing=false;
        if(record.palette>palette.size() || record.palette_count>palette.size()-record.palette)
            throw FormatError("model skin GPU palette base escapes its bank");
        int last=-1;
        for(unsigned j=0;j<4;++j) {
            if(!model_skin_admission_detail::bounded(record.weights[j],0x00800000u,0x3f800000u))throw FormatError("model skin GPU weight leaves its finite arithmetic domain");
            if(std::bit_cast<std::uint32_t>(record.weights[j])&0x7fffffffu)last=int(j);
        }
        for(int j=last;j>=0;--j) {
            const auto bone=record.bones[unsigned(j)];
            if(bone>std::numeric_limits<std::uint16_t>::max() || bone>=record.palette_count)
                throw FormatError("model skin GPU bone escapes its completed palette");
            // Unused rows may contain finite subnormals; active and interior
            // zero influences still read every referenced original row.
            const auto row=std::size_t(record.palette)+bone;
            if(row<normal_rows.size()*64) {
                if(!(normal_rows[row/64]&(std::uint64_t{1}<<(row%64))))
                    throw FormatError("model skin GPU referenced palette leaves its normal arithmetic domain");
            } else for(const auto value:palette[row])
                if(!model_skin_admission_detail::normal_bounded(
                    model_skin_admission_detail::absolute_bits(value),0x49800000u))
                    throw FormatError("model skin GPU referenced palette leaves its normal arithmetic domain");
        }
    }
    if(!increasing)for(std::size_t i=0;i<records.size();++i)for(std::size_t j=0;j<i;++j)
        if(records[i].destination==records[j].destination)
            throw FormatError("model skin GPU destinations escape or alias different jobs");
    const auto injection=prepare_injection();
    const auto max_words=p.limits.maxStorageBufferRange/4u;
    if(records.size()>max_words/11 || palette.size()>(max_words-records.size()*11)/12 ||
       destination_vertices>p.limits.maxStorageBufferRange/sizeof(Vertex) ||
       records.size()>std::size_t(p.limits.maxComputeWorkGroupCount[0])*64 || injection==PrepareInjection::quota)
        throw ModelSkinGpuPrepareRefused(ModelSkinGpuPrepareRefusalReason::device_quota);
    if(injection==PrepareInjection::device_lost)checked(VK_ERROR_DEVICE_LOST);
    const auto record_bytes=records.size()*44;
    const auto input_bytes=record_bytes+palette.size_bytes();
    if(p.streamed) {
        // Constructor-selected counterfactual: preserve the original complete
        // CPU pack, map/copy/unmap and descriptor update on every nonempty job.
        std::vector<std::uint32_t> words;
        try {
            if(injection==PrepareInjection::packing)throw std::bad_alloc{};
            words.resize(records.size()*11+palette.size()*12);
        }
        catch(const std::bad_alloc&) {throw ModelSkinGpuPrepareRefused(ModelSkinGpuPrepareRefusalReason::input_allocation);}
        for(std::size_t i=0;i<records.size();++i) {
            std::memcpy(words.data()+i*11,&records[i],44);
            words[i*11+1]=std::uint32_t(records.size()*11+std::size_t(records[i].palette)*12);
        }
        p.stats.record_pack_bytes=record_bytes;
        if(!palette.empty())std::memcpy(words.data()+records.size()*11,palette.data(),palette.size_bytes());
        p.allocate(input_bytes,injection,false);
        void *mapped{};VkResult mapped_result;
        if(injection==PrepareInjection::map)mapped_result=VK_ERROR_OUT_OF_HOST_MEMORY;
        else if(injection==PrepareInjection::map_range)mapped_result=VK_ERROR_MEMORY_MAP_FAILED;
        else {++p.stats.input_maps;mapped_result=vkMapMemory(p.device,p.memory,0,input_bytes,0,&mapped);}
        checked_input_mapping(mapped_result);
        std::memcpy(mapped,words.data(),input_bytes);
        p.stats.record_upload_bytes=record_bytes;p.stats.palette_upload_bytes=palette.size_bytes();
        vkUnmapMemory(p.device,p.memory);++p.stats.input_unmaps;
    } else {
        // Never read the mapped bank for equality: coherent memory need not be
        // host-cached. This owned full-record shadow also includes the original
        // palette_count, while every current record is validated above afresh.
        const bool same_records=p.records_valid && p.records.size()==records.size() &&
            p.palette_rows==palette.size() &&
            !std::memcmp(p.records.data(),records.data(),records.size_bytes());
        try {
            // Fault controls remain effective even on a fully resident hit.
            if(injection==PrepareInjection::packing)throw std::bad_alloc{};
            if(!same_records)p.records.resize(records.size());
        }
        catch(const std::bad_alloc&) {throw ModelSkinGpuPrepareRefused(ModelSkinGpuPrepareRefusalReason::input_allocation);}
        const bool replaced=p.allocate(input_bytes,injection,true);
        // Growth injects at the real replacement map stage. A warm bank is
        // already mapped, so refuse without illegally mapping that memory twice.
        if(injection==PrepareInjection::map)checked_input_mapping(VK_ERROR_OUT_OF_HOST_MEMORY);
        if(injection==PrepareInjection::map_range)checked_input_mapping(VK_ERROR_MEMORY_MAP_FAILED);
        if(!p.mapped)throw FormatError("model skin GPU retained input mapping is absent");
        auto *mapped=static_cast<std::byte*>(p.mapped);
        if(!same_records || replaced) {
            p.records_valid=false;
            for(std::size_t i=0;i<records.size();++i) {
                // GPU records contain eleven words; the twelfth host word is
                // the already checked owner extent, not a shader input.
                auto *output=mapped+i*44;
                std::memcpy(output,&records[i],44);
                const auto palette_word=std::uint32_t(records.size()*11+std::size_t(records[i].palette)*12);
                std::memcpy(output+4,&palette_word,sizeof(palette_word));
            }
            p.stats.record_pack_bytes=record_bytes;p.stats.record_upload_bytes=record_bytes;
            std::memcpy(p.records.data(),records.data(),records.size_bytes());
        } else ++p.stats.record_cache_hits;
        if(!palette.empty())std::memcpy(mapped+record_bytes,palette.data(),palette.size_bytes());
        p.stats.palette_upload_bytes=palette.size_bytes();
    }
    const auto destination_bytes=destination_vertices*sizeof(Vertex);
    if(p.streamed || !destination_generation || !p.descriptors_valid || p.descriptor_input!=p.input ||
       p.descriptor_destination!=destination || p.descriptor_input_bytes!=input_bytes ||
       p.descriptor_destination_bytes!=destination_bytes ||
       p.descriptor_destination_generation!=destination_generation) {
        VkDescriptorBufferInfo buffers[2]{{p.input,0,input_bytes},{destination,0,destination_bytes}};
        VkWriteDescriptorSet writes[2]{};
        for(unsigned i=0;i<2;++i) {
            writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=p.set;
            writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i].pBufferInfo=&buffers[i];
        }
        vkUpdateDescriptorSets(p.device,2,writes,0,nullptr);p.stats.descriptor_writes+=2;
        p.descriptor_input=p.input;p.descriptor_destination=destination;
        p.descriptor_input_bytes=input_bytes;p.descriptor_destination_bytes=destination_bytes;
        p.descriptor_destination_generation=destination_generation;
        p.descriptors_valid=true;
    }
    static_assert(sizeof(Vertex)%4==0 && offsetof(Vertex,position)%4==0 && offsetof(Vertex,normal)%4==0 &&
                  offsetof(Vertex,tangent)%4==0 && offsetof(Vertex,binormal)%4==0);
    p.push={std::uint32_t(records.size()),sizeof(Vertex)/4,offsetof(Vertex,position)/4,offsetof(Vertex,normal)/4,
            offsetof(Vertex,tangent)/4,offsetof(Vertex,binormal)/4};
    p.palette_rows=palette.size();p.records_valid=!p.streamed;
    p.count=records.size();failure.complete=true;
}
void ModelSkinGpu::record(VkCommandBuffer command)const {
    const auto &p=*impl_;if(!p.count)return;
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,p.pipeline);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,p.layout,0,1,&p.set,0,nullptr);
    vkCmdPushConstants(command,p.layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(p.push),&p.push);
    vkCmdDispatch(command,std::uint32_t((p.count+63)/64),1,1);
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
}
}
