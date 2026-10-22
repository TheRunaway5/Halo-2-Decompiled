#include "halo2/source_shadow_render.hpp"
#include "halo2/macos/bundle.hpp"
#include "halo2/source_shadow_plan.hpp"
#include "halo2/source_shadow_filter.hpp"
#include <cmath>
#include <cstring>
#include <fstream>

namespace halo2 {
namespace {
void check(VkResult result,const char *message) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(message)+": Vulkan "+std::to_string(result));
}
void check_draw(const SourceShadowDraw &draw,bool receiver) {
    if(!draw.authored || !draw.bind_and_draw)throw FormatError("shadow draw has no authored pass or commands");
    validate_source_shadow_plan_material(*draw.authored,
        receiver?SourceShadowPhase::Apply:SourceShadowPhase::Generate);
}
void check_raw_draws(std::span<const SourceShadowRawCallback> draws,
    SourceShadowPhase phase,const SourceShadowRawSubmission &root) {
    const SourceShadowRawCommandTu15 *previous{};
    for(const auto &draw:draws) {
        if(!draw.source || !draw.bind_and_draw)
            throw FormatError("raw shadow draw has no admitted source binding or commands");
        validate_source_shadow_raw_draw(*draw.source,phase,root.uniforms);
        const auto &command=draw.source->command;
        if(command.root_owner!=root.root_owner)
            throw FormatError("raw shadow draw belongs to another admitted root");
        if(previous && (command.sorted_ordinal<previous->sorted_ordinal ||
           (command.sorted_ordinal==previous->sorted_ordinal &&
            (command.descriptor!=previous->descriptor || command.command!=previous->command ||
             command.unit!=previous->unit || command.geometry_owner!=previous->geometry_owner))))
            throw FormatError("raw shadow descriptor/range order changed");
        previous=&command;
    }
}
}
struct SourceShadowRender::Impl {
    SourceShadowRenderContext context;
    VkImage image{};VkDeviceMemory memory{};VkImageView view{};
    VkRenderPass pass{};VkFramebuffer framebuffer{};
    VkImage scratch_image{};VkDeviceMemory scratch_memory{};VkImageView scratch_view{};
    VkRenderPass filter_pass{};VkFramebuffer scratch_framebuffer{};
    VkDescriptorSetLayout filter_set_layout{};VkPipelineLayout filter_layout{};
    VkDescriptorPool filter_pool{};VkPipeline filter_pipeline{};VkSampler filter_sampler{};
    std::array<VkDescriptorSet,2> filter_sets{};
    std::array<VkBuffer,2> filter_buffers{};
    std::array<VkDeviceMemory,2> filter_memories{};
    explicit Impl(SourceShadowRenderContext c):context(c) {
        if(!context.device || !context.physical || !context.continuation_pass ||
           !context.scene_extent.width || !context.scene_extent.height)
            throw FormatError("shadow rendering needs an actual Vulkan scene context");
        try {initialize();}catch(...) {release();throw;}
    }
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        VkPhysicalDeviceMemoryProperties props;vkGetPhysicalDeviceMemoryProperties(context.physical,&props);
        for(unsigned i=0;i<props.memoryTypeCount;++i)
            if((bits&(1u<<i)) && (props.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("shadow target device memory unavailable");
    }
    void initialize() {
        VkFormatProperties properties;vkGetPhysicalDeviceFormatProperties(context.physical,target_format,&properties);
        constexpr auto features=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if((properties.optimalTilingFeatures&features)!=features)throw FormatError("source shadow target format unsupported");
        VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=target_format;
        i.extent={target_width,target_height,1};i.mipLevels=i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;
        i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        check(vkCreateImage(context.device,&i,nullptr,&image),"shadow target image");
        VkMemoryRequirements req;vkGetImageMemoryRequirements(context.device,image,&req);
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=memory_type(req.memoryTypeBits);
        check(vkAllocateMemory(context.device,&a,nullptr,&memory),"shadow target memory");
        check(vkBindImageMemory(context.device,image,memory,0),"shadow target binding");
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=target_format;
        v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        check(vkCreateImageView(context.device,&v,nullptr,&view),"shadow target view");
        VkAttachmentDescription color{};color.format=target_format;color.samples=VK_SAMPLE_COUNT_1_BIT;
        color.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;color.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;color.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;color.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&ref;
        const VkSubpassDependency dependencies[]{
            {VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
             VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT},
            {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT}};
        VkRenderPassCreateInfo p{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};p.attachmentCount=1;p.pAttachments=&color;
        p.subpassCount=1;p.pSubpasses=&sub;p.dependencyCount=2;p.pDependencies=dependencies;
        check(vkCreateRenderPass(context.device,&p,nullptr,&pass),"shadow generation pass");
        VkFramebufferCreateInfo f{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};f.renderPass=pass;f.attachmentCount=1;f.pAttachments=&view;
        f.width=target_width;f.height=target_height;f.layers=1;
        check(vkCreateFramebuffer(context.device,&f,nullptr,&framebuffer),"shadow generation framebuffer");
        check(vkCreateImage(context.device,&i,nullptr,&scratch_image),"shadow scratch11 image");
        vkGetImageMemoryRequirements(context.device,scratch_image,&req);
        a.allocationSize=req.size;a.memoryTypeIndex=memory_type(req.memoryTypeBits);
        check(vkAllocateMemory(context.device,&a,nullptr,&scratch_memory),"shadow scratch11 memory");
        check(vkBindImageMemory(context.device,scratch_image,scratch_memory,0),"shadow scratch11 binding");
        v.image=scratch_image;check(vkCreateImageView(context.device,&v,nullptr,&scratch_view),"shadow scratch11 view");
        color.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        check(vkCreateRenderPass(context.device,&p,nullptr,&filter_pass),"shadow filter pass");
        f.renderPass=filter_pass;f.pAttachments=&scratch_view;
        check(vkCreateFramebuffer(context.device,&f,nullptr,&scratch_framebuffer),"shadow scratch11 framebuffer");
        make_filter();
    }
    VkShaderModule shader(const char *name) {
        auto directory=context.shader_directory;
        if(directory.empty())directory=runtime_shader_directory(HALO2_SHADOW_FILTER_SHADER_DIR);
        std::ifstream file(directory/name,std::ios::binary|std::ios::ate);
        const auto bytes=file.tellg();
        if(bytes<=0 || bytes%4)throw FormatError("source shadow filter SPIRV missing");
        std::vector<std::uint32_t> words(std::size_t(bytes)/4);file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(words.data()),bytes))throw FormatError("source shadow filter SPIRV truncated");
        VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};create.codeSize=bytes;create.pCode=words.data();
        VkShaderModule result;check(vkCreateShaderModule(context.device,&create,nullptr,&result),"shadow filter shader");return result;
    }
    void make_filter() {
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;
        sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU=sampler.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT; // Literal unconsumed2D R state.
        check(vkCreateSampler(context.device,&sampler,nullptr,&filter_sampler),"shadow filter clamp sampler");
        const VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set.bindingCount=2;set.pBindings=bindings;
        check(vkCreateDescriptorSetLayout(context.device,&set,nullptr,&filter_set_layout),"shadow filter descriptor layout");
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&filter_set_layout;
        check(vkCreatePipelineLayout(context.device,&layout,nullptr,&filter_layout),"shadow filter pipeline layout");
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2;pool.poolSizeCount=2;pool.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&pool,nullptr,&filter_pool),"shadow filter descriptor pool");
        const auto frames=source_shadow_filter_passes();
        struct Uniform {std::array<std::uint32_t,60> program;std::array<ShaderVector,4> taps;};
        static_assert(sizeof(Uniform)==304);
        for(unsigned stage=0;stage<2;++stage) {
            VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};buffer.size=sizeof(Uniform);buffer.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            check(vkCreateBuffer(context.device,&buffer,nullptr,&filter_buffers[stage]),"shadow filter uniform");
            VkMemoryRequirements req;vkGetBufferMemoryRequirements(context.device,filter_buffers[stage],&req);
            VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=req.size;
            allocation.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            check(vkAllocateMemory(context.device,&allocation,nullptr,&filter_memories[stage]),"shadow filter uniform memory");
            check(vkBindBufferMemory(context.device,filter_buffers[stage],filter_memories[stage],0),"shadow filter uniform binding");
            const Uniform uniform{frames[stage].program.words,frames[stage].taps};void *mapped;
            check(vkMapMemory(context.device,filter_memories[stage],0,sizeof(Uniform),0,&mapped),"shadow filter uniform map");
            std::memcpy(mapped,&uniform,sizeof(uniform));vkUnmapMemory(context.device,filter_memories[stage]);
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=filter_pool;
            allocate.descriptorSetCount=1;allocate.pSetLayouts=&filter_set_layout;
            check(vkAllocateDescriptorSets(context.device,&allocate,&filter_sets[stage]),"shadow filter descriptor");
            const VkDescriptorImageInfo image_info{filter_sampler,stage?scratch_view:view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const VkDescriptorBufferInfo buffer_info{filter_buffers[stage],0,sizeof(Uniform)};
            VkWriteDescriptorSet writes[2]{};
            for(unsigned binding=0;binding<2;++binding) {
                auto &write=writes[binding];write={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=filter_sets[stage];
                write.dstBinding=binding;write.descriptorCount=1;
                write.descriptorType=binding?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                if(binding)write.pBufferInfo=&buffer_info;else write.pImageInfo=&image_info;
            }
            vkUpdateDescriptorSets(context.device,2,writes,0,nullptr);
        }
        VkShaderModule vertex{},fragment{};
        try {
            vertex=shader("source_shadow_filter.vert.spv");fragment=shader("source_shadow_filter.frag.spv");
            const VkPipelineShaderStageCreateInfo shaders[]{
                {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
                {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment,"main",nullptr}};
            VkPipelineVertexInputStateCreateInfo inputs{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.depthClampEnable=true;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            VkPipelineColorBlendAttachmentState write{};write.colorWriteMask=15;
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&write;
            const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
            VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};create.stageCount=2;create.pStages=shaders;
            create.pVertexInputState=&inputs;create.pInputAssemblyState=&assembly;create.pViewportState=&viewport;
            create.pRasterizationState=&raster;create.pMultisampleState=&samples;create.pDepthStencilState=&depth;
            create.pColorBlendState=&blend;create.pDynamicState=&dynamic;create.layout=filter_layout;create.renderPass=filter_pass;
            check(vkCreateGraphicsPipelines(context.device,0,1,&create,nullptr,&filter_pipeline),"source shadow filter pipeline");
        }catch(...) {if(vertex)vkDestroyShaderModule(context.device,vertex,nullptr);if(fragment)vkDestroyShaderModule(context.device,fragment,nullptr);throw;}
        vkDestroyShaderModule(context.device,vertex,nullptr);vkDestroyShaderModule(context.device,fragment,nullptr);
    }
    void release() {
        if(filter_pipeline)vkDestroyPipeline(context.device,filter_pipeline,nullptr);
        if(filter_layout)vkDestroyPipelineLayout(context.device,filter_layout,nullptr);
        if(filter_pool)vkDestroyDescriptorPool(context.device,filter_pool,nullptr);
        if(filter_set_layout)vkDestroyDescriptorSetLayout(context.device,filter_set_layout,nullptr);
        if(filter_sampler)vkDestroySampler(context.device,filter_sampler,nullptr);
        for(unsigned i=0;i<2;++i) {
            if(filter_buffers[i])vkDestroyBuffer(context.device,filter_buffers[i],nullptr);
            if(filter_memories[i])vkFreeMemory(context.device,filter_memories[i],nullptr);
        }
        if(scratch_framebuffer)vkDestroyFramebuffer(context.device,scratch_framebuffer,nullptr);
        if(filter_pass)vkDestroyRenderPass(context.device,filter_pass,nullptr);
        if(scratch_view)vkDestroyImageView(context.device,scratch_view,nullptr);
        if(scratch_image)vkDestroyImage(context.device,scratch_image,nullptr);
        if(scratch_memory)vkFreeMemory(context.device,scratch_memory,nullptr);
        if(framebuffer)vkDestroyFramebuffer(context.device,framebuffer,nullptr);
        if(pass)vkDestroyRenderPass(context.device,pass,nullptr);
        if(view)vkDestroyImageView(context.device,view,nullptr);
        if(image)vkDestroyImage(context.device,image,nullptr);
        if(memory)vkFreeMemory(context.device,memory,nullptr);
    }
    ~Impl(){release();}
    template<class Submission,class Validate,class Admitted>
    void record(VkCommandBuffer cmd,const SourceShadowContinuation &continuation,
                std::span<const Submission> roots,Validate validate,Admitted admitted,const SourceShadowTargetObserver& observer) {
        if(!cmd || !continuation.framebuffer)throw FormatError("shadow recording needs actual command buffer and continuation");
        if(!continuation.scissor.extent.width || !continuation.scissor.extent.height ||
           continuation.scissor.offset.x<0 || continuation.scissor.offset.y<0 ||
           std::uint64_t(continuation.scissor.offset.x)+continuation.scissor.extent.width>context.scene_extent.width ||
           std::uint64_t(continuation.scissor.offset.y)+continuation.scissor.extent.height>context.scene_extent.height)
            throw FormatError("shadow receiver scissor outside scene");
        for(const auto &root:roots) {
            if(!admitted(root))continue;
            if(root.uniforms.texture_width!=target_width || root.uniforms.texture_height!=target_height)
                throw FormatError("shadow uniforms differ from fixed target10 extent");
            validate(root);
        }
        std::size_t transitions{};
        const auto observed_begin=[&](SourceShadowTargetPhase phase,const VkRenderPassBeginInfo& begin) {
            ++transitions;if(observer)observer({phase,&begin,transitions});
        };
        for(const auto &root:roots) {
            if(!admitted(root))continue;
            vkCmdEndRenderPass(cmd);
            VkClearValue white{};white.color={{1,1,1,0}};
            VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=framebuffer;
            begin.renderArea={{0,0},{target_width,target_height}};begin.clearValueCount=1;begin.pClearValues=&white;
            vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);observed_begin(SourceShadowTargetPhase::Generate,begin);
            const VkViewport viewport{0,0,float(target_width),float(target_height),0,1};
            const VkRect2D scissor{{0,0},{target_width,target_height}};
            vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&scissor);
            for(const auto &draw:root.casters)draw.bind_and_draw(cmd,root.uniforms);
            vkCmdEndRenderPass(cmd);
            if(root.filter)for(unsigned stage=0;stage<2;++stage) {
                begin.renderPass=filter_pass;begin.framebuffer=stage?framebuffer:scratch_framebuffer;
                begin.clearValueCount=0;begin.pClearValues=nullptr;
                vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);observed_begin(stage?SourceShadowTargetPhase::FilterTo10:SourceShadowTargetPhase::FilterTo11,begin);
                vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&scissor);
                vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,filter_pipeline);
                vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,filter_layout,0,1,&filter_sets[stage],0,nullptr);
                vkCmdDraw(cmd,3,1,0,0);vkCmdEndRenderPass(cmd);
            }
            begin.renderPass=context.continuation_pass;begin.framebuffer=continuation.framebuffer;
            begin.renderArea={{0,0},context.scene_extent};begin.clearValueCount=0;begin.pClearValues=nullptr;
            vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);observed_begin(SourceShadowTargetPhase::Apply,begin);
            vkCmdSetViewport(cmd,0,1,&continuation.viewport);vkCmdSetScissor(cmd,0,1,&continuation.scissor);
            for(const auto &draw:root.receivers)draw.bind_and_draw(cmd,root.uniforms);
        }
        if(observer)observer({SourceShadowTargetPhase::Complete,nullptr,transitions});
    }
};
SourceShadowRender::SourceShadowRender(SourceShadowRenderContext c):impl_(std::make_unique<Impl>(c)){}
SourceShadowRender::~SourceShadowRender()=default;
VkImageView SourceShadowRender::target_view()const{return impl_->view;}
VkImage SourceShadowRender::target_image()const{return impl_->image;}
VkRenderPass SourceShadowRender::generation_pass()const{return impl_->pass;}
void SourceShadowRender::record(VkCommandBuffer cmd,const SourceShadowContinuation &continuation,
    std::span<const SourceShadowSubmission> roots,const SourceShadowTargetObserver& observer) {
    impl_->record(cmd,continuation,roots,[](const SourceShadowSubmission &root) {
        for(const auto &draw:root.casters)check_draw(draw,false);
        for(const auto &draw:root.receivers)check_draw(draw,true);
    },[](const SourceShadowSubmission &root) {
        return !root.casters.empty() && !root.receivers.empty();
    },observer);
}
void SourceShadowRender::record(VkCommandBuffer cmd,const SourceShadowContinuation &continuation,
    std::span<const SourceShadowRawSubmission> roots,const SourceShadowTargetObserver& observer) {
    impl_->record(cmd,continuation,roots,[](const SourceShadowRawSubmission &root) {
        if(root.root_owner==0xffffffffu)throw FormatError("raw shadow submission has no admitted root owner");
        check_raw_draws(root.casters,SourceShadowPhase::Generate,root);
        check_raw_draws(root.receivers,SourceShadowPhase::Apply,root);
    },[](const SourceShadowRawSubmission &root) {
        return (root.command_masks[0]&(1u<<10)) && (root.command_masks[1]&(1u<<11));
    },observer);
}
}
