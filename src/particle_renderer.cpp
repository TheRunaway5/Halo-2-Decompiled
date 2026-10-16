#include "halo2/particle_renderer.hpp"
#include "halo2/sampler.hpp"
#include "halo2/texture.hpp"
#include "halo2/vertex_program.hpp"
#include <bit>
#include "particle_program.hpp"
#include "particle_directional_program.hpp"
#include "particle_perpendicular_program.hpp"
#include "particle_model_program.hpp"
#include "particle_distortion_program.hpp"
#include <cstring>
#include <fstream>
#include <map>

namespace halo2 {
namespace {
void check(VkResult r) {
  if(r!=VK_SUCCESS)throw FormatError("particle graphics Vulkan failure: "+std::to_string(r));
}
VkBlendFactor blend_factor(unsigned value) {
  switch(value) {
  case 0:return VK_BLEND_FACTOR_ZERO;case 1:return VK_BLEND_FACTOR_ONE;
  case 0x300:return VK_BLEND_FACTOR_SRC_COLOR;case 0x301:return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  case 0x302:return VK_BLEND_FACTOR_SRC_ALPHA;case 0x303:return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  case 0x304:return VK_BLEND_FACTOR_DST_ALPHA;case 0x305:return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  case 0x306:return VK_BLEND_FACTOR_DST_COLOR;case 0x307:return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  case 0x308:return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
  default:throw FormatError("unsupported original particle blend factor");
  }
}
}
struct ParticleRenderer::Impl {
  VkPhysicalDevice physical;VkDevice device;VkQueue queue;VkRenderPass pass;
  VkPhysicalDeviceProperties properties{};VkCommandPool uploads{};
  VkDescriptorSetLayout set_layout{};VkPipelineLayout layout{};VkBuffer corners{};VkDeviceMemory corner_memory{};
  std::array<VkShaderModule,8> shaders{};
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};};
  struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkSampler sampler{};};
  struct MaterialGpu {
    std::shared_ptr<const ParticleMaterial> source;
    std::array<Image,2> images;VkPipeline pipeline{};bool external23{};
  };
  struct MaterialUniform {
    std::array<std::uint32_t,60> program;std::array<std::uint32_t,4> alpha;
    ShaderVector bump_matrix,coordinate_scale;
  };
  static_assert(sizeof(MaterialUniform)==288);
  struct ModelGpu {std::shared_ptr<const ParticleModelDefinition> source;Buffer vertices,indices;};
  struct DrawGpu {MaterialGpu *material{};VkDescriptorSet set{};ModelGpu *model{};unsigned first{},count{};
    bool scene_resolved{};mutable bool scene_recorded{};};
  std::map<const ParticleMaterial*,MaterialGpu> materials;
  std::map<const ParticleModelDefinition*,ModelGpu> models;
  std::vector<DrawGpu> draws;VkDescriptorPool descriptors{};
  Buffer uniforms;void *uniform_data{};std::size_t draw_count{},material_offset{},depth_offset{},draw_stride{};
  Impl(VkPhysicalDevice p,VkDevice d,VkQueue q,unsigned family,VkRenderPass rp,const std::filesystem::path &directory)
    :physical(p),device(d),queue(q),pass(rp) {
    try {
    VkPhysicalDeviceFeatures features{};vkGetPhysicalDeviceFeatures(physical,&features);
    if(!features.geometryShader || !features.shaderTessellationAndGeometryPointSize || !features.depthClamp)
      throw FormatError("original particle quads require geometry/point-size/depth-clamp device support");
    vkGetPhysicalDeviceProperties(physical,&properties);
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;
    check(vkCreateCommandPool(device,&pool,nullptr,&uploads));
    const std::array<VkDescriptorSetLayoutBinding,5> bindings{{
      {0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
      {1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
      {2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
      {4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
      {5,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}}};
    VkDescriptorSetLayoutCreateInfo si{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};si.bindingCount=bindings.size();si.pBindings=bindings.data();
    check(vkCreateDescriptorSetLayout(device,&si,nullptr,&set_layout));
    VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&set_layout;
    check(vkCreatePipelineLayout(device,&li,nullptr,&layout));
    constexpr std::array<std::array<std::int16_t,4>,4> original_corners{{{0,0,0,0},{1,0,0,0},{1,1,0,0},{0,1,0,0}}};
    const auto buffer=make_buffer(sizeof(original_corners),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,original_corners.data());
    corners=buffer.handle;corner_memory=buffer.memory;
    const char *names[]{"particle.vert.spv","particle.geom.spv","particle.frag.spv","particle_directional.vert.spv",
      "particle_perpendicular.vert.spv","particle_model.vert.spv","particle_model.geom.spv","particle_distortion.vert.spv"};
    for(unsigned n=0;n<shaders.size();++n) {
      std::ifstream file(directory/names[n],std::ios::binary|std::ios::ate);
      if(!file || file.tellg()<=0 || file.tellg()%4)throw FormatError("compiled original particle shader missing");
      std::vector<unsigned> code(std::size_t(file.tellg())/4);file.seekg(0);
      if(!file.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("short particle shader");
      VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=code.size()*4;sm.pCode=code.data();
      check(vkCreateShaderModule(device,&sm,nullptr,&shaders[n]));
    }
    } catch(...) {release();throw;}
  }
  unsigned memory_type(unsigned mask,VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(unsigned n=0;n<p.memoryTypeCount;++n)if((mask&(1u<<n)) && (p.memoryTypes[n].propertyFlags&flags)==flags)return n;
    throw FormatError("particle graphics memory unavailable");
  }
  Buffer make_buffer(std::size_t bytes,VkBufferUsageFlags usage,const void *data) {
    Buffer b;
    try {
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=bytes;ci.usage=usage;
    check(vkCreateBuffer(device,&ci,nullptr,&b.handle));VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,b.handle,&req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;
    ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&b.memory));check(vkBindBufferMemory(device,b.handle,b.memory,0));
    void *mapped;check(vkMapMemory(device,b.memory,0,bytes,0,&mapped));if(data)std::memcpy(mapped,data,bytes);vkUnmapMemory(device,b.memory);
    return b;
    }catch(...){free_buffer(b);throw;}
  }
  void free_buffer(Buffer b) {
    if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);
    if(b.memory)vkFreeMemory(device,b.memory,nullptr);
  }
  Image make_image(const Texture &input,const ShaderTextureState &state) {
    const auto source=prepare_signed_texture(input,state.color_sign);
    if((source.format!=11 && source.format!=native_rgba32f) || source.layers!=1 || source.depth || source.mips.empty())throw FormatError("particle upload requires original-decoded 2D mips");
    if(state.color_key_op)throw FormatError("unsupported original particle color-key operation: "+std::to_string(state.color_key_op));
    auto sampler=native_sampler(state,properties.limits,false);
    if(sampler.custom_border)throw FormatError("particle custom sampler border requires original device support");
    Image image;Buffer staging{};VkCommandBuffer command{};
    try {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;
    ci.format=source.format==native_rgba32f?VK_FORMAT_R32G32B32A32_SFLOAT:VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties format_properties;vkGetPhysicalDeviceFormatProperties(physical,ci.format,&format_properties);
    const auto required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if((format_properties.optimalTilingFeatures&required)!=required)throw FormatError("particle signed texture filtering unavailable");
    ci.extent={source.width,source.height,1};ci.mipLevels=source.mips.size();ci.arrayLayers=1;
    ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    check(vkCreateImage(device,&ci,nullptr,&image.handle));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,image.handle,&req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&image.memory));check(vkBindImageMemory(device,image.handle,image.memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image.handle;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;
    vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,ci.mipLevels,0,1};check(vkCreateImageView(device,&vi,nullptr,&image.view));
    check(vkCreateSampler(device,&sampler.info,nullptr,&image.sampler));
    staging=make_buffer(source.pixels.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,source.pixels.data());
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=uploads;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
    check(vkAllocateCommandBuffers(device,&ca,&command));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=image.handle;barrier.subresourceRange=vi.subresourceRange;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    std::vector<VkBufferImageCopy> regions;
    for(unsigned mip=0;mip<source.mips.size();++mip) {
      const auto &s=source.mips[mip];VkBufferImageCopy copy{};copy.bufferOffset=s.offset;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,mip,0,1};copy.imageExtent={s.width,s.height,1};regions.push_back(copy);
    }
    vkCmdCopyBufferToImage(command,staging.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,regions.size(),regions.data());
    barrier.oldLayout=barrier.newLayout;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));vkFreeCommandBuffers(device,uploads,1,&command);free_buffer(staging);
    return image;
    }catch(...) {
      if(command)vkFreeCommandBuffers(device,uploads,1,&command);
      free_buffer(staging);
      if(image.sampler)vkDestroySampler(device,image.sampler,nullptr);
      if(image.view)vkDestroyImageView(device,image.view,nullptr);
      if(image.handle)vkDestroyImage(device,image.handle,nullptr);
      if(image.memory)vkFreeMemory(device,image.memory,nullptr);
      throw;
    }
  }
  VkPipeline make_pipeline(const MaterialPass &source,unsigned classification,bool distortion) {
    ForwardRasterState raster_state;bool blend=false;unsigned src=1,dst=0,equation=0x8006;
    for(const auto &s:source.states)switch(s.index) {
      case 57:raster_state.depth_function=s.value;break;case 59:blend=s.value!=0;break;
      case 58:raster_state.alpha_function=s.value;break;
      case 60:raster_state.alpha_test=s.value;break;case 61:raster_state.alpha_reference=s.value;break;
      case 62:src=s.value;break;case 63:dst=s.value;break;case 64:raster_state.depth_write=s.value;break;
      case 67:raster_state.color_write=s.value;break;case 74:equation=s.value;break;
      case 143:raster_state.depth_enable=s.value;break;case 147:raster_state.cull=s.value;break;
      // Source SDK state158 is line width. Both the geometry programs and
      // model path rasterize filled triangles, so it has no raster effect.
      case 158:break;
      default:throw FormatError("unsupported original particle raster state "+std::to_string(s.index));
    }
    if(equation!=0x8006 || raster_state.depth_enable!=2 || raster_state.depth_function<0x200 || raster_state.depth_function>0x207 || raster_state.alpha_function<0x200 || raster_state.alpha_function>0x207)
      throw FormatError("unsupported original particle blend/depth operation");
    VkPipelineShaderStageCreateInfo stages[3]{};
    const VkShaderStageFlagBits flags[]{VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_GEOMETRY_BIT,VK_SHADER_STAGE_FRAGMENT_BIT};
    for(unsigned n=0;n<3;++n)stages[n]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,flags[n],shaders[n],"main",nullptr};
    if(classification==8)stages[0].module=shaders[3];
    if(classification==9)stages[0].module=shaders[4];
    if(distortion)stages[0].module=shaders[7];
    const bool model=classification==1;
    if(model){stages[0].module=shaders[5];stages[1].module=shaders[6];}
    const VkVertexInputBindingDescription binding{0,model?80u:8u,VK_VERTEX_INPUT_RATE_VERTEX};
    const std::array<VkVertexInputAttributeDescription,2> attributes{{
      {0,0,model?VK_FORMAT_R32G32B32A32_SFLOAT:VK_FORMAT_R16G16B16A16_SSCALED,0},
      {1,0,VK_FORMAT_R32G32B32A32_SFLOAT,16}}};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;
    vi.vertexAttributeDescriptionCount=model?2:1;vi.pVertexBindingDescriptions=&binding;vi.pVertexAttributeDescriptions=attributes.data();
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=model?VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
    VkPipelineViewportStateCreateInfo vs{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vs.viewportCount=vs.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;rs.frontFace=VK_FRONT_FACE_CLOCKWISE;
    // Original Xbox W-depth is clipped in the fragment stage. The captured
    // NV2A Vulkan pipeline disables fixed-function Z clipping and clamps Z.
    rs.depthClampEnable=VK_TRUE;
    if(raster_state.cull!=0 && raster_state.cull!=0x900 && raster_state.cull!=0x901)
      throw FormatError("unsupported original particle cull state");
    // D3DCULL names the screen winding to remove (CW=900,CCW=901).
    // This pipeline declares CW front faces; the ordinary world pipeline
    // declares CCW, so its front/back selection is correspondingly reversed.
    rs.cullMode=raster_state.cull==0x900?VK_CULL_MODE_FRONT_BIT:
        raster_state.cull==0x901?VK_CULL_MODE_BACK_BIT:VK_CULL_MODE_NONE;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=raster_state.depth_enable!=0;ds.depthWriteEnable=raster_state.depth_write!=0;ds.depthCompareOp=VkCompareOp(raster_state.depth_function-0x200);
    VkPipelineColorBlendAttachmentState cb{};cb.blendEnable=blend;cb.srcColorBlendFactor=cb.srcAlphaBlendFactor=blend_factor(src);cb.dstColorBlendFactor=cb.dstAlphaBlendFactor=blend_factor(dst);cb.colorBlendOp=cb.alphaBlendOp=VK_BLEND_OP_ADD;
    constexpr unsigned shift[]{16,8,0,24};for(unsigned n=0;n<4;++n)if((raster_state.color_write>>shift[n])&1)cb.colorWriteMask|=1u<<n;
    VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
    const VkDynamicState dynamic[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo di{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};di.dynamicStateCount=2;di.pDynamicStates=dynamic;
    VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=3;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vs;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pDepthStencilState=&ds;pi.pColorBlendState=&bs;pi.pDynamicState=&di;pi.layout=layout;pi.renderPass=pass;
    VkPipeline pipeline;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,nullptr,&pipeline));return pipeline;
  }
  MaterialGpu &material(std::shared_ptr<const ParticleMaterial> source) {
    if(!source || (source->vertex.classification!=1 && source->vertex.classification!=7 && source->vertex.classification!=8 && source->vertex.classification!=9))throw FormatError("unsupported original particle vertex classification");
    const auto modes=source->pass.program.words[54];
    if(source->textures[2] || source->textures[3] || (modes>>10u)!=0u ||
       (modes&31u)>1u || (((modes>>5u)&31u)>1u && ((modes>>5u)&31u)!=6u))
      throw FormatError("original particle texture modes require an unimplemented sampling path");
    bool external23=false;
    for(const auto &binding:source->pass.textures)if(binding.external) {
      if(binding.stage!=1 || binding.external!=23 || binding.parameter!=255 || binding.flags || external23)
        throw FormatError("unsupported particle external texture binding");
      external23=true;
    }
    if(external23 && (source->textures[1] || modes!=0xc1u))
      throw FormatError("particle external23 requires the original dependent framebuffer lookup");
    if(((modes>>5u)&31u)==6u && !source->textures[1] && !external23)
      throw FormatError("particle bump lookup requires its completed second texture binding");
    if(((modes>>5u)&31u)==6u && (modes&31u)!=1u)
      throw FormatError("particle bump lookup requires an active preceding texture stage");
    std::uint64_t hash=14695981039346656037ull;for(auto byte:source->vertex.program)hash=(hash^std::to_integer<unsigned>(byte))*1099511628211ull;
    const auto expected=source->vertex.classification==1?particle_model_program_hash:
        source->vertex.classification==7?particle_program_hash:
        source->vertex.classification==8?particle_directional_program_hash:particle_perpendicular_program_hash;
    const bool distortion=source->vertex.classification==7 && hash==particle_distortion_program_hash;
    if(hash!=expected && !distortion)throw FormatError("particle program differs from compiled original disc tokens");
    auto [entry,inserted]=materials.try_emplace(source.get());auto &gpu=entry->second;
    if(!inserted) {
      if(!gpu.pipeline)throw FormatError("particle material initialization previously failed");
      return gpu;
    }
    gpu.source=source;
    gpu.external23=external23;
    if(!source->textures[0] || source->pass.texture_states.empty())throw FormatError("particle primary texture missing");
    for(unsigned stage=0;stage<2;++stage) {
      if(stage==1 && external23) {
        if(source->pass.texture_states.size()<2)throw FormatError("particle external23 sampler missing");
        const auto &state=source->pass.texture_states[1];
        if(state.alpha_kill || state.color_key_op || state.color_sign)
          throw FormatError("unsupported particle framebuffer texture operation");
        auto sampler=native_sampler(state,properties.limits,false);
        if(sampler.custom_border)throw FormatError("particle custom framebuffer border unsupported");
        check(vkCreateSampler(device,&sampler.info,nullptr,&gpu.images[stage].sampler));
        continue;
      }
      const unsigned bound=source->textures[stage]?stage:0;
      if(bound>=source->pass.texture_states.size())throw FormatError("particle sampler missing");
      gpu.images[stage]=make_image(*source->textures[bound],source->pass.texture_states[bound]);
    }
    gpu.pipeline=make_pipeline(source->pass,source->vertex.classification,distortion);return gpu;
  }
  RendererParticleProgramConfiguration program(std::shared_ptr<const ParticleMaterial> source) {
    if(!source)throw FormatError("Native particle program has no material owner");
    const auto decoded=decode_vertex_program(source->vertex.program);auto &gpu=material(source);
    std::uint64_t hash=14695981039346656037ull;
    for(auto byte:source->vertex.program)hash=(hash^std::to_integer<unsigned>(byte))*1099511628211ull;
    const auto classification=source->vertex.classification;
    const auto shader=classification==1?shaders[5]:classification==8?shaders[3]:classification==9?shaders[4]:
        hash==particle_distortion_program_hash?shaders[7]:shaders[0];
    return {std::bit_cast<std::uint64_t>(gpu.pipeline),std::bit_cast<std::uint64_t>(shader),
        std::bit_cast<std::uint64_t>(layout),hash,0,classification,decoded.count,
        NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&gpu))}};
  }
  void clear_draws() {
    if(uniform_data)vkUnmapMemory(device,uniforms.memory);
    uniform_data=nullptr;free_buffer(uniforms);uniforms={};
    draws.clear();draw_count=0;
    if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
    descriptors=VK_NULL_HANDLE;
  }
  void reserve_draws(std::size_t count) {
    if(count<=draws.size())return;
    const auto capacity=std::max(count,std::max<std::size_t>(64,draws.size()*2));
    clear_draws();
    const auto alignment=std::max<VkDeviceSize>(1,properties.limits.minUniformBufferOffsetAlignment);
    const auto aligned=[&](std::size_t size){return (size+alignment-1)/alignment*alignment;};
    material_offset=aligned(sizeof(ParticleVertexState));
    depth_offset=material_offset+aligned(sizeof(MaterialUniform));
    draw_stride=depth_offset+aligned(sizeof(ParticleDepthState));
    uniforms=make_buffer(capacity*draw_stride,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,nullptr);
    check(vkMapMemory(device,uniforms.memory,0,capacity*draw_stride,0,&uniform_data));
    std::array<VkDescriptorPoolSize,2> sizes{{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,std::uint32_t(capacity*2)},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,std::uint32_t(capacity*3)}}};
    VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pi.maxSets=capacity;pi.poolSizeCount=2;pi.pPoolSizes=sizes.data();
    check(vkCreateDescriptorPool(device,&pi,nullptr,&descriptors));
    const std::vector<VkDescriptorSetLayout> layouts(capacity,set_layout);
    std::vector<VkDescriptorSet> sets(capacity);
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=descriptors;ai.descriptorSetCount=capacity;ai.pSetLayouts=layouts.data();
    check(vkAllocateDescriptorSets(device,&ai,sets.data()));
    draws.resize(capacity);
    const unsigned binding[]{2,4,5};
    const std::size_t offsets[]{0,material_offset,depth_offset};
    const std::size_t sizes_bytes[]{sizeof(ParticleVertexState),sizeof(MaterialUniform),sizeof(ParticleDepthState)};
    for(std::size_t index=0;index<capacity;++index) {
      draws[index].set=sets[index];
      for(unsigned n=0;n<3;++n) {
        VkDescriptorBufferInfo bi{uniforms.handle,index*draw_stride+offsets[n],sizes_bytes[n]};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=sets[index];w.dstBinding=binding[n];w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w.pBufferInfo=&bi;
        vkUpdateDescriptorSets(device,1,&w,0,nullptr);
      }
    }
  }
  void prepare(std::span<const ParticleDraw> source,ParticleSceneTexture scene) {
    // The owner waits for its frame fence before prepare. Retain one mapped
    // uniform arena and descriptor bank across frames instead of allocating
    // three Vulkan memory objects per live particle on every upload.
    draw_count=0;reserve_draws(source.size());
    for(std::size_t index=0;index<source.size();++index) {
      const auto &d=source[index];auto &m=material(d.material);auto &gpu=draws[index];
      if(m.external23 && !scene.view)throw FormatError("particle external23 requires its current-surface texture owner");
      gpu.scene_resolved=bool(d.bump_texture);gpu.scene_recorded=false;
      gpu.model=nullptr;
      if(d.model) {
        if(d.material->vertex.classification!=1 || d.model_first>d.model->indices.size() ||
            d.model_count>d.model->indices.size()-d.model_first || d.model_count%3)
          throw FormatError("Particle model draw escapes authored triangle bank");
        auto [at,created]=models.try_emplace(d.model.get());auto &model=at->second;
        if(created) {
          model.source=d.model;
          model.vertices=make_buffer(d.model->vertices.size()*sizeof(d.model->vertices[0]),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,d.model->vertices.data());
          model.indices=make_buffer(d.model->indices.size()*2,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,d.model->indices.data());
        }
        gpu.model=&model;gpu.first=d.model_first;gpu.count=d.model_count;
      } else if(d.material->vertex.classification==1)throw FormatError("Particle model has no authored geometry");
      auto *destination=static_cast<std::byte*>(uniform_data)+index*draw_stride;
      std::memcpy(destination,&d.vertex,sizeof(d.vertex));
      MaterialUniform uniform{d.material->pass.program.words,{0,0x207,0,0},{},{1,1,0,0}};
      if(((uniform.program[54]>>5u)&31u)==6u) {
        if(d.bump_texture) {
          uniform.bump_matrix=d.bump_texture->matrix;uniform.coordinate_scale=d.bump_texture->coordinate_scale;
        } else if(!m.external23 || d.material->sampler_writes.empty())
          throw FormatError("particle bump lookup requires completed original texture-stage state");
      }
      uniform.alpha[3]=d.material->pass.texture_states[0].color_sign>>28;
      // Original TEXCTL0 ALPHAKILLEN bit2 belongs to each fetched stage,
      // before combiners. Upper nibbles leave the existing sign bits intact.
      for(unsigned stage=0;stage<2 && stage<d.material->pass.texture_states.size();++stage)
        uniform.alpha[3]|=(d.material->pass.texture_states[stage].alpha_kill&4u)<<(16u+stage*4u);
      for(const auto&s:d.material->pass.states){
        if(s.index==60)uniform.alpha[0]=s.value;
        if(s.index==58)uniform.alpha[1]=s.value;
        if(s.index==61)uniform.alpha[2]=s.value&255;
      }
      std::memcpy(destination+material_offset,&uniform,sizeof(uniform));
      std::memcpy(destination+depth_offset,&d.depth,sizeof(d.depth));
      if(gpu.material!=&m || m.external23) {
        for(unsigned stage=0;stage<2;++stage) {
          const auto &image=m.images[stage];VkDescriptorImageInfo ii{image.sampler,
              stage==1 && m.external23?scene.view:image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
          VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=gpu.set;w.dstBinding=stage;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.pImageInfo=&ii;
          vkUpdateDescriptorSets(device,1,&w,0,nullptr);
        }
        gpu.material=&m;
      }
    }
    draw_count=source.size();
  }
  void record(VkCommandBuffer command,VkViewport viewport,VkRect2D scissor,std::size_t first,std::size_t count) const {
    if(first>draw_count || count>draw_count-first)throw FormatError("particle draw range outside prepared state");
    if(!count)return;
    for(std::size_t index=first;index<first+count;++index)if(draws[index].material->external23) {
      if(!draws[index].scene_resolved || draws[index].scene_recorded)
        throw FormatError("particle external23 requires one completed ordered scene lookup per draw");
      draws[index].scene_recorded=true;
    }
    const VkDeviceSize offset=0;vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
    vkCmdBindVertexBuffers(command,0,1,&corners,&offset);
    for(std::size_t n=first;n<first+count;++n) {
      const auto &d=draws[n];
      vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,d.material->pipeline);
      vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&d.set,0,nullptr);
      if(d.model) {
        vkCmdBindVertexBuffers(command,0,1,&d.model->vertices.handle,&offset);
        vkCmdBindIndexBuffer(command,d.model->indices.handle,0,VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command,d.count,1,d.first,0,0);
      } else {vkCmdBindVertexBuffers(command,0,1,&corners,&offset);vkCmdDraw(command,4,1,0,0);}
    }
  }
  void release() {
    for(auto &[key,model]:models){free_buffer(model.vertices);free_buffer(model.indices);}
    clear_draws();for(auto &[key,m]:materials) {
      if(m.pipeline)vkDestroyPipeline(device,m.pipeline,nullptr);
      for(auto i:m.images) {
        if(i.sampler)vkDestroySampler(device,i.sampler,nullptr);
        if(i.view)vkDestroyImageView(device,i.view,nullptr);
        if(i.handle)vkDestroyImage(device,i.handle,nullptr);
        if(i.memory)vkFreeMemory(device,i.memory,nullptr);
      }
    }
    free_buffer({corners,corner_memory});for(auto shader:shaders)if(shader)vkDestroyShaderModule(device,shader,nullptr);
    if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
    if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);
    if(uploads)vkDestroyCommandPool(device,uploads,nullptr);
  }
  ~Impl(){release();}
};
static_assert(sizeof(ParticleVertexState)==3360 && offsetof(ParticleVertexState,constants)==256);
static_assert(sizeof(ParticleDepthState)==32);
ParticleRenderer::ParticleRenderer(VkPhysicalDevice p,VkDevice d,VkQueue q,unsigned family,VkRenderPass rp,const std::filesystem::path &directory)
  :impl_(std::make_unique<Impl>(p,d,q,family,rp,directory)){}
