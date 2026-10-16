#include "halo2/source_lens_query_gpu.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <cstring>
#include <cmath>
#include <fstream>
#include <map>
#include <set>

namespace halo2 {
namespace {
void checked(VkResult result) {
    if(result!=VK_SUCCESS)throw FormatError("Lens query Vulkan failure: "+std::to_string(result));
}
VkShaderModule shader(VkDevice device,const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file)throw FormatError("Lens query shader is absent: "+path.string());
    const std::streamsize size=file.tellg();
    if(size<=0 || size%4)throw FormatError("Lens query shader has invalid word storage");
    std::vector<std::uint32_t> words(std::size_t(size)/4);file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(words.data()),size))throw FormatError("Lens query shader read failed");
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize=std::size_t(size);info.pCode=words.data();VkShaderModule result{};
    checked(vkCreateShaderModule(device,&info,nullptr,&result));return result;
}
}
struct SourceLensQueryGpu::Impl {
    SourceLensQueryGpuContext context;
    VkQueryPool samples{},timestamps{};
    VkPipelineLayout layout{};
    VkPipeline pipeline{};
    VkBuffer buffer{};VkDeviceMemory memory{};
    std::vector<SourceLensQueryDraw> prepared;
    std::map<std::uint32_t,std::uint64_t> pending,staged;
    std::map<std::uint32_t,unsigned> first;
    bool reset_recorded{};
    explicit Impl(SourceLensQueryGpuContext c):context(std::move(c)) {}
    void initialize() {
        if(!context.physical || !context.device || !context.render_pass || !context.precise_queries_enabled)
            throw FormatError("Lens queries require a real device/pass with precise query support enabled");
        std::uint32_t count{};vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,families.data());
        if(context.queue_family>=count || !families[context.queue_family].timestampValidBits ||
           !(families[context.queue_family].queueFlags&VK_QUEUE_GRAPHICS_BIT))
            throw FormatError("Lens query queue lacks graphics/timestamp support");
        VkQueryPoolCreateInfo query{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        query.queryCount=source_lens_query_capacity;query.queryType=VK_QUERY_TYPE_OCCLUSION;
        checked(vkCreateQueryPool(context.device,&query,nullptr,&samples));
        query.queryType=VK_QUERY_TYPE_TIMESTAMP;
        checked(vkCreateQueryPool(context.device,&query,nullptr,&timestamps));
        VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT,0,8};
        VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        li.pushConstantRangeCount=1;li.pPushConstantRanges=&push;
        checked(vkCreatePipelineLayout(context.device,&li,nullptr,&layout));
        const auto vertex=shader(context.device,context.shader_directory/"source_lens_query.vert.spv");
        VkShaderModule fragment{};
        try {
            fragment=shader(context.device,context.shader_directory/"source_lens_query.frag.spv");
            std::array<VkPipelineShaderStageCreateInfo,2> stages{};
            for(auto &stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}
            stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vertex;
            stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fragment;
            VkVertexInputBindingDescription binding{0,16,VK_VERTEX_INPUT_RATE_VERTEX};
            VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
            VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
            input.vertexAttributeDescriptionCount=1;input.pVertexAttributeDescriptions=&attribute;
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport.viewportCount=1;viewport.scissorCount=1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo multi{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            multi.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            depth.depthTestEnable=VK_TRUE;depth.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
            VkPipelineColorBlendAttachmentState attachment{};
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.attachmentCount=1;blend.pAttachments=&attachment;
            std::array<VkDynamicState,2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            dynamic.dynamicStateCount=dynamic_states.size();dynamic.pDynamicStates=dynamic_states.data();
            VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            info.stageCount=stages.size();info.pStages=stages.data();info.pVertexInputState=&input;
            info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;info.pRasterizationState=&raster;
            info.pMultisampleState=&multi;info.pDepthStencilState=&depth;info.pColorBlendState=&blend;
            info.pDynamicState=&dynamic;info.layout=layout;info.renderPass=context.render_pass;
            checked(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline));
        } catch(...) {if(fragment)vkDestroyShaderModule(context.device,fragment,nullptr);vkDestroyShaderModule(context.device,vertex,nullptr);throw;}
        vkDestroyShaderModule(context.device,fragment,nullptr);vkDestroyShaderModule(context.device,vertex,nullptr);
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size=source_lens_query_capacity*4*16;bi.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        checked(vkCreateBuffer(context.device,&bi,nullptr,&buffer));
        VkMemoryRequirements needs{};vkGetBufferMemoryRequirements(context.device,buffer,&needs);
        VkPhysicalDeviceMemoryProperties properties{};vkGetPhysicalDeviceMemoryProperties(context.physical,&properties);
        unsigned type=properties.memoryTypeCount;
        for(unsigned i=0;i<properties.memoryTypeCount;++i)
            if((needs.memoryTypeBits&(1u<<i)) && (properties.memoryTypes[i].propertyFlags&
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
        if(type==properties.memoryTypeCount)throw FormatError("Lens query vertices lack coherent native backing");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize=needs.size;allocation.memoryTypeIndex=type;
        checked(vkAllocateMemory(context.device,&allocation,nullptr,&memory));
        checked(vkBindBufferMemory(context.device,buffer,memory,0));
    }
    ~Impl() {
        if(context.device)vkDeviceWaitIdle(context.device);
        if(buffer)vkDestroyBuffer(context.device,buffer,nullptr);
        if(memory)vkFreeMemory(context.device,memory,nullptr);
        if(pipeline)vkDestroyPipeline(context.device,pipeline,nullptr);
        if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);
        if(timestamps)vkDestroyQueryPool(context.device,timestamps,nullptr);
        if(samples)vkDestroyQueryPool(context.device,samples,nullptr);
    }
    std::vector<SourceLensQueryResult> completed() {
        std::vector<SourceLensQueryResult> out;
        for(auto i=pending.begin();i!=pending.end();) {
            std::array<std::uint64_t,2> count{},time{};
            const auto flags=VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;
            const auto a=vkGetQueryPoolResults(context.device,samples,i->first,1,sizeof(count),count.data(),sizeof(count),flags);
            const auto b=vkGetQueryPoolResults(context.device,timestamps,i->first,1,sizeof(time),time.data(),sizeof(time),flags);
            if(a!=VK_SUCCESS && a!=VK_NOT_READY)checked(a);
            if(b!=VK_SUCCESS && b!=VK_NOT_READY)checked(b);
            if(!count[1] || !time[1]){++i;continue;}
            out.push_back({i->first,i->second,count[0],time[0]});i=pending.erase(i);
        }
        return out;
    }
};
SourceLensQueryGpu::SourceLensQueryGpu(SourceLensQueryGpuContext context):impl_(std::make_unique<Impl>(std::move(context))){impl_->initialize();}
SourceLensQueryGpu::~SourceLensQueryGpu()=default;
void SourceLensQueryGpu::prepare(std::span<const SourceLensQueryDraw> draws) {
    auto &s=*impl_;std::set<std::uint32_t> seen;
    if(!s.staged.empty())throw FormatError("Lens query prepare would discard an unsubmitted recording");
    for(const auto &draw:draws) {
        if(draw.id>=source_lens_query_capacity || !seen.insert(draw.id).second || s.pending.contains(draw.id))
            throw FormatError("Lens query ID is invalid, duplicated or still in flight");
        if(!draw.generation)throw FormatError("Lens query requires its actual request generation");
        if(draw.has_quad)for(const auto &v:draw.vertices) {
            if(!(v[3]>0))throw FormatError("Lens query vertex requires positive original W");
            for(float value:v)if(!std::isfinite(value))throw FormatError("Lens query vertex is not finite");
        }
    }
    s.prepared.assign(draws.begin(),draws.end());s.first.clear();s.reset_recorded=false;
    if(draws.empty())return;
    void *mapped{};checked(vkMapMemory(s.context.device,s.memory,0,VK_WHOLE_SIZE,0,&mapped));
    unsigned first{};
    for(const auto &draw:draws){s.first.emplace(draw.id,first);std::memcpy(static_cast<std::byte*>(mapped)+first*16,draw.vertices.data(),64);first+=4;}
    vkUnmapMemory(s.context.device,s.memory);
}
void SourceLensQueryGpu::reset(VkCommandBuffer command) {
    auto &s=*impl_;if(!command || s.reset_recorded)throw FormatError("Lens query reset lacks a new recording");
    for(const auto &draw:s.prepared){vkCmdResetQueryPool(command,s.samples,draw.id,1);vkCmdResetQueryPool(command,s.timestamps,draw.id,1);}
    s.reset_recorded=true;
}
void SourceLensQueryGpu::record(VkCommandBuffer command,std::span<const SourceLensQueryDraw> draws,VkExtent2D extent,VkRect2D clip) {
    auto &s=*impl_;if(!command || !s.reset_recorded || !extent.width || !extent.height)
        throw FormatError("Lens query draw lacks reset/command/surface inputs");
    const VkViewport viewport{0,0,float(extent.width),float(extent.height),0,1};
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,s.pipeline);
    vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&clip);
    const std::array<float,2> dimensions{float(extent.width),float(extent.height)};
    vkCmdPushConstants(command,s.layout,VK_SHADER_STAGE_VERTEX_BIT,0,8,dimensions.data());
    VkDeviceSize offset{};vkCmdBindVertexBuffers(command,0,1,&s.buffer,&offset);
    for(const auto &draw:draws) {
        const auto first=s.first.find(draw.id);
        if(first==s.first.end() || s.pending.contains(draw.id) || s.staged.contains(draw.id))throw FormatError("Lens query draw escaped its prepared identity");
        const auto &saved=s.prepared.at(first->second/4);
        if(saved.generation!=draw.generation || saved.has_quad!=draw.has_quad ||
           std::memcmp(saved.vertices.data(),draw.vertices.data(),64))
            throw FormatError("Lens query changed after its prepared vertex upload");
        vkCmdBeginQuery(command,s.samples,draw.id,VK_QUERY_CONTROL_PRECISE_BIT);
        if(draw.has_quad)vkCmdDraw(command,4,1,first->second,0);
        vkCmdEndQuery(command,s.samples,draw.id);
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,s.timestamps,draw.id);
        s.staged.emplace(draw.id,draw.generation);
    }
}
void SourceLensQueryGpu::submitted() {
    auto &s=*impl_;if(!s.reset_recorded)throw FormatError("Lens query submission has no prepared recording");
    s.pending.merge(s.staged);s.reset_recorded=false;
}
void SourceLensQueryGpu::discard_unsubmitted() {
    auto &s=*impl_;s.staged.clear();s.prepared.clear();s.first.clear();s.reset_recorded=false;
}
std::vector<SourceLensQueryResult> SourceLensQueryGpu::completed(){return impl_->completed();}
}
