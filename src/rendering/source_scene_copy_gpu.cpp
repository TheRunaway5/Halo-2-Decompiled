#include "halo2/source_scene_copy_gpu.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/cache.hpp"
#include "halo2/sampler.hpp"
#include <cstring>
#include <fstream>
#include <thread>

namespace halo2 {
namespace {
void check(VkResult value,const char *operation) {
    if(value!=VK_SUCCESS)throw FormatError(std::string(operation)+": Vulkan result "+std::to_string(value));
}
bool same(VkExtent2D a,VkExtent2D b){return a.width==b.width&&a.height==b.height;}
void barrier(VkCommandBuffer c,VkImage i,VkImageLayout old,VkImageLayout next,
             VkAccessFlags before,VkAccessFlags after,VkPipelineStageFlags first,VkPipelineStageFlags last) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=i;b.oldLayout=old;b.newLayout=next;
    b.srcAccessMask=before;b.dstAccessMask=after;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkCmdPipelineBarrier(c,first,last,0,0,nullptr,0,nullptr,1,&b);
}
}
struct SourceSceneCopyGpu::Impl {
    struct Image {VkImage image{};VkImageView view{};VkDeviceMemory memory{};};
    SourceSceneCopyGpuContext context;
    std::thread::id thread=std::this_thread::get_id();
    VkDescriptorSetLayout descriptors{};VkDescriptorPool pool{};VkDescriptorSet set{};
    VkPipelineLayout layout{};VkPipeline pipeline{};VkRenderPass pass{};VkFramebuffer framebuffer{};
    VkSampler sampler{};VkBuffer uniform{};VkDeviceMemory uniform_memory{};void *mapped{};
    Image source_copy,target;
    VkExtent2D source_extent{},target_extent{};
    bool prepared{},initialized{},before_record_initialized{};
    VkFence pending{};VkCommandBuffer recorded_command{};
    explicit Impl(SourceSceneCopyGpuContext c):context(std::move(c)) {
        try{initialize();}catch(...){destroy();throw;}
    }
    ~Impl(){if(pending)vkWaitForFences(context.device,1,&pending,VK_TRUE,UINT64_MAX);destroy();}
    void owner()const {
        if(thread!=std::this_thread::get_id())throw FormatError("Scene-copy resources belong to another renderer thread");
    }
    unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags)const {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(context.physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw FormatError("Scene-copy native memory type is unavailable");
    }
    void destroy(Image &i) {
        if(i.view)vkDestroyImageView(context.device,i.view,nullptr);
        if(i.image)vkDestroyImage(context.device,i.image,nullptr);
        if(i.memory)vkFreeMemory(context.device,i.memory,nullptr);i={};
    }
    void destroy_images() {
        if(framebuffer)vkDestroyFramebuffer(context.device,framebuffer,nullptr);framebuffer={};
        destroy(target);destroy(source_copy);source_extent={};target_extent={};prepared=initialized=false;
    }
    void destroy() {
        destroy_images();const auto d=context.device;if(!d)return;
        if(mapped)vkUnmapMemory(d,uniform_memory);
        if(uniform)vkDestroyBuffer(d,uniform,nullptr);if(uniform_memory)vkFreeMemory(d,uniform_memory,nullptr);
        if(pipeline)vkDestroyPipeline(d,pipeline,nullptr);if(layout)vkDestroyPipelineLayout(d,layout,nullptr);
        if(pass)vkDestroyRenderPass(d,pass,nullptr);if(pool)vkDestroyDescriptorPool(d,pool,nullptr);
        if(descriptors)vkDestroyDescriptorSetLayout(d,descriptors,nullptr);if(sampler)vkDestroySampler(d,sampler,nullptr);
    }
    VkShaderModule shader(std::string_view name)const {
        const auto path=context.shader_directory/std::string(name);std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file||file.tellg()<=0||file.tellg()%4)throw FormatError("Scene-copy native shader absent: "+path.string());
        std::vector<std::uint32_t> words(std::size_t(file.tellg())/4);file.seekg(0);
        file.read(reinterpret_cast<char*>(words.data()),std::streamsize(words.size()*4));
        if(!file)throw FormatError("Scene-copy native shader truncated");
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=words.size()*4;info.pCode=words.data();
        VkShaderModule out;check(vkCreateShaderModule(context.device,&info,nullptr,&out),"create scene-copy shader");return out;
    }
    void initialize() {
        if(!context.device||!context.physical ||
           (context.color_format!=VK_FORMAT_R8G8B8A8_UNORM&&context.color_format!=VK_FORMAT_B8G8R8A8_UNORM))
            throw FormatError("Scene-copy needs its actual device and ordinary UNORM color format");
        unsigned count;vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,nullptr);
        std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,families.data());
        if(context.queue_family>=count||!(families[context.queue_family].queueFlags&VK_QUEUE_GRAPHICS_BIT))
            throw FormatError("Scene-copy queue lacks graphics ownership");
        VkFormatProperties p;vkGetPhysicalDeviceFormatProperties(context.physical,context.color_format,&p);
        constexpr auto required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        if((p.optimalTilingFeatures&required)!=required)throw FormatError("Scene-copy native format lacks required sampling/copy features");
        const VkDescriptorSetLayoutBinding bindings[]{
            {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            {2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
        VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dl.bindingCount=2;dl.pBindings=bindings;
        check(vkCreateDescriptorSetLayout(context.device,&dl,nullptr,&descriptors),"create scene-copy bindings");
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1}};
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=2;dp.pPoolSizes=sizes;
        check(vkCreateDescriptorPool(context.device,&dp,nullptr,&pool),"create scene-copy descriptors");
        VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool;da.descriptorSetCount=1;da.pSetLayouts=&descriptors;
        check(vkAllocateDescriptorSets(context.device,&da,&set),"allocate scene-copy descriptor");
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=sizeof(SourceSceneCopyShaderUniform);bi.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        check(vkCreateBuffer(context.device,&bi,nullptr,&uniform),"create scene-copy uniform");
        VkMemoryRequirements mr;vkGetBufferMemoryRequirements(context.device,uniform,&mr);
        VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ma.allocationSize=mr.size;
        ma.memoryTypeIndex=memory_type(mr.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(context.device,&ma,nullptr,&uniform_memory),"allocate scene-copy uniform");
        check(vkBindBufferMemory(context.device,uniform,uniform_memory,0),"bind scene-copy uniform");
        check(vkMapMemory(context.device,uniform_memory,0,bi.size,0,&mapped),"map scene-copy uniform");
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};si.magFilter=si.minFilter=VK_FILTER_LINEAR;
        // Whole retail352E0:35386..353AB (TU1.5 counterpart352D0)
        // writes addressU/V/W3, MIN/MAG2 and MIP0 for every copy stage.
        si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.addressModeU=si.addressModeV=si.addressModeW=sampler_address(3);
        check(vkCreateSampler(context.device,&si,nullptr,&sampler),"create scene-copy source sampler");
        VkAttachmentDescription a{};a.format=context.color_format;a.samples=VK_SAMPLE_COUNT_1_BIT;
        a.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;a.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        const VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
        const VkSubpassDependency dependency{0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT};
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=1;rp.pAttachments=&a;
        rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=1;rp.pDependencies=&dependency;
        check(vkCreateRenderPass(context.device,&rp,nullptr,&pass),"create scene-copy pass");
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&descriptors;
        check(vkCreatePipelineLayout(context.device,&pl,nullptr,&layout),"create scene-copy pipeline layout");
        VkShaderModule vertex{},fragment{};
        try {
            vertex=shader("fog.vert.spv");fragment=shader(native_fragment_kernel_path("scene-copy",source_scene_copy_program().words));
            VkPipelineShaderStageCreateInfo stages[2]{};
            for(unsigned i=0;i<2;++i){stages[i]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};stages[i].stage=i?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT;stages[i].module=i?fragment:vertex;stages[i].pName="main";}
            VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
            VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&ba;
            const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};ds.dynamicStateCount=2;ds.pDynamicStates=states;
            VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=2;pi.pStages=stages;
            pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;pi.pRasterizationState=&rs;
            pi.pMultisampleState=&ms;pi.pColorBlendState=&blend;pi.pDynamicState=&ds;pi.layout=layout;pi.renderPass=pass;
            check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&pi,nullptr,&pipeline),"create scene-copy pipeline");
        }catch(...){if(vertex)vkDestroyShaderModule(context.device,vertex,nullptr);if(fragment)vkDestroyShaderModule(context.device,fragment,nullptr);throw;}
        vkDestroyShaderModule(context.device,vertex,nullptr);vkDestroyShaderModule(context.device,fragment,nullptr);
    }
    void image(Image &out,VkExtent2D extent,VkImageUsageFlags usage) {
        VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=context.color_format;
        ci.extent={extent.width,extent.height,1};ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
        check(vkCreateImage(context.device,&ci,nullptr,&out.image),"create scene-copy image");
        VkMemoryRequirements r;vkGetImageMemoryRequirements(context.device,out.image,&r);
        VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ma.allocationSize=r.size;ma.memoryTypeIndex=memory_type(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(context.device,&ma,nullptr,&out.memory),"allocate scene-copy image");
        check(vkBindImageMemory(context.device,out.image,out.memory,0),"bind scene-copy image");
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=out.image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=context.color_format;
        vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(context.device,&vi,nullptr,&out.view),"create scene-copy image view");
    }
    SourceSceneCopyGpuBinding prepare(const SourceSceneCopyGpuInput &in) {
        owner();if(pending)throw FormatError("Scene-copy frame still owns pending GPU work");
        auto values=source_scene_copy_shader_uniform(in.copy,in.coordinate_extent);
        values.program=native_fragment_uniform_words(values.program);
        VkPhysicalDeviceProperties p;vkGetPhysicalDeviceProperties(context.physical,&p);
        const VkExtent2D destination{std::uint32_t(in.copy.destination_width),std::uint32_t(in.copy.destination_height)};
        if(!in.image_extent.width||!in.image_extent.height||in.image_extent.width>p.limits.maxImageDimension2D||
           in.image_extent.height>p.limits.maxImageDimension2D||destination.width>p.limits.maxImageDimension2D||destination.height>p.limits.maxImageDimension2D)
            throw FormatError("Scene-copy source/destination native extent unavailable");
        if(!same(source_extent,in.image_extent)||!same(target_extent,destination)||!framebuffer) {
            destroy_images();
            try {
                image(source_copy,in.image_extent,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);
                image(target,destination,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
                VkFramebufferCreateInfo f{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};f.renderPass=pass;f.attachmentCount=1;f.pAttachments=&target.view;
                f.width=destination.width;f.height=destination.height;f.layers=1;
                check(vkCreateFramebuffer(context.device,&f,nullptr,&framebuffer),"create scene-copy destination framebuffer");
                source_extent=in.image_extent;target_extent=destination;
            }catch(...){destroy_images();throw;}
            const VkDescriptorImageInfo texture{sampler,source_copy.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            const VkDescriptorBufferInfo buffer{uniform,0,sizeof(values)};
            VkWriteDescriptorSet writes[2]{};
            for(auto &w:writes){w={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=set;w.descriptorCount=1;}
            writes[0].dstBinding=0;writes[0].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[0].pImageInfo=&texture;
            writes[1].dstBinding=2;writes[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[1].pBufferInfo=&buffer;
            vkUpdateDescriptorSets(context.device,2,writes,0,nullptr);
        }
        std::memcpy(mapped,&values,sizeof(values));prepared=true;
        return {target.view,context.color_format,target_extent};
    }
    SourceSceneCopyGpuImage record(const SourceSceneCopyGpuRecord &in) {
        owner();
        if(!prepared||!in.command||!in.source||!in.completion||in.owner!=context.device||in.format!=context.color_format||
           !same(in.extent,source_extent)||!(in.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)||
           in.layout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL||in.samples!=VK_SAMPLE_COUNT_1_BIT||in.mip_levels!=1||in.array_layers!=1||
           in.queue_family!=context.queue_family||!in.outside_render_pass||!in.exclusive_access||
           in.source==source_copy.image||in.source==target.image)
            throw FormatError("Scene-copy record lacks its actual ordinary main-image/command ownership");
        if(pending&&(pending!=in.completion||recorded_command!=in.command))
            throw FormatError("Scene-copy frame belongs to another live command/fence");
        const auto status=vkGetFenceStatus(context.device,in.completion);
        if(status!=VK_NOT_READY){if(status!=VK_SUCCESS)check(status,"query scene-copy completion");throw FormatError("Scene-copy command requires its reset unsignaled completion fence");}
        if(!pending){pending=in.completion;recorded_command=in.command;before_record_initialized=initialized;}
        const auto c=in.command;
        barrier(c,in.source,in.layout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
        barrier(c,source_copy.image,initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,initialized?VK_ACCESS_SHADER_READ_BIT:0,VK_ACCESS_TRANSFER_WRITE_BIT,
            initialized?VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent={source_extent.width,source_extent.height,1};
        vkCmdCopyImage(c,in.source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,source_copy.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barrier(c,in.source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,in.layout,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        barrier(c,source_copy.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        barrier(c,target.image,initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            initialized?VK_ACCESS_SHADER_READ_BIT:0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            initialized?VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=framebuffer;begin.renderArea.extent=target_extent;
        vkCmdBeginRenderPass(c,&begin,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(c,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        const VkViewport viewport{0,0,float(target_extent.width),float(target_extent.height),0,1};const VkRect2D scissor{{0,0},target_extent};
        vkCmdSetViewport(c,0,1,&viewport);vkCmdSetScissor(c,0,1,&scissor);
        vkCmdBindDescriptorSets(c,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);vkCmdDraw(c,3,1,0,0);vkCmdEndRenderPass(c);
        initialized=true;return {target.image,target.view,context.color_format,target_extent};
    }
    bool retire() {
        owner();if(!pending)return true;const auto status=vkGetFenceStatus(context.device,pending);
        if(status==VK_NOT_READY)return false;check(status,"retire scene-copy frame");pending={};recorded_command={};return true;
    }
    void discard(VkCommandBuffer reset_command) {
        owner();if(!pending||reset_command!=recorded_command)throw FormatError("Scene-copy discard has no matching reset unsubmitted command");
        initialized=before_record_initialized;pending={};recorded_command={};
    }
};
SourceSceneCopyGpu::SourceSceneCopyGpu(SourceSceneCopyGpuContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceSceneCopyGpu::~SourceSceneCopyGpu()=default;
SourceSceneCopyGpuBinding SourceSceneCopyGpu::prepare(const SourceSceneCopyGpuInput &in){return impl_->prepare(in);}
SourceSceneCopyGpuImage SourceSceneCopyGpu::record(const SourceSceneCopyGpuRecord &in){return impl_->record(in);}
bool SourceSceneCopyGpu::retire(){return impl_->retire();}
void SourceSceneCopyGpu::discard_unsubmitted(VkCommandBuffer c){impl_->discard(c);}
}
