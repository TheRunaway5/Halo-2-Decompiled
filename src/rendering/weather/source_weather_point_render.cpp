#include "halo2/source_weather_point_render.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>

namespace halo2 {
namespace {
void check(VkResult r){if(r!=VK_SUCCESS)throw FormatError("weather point Vulkan failure: "+std::to_string(r));}
struct VertexUniform {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,12> camera{};
    std::array<std::uint32_t,4> flags{};
    ShaderVector clip{},surface{},depth{};
};
static_assert(sizeof(VertexUniform)==2432);
std::uint64_t fingerprint(std::span<const std::byte> bytes) {
    std::uint64_t h=14695981039346656037ull;
    for(auto b:bytes)h=(h^std::to_integer<unsigned>(b))*1099511628211ull;
    return h;
}
std::array<std::uint32_t,60> original_pixel() {
    std::array<std::uint32_t,60> w{};
    w[0]=0xd4db1010;w[8]=0x0c200000;w[9]=0x1c80;w[26]=0xc0;
    w[34]=0xc4cb0000;w[45]=0xc0;w[53]=0x11101;w[54]=0x8000;
    w[57]=w[58]=0xffffffff;w[59]=0x1ff;return w;
}
}
struct SourceWeatherPointRender::Impl {
    SourceWeatherPointRenderContext context;
    VkPhysicalDeviceProperties properties{};
    VkDescriptorSetLayout descriptor_layout{};VkPipelineLayout layout{};
    std::array<VkShaderModule,2> shaders{};
    VkDescriptorPool descriptor_pool{};VkDescriptorSet descriptor{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};};
    std::array<Buffer,3> buffers{};
    std::map<std::pair<unsigned,bool>,VkPipeline> pipelines;
    VkPipeline current{};std::uint32_t count{};VkExtent2D prepared_surface{};
    explicit Impl(SourceWeatherPointRenderContext c):context(std::move(c)) {
        try {
            if(!context.device||!context.physical||!context.pass||!context.large_points_enabled)
                throw FormatError("weather point context requires an actual pass/device with largePoints enabled");
            VkPhysicalDeviceFeatures f;vkGetPhysicalDeviceFeatures(context.physical,&f);
            if(!f.largePoints)throw FormatError("weather point physical device lacks largePoints");
            vkGetPhysicalDeviceProperties(context.physical,&properties);
            const VkDescriptorSetLayoutBinding bindings[]{
                {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
            VkDescriptorSetLayoutCreateInfo si{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};si.bindingCount=3;si.pBindings=bindings;
            check(vkCreateDescriptorSetLayout(context.device,&si,nullptr,&descriptor_layout));
            VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&descriptor_layout;
            check(vkCreatePipelineLayout(context.device,&li,nullptr,&layout));
            const char *names[]{"source_weather_point.vert.spv","source_weather_point.frag.spv"};
            for(unsigned i=0;i<2;++i){
                std::ifstream in(context.shader_directory/names[i],std::ios::binary|std::ios::ate);
                if(!in||in.tellg()<=0||in.tellg()%4)throw FormatError("compiled weather point shader missing");
                std::vector<std::uint32_t> code(std::size_t(in.tellg())/4);in.seekg(0);
                if(!in.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("short weather point shader");
                VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=code.size()*4;sm.pCode=code.data();
                check(vkCreateShaderModule(context.device,&sm,nullptr,&shaders[i]));
            }
        }catch(...){release();throw;}
    }
    unsigned memory(unsigned bits) const {
        VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(context.physical,&p);
        for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i))&&
            (p.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))return i;
        throw FormatError("weather point coherent memory unavailable");
    }
    Buffer buffer(std::size_t size,VkBufferUsageFlags usage,const void *data) {
        Buffer b;
        try {
            VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=size;ci.usage=usage;
            check(vkCreateBuffer(context.device,&ci,nullptr,&b.handle));VkMemoryRequirements r;vkGetBufferMemoryRequirements(context.device,b.handle,&r);
            VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=r.size;ai.memoryTypeIndex=memory(r.memoryTypeBits);
            check(vkAllocateMemory(context.device,&ai,nullptr,&b.memory));check(vkBindBufferMemory(context.device,b.handle,b.memory,0));
            void *mapped;check(vkMapMemory(context.device,b.memory,0,size,0,&mapped));std::memcpy(mapped,data,size);vkUnmapMemory(context.device,b.memory);
            return b;
        }catch(...){free(b);throw;}
    }
    void free(Buffer b){if(b.handle)vkDestroyBuffer(context.device,b.handle,nullptr);if(b.memory)vkFreeMemory(context.device,b.memory,nullptr);}
    VkPipeline pipeline(unsigned mask,bool clamp) {
        const auto key=std::pair(mask,clamp);if(auto it=pipelines.find(key);it!=pipelines.end())return it->second;
        VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,shaders[0],"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,shaders[1],"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(SourceWeatherPointInput),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32_UINT,16}};
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&binding;vi.vertexAttributeDescriptionCount=2;vi.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.depthClampEnable=clamp;rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=true;ds.depthWriteEnable=false;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState cb{};cb.blendEnable=true;cb.srcColorBlendFactor=cb.srcAlphaBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;cb.dstColorBlendFactor=cb.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;cb.colorBlendOp=cb.alphaBlendOp=VK_BLEND_OP_ADD;cb.colorWriteMask=mask;
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dy.dynamicStateCount=2;dy.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=2;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pDepthStencilState=&ds;pi.pColorBlendState=&bs;pi.pDynamicState=&dy;pi.layout=layout;pi.renderPass=context.pass;
        VkPipeline result;check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&pi,nullptr,&result));pipelines.emplace(key,result);return result;
    }
    void clear() {
        if(descriptor_pool)vkDestroyDescriptorPool(context.device,descriptor_pool,nullptr);descriptor_pool={};descriptor={};
        for(auto &b:buffers){free(b);b={};}current={};count=0;prepared_surface={};
    }
    void prepare(const SourceWeatherPointPacket &p,bool native_point_range=false) {
        // All source admission checks precede destruction/mutation of queued
        // bindings. Caller synchronization remains mandatory for valid input.
        if(fingerprint(p.vertex_program)!=0x695a03e50e8dba7dull)
            throw FormatError("weather point program differs from original VS22/class0");
        const auto program=decode_vertex_program(p.vertex_program);
        if(program.count!=9||p.pixel.words!=original_pixel())throw FormatError("unsupported weather point program/39780 pixel definition");
        if(p.raster.point_parameters_enabled!=1||p.raster.point_sprite_enabled!=1)
            throw FormatError("weather point prototype requires actual enabled programmable points/sprites");
        if(!p.raster.stencil_enabled||*p.raster.stencil_enabled)
            throw FormatError("weather point prototype requires explicit source stencil-disabled state");
        if(!p.raster.sprite_coordinates||*p.raster.sprite_coordinates!=SourceWeatherPointSpriteCoordinates::OriginalGlRawRowsUpperLeft)
            throw FormatError("weather point prototype requires explicit original GL raw-row/upper-left point coordinates");
        if(!p.sprite_view||!p.sprite_sampler||p.sprite_format!=VK_FORMAT_BC3_UNORM_BLOCK||p.sprite_extent.width!=32||p.sprite_extent.height!=32||p.sprite_mips!=4)
            throw FormatError("weather point prototype requires the actual32-square/four-mipBC3 sprite binding");
        if(p.points.size()>0xffffffffu||p.raster.color_write_mask>15)
            throw FormatError("weather point draw count/color mask exceeds native bounds");
        for(const auto &c:p.camera)for(float v:c)if(!std::isfinite(v))throw FormatError("nonfinite weather camera input");
        for(float v:p.raster.clip_range)if(!std::isfinite(v))throw FormatError("nonfinite weather clip range");
        for(float v:p.raster.surface)if(!std::isfinite(v))throw FormatError("nonfinite weather surface input");
        if(p.raster.clip_range[0]!=0||p.raster.clip_range[1]!=16777215.f||p.raster.clip_range[2]>p.raster.clip_range[3]||
            !(p.raster.surface[0]>0&&p.raster.surface[1]>0)||!std::isfinite(p.raster.depth_factor)||!std::isfinite(p.raster.depth_offset))
            throw FormatError("weather point requires explicit valid D24/surface/depth inputs");
        for(unsigned a=0;a<2;++a)if(std::floor(p.raster.surface[a])!=p.raster.surface[a]||
            p.raster.surface[a]>float(properties.limits.maxViewportDimensions[a]))
            throw FormatError("weather point requires an admitted integer full-surface target");
        // Pinned source point raster does not enable polygon offset for a
        // POINTS primitive. A different per-primitive contract is unproved.
        if(p.raster.depth_factor!=0||p.raster.depth_offset!=0)
            throw FormatError("weather point requires original zero point depth-offset/factor");
        if(p.raster.depth_clamp&&!context.depth_clamp_enabled)throw FormatError("weather point depthClamp not enabled on logical device");
        for(const auto &point:p.points)for(float v:point.position)if(!std::isfinite(v))throw FormatError("nonfinite immediate weather input");
        for(const auto &point:p.points){
            VertexProgramState state;std::copy(p.camera.begin(),p.camera.end(),state.constants.begin());state.attributes[0]=point.position;
            state.attributes[1]={float((point.color>>16)&255)/255,float((point.color>>8)&255)/255,float(point.color&255)/255,float(point.color>>24)/255};
            execute_vertex_program(program,state);
            for(float v:state.outputs[0])if(!std::isfinite(v))throw FormatError("weather point program produces a nonfinite position");
            const auto w=state.outputs[0][3],diameter=state.outputs[6][0];
            if(!(w>0)||!std::isfinite(diameter)||!(diameter>0))
                throw FormatError("weather point program produces unproved nonpositive W/point diameter");
            if(!native_point_range && (diameter<properties.limits.pointSizeRange[0]||diameter>properties.limits.pointSizeRange[1]))
                throw FormatError("weather point diameter exceeds actual native pointSizeRange; no shader cap is substituted");
        }
        clear();if(p.points.empty())return;
        try {
            VertexUniform v;v.instructions=program.instructions;v.camera=p.camera;v.flags={p.raster.depth_clipping,1,native_point_range,program.count};v.clip=p.raster.clip_range;v.surface=p.raster.surface;v.depth={p.raster.depth_factor,p.raster.depth_offset,properties.limits.pointSizeRange[0],properties.limits.pointSizeRange[1]};
            buffers[0]=buffer(sizeof(v),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&v);
            buffers[1]=buffer(sizeof(p.pixel.words),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,p.pixel.words.data());
            buffers[2]=buffer(p.points.size_bytes(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,p.points.data());
            const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1}};
            VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};ci.maxSets=1;ci.poolSizeCount=2;ci.pPoolSizes=sizes;
            check(vkCreateDescriptorPool(context.device,&ci,nullptr,&descriptor_pool));
            VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=descriptor_pool;ai.descriptorSetCount=1;ai.pSetLayouts=&descriptor_layout;
            check(vkAllocateDescriptorSets(context.device,&ai,&descriptor));
            const VkDescriptorBufferInfo bi[]{{buffers[0].handle,0,sizeof(v)},{buffers[1].handle,0,sizeof(p.pixel.words)}};
            const VkDescriptorImageInfo ii{p.sprite_sampler,p.sprite_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet w[3]{};
            for(unsigned i=0;i<3;++i){w[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w[i].dstSet=descriptor;w[i].dstBinding=i;w[i].descriptorCount=1;w[i].descriptorType=i==2?VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;if(i==2)w[i].pImageInfo=&ii;else w[i].pBufferInfo=&bi[i];}
            vkUpdateDescriptorSets(context.device,3,w,0,nullptr);current=pipeline(p.raster.color_write_mask,p.raster.depth_clamp);count=p.points.size();prepared_surface={unsigned(p.raster.surface[0]),unsigned(p.raster.surface[1])};
        }catch(...){clear();throw;}
    }
    void record(VkCommandBuffer cmd,VkViewport viewport,VkRect2D scissor)const {
        if(!count)return;
        // The original gl/draw.c viewport addresses the entire render surface.
        // A second arbitrary subviewport would retransform already projected
        // source pixels. Split-view conversion is not admitted by this module.
        if(viewport.x!=0||viewport.y!=0||viewport.width!=float(prepared_surface.width)||viewport.height!=float(prepared_surface.height)||viewport.minDepth!=0||viewport.maxDepth!=1)
            throw FormatError("weather point record requires the prepared full-surface viewport");
        if(scissor.offset.x<0||scissor.offset.y<0||unsigned(scissor.offset.x)>prepared_surface.width||unsigned(scissor.offset.y)>prepared_surface.height||
            scissor.extent.width>prepared_surface.width-unsigned(scissor.offset.x)||scissor.extent.height>prepared_surface.height-unsigned(scissor.offset.y))
            throw FormatError("weather point scissor lies outside the prepared full surface");
        vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&scissor);
        const VkDeviceSize offset=0;vkCmdBindVertexBuffers(cmd,0,1,&buffers[2].handle,&offset);
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,current);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);
        vkCmdDraw(cmd,count,1,0,0);
    }
    void release(){clear();for(auto [key,p]:pipelines)vkDestroyPipeline(context.device,p,nullptr);pipelines.clear();for(auto s:shaders)if(s)vkDestroyShaderModule(context.device,s,nullptr);shaders={};if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);layout={};if(descriptor_layout)vkDestroyDescriptorSetLayout(context.device,descriptor_layout,nullptr);descriptor_layout={};}
    ~Impl(){release();}
};
SourceWeatherPointRender::SourceWeatherPointRender(SourceWeatherPointRenderContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceWeatherPointRender::~SourceWeatherPointRender()=default;
void SourceWeatherPointRender::prepare(const SourceWeatherPointPacket &p){impl_->prepare(p);}
void SourceWeatherPointRender::prepare_world(const SourceWeatherPointPacket &p){
    const auto program=decode_vertex_program(p.vertex_program);
    std::vector<SourceWeatherPointInput> visible;
    visible.reserve(p.points.size());
    for(const auto &point:p.points) {
        for(float value:point.position)if(!std::isfinite(value))
            throw FormatError("nonfinite live weather point");
        VertexProgramState state;
        std::copy(p.camera.begin(),p.camera.end(),state.constants.begin());
        state.attributes[0]=point.position;
        execute_vertex_program(program,state);
        const float w=state.outputs[0][3];
        if(!std::isfinite(w))throw FormatError("nonfinite live weather W");
        // The wrapper multiplies clip XYZ by this W. Negative homogeneous W
        // cannot enter the clip volume; each point fragment has constant W.
        if(w<=0 || (p.raster.depth_clipping &&
            (w<p.raster.clip_range[2] || w>p.raster.clip_range[3])))continue;
        visible.push_back(point);
    }
    auto clipped=p;clipped.points=visible;
    impl_->prepare(clipped,true);
}
void SourceWeatherPointRender::record(VkCommandBuffer c,VkViewport v,VkRect2D s)const{impl_->record(c,v,s);}
}
