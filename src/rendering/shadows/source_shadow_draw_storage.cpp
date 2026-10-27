#include "halo2/source_shadow_draw_storage.hpp"
#include "halo2/native_world_kernel.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/world_material_uniform.hpp"
#include "native_world_kernels.generated.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <tuple>

namespace halo2 {
namespace {
void check(VkResult result,const char *operation) {
    if(result!=VK_SUCCESS)
        throw std::runtime_error(std::string(operation)+": Vulkan result "+std::to_string(result));
}
std::vector<std::uint32_t> shader(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    const auto size=input.tellg();
    if(!input || size<=0 || size%4)throw FormatError("invalid shadow draw shader "+path.string());
    std::vector<std::uint32_t> result(std::size_t(size)/4);input.seekg(0);
    if(!input.read(reinterpret_cast<char *>(result.data()),size))
        throw FormatError("truncated shadow draw shader "+path.string());
    return result;
}
}
struct SourceShadowDrawStorage::Impl {
    SourceShadowDrawStorageContext context;
    SourceWorldDrawStorage storage;
    std::map<std::tuple<SourceShadowPhase,unsigned,bool,std::string_view,std::string_view>,VkPipeline> pipelines;
    explicit Impl(SourceShadowDrawStorageContext input):context(std::move(input)),
        storage({context.device,context.physical,context.texture_layout,context.material_uniform_size}) {
        if(!context.pipeline_layout || !context.generation_pass || !context.application_pass)
            throw FormatError("incomplete shadow pipeline context");
    }
    ~Impl() {
        for(const auto &[key,pipeline]:pipelines)vkDestroyPipeline(context.device,pipeline,nullptr);
    }
    std::span<const VkDescriptorSet> upload(std::span<const SourceShadowDrawUpload> draws) {
        std::vector<WorldMaterialUniform> uniforms;uniforms.reserve(draws.size());
        std::vector<SourceShadowDrawUpload> native;native.reserve(draws.size());
        for(const auto &draw:draws) {
            if(draw.vertex_bank.range!=std::array<std::uint32_t,4>{0,49,0,1})
                throw FormatError("incomplete queued shadow vertex bank");
            if(draw.material_uniform.size()!=sizeof(WorldMaterialUniform))
                throw FormatError("queued native shadow has an incompatible material ABI");
            WorldMaterialUniform value;std::memcpy(&value,draw.material_uniform.data(),sizeof(value));
            uniforms.push_back(native_material_gpu_uniform(value));
            auto copy=draw;copy.material_uniform={reinterpret_cast<const std::byte*>(&uniforms.back()),sizeof(value)};
            native.push_back(copy);
        }
        return storage.upload(native);
    }
    VkPipeline pipeline(const SourceShadowMaterial &material,SourceShadowPhase phase,SourceShadowRasterInput raster_input) {
        validate_source_shadow_plan_material(material,phase);
        if(raster_input.cull!=0 && raster_input.cull!=0x900 && raster_input.cull!=0x901)
            throw FormatError("invalid explicit shadow cull input");
        const auto& authored=material.material;
        const auto declaration=pack_source_vertex_declaration(authored.vertex_declaration,authored.world_vertex_input_transform);
        const auto found=std::find_if(generated_native_world_kernels::entries.begin(),generated_native_world_kernels::entries.end(),
            [&](const auto& entry) {
                const auto bytes=std::as_bytes(entry.vertex_program);
                // This explicit conduit always uploads original c0..48 with
                // camera_inputs=true, independently of the resident material.
                return entry.declaration==declaration && entry.extended_vertex_bank &&
                    bytes.size()==authored.vertex_program.size() && std::equal(bytes.begin(),bytes.end(),authored.vertex_program.begin());
            });
        if(found==generated_native_world_kernels::entries.end())
            throw FormatError("queued shadow has no complete native vertex kernel");
        const auto fragment_path=native_fragment_kernel_path("world",authored.forward_program.words);
        const auto key=std::tuple{phase,raster_input.cull,raster_input.depth_clamp,found->vertex_path,fragment_path};
        if(const auto found=pipelines.find(key);found!=pipelines.end())return found->second;
        const auto vertex=shader(context.shader_directory/found->vertex_path);
        const auto fragment=shader(context.shader_directory/fragment_path);
        VkShaderModule vert{},frag{};
        struct Modules {VkDevice device;VkShaderModule &vert,&frag;~Modules(){if(vert)vkDestroyShaderModule(device,vert,nullptr);if(frag)vkDestroyShaderModule(device,frag,nullptr);}} modules{context.device,vert,frag};
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};module.codeSize=vertex.size()*4;module.pCode=vertex.data();
        check(vkCreateShaderModule(context.device,&module,nullptr,&vert),"create original shadow world vertex shader");
        module.codeSize=fragment.size()*4;module.pCode=fragment.data();
        check(vkCreateShaderModule(context.device,&module,nullptr,&frag),"create original shadow world fragment shader");
        const VkPipelineShaderStageCreateInfo stages[]{
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vert,"main",nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,frag,"main",nullptr}};
        const VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,position)},
            {1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,color)},
            {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,uv)},
            {3,0,VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex,lightmap_uv)},
            {4,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,normal)},
            {5,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,tangent)},
            {6,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,binormal)},
            {7,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,incident)},
            {8,0,VK_FORMAT_R32_UINT,offsetof(Vertex,lightmap)},
            {9,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,prt_cluster)},
            {10,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(Vertex,prt_weights)},
            {11,0,VK_FORMAT_R32_UINT,offsetof(Vertex,source_stipple_level)},
            {12,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,source_local_position)},
            {13,0,VK_FORMAT_R32_SFLOAT,offsetof(Vertex,source_color_alpha)}};
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
        input.vertexAttributeDescriptionCount=14;input.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.depthClampEnable=raster_input.depth_clamp;raster.polygonMode=VK_POLYGON_MODE_FILL;
        raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.cullMode=raster_input.cull==0x901?VK_CULL_MODE_FRONT_BIT:raster_input.cull==0x900?VK_CULL_MODE_BACK_BIT:VK_CULL_MODE_NONE;
        raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        const bool receiver=phase==SourceShadowPhase::Apply;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable=receiver;depth.depthWriteEnable=VK_FALSE;depth.depthCompareOp=VK_COMPARE_OP_EQUAL;
        VkPipelineColorBlendAttachmentState blend{};blend.blendEnable=receiver;
        blend.colorBlendOp=blend.alphaBlendOp=VK_BLEND_OP_ADD;
        blend.srcColorBlendFactor=VK_BLEND_FACTOR_DST_COLOR;blend.dstColorBlendFactor=VK_BLEND_FACTOR_ZERO;
        blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
        blend.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
        if(!receiver)blend.colorWriteMask|=VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo color{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};color.attachmentCount=1;color.pAttachments=&blend;
        const VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamic_states;
        VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        create.stageCount=2;create.pStages=stages;create.pVertexInputState=&input;create.pInputAssemblyState=&assembly;
        create.pViewportState=&viewport;create.pRasterizationState=&raster;create.pMultisampleState=&samples;
        create.pDepthStencilState=&depth;create.pColorBlendState=&color;create.pDynamicState=&dynamic;
        create.layout=context.pipeline_layout;create.renderPass=receiver?context.application_pass:context.generation_pass;
        VkPipeline result{};check(vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&create,nullptr,&result),"create explicit authored shadow pipeline");
        pipelines.emplace(key,result);return result;
    }
};
SourceShadowDrawStorage::SourceShadowDrawStorage(SourceShadowDrawStorageContext context):impl_(std::make_unique<Impl>(std::move(context))){}
SourceShadowDrawStorage::~SourceShadowDrawStorage()=default;
std::span<const VkDescriptorSet> SourceShadowDrawStorage::upload(std::span<const SourceShadowDrawUpload> draws){return impl_->upload(draws);}
VkPipeline SourceShadowDrawStorage::pipeline(const SourceShadowMaterial &material,SourceShadowPhase phase,SourceShadowRasterInput raster){return impl_->pipeline(material,phase,raster);}
}
