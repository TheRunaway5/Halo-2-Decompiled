#include "halo2/source_weather_line_render.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include "halo2/source_weather_assets.hpp"

namespace halo2 {
namespace {
void check(VkResult r){if(r!=VK_SUCCESS)throw FormatError("weather line Vulkan failure: "+std::to_string(r));}
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

}
struct SourceWeatherLineRender::Impl {
    SourceWeatherLineRenderContext context;
    VkPhysicalDeviceProperties properties{};
    VkDescriptorSetLayout descriptor_layout{};VkPipelineLayout layout{};
    std::array<VkShaderModule,3> shaders{};
    VkDescriptorPool descriptor_pool{};VkDescriptorSet descriptor{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};};
    std::array<Buffer,3> buffers{};
    std::map<std::pair<unsigned,bool>,VkPipeline> pipelines;
    VkPipeline current{};std::uint32_t count{};VkExtent2D prepared_surface{};float line_width=1;
    explicit Impl(SourceWeatherLineRenderContext c):context(std::move(c)) {
        try {
            if(!context.device||!context.physical||!context.pass||!context.geometry_shader_enabled)
                throw FormatError("weather rain requires an actual pass/device with geometryShader enabled");
            vkGetPhysicalDeviceProperties(context.physical,&properties);
            const VkDescriptorSetLayoutBinding bindings[]{
                {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
            };
            VkDescriptorSetLayoutCreateInfo si{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};si.bindingCount=2;si.pBindings=bindings;
            check(vkCreateDescriptorSetLayout(context.device,&si,nullptr,&descriptor_layout));
            VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&descriptor_layout;
            check(vkCreatePipelineLayout(context.device,&li,nullptr,&layout));
            const char *names[]{"source_weather_line.vert.spv","source_weather_line.geom.spv","source_weather_line.frag.spv"};
            for(unsigned i=0;i<3;++i){
                std::ifstream in(context.shader_directory/names[i],std::ios::binary|std::ios::ate);
                if(!in||in.tellg()<=0||in.tellg()%4)throw FormatError("compiled weather line shader missing");
                std::vector<std::uint32_t> code(std::size_t(in.tellg())/4);in.seekg(0);
                if(!in.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("short weather line shader");
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
        throw FormatError("weather line coherent memory unavailable");
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
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,shaders[1],"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,shaders[2],"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(SourceWeatherLineInput),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{{0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0},{1,0,VK_FORMAT_R32_UINT,16}};
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&binding;vi.vertexAttributeDescriptionCount=2;vi.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.depthClampEnable=clamp;rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=true;ds.depthWriteEnable=false;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState cb{};cb.blendEnable=true;cb.srcColorBlendFactor=cb.srcAlphaBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;cb.dstColorBlendFactor=cb.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;cb.colorBlendOp=cb.alphaBlendOp=VK_BLEND_OP_ADD;cb.colorWriteMask=mask;
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_LINE_WIDTH};VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dy.dynamicStateCount=3;dy.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=3;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pDepthStencilState=&ds;pi.pColorBlendState=&bs;pi.pDynamicState=&dy;pi.layout=layout;pi.renderPass=context.pass;
        VkPipeline result;check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&pi,nullptr,&result));pipelines.emplace(key,result);return result;
    }
    void clear() {
        if(descriptor_pool)vkDestroyDescriptorPool(context.device,descriptor_pool,nullptr);descriptor_pool={};descriptor={};
        for(auto &b:buffers){free(b);b={};}current={};count=0;prepared_surface={};
    }
    void prepare(const SourceWeatherLinePacket &p) {
        // All source admission checks precede destruction/mutation of queued
        // bindings. Caller synchronization remains mandatory for valid input.
        if(fingerprint(p.vertex_program)!=0x695a03e50e8dba7dull)
            throw FormatError("weather line program differs from original VS22/class0");
        const auto program=decode_vertex_program(p.vertex_program);
        if(program.count!=9||p.pixel.words!=source_weather_pixel_program(true).words)throw FormatError("unsupported weather line program/39780 pixel definition");
        if(p.raster.stencil_enabled)throw FormatError("weather rain requires explicit stencil-disabled state");
        if(p.vertices.size()%2)throw FormatError("weather rain requires complete immediate line pairs");
        if(!std::isfinite(p.raster.line_width) || p.raster.line_width<=0 ||
           p.raster.line_width<properties.limits.lineWidthRange[0] || p.raster.line_width>properties.limits.lineWidthRange[1] ||
           (p.raster.line_width!=1 && !context.wide_lines_enabled))
            throw FormatError("authored weather rain width is not admitted by the actual logical device");
        if(p.vertices.size()>0xffffffffu||p.raster.color_write_mask>15)
            throw FormatError("weather line draw count/color mask exceeds native bounds");
        for(const auto &c:p.camera)for(float v:c)if(!std::isfinite(v))throw FormatError("nonfinite weather camera input");
        for(float v:p.raster.clip_range)if(!std::isfinite(v))throw FormatError("nonfinite weather clip range");
        for(float v:p.raster.surface)if(!std::isfinite(v))throw FormatError("nonfinite weather surface input");
        if(p.raster.clip_range[0]!=0||p.raster.clip_range[1]!=16777215.f||p.raster.clip_range[2]>p.raster.clip_range[3]||
            !(p.raster.surface[0]>0&&p.raster.surface[1]>0)||!std::isfinite(p.raster.depth_factor)||!std::isfinite(p.raster.depth_offset))
            throw FormatError("weather line requires explicit valid D24/surface/depth inputs");
        for(unsigned a=0;a<2;++a)if(std::floor(p.raster.surface[a])!=p.raster.surface[a]||
            p.raster.surface[a]>float(properties.limits.maxViewportDimensions[a]))
            throw FormatError("weather line requires an admitted integer full-surface target");
        // Native rain currently admits explicit zero offset/factor. A different
        // inherited source offset contract requires its own evidence.
        if(p.raster.depth_factor!=0||p.raster.depth_offset!=0)
            throw FormatError("weather rain requires explicit zero depth-offset/factor");
        if(p.raster.depth_clamp&&!context.depth_clamp_enabled)throw FormatError("weather line depthClamp not enabled on logical device");
        for(const auto &point:p.vertices)for(float v:point.position)if(!std::isfinite(v))throw FormatError("nonfinite immediate weather input");
        clear();if(p.vertices.empty())return;
        try {
            VertexUniform v;v.instructions=program.instructions;v.camera=p.camera;v.flags={p.raster.depth_clipping,0,0,program.count};v.clip=p.raster.clip_range;v.surface=p.raster.surface;v.depth={p.raster.depth_factor,p.raster.depth_offset,0,0};
            buffers[0]=buffer(sizeof(v),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&v);
            buffers[1]=buffer(sizeof(p.pixel.words),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,p.pixel.words.data());
            buffers[2]=buffer(p.vertices.size_bytes(),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,p.vertices.data());
            const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2}};
            VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};ci.maxSets=1;ci.poolSizeCount=1;ci.pPoolSizes=sizes;
            check(vkCreateDescriptorPool(context.device,&ci,nullptr,&descriptor_pool));
            VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=descriptor_pool;ai.descriptorSetCount=1;ai.pSetLayouts=&descriptor_layout;
            check(vkAllocateDescriptorSets(context.device,&ai,&descriptor));
            const VkDescriptorBufferInfo bi[]{{buffers[0].handle,0,sizeof(v)},{buffers[1].handle,0,sizeof(p.pixel.words)}};
            VkWriteDescriptorSet w[2]{};
            for(unsigned i=0;i<2;++i){w[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w[i].dstSet=descriptor;w[i].dstBinding=i;w[i].descriptorCount=1;w[i].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w[i].pBufferInfo=&bi[i];}
            vkUpdateDescriptorSets(context.device,2,w,0,nullptr);current=pipeline(p.raster.color_write_mask,p.raster.depth_clamp);count=p.vertices.size();prepared_surface={unsigned(p.raster.surface[0]),unsigned(p.raster.surface[1])};line_width=p.raster.line_width;
        }catch(...){clear();throw;}
    }
    void record(VkCommandBuffer cmd,VkViewport viewport,VkRect2D scissor)const {
        if(!count)return;
        // The original gl/draw.c viewport addresses the entire render surface.
        // A second arbitrary subviewport would retransform already projected
        // source pixels. Split-view conversion is not admitted by this module.
        if(viewport.x!=0||viewport.y!=0||viewport.width!=float(prepared_surface.width)||viewport.height!=float(prepared_surface.height)||viewport.minDepth!=0||viewport.maxDepth!=1)
            throw FormatError("weather line record requires the prepared full-surface viewport");
        if(scissor.offset.x<0||scissor.offset.y<0||unsigned(scissor.offset.x)>prepared_surface.width||unsigned(scissor.offset.y)>prepared_surface.height||
            scissor.extent.width>prepared_surface.width-unsigned(scissor.offset.x)||scissor.extent.height>prepared_surface.height-unsigned(scissor.offset.y))
            throw FormatError("weather line scissor lies outside the prepared full surface");
        vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&scissor);
        const VkDeviceSize offset=0;vkCmdBindVertexBuffers(cmd,0,1,&buffers[2].handle,&offset);
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,current);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);
        vkCmdSetLineWidth(cmd,line_width);vkCmdDraw(cmd,count,1,0,0);
    }
    void release(){clear();for(auto [key,p]:pipelines)vkDestroyPipeline(context.device,p,nullptr);pipelines.clear();for(auto s:shaders)if(s)vkDestroyShaderModule(context.device,s,nullptr);shaders={};if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);layout={};if(descriptor_layout)vkDestroyDescriptorSetLayout(context.device,descriptor_layout,nullptr);descriptor_layout={};}
    ~Impl(){release();}
};
SourceWeatherLineRender::SourceWeatherLineRender(SourceWeatherLineRenderContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceWeatherLineRender::~SourceWeatherLineRender()=default;
void SourceWeatherLineRender::prepare(const SourceWeatherLinePacket &p){impl_->prepare(p);}
void SourceWeatherLineRender::record(VkCommandBuffer c,VkViewport v,VkRect2D s)const{impl_->record(c,v,s);}
}
