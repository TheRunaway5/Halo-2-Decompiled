#include "halo2/source_sky_stencil.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

namespace halo2 {
namespace {
void check(VkResult r) {if(r!=VK_SUCCESS)throw FormatError("source sky stencil Vulkan failure: "+std::to_string(r));}
struct Scope {VkPipelineStageFlags stage;VkAccessFlags access;};
constexpr Scope attachment{VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
Scope scope(VkImageLayout layout) {
    switch(layout) {
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:return attachment;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:return {attachment.stage|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_SHADER_READ_BIT};
    case VK_IMAGE_LAYOUT_GENERAL:return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:return {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:return {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_TRANSFER_WRITE_BIT};
    default:throw FormatError("source sky stencil requires an initialized combined depth/stencil layout");
    }
}
void barrier(VkCommandBuffer command,VkImage image,VkImageLayout from,VkImageLayout to,Scope a,Scope b) {
    VkImageMemoryBarrier m{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};m.srcAccessMask=a.access;m.dstAccessMask=b.access;m.oldLayout=from;m.newLayout=to;
    m.srcQueueFamilyIndex=m.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;m.image=image;m.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
    vkCmdPipelineBarrier(command,a.stage,b.stage,0,0,nullptr,0,nullptr,1,&m);
}
}
void validate_source_sky_stencil_draw(const SourceSkyStencilDraw &d) {
    if(d.vertices.empty()||d.vertices.size()>std::numeric_limits<std::uint32_t>::max() || d.indices.size()<3||d.indices.size()>std::numeric_limits<std::uint32_t>::max())
        throw FormatError("source sky stencil requires bounded retained triangle-strip vertices/indices");
    for(auto i:d.indices)if(i>=d.vertices.size())throw FormatError("source sky stencil index outside retained VSOut; primitive restart not admitted");
    for(const auto &v:d.vertices)for(auto p:{v.clip,v.window})for(float x:p)if(!std::isfinite(x))throw FormatError("source sky stencil retained vertex is not finite");
    for(float x:d.uniform.clip_range)if(!std::isfinite(x))throw FormatError("source sky stencil clip range is not finite");
    if(d.uniform.clip_range[2]<0 || d.uniform.clip_range[2]>d.uniform.clip_range[3] || d.uniform.clip_range[3]>16777215.0f)
        throw FormatError("source sky stencil integer depth clamp range invalid");
    for(float x:d.uniform.depth_factor_offset)if(!std::isfinite(x))throw FormatError("source sky stencil depth offset/factor is not finite");
    for(auto x:d.uniform.surface_scale)if(x<=0)throw FormatError("source sky stencil actual surface scale must be positive");
    for(auto x:d.rotations)if(x>2)throw FormatError("source sky stencil captured cyclic rotation invalid");
}
void validate_source_sky_stencil_draw(const SourceSkyStencilPostGeometryDraw &d) {
    if(d.vertices.empty()||d.vertices.size()%3||d.vertices.size()>std::numeric_limits<std::uint32_t>::max())throw FormatError("source sky stencil requires bounded complete retained GSOut triangles");
    for(const auto &v:d.vertices) {
        for(auto p:{v.clip,v.window0,v.window1,v.window2})for(float x:p)if(!std::isfinite(x))throw FormatError("source sky stencil retained GSOut vertex is not finite");
        if(!std::isfinite(v.tri_mz))throw FormatError("source sky stencil retained GSOut slope is not finite");
    }
    for(std::size_t i=0;i<d.vertices.size();i+=3)for(unsigned j=1;j<3;++j)
        if(std::memcmp(&d.vertices[i].window0,&d.vertices[i+j].window0,52))throw FormatError("source sky stencil GSOut triangle has inconsistent flat lanes");
    const SourceSkyStencilVertex dummy{{0,0,0,1},{0,0,0,1}};const std::array<std::uint32_t,3> indices{0,0,0};
    validate_source_sky_stencil_draw(SourceSkyStencilDraw{{&dummy,1},indices,d.uniform,{0,0}});
}

struct SourceSkyStencil::Impl {
    SourceSkyStencilContext context;
    VkPhysicalDeviceProperties properties{};
    VkRenderPass pass{};VkDescriptorSetLayout descriptors{};VkPipelineLayout layout{};std::array<VkPipeline,2> pipelines{};
    std::array<VkShaderModule,4> modules{};
    VkFence pending{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};};
    struct Resources {
        VkDevice device{};SourceWeatherDepthAliasImage image;
        std::array<Buffer,3> buffers{};
        VkImageView view{};VkFramebuffer frame{};VkDescriptorPool pool{};VkDescriptorSet set{};
        std::uint32_t index_count{},vertex_count{};std::array<std::uint32_t,2> rotations;
        bool post_geometry{};
        ~Resources() {
            if(frame)vkDestroyFramebuffer(device,frame,nullptr);
            if(view)vkDestroyImageView(device,view,nullptr);
            if(pool)vkDestroyDescriptorPool(device,pool,nullptr);
            for(auto b:buffers){if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);if(b.memory)vkFreeMemory(device,b.memory,nullptr);}
        }
    };
    std::unique_ptr<Resources> resources;
    explicit Impl(SourceSkyStencilContext c):context(std::move(c)) {
        try {
            if(!context.physical||!context.device||!context.geometry_shader_enabled||!context.depth_clamp_enabled)
                throw FormatError("source sky stencil requires actual device handles and enabled geometryShader/depthClamp");
            vkGetPhysicalDeviceProperties(context.physical,&properties);
            if(properties.apiVersion<VK_API_VERSION_1_1)throw FormatError("source sky stencil requires Vulkan 1.1");
            VkPhysicalDeviceFeatures features;vkGetPhysicalDeviceFeatures(context.physical,&features);
            if(!features.geometryShader||!features.depthClamp)throw FormatError("source sky stencil physical features unsupported");
            std::uint32_t count;vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,nullptr);
            std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(context.physical,&count,families.data());
            if(context.queue_family>=count||!families[context.queue_family].queueCount||(families[context.queue_family].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))!=(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))throw FormatError("source sky stencil requires an actual graphics+compute queue family for restored shader-read scopes");
            const auto &l=properties.limits;
            if(l.maxGeometryInputComponents<8||l.maxGeometryOutputComponents<17||l.maxGeometryOutputVertices<3||l.maxGeometryTotalOutputComponents<51||l.maxPushConstantsSize<8||l.maxUniformBufferRange<sizeof(SourceSkyStencilUniform)||l.maxVertexInputAttributes<5||l.maxVertexInputBindingStride<68)throw FormatError("source sky stencil shader limits unsupported");
            VkFormatProperties format;vkGetPhysicalDeviceFormatProperties(context.physical,VK_FORMAT_D24_UNORM_S8_UINT,&format);
            if(!(format.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))throw FormatError("source sky stencil D24S8 attachment unsupported");
            VkAttachmentDescription a{};a.format=VK_FORMAT_D24_UNORM_S8_UINT;a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.storeOp=a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_STORE;a.initialLayout=a.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            VkAttachmentReference depth{0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.pDepthStencilAttachment=&depth;
            const VkSubpassDependency deps[]{{VK_SUBPASS_EXTERNAL,0,attachment.stage,attachment.stage,attachment.access,attachment.access,VK_DEPENDENCY_BY_REGION_BIT},{0,VK_SUBPASS_EXTERNAL,attachment.stage,attachment.stage,attachment.access,attachment.access,VK_DEPENDENCY_BY_REGION_BIT}};
            VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=1;rp.pAttachments=&a;rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=2;rp.pDependencies=deps;check(vkCreateRenderPass(context.device,&rp,nullptr,&pass));
            const VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
            VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dl.bindingCount=1;dl.pBindings=&binding;check(vkCreateDescriptorSetLayout(context.device,&dl,nullptr,&descriptors));
            const VkPushConstantRange push{VK_SHADER_STAGE_GEOMETRY_BIT,0,8};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&descriptors;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;check(vkCreatePipelineLayout(context.device,&pl,nullptr,&layout));
            const char *names[]{"source_sky_stencil.vert.spv","source_sky_stencil.geom.spv","source_sky_stencil.frag.spv","source_sky_stencil_passthrough.vert.spv"};
            std::array<VkPipelineShaderStageCreateInfo,3> stages{};const VkShaderStageFlagBits stagebits[]{VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_GEOMETRY_BIT,VK_SHADER_STAGE_FRAGMENT_BIT};
            for(unsigned i=0;i<3;++i){std::ifstream f(context.shader_directory/names[i],std::ios::binary|std::ios::ate);if(!f||f.tellg()<20||std::uint64_t(f.tellg())%4)throw FormatError("source sky stencil SPIR-V unavailable");std::vector<std::uint32_t> words(std::size_t(f.tellg())/4);f.seekg(0);if(!f.read(reinterpret_cast<char*>(words.data()),words.size()*4)||words[0]!=0x07230203)throw FormatError("source sky stencil SPIR-V invalid");VkShaderModuleCreateInfo m{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};m.codeSize=words.size()*4;m.pCode=words.data();check(vkCreateShaderModule(context.device,&m,nullptr,&modules[i]));stages[i]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};stages[i].stage=stagebits[i];stages[i].module=modules[i];stages[i].pName="main";}
            {std::ifstream f(context.shader_directory/names[3],std::ios::binary|std::ios::ate);if(!f||f.tellg()<20||std::uint64_t(f.tellg())%4)throw FormatError("source sky stencil passthrough SPIR-V unavailable");std::vector<std::uint32_t> words(std::size_t(f.tellg())/4);f.seekg(0);if(!f.read(reinterpret_cast<char*>(words.data()),words.size()*4)||words[0]!=0x07230203)throw FormatError("source sky stencil passthrough SPIR-V invalid");VkShaderModuleCreateInfo m{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};m.codeSize=words.size()*4;m.pCode=words.data();check(vkCreateShaderModule(context.device,&m,nullptr,&modules[3]));}
            const VkVertexInputBindingDescription vb{0,sizeof(SourceSkyStencilVertex),VK_VERTEX_INPUT_RATE_VERTEX};
            const VkVertexInputAttributeDescription attrs[]{{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32A32_SFLOAT,16}};
            VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&vb;vi.vertexAttributeDescriptionCount=2;vi.pVertexAttributeDescriptions=attrs;
            VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;ia.primitiveRestartEnable=VK_FALSE;
            VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.depthClampEnable=VK_TRUE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.frontFace=VK_FRONT_FACE_CLOCKWISE;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=VK_TRUE;ds.depthWriteEnable=VK_FALSE;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;ds.stencilTestEnable=VK_TRUE;ds.front=ds.back={VK_STENCIL_OP_KEEP,VK_STENCIL_OP_REPLACE,VK_STENCIL_OP_KEEP,VK_COMPARE_OP_ALWAYS,255,255,255};
            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            const VkDynamicState dynamics[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dy.dynamicStateCount=2;dy.pDynamicStates=dynamics;
            VkGraphicsPipelineCreateInfo pc{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pc.stageCount=3;pc.pStages=stages.data();pc.pVertexInputState=&vi;pc.pInputAssemblyState=&ia;pc.pViewportState=&vp;pc.pRasterizationState=&raster;pc.pMultisampleState=&ms;pc.pDepthStencilState=&ds;pc.pColorBlendState=&blend;pc.pDynamicState=&dy;pc.layout=layout;pc.renderPass=pass;check(vkCreateGraphicsPipelines(context.device,{},1,&pc,nullptr,&pipelines[0]));
            const VkVertexInputBindingDescription gb{0,sizeof(SourceSkyStencilPostGeometryVertex),VK_VERTEX_INPUT_RATE_VERTEX};const VkVertexInputAttributeDescription ga[]{{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32A32_SFLOAT,16},{2,0,VK_FORMAT_R32G32B32A32_SFLOAT,32},{3,0,VK_FORMAT_R32G32B32A32_SFLOAT,48},{4,0,VK_FORMAT_R32_SFLOAT,64}};
            vi.pVertexBindingDescriptions=&gb;vi.vertexAttributeDescriptionCount=5;vi.pVertexAttributeDescriptions=ga;ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            const std::array<VkPipelineShaderStageCreateInfo,2> gs_stages{{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,modules[3],"main",nullptr},stages[2]}};
            pc.stageCount=2;pc.pStages=gs_stages.data();check(vkCreateGraphicsPipelines(context.device,{},1,&pc,nullptr,&pipelines[1]));
        } catch(...) {cleanup();throw;}
    }
    void cleanup(){resources.reset();for(auto p:pipelines)if(p)vkDestroyPipeline(context.device,p,nullptr);for(auto m:modules)if(m)vkDestroyShaderModule(context.device,m,nullptr);if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);if(descriptors)vkDestroyDescriptorSetLayout(context.device,descriptors,nullptr);if(pass)vkDestroyRenderPass(context.device,pass,nullptr);}
    ~Impl(){if(pending)vkWaitForFences(context.device,1,&pending,VK_TRUE,std::numeric_limits<std::uint64_t>::max());cleanup();}
    Buffer upload(Resources &r,unsigned slot,const void *data,VkDeviceSize size,VkBufferUsageFlags usage) {
        auto &b=r.buffers[slot];VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=size;ci.usage=usage;check(vkCreateBuffer(context.device,&ci,nullptr,&b.handle));
        VkMemoryRequirements req;vkGetBufferMemoryRequirements(context.device,b.handle,&req);VkPhysicalDeviceMemoryProperties props;vkGetPhysicalDeviceMemoryProperties(context.physical,&props);unsigned type=props.memoryTypeCount;
        for(unsigned i=0;i<props.memoryTypeCount;++i)if((req.memoryTypeBits&(1u<<i))&&(props.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
        if(type==props.memoryTypeCount)throw FormatError("source sky stencil coherent upload memory unavailable");
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=type;check(vkAllocateMemory(context.device,&a,nullptr,&b.memory));check(vkBindBufferMemory(context.device,b.handle,b.memory,0));void *mapped;check(vkMapMemory(context.device,b.memory,0,size,0,&mapped));std::memcpy(mapped,data,size);vkUnmapMemory(context.device,b.memory);return b;
    }
    std::unique_ptr<Resources> prepare_image(const SourceWeatherDepthAliasImage &s) {
        if(pending)throw FormatError("source sky stencil submission must be retired before prepare");
        if(!s.image||s.owner!=context.device||s.queue_family!=context.queue_family||s.sharing!=VK_SHARING_MODE_EXCLUSIVE||s.format!=VK_FORMAT_D24_UNORM_S8_UINT||s.samples!=VK_SAMPLE_COUNT_1_BIT||s.mip_levels!=1||s.array_layers!=1||s.type!=VK_IMAGE_TYPE_2D||s.tiling!=VK_IMAGE_TILING_OPTIMAL||s.create_flags||!(s.usage&VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))throw FormatError("source sky stencil requires actual exclusive same-device/family ordinary D24S8 attachment");
        scope(s.layout);
        const auto &l=properties.limits;
        if(!s.extent.width||!s.extent.height||s.extent.width>l.maxFramebufferWidth||s.extent.height>l.maxFramebufferHeight||s.extent.width>l.maxViewportDimensions[0]||s.extent.height>l.maxViewportDimensions[1]||s.extent.width>l.viewportBoundsRange[1]||s.extent.height>l.viewportBoundsRange[1])throw FormatError("source sky stencil extent exceeds actual raster limits");
        VkImageFormatProperties supported;check(vkGetPhysicalDeviceImageFormatProperties(context.physical,s.format,s.type,s.tiling,s.usage,s.create_flags,&supported));
        if(s.extent.width>supported.maxExtent.width||s.extent.height>supported.maxExtent.height||!(supported.sampleCounts&VK_SAMPLE_COUNT_1_BIT))throw FormatError("source sky stencil actual image usage/extent unsupported");
        auto r=std::make_unique<Resources>();r->device=context.device;r->image=s;
        VkImageViewCreateInfo vc{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vc.image=s.image;vc.viewType=VK_IMAGE_VIEW_TYPE_2D;vc.format=s.format;vc.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};check(vkCreateImageView(context.device,&vc,nullptr,&r->view));
        VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fc.renderPass=pass;fc.attachmentCount=1;fc.pAttachments=&r->view;fc.width=s.extent.width;fc.height=s.extent.height;fc.layers=1;check(vkCreateFramebuffer(context.device,&fc,nullptr,&r->frame));
        const VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&ps;check(vkCreateDescriptorPool(context.device,&pool,nullptr,&r->pool));VkDescriptorSetAllocateInfo ac{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ac.descriptorPool=r->pool;ac.descriptorSetCount=1;ac.pSetLayouts=&descriptors;check(vkAllocateDescriptorSets(context.device,&ac,&r->set));
        return r;
    }
    void finish_prepare(std::unique_ptr<Resources> r,const SourceSkyStencilUniform &uniform) {
        upload(*r,2,&uniform,sizeof(uniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        const VkDescriptorBufferInfo bi{r->buffers[2].handle,0,sizeof(uniform)};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=r->set;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&bi;vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);resources=std::move(r);
    }
    void prepare(const SourceWeatherDepthAliasImage &s,const SourceSkyStencilDraw &d) {
        validate_source_sky_stencil_draw(d);auto r=prepare_image(s);r->index_count=d.indices.size();r->rotations=d.rotations;
        upload(*r,0,d.vertices.data(),d.vertices.size_bytes(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);upload(*r,1,d.indices.data(),d.indices.size_bytes(),VK_BUFFER_USAGE_INDEX_BUFFER_BIT);finish_prepare(std::move(r),d.uniform);
    }
    void prepare(const SourceWeatherDepthAliasImage &s,const SourceSkyStencilPostGeometryDraw &d) {
        validate_source_sky_stencil_draw(d);auto r=prepare_image(s);r->vertex_count=d.vertices.size();r->post_geometry=true;
        upload(*r,0,d.vertices.data(),d.vertices.size_bytes(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);finish_prepare(std::move(r),d.uniform);
    }
    void record(const SourceSkyStencilRecord &r) {
        if(!resources||!r.command||!r.completion)throw FormatError("source sky stencil requires prepared resources and actual command/fence");
        if(pending)throw FormatError("source sky stencil submission must be retired before record");
        if(r.command_owner!=context.device||r.command_queue_family!=context.queue_family||!r.outside_render_pass||!r.exclusive_access)throw FormatError("source sky stencil requires exclusive same-device/family command outside render pass");
        auto result=vkGetFenceStatus(context.device,r.completion);if(result==VK_SUCCESS)throw FormatError("source sky stencil completion fence must be reset/unsignaled");if(result!=VK_NOT_READY)check(result);
        auto &p=*resources;const auto &s=p.image;
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_INDEX_READ_BIT|VK_ACCESS_UNIFORM_READ_BIT;
        vkCmdPipelineBarrier(r.command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
        barrier(r.command,s.image,s.layout,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,scope(s.layout),attachment);
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=p.frame;begin.renderArea={{0,0},s.extent};vkCmdBeginRenderPass(r.command,&begin,VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0,0,float(s.extent.width),float(s.extent.height),0,1};const VkRect2D rect{{0,0},s.extent};vkCmdSetViewport(r.command,0,1,&viewport);vkCmdSetScissor(r.command,0,1,&rect);vkCmdBindPipeline(r.command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipelines[p.post_geometry?1:0]);
        VkDeviceSize zero=0;vkCmdBindVertexBuffers(r.command,0,1,&p.buffers[0].handle,&zero);vkCmdBindDescriptorSets(r.command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&p.set,0,nullptr);
        if(p.post_geometry)vkCmdDraw(r.command,p.vertex_count,1,0,0);
        else {vkCmdBindIndexBuffer(r.command,p.buffers[1].handle,0,VK_INDEX_TYPE_UINT32);vkCmdPushConstants(r.command,layout,VK_SHADER_STAGE_GEOMETRY_BIT,0,8,p.rotations.data());vkCmdDrawIndexed(r.command,p.index_count,1,0,0,0);}
        vkCmdEndRenderPass(r.command);
        barrier(r.command,s.image,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,s.layout,attachment,scope(s.layout));pending=r.completion;
    }
};
SourceSkyStencil::SourceSkyStencil(SourceSkyStencilContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceSkyStencil::~SourceSkyStencil()=default;
void SourceSkyStencil::prepare(const SourceWeatherDepthAliasImage &s,const SourceSkyStencilDraw &d){impl_->prepare(s,d);}
void SourceSkyStencil::prepare(const SourceWeatherDepthAliasImage &s,const SourceSkyStencilPostGeometryDraw &d){impl_->prepare(s,d);}
void SourceSkyStencil::record(const SourceSkyStencilRecord &r){impl_->record(r);}
void SourceSkyStencil::retire(VkFence fence) {
    if(!fence||fence!=impl_->pending)throw FormatError("source sky stencil requires its outstanding submission fence");const auto result=vkGetFenceStatus(impl_->context.device,fence);if(result!=VK_SUCCESS){if(result!=VK_NOT_READY)check(result);throw FormatError("source sky stencil submission has not completed");}impl_->pending={};
}
}
