#include "halo2/scope_render.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include <cstring>
#include <fstream>

namespace halo2 {
namespace {
void check(VkResult result,const char *what) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(what)+": Vulkan "+std::to_string(result));
}
struct Image {VkImage image{};VkDeviceMemory memory{};VkImageView view{};};
struct Buffer {VkBuffer buffer{};VkDeviceMemory memory{};};
struct CopyUniform {std::array<std::uint32_t,60> program;ShaderVector rectangle;};
static_assert(sizeof(CopyUniform)==256);
}
struct ScopeRender::Impl {
    ScopeRenderContext context;
    Image scene,target;
    VkRenderPass pass{};
    VkFramebuffer framebuffer{};
    VkDescriptorSetLayout set_layout{};
    VkPipelineLayout layout{};
    VkDescriptorPool pool{};
    VkPipeline pipeline{};
    VkSampler sampler{};
    std::array<VkDescriptorSet,4> descriptors{};
    std::array<Buffer,4> uniforms;
    std::array<bool,4> active{};
    bool scene_initialized{};
    explicit Impl(ScopeRenderContext c):context(std::move(c)) {
        make_image(scene,context.extent,context.scene_format,VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        make_image(target,{64,64},VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter=si.minFilter=VK_FILTER_LINEAR;
        si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(context.device,&si,nullptr,&sampler),"scope copy sampler");
        const VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        set.bindingCount=2;set.pBindings=bindings;
        check(vkCreateDescriptorSetLayout(context.device,&set,nullptr,&set_layout),"scope copy layout");
        VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&set_layout;
        check(vkCreatePipelineLayout(context.device,&li,nullptr,&layout),"scope copy pipeline layout");
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,4}};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pi.maxSets=4;pi.poolSizeCount=2;pi.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&pi,nullptr,&pool),"scope copy pool");
        for(unsigned i=0;i<4;++i) {
            VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=pool;ai.descriptorSetCount=1;ai.pSetLayouts=&set_layout;
            check(vkAllocateDescriptorSets(context.device,&ai,&descriptors[i]),"scope copy descriptor");
            make_buffer(uniforms[i]);
            const VkDescriptorImageInfo image{sampler,scene.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const VkDescriptorBufferInfo buffer{uniforms[i].buffer,0,sizeof(CopyUniform)};
            VkWriteDescriptorSet writes[2]{};
            for(unsigned j=0;j<2;++j) {
                auto &w=writes[j];w={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=descriptors[i];w.dstBinding=j;w.descriptorCount=1;
                w.descriptorType=j?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                if(j)w.pBufferInfo=&buffer;else w.pImageInfo=&image;
            }
            vkUpdateDescriptorSets(context.device,2,writes,0,nullptr);
        }
        make_pass();make_pipeline();
    }
    ~Impl() {
        for(auto &b:uniforms) {
            if(b.buffer)vkDestroyBuffer(context.device,b.buffer,nullptr);
            if(b.memory)vkFreeMemory(context.device,b.memory,nullptr);
        }
        if(pipeline)vkDestroyPipeline(context.device,pipeline,nullptr);
        if(framebuffer)vkDestroyFramebuffer(context.device,framebuffer,nullptr);
        if(pass)vkDestroyRenderPass(context.device,pass,nullptr);
        if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);
        if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);
        if(set_layout)vkDestroyDescriptorSetLayout(context.device,set_layout,nullptr);
        if(sampler)vkDestroySampler(context.device,sampler,nullptr);
        for(auto *i:{&scene,&target}) {
            if(i->view)vkDestroyImageView(context.device,i->view,nullptr);
            if(i->image)vkDestroyImage(context.device,i->image,nullptr);
            if(i->memory)vkFreeMemory(context.device,i->memory,nullptr);
        }
    }
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(context.physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i)) && (p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("scope copy memory type unavailable");
    }
    void make_image(Image &out,VkExtent2D extent,VkFormat format,VkImageUsageFlags usage) {
        VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=format;i.extent={extent.width,extent.height,1};
        i.mipLevels=i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=usage|VK_IMAGE_USAGE_SAMPLED_BIT;
        check(vkCreateImage(context.device,&i,nullptr,&out.image),"scope copy image");
        VkMemoryRequirements r;vkGetImageMemoryRequirements(context.device,out.image,&r);
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memory_type(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(context.device,&a,nullptr,&out.memory),"scope copy image memory");
        check(vkBindImageMemory(context.device,out.image,out.memory,0),"scope copy image binding");
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=out.image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        check(vkCreateImageView(context.device,&v,nullptr,&out.view),"scope copy image view");
    }
    void make_buffer(Buffer &out) {
        VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=sizeof(CopyUniform);b.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        check(vkCreateBuffer(context.device,&b,nullptr,&out.buffer),"scope copy buffer");
        VkMemoryRequirements r;vkGetBufferMemoryRequirements(context.device,out.buffer,&r);
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memory_type(r.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(context.device,&a,nullptr,&out.memory),"scope copy buffer memory");
        check(vkBindBufferMemory(context.device,out.buffer,out.memory,0),"scope copy buffer binding");
    }
    void make_pass() {
        VkAttachmentDescription a{};a.format=VK_FORMAT_R8G8B8A8_UNORM;a.samples=VK_SAMPLE_COUNT_1_BIT;
        // 14850 allocates without clearing. 2B000 and the active scope
        // consumer both write RGB only; the consumer never reads alpha.
        a.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;a.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
        const VkSubpassDependency deps[]{
            {VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT},
            {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT}};
        VkRenderPassCreateInfo p{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};p.attachmentCount=1;p.pAttachments=&a;p.subpassCount=1;p.pSubpasses=&sub;p.dependencyCount=2;p.pDependencies=deps;
        check(vkCreateRenderPass(context.device,&p,nullptr,&pass),"scope copy render pass");
        VkFramebufferCreateInfo f{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};f.renderPass=pass;f.attachmentCount=1;f.pAttachments=&target.view;f.width=f.height=64;f.layers=1;
        check(vkCreateFramebuffer(context.device,&f,nullptr,&framebuffer),"scope copy framebuffer");
    }
    VkShaderModule shader(std::string_view name) {
        std::ifstream f(context.shader_directory/name,std::ios::binary|std::ios::ate);
        const auto size=f.tellg();if(size<=0 || size%4)throw FormatError("invalid scope copy shader");
        std::vector<std::uint32_t> words(std::size_t(size)/4);f.seekg(0);
        if(!f.read(reinterpret_cast<char *>(words.data()),size))throw FormatError("short scope copy shader");
        VkShaderModuleCreateInfo s{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};s.codeSize=std::size_t(size);s.pCode=words.data();
        VkShaderModule module{};check(vkCreateShaderModule(context.device,&s,nullptr,&module),"scope copy shader");return module;
    }
    void make_pipeline() {
        const auto vertex=shader("fog.vert.spv"),pixel=shader(native_fragment_kernel_path("scope-copy",scope_copy_frame({}).program.words));
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,pixel,"main",nullptr}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkPipelineColorBlendAttachmentState blend{};blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        VkPipelineColorBlendStateCreateInfo color{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};color.attachmentCount=1;color.pAttachments=&blend;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo p{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};p.stageCount=2;p.pStages=stages;p.pVertexInputState=&input;p.pInputAssemblyState=&assembly;p.pViewportState=&viewport;p.pRasterizationState=&raster;p.pMultisampleState=&samples;p.pDepthStencilState=&depth;p.pColorBlendState=&color;p.pDynamicState=&dynamic;p.layout=layout;p.renderPass=pass;
        const auto result=vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&p,nullptr,&pipeline);
        vkDestroyShaderModule(context.device,vertex,nullptr);vkDestroyShaderModule(context.device,pixel,nullptr);
        check(result,"scope copy pipeline");
    }
    void prepare(std::span<const std::optional<ScopeCopyFrame>> frames) {
        if(frames.size()>4)throw FormatError("scope copy local view count exceeds four");
        active.fill(false);
        for(unsigned i=0;i<frames.size();++i)if(frames[i]) {
            if(native_fragment_kernel_path("scope-copy",frames[i]->program.words)!=
               native_fragment_kernel_path("scope-copy",scope_copy_frame({}).program.words))
                throw FormatError("scope copy program differs from its compiled original kernel");
            active[i]=true;const CopyUniform data{native_fragment_uniform_words(frames[i]->program.words),frames[i]->source_rectangle};
            void *mapped{};check(vkMapMemory(context.device,uniforms[i].memory,0,sizeof(data),0,&mapped),"scope copy uniform map");
            std::memcpy(mapped,&data,sizeof(data));vkUnmapMemory(context.device,uniforms[i].memory);
        }
    }
    void record(unsigned view,VkCommandBuffer cmd,VkImage color,VkFramebuffer continuation) {
        if(view>=4 || !active[view])return;
        vkCmdEndRenderPass(cmd);
        std::array<VkImageMemoryBarrier,2> barriers{};
        for(auto &b:barriers) {b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
        auto &src=barriers[0],&dst=barriers[1];
        src.image=color;src.oldLayout=context.scene_layout;src.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;src.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;src.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        dst.image=scene.image;dst.oldLayout=scene_initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;dst.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;dst.srcAccessMask=scene_initialized?VK_ACCESS_SHADER_READ_BIT:0;dst.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers.data());
        VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent={context.extent.width,context.extent.height,1};
        vkCmdCopyImage(cmd,color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,scene.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        src.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;src.newLayout=context.scene_layout;src.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;src.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dst.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;dst.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;dst.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;dst.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,2,barriers.data());scene_initialized=true;
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=framebuffer;begin.renderArea={{0,0},{64,64}};
        vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0,0,64,64,0,1};const VkRect2D clip{{0,0},{64,64}};
        vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&clip);
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptors[view],0,nullptr);
        vkCmdDraw(cmd,3,1,0,0);vkCmdEndRenderPass(cmd);
        begin.renderPass=context.continuation_pass;begin.framebuffer=continuation;begin.renderArea={{0,0},context.extent};
        vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);
    }
};
ScopeRender::ScopeRender(ScopeRenderContext context):impl_(std::make_unique<Impl>(std::move(context))){}
ScopeRender::~ScopeRender()=default;
VkImageView ScopeRender::target_view()const{return impl_->target.view;}
void ScopeRender::prepare(std::span<const std::optional<ScopeCopyFrame>> frames){impl_->prepare(frames);}
void ScopeRender::record(unsigned view,VkCommandBuffer command,VkImage scene,VkFramebuffer continuation){impl_->record(view,command,scene,continuation);}
}
