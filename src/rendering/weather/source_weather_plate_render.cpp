#include "halo2/source_weather_plate_render.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>

namespace halo2 {
namespace {
void check(VkResult r){if(r!=VK_SUCCESS)throw FormatError("weather plate Vulkan failure: "+std::to_string(r));}
struct VertexUniform {
    std::array<ShaderVector,3> scroll{};
    ShaderVector clip{},surface{};
    std::array<std::uint32_t,4> flags{};
    std::array<std::array<std::int32_t,4>,8> regions{};
};
static_assert(sizeof(VertexUniform)==224);
struct PixelUniform {
    std::array<std::uint32_t,60> program{};
    std::array<ShaderVector,18> constants{};
};
static_assert(sizeof(PixelUniform)==528);
std::uint64_t fingerprint(std::span<const std::byte> bytes) {
    std::uint64_t h=14695981039346656037ull;
    for(auto b:bytes)h=(h^std::to_integer<unsigned>(b))*1099511628211ull;
    return h;
}
std::array<std::uint32_t,60> original_pixel() {
    // Complete unchanged3A5C0 mode3 output, actual prior-391F0 definition,
    // all captured original event215778 expanded constants checked exactly.
    return {0x00000000u,0x04209140u,0x18201140u,0xdd202df1u,0x1d201d20u,0x1d201d00u,0x1d120c02u,0x1d2d0000u,0x1d010000u,0x00001d00u,0x00ff0000u,0x80000000u,0x02000000u,0x01000000u,0x00008000u,0x0000e500u,0x00000000u,0x00000000u,0x0000ff00u,0x00000000u,0x00000000u,0x00000000u,0x0000cc00u,0x00000000u,0xcc000019u,0x00000000u,0x00000000u,0x00000c00u,0x00000d00u,0x00000d00u,0x00020d00u,0x00020d00u,0x00010d00u,0x000000d0u,0x08010802u,0x04201140u,0x00002020u,0xcc202d20u,0x19011a02u,0x24251b01u,0x04250000u,0x00000000u,0x00000000u,0x00ffffffu,0x00000000u,0x00003045u,0x00000c00u,0x00004d00u,0x00000c00u,0x00003045u,0x00001045u,0x000000d0u,0x00000000u,0x00011108u,0x00008421u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u};
}
}
struct SourceWeatherPlateRender::Impl {
    SourceWeatherPlateRenderContext context;
    VkPhysicalDeviceProperties properties{};
    VkDescriptorSetLayout descriptor_layout{};VkPipelineLayout layout{};
    std::array<VkShaderModule,2> shaders{};
    VkDescriptorPool descriptor_pool{};VkDescriptorSet descriptor{};
    struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};};
    std::array<Buffer,3> buffers{};
    std::map<bool,VkPipeline> pipelines;
    VkPipeline current{};std::uint32_t count{};VkExtent2D prepared_surface{};
    explicit Impl(SourceWeatherPlateRenderContext c):context(std::move(c)) {
        try {
            if(!context.device||!context.physical||!context.pass)
                throw FormatError("weather plate context requires an actual pass/device");
            VkPhysicalDeviceFeatures f;vkGetPhysicalDeviceFeatures(context.physical,&f);
            if(context.depth_clamp_enabled&&!f.depthClamp)
                throw FormatError("weather plate physical device lacks depthClamp");
            vkGetPhysicalDeviceProperties(context.physical,&properties);
            const VkDescriptorSetLayoutBinding bindings[]{
                {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
                {5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
            VkDescriptorSetLayoutCreateInfo si{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};si.bindingCount=6;si.pBindings=bindings;
            check(vkCreateDescriptorSetLayout(context.device,&si,nullptr,&descriptor_layout));
            VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&descriptor_layout;
            check(vkCreatePipelineLayout(context.device,&li,nullptr,&layout));
            const char *names[]{"source_weather_plate.vert.spv","source_weather_plate.frag.spv"};
            for(unsigned i=0;i<2;++i){
                std::ifstream in(context.shader_directory/names[i],std::ios::binary|std::ios::ate);
                if(!in||in.tellg()<=0||in.tellg()%4)throw FormatError("compiled weather plate shader missing");
                std::vector<std::uint32_t> code(std::size_t(in.tellg())/4);in.seekg(0);
                if(!in.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("short weather plate shader");
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
        throw FormatError("weather plate coherent memory unavailable");
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
    VkPipeline pipeline(bool clamp) {
        const auto key=clamp;if(auto it=pipelines.find(key);it!=pipelines.end())return it->second;
        VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,shaders[0],"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,shaders[1],"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(SourceWeatherPlateVertex),VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[5]{};
        for(unsigned i=0;i<5;++i)attributes[i]={i,0,VK_FORMAT_R32G32B32A32_SFLOAT,i*16};
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&binding;vi.vertexAttributeDescriptionCount=5;vi.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.depthClampEnable=clamp;rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=true;ds.depthWriteEnable=false;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState cb{};cb.blendEnable=true;cb.srcColorBlendFactor=cb.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;cb.dstColorBlendFactor=cb.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;cb.colorBlendOp=cb.alphaBlendOp=VK_BLEND_OP_ADD;cb.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dy.dynamicStateCount=2;dy.pDynamicStates=states;
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=2;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pDepthStencilState=&ds;pi.pColorBlendState=&bs;pi.pDynamicState=&dy;pi.layout=layout;pi.renderPass=context.pass;
        VkPipeline result;check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&pi,nullptr,&result));pipelines.emplace(key,result);return result;
    }
    void clear() {
        if(descriptor_pool)vkDestroyDescriptorPool(context.device,descriptor_pool,nullptr);descriptor_pool={};descriptor={};
        for(auto &b:buffers){free(b);b={};}current={};count=0;prepared_surface={};
    }
    void prepare(const SourceWeatherPlatePacket &p,const std::array<std::uint32_t,60> &expected) {
        if(fingerprint(p.vertex_program)!=0xc11e822b62bd252aull)
            throw FormatError("weather plate requires the exact original VS21/class0");
        const auto program=decode_vertex_program(p.vertex_program);
        if(program.count!=5)throw FormatError("weather plate requires all five original VS21 instructions");
        if(p.pixel.words!=expected)throw FormatError("weather plate requires the admitted complete original mode3 pixel definition");
        for(const auto &c:p.scroll)for(float v:c)if(!std::isfinite(v))throw FormatError("nonfinite original plate scroll");
        for(float v:p.clip_range)if(!std::isfinite(v))throw FormatError("nonfinite original plate clip range");
        for(float v:p.surface)if(!std::isfinite(v))throw FormatError("nonfinite original plate surface");
        if(p.clip_range[0]!=0 || p.clip_range[1]!=16777215.f || p.clip_range[2]>p.clip_range[3])
            throw FormatError("weather plate requires explicit original D24 clip range");
        for(unsigned a=0;a<2;++a)if(!(p.surface[a]>0) || std::floor(p.surface[a])!=p.surface[a] ||
            p.surface[a]>float(properties.limits.maxViewportDimensions[a]))
            throw FormatError("weather plate requires admitted integer full-surface dimensions");
        if(p.depth_clamp&&!context.depth_clamp_enabled)throw FormatError("weather plate logical device depthClamp is not enabled");
        const auto &first=p.vertices[0].registers[0];
        if(!(first[3]>0)||first[2]<0||first[2]>16777215.f)
            throw FormatError("weather plate projected Z/positive W outside admitted range");
        for(const auto &v:p.vertices) {
            for(const auto &reg:v.registers)for(float value:reg)if(!std::isfinite(value))throw FormatError("nonfinite original plate immediate input");
            if(v.registers[0][2]!=first[2]||v.registers[0][3]!=first[3])
                throw FormatError("weather plate requires original equal projected Z/W at all four vertices");
            for(unsigned i=1;i<5;++i)if(v.registers[i][2]!=0||v.registers[i][3]!=1)
                throw FormatError("weather plate requires the original immediate UV0/1 components");
        }
        for(unsigned i=0;i<4;++i) {
            const auto &t=p.textures[i];
            if(!t.view||!t.sampler||t.format!=VK_FORMAT_R8G8B8A8_UNORM||!t.extent.width||!t.extent.height||!t.mips)
                throw FormatError("weather plate requires actual original RGBA alias/layer bindings");
            if(i==0&&(t.extent.width!=unsigned(p.surface[0])||t.extent.height!=unsigned(p.surface[1])||t.mips!=1))
                throw FormatError("weather plate requires the actual full-surface packed RGBA depth alias");
        }
        // Finite inputs alone do not establish finite shader outputs. Check
        // the actual masked ADDs and every intermediate projection operation
        // before releasing the existing descriptor/vertex binding.
        for(const auto &v:p.vertices) {
            VertexProgramState state;
            std::copy(v.registers.begin(),v.registers.end(),state.attributes.begin());
            std::copy(p.scroll.begin(),p.scroll.end(),state.constants.begin()+21);
            execute_vertex_program(program,state);
            for(unsigned i: {0u,9u,10u,11u,12u})for(float value:state.outputs[i])
                if(!std::isfinite(value))throw FormatError("weather plate program produces a nonfinite output");
            const auto finite=[](float value){
                if(!std::isfinite(value))throw FormatError("weather plate projection produces a nonfinite intermediate");
                return value;
            };
            const auto w=std::clamp(first[3],std::bit_cast<float>(0x1f800000u),std::bit_cast<float>(0x5f800000u));
            for(unsigned xy=0;xy<2;++xy){
                const auto screen=finite(std::trunc(finite(v.registers[0][xy]*16.f))/16.f);
                const auto clip=finite(finite(finite(2.f*screen)-p.surface[xy])/p.surface[xy]);
                finite(clip*w);
            }
            finite(finite(first[2]/p.clip_range[1])*w);
        }
        clear();
        try {
            VertexUniform v;v.scroll=p.scroll;v.clip=p.clip_range;v.surface=p.surface;v.flags={p.depth_clipping,0,0,0};v.regions=p.clip_regions;
            PixelUniform pixel;pixel.program=p.pixel.words;
            const auto expand=[](std::uint32_t word){return ShaderVector{float((word>>16)&255)/255.f,float((word>>8)&255)/255.f,float(word&255)/255.f,float(word>>24)/255.f};};
            for(unsigned i=0;i<8;++i){pixel.constants[i*2]=expand(pixel.program[10+i]);pixel.constants[i*2+1]=expand(pixel.program[18+i]);}
            pixel.constants[16]=expand(pixel.program[43]);pixel.constants[17]=expand(pixel.program[44]);
            buffers[0]=buffer(sizeof(v),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&v);
            buffers[1]=buffer(sizeof(pixel),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,&pixel);
            buffers[2]=buffer(sizeof(p.vertices),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,p.vertices.data());
            const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4}};
            VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};ci.maxSets=1;ci.poolSizeCount=2;ci.pPoolSizes=sizes;
            check(vkCreateDescriptorPool(context.device,&ci,nullptr,&descriptor_pool));
            VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=descriptor_pool;ai.descriptorSetCount=1;ai.pSetLayouts=&descriptor_layout;
            check(vkAllocateDescriptorSets(context.device,&ai,&descriptor));
            const VkDescriptorBufferInfo bi[]{{buffers[0].handle,0,sizeof(v)},{buffers[1].handle,0,sizeof(pixel)}};
            VkDescriptorImageInfo ii[4]{};for(unsigned i=0;i<4;++i)ii[i]={p.textures[i].sampler,p.textures[i].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet writes[6]{};
            for(unsigned i=0;i<6;++i){auto &w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=descriptor;w.dstBinding=i;w.descriptorCount=1;w.descriptorType=i<2?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;if(i<2)w.pBufferInfo=&bi[i];else w.pImageInfo=&ii[i-2];}
            vkUpdateDescriptorSets(context.device,6,writes,0,nullptr);current=pipeline(p.depth_clamp);count=4;prepared_surface={unsigned(p.surface[0]),unsigned(p.surface[1])};
        }catch(...){clear();throw;}
    }
    void record(VkCommandBuffer cmd,VkViewport viewport,VkRect2D scissor)const {
        if(!count)return;
        // The original gl/draw.c viewport addresses the entire render surface.
        // A second arbitrary subviewport would retransform already projected
        // source pixels. Split-view conversion is not admitted by this module.
        if(viewport.x!=0||viewport.y!=0||viewport.width!=float(prepared_surface.width)||viewport.height!=float(prepared_surface.height)||viewport.minDepth!=0||viewport.maxDepth!=1)
            throw FormatError("weather plate record requires the prepared full-surface viewport");
        if(scissor.offset.x<0||scissor.offset.y<0||unsigned(scissor.offset.x)>prepared_surface.width||unsigned(scissor.offset.y)>prepared_surface.height||
            scissor.extent.width>prepared_surface.width-unsigned(scissor.offset.x)||scissor.extent.height>prepared_surface.height-unsigned(scissor.offset.y))
            throw FormatError("weather plate scissor lies outside the prepared full surface");
        vkCmdSetViewport(cmd,0,1,&viewport);vkCmdSetScissor(cmd,0,1,&scissor);
        const VkDeviceSize offset=0;vkCmdBindVertexBuffers(cmd,0,1,&buffers[2].handle,&offset);
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,current);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);
        vkCmdDraw(cmd,count,1,0,0);
    }
    void release(){clear();for(auto [key,p]:pipelines)vkDestroyPipeline(context.device,p,nullptr);pipelines.clear();for(auto s:shaders)if(s)vkDestroyShaderModule(context.device,s,nullptr);shaders={};if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);layout={};if(descriptor_layout)vkDestroyDescriptorSetLayout(context.device,descriptor_layout,nullptr);descriptor_layout={};}
    ~Impl(){release();}
};
SourceWeatherPlateRender::SourceWeatherPlateRender(SourceWeatherPlateRenderContext c):impl_(std::make_unique<Impl>(std::move(c))){}
SourceWeatherPlateRender::~SourceWeatherPlateRender()=default;
void SourceWeatherPlateRender::prepare(const SourceWeatherPlatePacket &p){impl_->prepare(p,original_pixel());}
void SourceWeatherPlateRender::prepare_admitted(const SourceWeatherPlatePacket &p,const std::array<std::uint32_t,60> &expected){impl_->prepare(p,expected);}
void SourceWeatherPlateRender::record(VkCommandBuffer c,VkViewport v,VkRect2D s)const{impl_->record(c,v,s);}
}