ParticleRenderer::~ParticleRenderer()=default;
RendererParticleProgramConfiguration ParticleRenderer::program(std::shared_ptr<const ParticleMaterial> material){return impl_->program(std::move(material));}
void ParticleRenderer::prepare(std::span<const ParticleDraw> d,ParticleSceneTexture scene){impl_->prepare(d,scene);}
bool ParticleRenderer::reads_scene_texture(std::size_t index) const {
  if(index>=impl_->draw_count)throw FormatError("particle scene texture query outside prepared draws");
  return impl_->draws[index].material->external23;
}
void ParticleRenderer::resolve_scene_lookup(std::size_t index,const ParticleBumpTextureState &state) {
  if(index>=impl_->draw_count || !impl_->draws[index].material->external23 || impl_->draws[index].scene_recorded)
    throw FormatError("particle scene lookup resolution escapes its unrecorded draw");
  auto *destination=static_cast<std::byte*>(impl_->uniform_data)+index*impl_->draw_stride+impl_->material_offset;
  std::memcpy(destination+offsetof(Impl::MaterialUniform,bump_matrix),state.matrix.data(),sizeof(state.matrix));
  std::memcpy(destination+offsetof(Impl::MaterialUniform,coordinate_scale),state.coordinate_scale.data(),sizeof(state.coordinate_scale));
  impl_->draws[index].scene_resolved=true;
}
void ParticleRenderer::record(VkCommandBuffer c,VkViewport v,VkRect2D s,std::size_t first,std::size_t count)const{impl_->record(c,v,s,first,count);}
} // namespace halo2
