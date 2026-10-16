#include "halo2/pstencil/render.hpp"
#include "halo2/pstencil/shadow.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>

namespace halo2::pstencil {
namespace {
void check(VkResult result,const char* operation) {
    if(result!=VK_SUCCESS)throw FormatError(std::string("pStencil ")+operation+": "+std::to_string(result));
}
}
struct Render::Impl {
    RenderContext context;
    VkImage depth{};VkDeviceMemory depth_memory{};VkImageView depth_view{};
    VkBuffer buffer{};VkDeviceMemory buffer_memory{};void* mapped{};
    VkDeviceSize capacity{};
    VkPipelineLayout layout{};VkPipeline pipeline{};
    VkPipelineLayout lightmap_layout{};VkPipeline lightmap_pipeline{};
    std::vector<VkFramebuffer> framebuffers;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> batches;
    ~Impl() {
        const auto device=context.device;
        if(mapped)vkUnmapMemory(device,buffer_memory);
        if(buffer)vkDestroyBuffer(device,buffer,nullptr);
        if(buffer_memory)vkFreeMemory(device,buffer_memory,nullptr);
        if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
        if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
        if(lightmap_pipeline)vkDestroyPipeline(device,lightmap_pipeline,nullptr);
        if(lightmap_layout)vkDestroyPipelineLayout(device,lightmap_layout,nullptr);
        for(auto framebuffer:framebuffers)vkDestroyFramebuffer(device,framebuffer,nullptr);
        if(depth_view)vkDestroyImageView(device,depth_view,nullptr);
        if(depth)vkDestroyImage(device,depth,nullptr);
        if(depth_memory)vkFreeMemory(device,depth_memory,nullptr);
    }
    unsigned memory_type(std::uint32_t mask,VkMemoryPropertyFlags flags) const {
        VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(context.physical,&properties);
        for(unsigned i=0;i<properties.memoryTypeCount;++i)
            if((mask&(1u<<i)) && (properties.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("pStencil suitable Vulkan memory type unavailable");
    }
    VkShaderModule shader(const char* name) const {
        auto directory=context.shaders;if(directory.empty())directory=HALO2_PSTENCIL_SHADER_DIR;
        std::ifstream input(directory/"pstencil"/name,std::ios::binary|std::ios::ate);
        const auto length=input.tellg();
        if(length<=0 || length%4)throw FormatError("pStencil shader unavailable");
        std::vector<std::uint32_t> words(std::size_t(length)/4);input.seekg(0);
        if(!input.read(reinterpret_cast<char*>(words.data()),length))throw FormatError("pStencil shader truncated");
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=std::size_t(length);info.pCode=words.data();
        VkShaderModule result{};check(vkCreateShaderModule(context.device,&info,nullptr,&result),"create shader");return result;
    }
    void create_lightmap() {
        const auto device=context.device;
        VkPushConstantRange range{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(ShaderVector)};
        VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.pushConstantRangeCount=1;layout_info.pPushConstantRanges=&range;
        check(vkCreatePipelineLayout(device,&layout_info,nullptr,&lightmap_layout),"create lightmap composite layout");
        const auto vertex=shader("lightmap.vert.spv"),fragment=shader("lightmap.frag.spv");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment,"main",nullptr}};
        VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.stencilTestEnable=VK_TRUE;
        depth.front={VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_COMPARE_OP_NOT_EQUAL,255,0,shadow_stencil_clear};
        depth.back=depth.front;
        VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=15;attachment.blendEnable=VK_TRUE;
        attachment.srcColorBlendFactor=VK_BLEND_FACTOR_ZERO;attachment.dstColorBlendFactor=VK_BLEND_FACTOR_SRC_COLOR;
        attachment.colorBlendOp=VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;attachment.dstAlphaBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.alphaBlendOp=VK_BLEND_OP_ADD;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};info.stageCount=2;info.pStages=stages;
        info.pVertexInputState=&vertices;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;
        info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.pDepthStencilState=&depth;
        info.pColorBlendState=&blend;info.pDynamicState=&dynamic;info.layout=lightmap_layout;info.renderPass=context.continuation;
        const auto result=vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&lightmap_pipeline);
        vkDestroyShaderModule(device,fragment,nullptr);vkDestroyShaderModule(device,vertex,nullptr);
        check(result,"create original lightmap-shadow multiply pipeline");
    }
    void create() {
        const auto device=context.device;
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};image.imageType=VK_IMAGE_TYPE_2D;
        image.format=VK_FORMAT_D24_UNORM_S8_UINT;image.extent={context.extent.width,context.extent.height,1};
        image.mipLevels=image.arrayLayers=1;image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;
        image.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        check(vkCreateImage(device,&image,nullptr,&depth),"create private depth/stencil");
        VkMemoryRequirements requirements;vkGetImageMemoryRequirements(device,depth,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&allocation,nullptr,&depth_memory),"allocate private depth/stencil");
        check(vkBindImageMemory(device,depth,depth_memory,0),"bind private depth/stencil");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=depth;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
        view.format=image.format;view.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
        check(vkCreateImageView(device,&view,nullptr,&depth_view),"create private attachment view");
        for(auto color:context.scene_views) {
            const std::array attachments{color,depth_view};
            VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};info.renderPass=context.continuation;
            info.attachmentCount=2;info.pAttachments=attachments.data();info.width=context.extent.width;
            info.height=context.extent.height;info.layers=1;VkFramebuffer framebuffer{};
            check(vkCreateFramebuffer(device,&info,nullptr,&framebuffer),"create private framebuffer");framebuffers.push_back(framebuffer);
        }
        VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(VolumeCamera)};
        VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.pushConstantRangeCount=1;layout_info.pPushConstantRanges=&range;
        check(vkCreatePipelineLayout(device,&layout_info,nullptr,&layout),"create volume layout");
        const auto vertex=shader("volume.vert.spv");VkShaderModule fragment{};
        try {
            fragment=shader("volume.frag.spv");
            const VkPipelineShaderStageCreateInfo stages[]{
                {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
                {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment,"main",nullptr}};
            const VkVertexInputBindingDescription binding{0,sizeof(ShadowVertex),VK_VERTEX_INPUT_RATE_VERTEX};
            const VkVertexInputAttributeDescription attributes[]{
                {0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},
                {1,0,VK_FORMAT_R32_UINT,offsetof(ShadowVertex,stipple_level)}};
            VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            vertices.vertexBindingDescriptionCount=1;vertices.vertexAttributeDescriptionCount=2;
            vertices.pVertexBindingDescriptions=&binding;vertices.pVertexAttributeDescriptions=attributes;
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster.depthClampEnable=VK_TRUE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;
            raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            stencil.depthTestEnable=stencil.stencilTestEnable=VK_TRUE;stencil.depthCompareOp=VK_COMPARE_OP_LESS;
            stencil.front={VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_STENCIL_OP_DECREMENT_AND_WRAP,VK_COMPARE_OP_ALWAYS,255,255,1};
            stencil.back=stencil.front;stencil.back.depthFailOp=VK_STENCIL_OP_INCREMENT_AND_WRAP;
            VkPipelineColorBlendAttachmentState attachment{}; // Original color-write0.
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
            const VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamic_states;
            VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};info.stageCount=2;info.pStages=stages;
            info.pVertexInputState=&vertices;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;
            info.pRasterizationState=&raster;info.pMultisampleState=&samples;info.pDepthStencilState=&stencil;
            info.pColorBlendState=&blend;info.pDynamicState=&dynamic;info.layout=layout;info.renderPass=context.continuation;
            check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline),"create capped-volume pipeline");
        } catch(...) {if(fragment)vkDestroyShaderModule(device,fragment,nullptr);vkDestroyShaderModule(device,vertex,nullptr);throw;}
        vkDestroyShaderModule(device,fragment,nullptr);vkDestroyShaderModule(device,vertex,nullptr);
        create_lightmap();
    }
};
Render::Render(RenderContext context):impl_(std::make_unique<Impl>()) {
    impl_->context=context;impl_->create();
}
Render::~Render()=default;
void Render::prepare(std::span<const VolumeBatch> input) {
    auto& p=*impl_;p.batches.clear();std::uint64_t count{};
    for(const auto& batch:input) {
        if(batch.vertices.size()%3 || count+batch.vertices.size()>std::numeric_limits<std::uint32_t>::max())
            throw FormatError("pStencil volume triangle bank exceeds draw limits");
        p.batches.emplace_back(std::uint32_t(count),std::uint32_t(batch.vertices.size()));count+=batch.vertices.size();
    }
    const auto required=VkDeviceSize(count)*sizeof(ShadowVertex);
    if(required>p.capacity) {
        if(p.mapped)vkUnmapMemory(p.context.device,p.buffer_memory);
        p.mapped=nullptr;
        if(p.buffer)vkDestroyBuffer(p.context.device,p.buffer,nullptr);
        p.buffer=VK_NULL_HANDLE;
        if(p.buffer_memory)vkFreeMemory(p.context.device,p.buffer_memory,nullptr);
        p.buffer_memory=VK_NULL_HANDLE;
        p.capacity=std::max(required,p.capacity*2);
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=p.capacity;info.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        check(vkCreateBuffer(p.context.device,&info,nullptr,&p.buffer),"create volume buffer");
        VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(p.context.device,p.buffer,&requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
        allocation.memoryTypeIndex=p.memory_type(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(p.context.device,&allocation,nullptr,&p.buffer_memory),"allocate volume buffer");
        check(vkBindBufferMemory(p.context.device,p.buffer,p.buffer_memory,0),"bind volume buffer");
        check(vkMapMemory(p.context.device,p.buffer_memory,0,p.capacity,0,&p.mapped),"map volume buffer");
    }
    for(std::size_t i=0;i<input.size();++i)if(!input[i].vertices.empty())
        std::memcpy(static_cast<std::byte*>(p.mapped)+std::size_t(p.batches[i].first)*sizeof(ShadowVertex),
            input[i].vertices.data(),input[i].vertices.size_bytes());
}
void Render::begin(VkCommandBuffer command,unsigned image) {
    auto& p=*impl_;vkCmdEndRenderPass(command);
    std::array<VkImageMemoryBarrier,2> barriers{};
    for(auto& barrier:barriers) {
        barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    barriers[0].image=p.context.original_depth;barriers[0].oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    barriers[1].image=p.depth;barriers[1].oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers.data());
    VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};
    copy.extent={p.context.extent.width,p.context.extent.height,1};
    vkCmdCopyImage(command,p.context.original_depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,p.depth,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    const VkClearDepthStencilValue clear{1,shadow_stencil_clear};
    const VkImageSubresourceRange stencil{VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
    vkCmdClearDepthStencilImage(command,p.depth,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&stencil);
    for(auto& barrier:barriers) {
        std::swap(barrier.oldLayout,barrier.newLayout);std::swap(barrier.srcAccessMask,barrier.dstAccessMask);
        barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,2,barriers.data());
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=p.context.continuation;
    begin.framebuffer=p.framebuffers.at(image);begin.renderArea={{0,0},p.context.extent};
    vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
}
void Render::light(VkCommandBuffer command,std::size_t batch,const VolumeCamera& camera,VkViewport viewport,VkRect2D clip,
    const std::function<void()>& receivers) {
    const auto& p=*impl_;const auto [first,count]=p.batches.at(batch);
    VkClearAttachment clear{};clear.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;clear.clearValue.depthStencil.stencil=shadow_stencil_clear;
    const VkClearRect area{clip,0,1};vkCmdClearAttachments(command,1,&clear,1,&area);
    if(count) {
        vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&clip);
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,p.pipeline);
        constexpr VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&p.buffer,&offset);
        vkCmdPushConstants(command,p.layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(camera),&camera);
        vkCmdDraw(command,count,1,first,0);
    }
    receivers();
}
void Render::lightmap(VkCommandBuffer command,std::span<const VolumeDraw> draws,VkViewport viewport,VkRect2D clip) {
    const auto& p=*impl_;
    VkClearAttachment clear{};clear.aspectMask=VK_IMAGE_ASPECT_STENCIL_BIT;clear.clearValue.depthStencil.stencil=shadow_stencil_clear;
    const VkClearRect area{clip,0,1};vkCmdClearAttachments(command,1,&clear,1,&area);
    vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&clip);
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,p.pipeline);
    constexpr VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&p.buffer,&offset);
    for(const auto& draw:draws) {
        const auto [first,count]=p.batches.at(draw.batch);
        vkCmdPushConstants(command,p.layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(draw.camera),&draw.camera);
        vkCmdDraw(command,count,1,first,0);
    }
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,p.lightmap_pipeline);
    constexpr float rgb=float(lightmap_shadow_rgb)/255.f;
    const ShaderVector color{rgb,rgb,rgb,1};
    vkCmdPushConstants(command,p.lightmap_layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(color),color.data());
    vkCmdDraw(command,3,1,0,0);
}
void Render::end(VkCommandBuffer command,VkFramebuffer original) {
    const auto& p=*impl_;vkCmdEndRenderPass(command);
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=p.context.continuation;
    begin.framebuffer=original;begin.renderArea={{0,0},p.context.extent};vkCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
}
} // namespace halo2::pstencil
