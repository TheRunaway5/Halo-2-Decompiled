#include "halo2/particle_renderer.hpp"
#include "halo2/sampler.hpp"
#include "halo2/texture.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/particle_material_validation.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <map>
#include <cmath>

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
  std::filesystem::path shader_directory;
  bool mirror_clamp_enabled{};
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};};
  struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkSampler sampler{};};
  struct MaterialGpu {
    std::shared_ptr<const ParticleMaterial> source;
    std::array<Image,2> images;VkPipeline pipeline{};bool external23{};
    std::string fragment_kernel;
    ParticleMaterialValidation validation;
  };
  struct MaterialUniform {
    std::array<std::uint32_t,60> program;std::array<std::uint32_t,4> alpha;
    ShaderVector bump_matrix,coordinate_scale;
  };
  static_assert(sizeof(MaterialUniform)==288);
  struct ModelGpu {std::shared_ptr<const ParticleModelDefinition> source;Buffer vertices,indices;};
  struct DrawGpu {MaterialGpu *material{};VkDescriptorSet set{};ModelGpu *model{};unsigned first{},count{};
    bool scene_resolved{};mutable bool scene_recorded{},view_recorded{};bool view_prepared{},view_published{};
    mutable VkViewport viewport{};mutable VkRect2D scissor{};
    std::optional<ParticlePreparedViewBounds> required_view;
    unsigned local_view{};bool foreground{},material_bindings_ready{},active{};};
  std::map<const ParticleMaterial*,MaterialGpu> materials;
  struct FailedMaterial {std::shared_ptr<const ParticleMaterial> source;std::string diagnostic;};
  std::map<const ParticleMaterial*,FailedMaterial> unavailable_materials;
  std::map<const ParticleModelDefinition*,ModelGpu> models;
  std::vector<DrawGpu> draws;VkDescriptorPool descriptors{};
  std::vector<std::shared_ptr<const ParticleMaterial>> diagnosed_materials;
  std::vector<std::string> resource_diagnostics;
  bool diagnostic_limit_reported{};
  Buffer uniforms;void *uniform_data{};std::size_t draw_count{},material_offset{},depth_offset{},draw_stride{};
  mutable std::uint64_t view_revision{};
  Impl(VkPhysicalDevice p,VkDevice d,VkQueue q,unsigned family,VkRenderPass rp,const std::filesystem::path &directory,bool mirror_clamp)
    :physical(p),device(d),queue(q),pass(rp),shader_directory(directory),mirror_clamp_enabled(mirror_clamp) {
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
      if(n==2)continue; // Each complete source program selects its compiled FS.
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
  NativeSampler sampler(const ShaderTextureState& state)const {
    if(!mirror_clamp_enabled && (state.address_u==5 || state.address_v==5 || state.address_w==5))
      throw ParticleResourceError("Particle mirror-once addressing has no enabled sampler capability");
    try {
      auto result=native_sampler(state,properties.limits,false);
      if(result.custom_border)throw ParticleResourceError("particle custom sampler border requires original device support");
      return result;
    }catch(const ParticleResourceError&){throw;}
    catch(const FormatError& error){throw ParticleResourceError(error.what());}
  }
  void free_material(MaterialGpu& material) {
    if(material.pipeline)vkDestroyPipeline(device,material.pipeline,nullptr);
    for(auto image:material.images) {
      if(image.sampler)vkDestroySampler(device,image.sampler,nullptr);
      if(image.view)vkDestroyImageView(device,image.view,nullptr);
      if(image.handle)vkDestroyImage(device,image.handle,nullptr);
      if(image.memory)vkFreeMemory(device,image.memory,nullptr);
    }
    material.pipeline=VK_NULL_HANDLE;material.images={};
  }
  void diagnose(const std::shared_ptr<const ParticleMaterial>& source,std::string_view message) {
    if(std::find(diagnosed_materials.begin(),diagnosed_materials.end(),source)!=diagnosed_materials.end())return;
    constexpr std::size_t limit=64;
    if(diagnosed_materials.size()>=limit) {
      if(!diagnostic_limit_reported){resource_diagnostics.emplace_back("Additional unavailable particle materials omitted; diagnostic limit reached");diagnostic_limit_reported=true;}
      return;
    }
    diagnosed_materials.push_back(source);
    const auto owner=source?source->pass.name.substr(0,240):std::string("missing material owner");
    resource_diagnostics.push_back(owner+": "+std::string(message.substr(0,512)));
  }
  void reject_material(const std::shared_ptr<const ParticleMaterial>& source,std::string_view message) {
    // This material is immutable for its source lifetime. Live draws need not
    // repeat failed uploads; strict program() remains an explicit repair retry.
    unavailable_materials.try_emplace(source.get(),FailedMaterial{source,std::string(message.substr(0,512))});
    diagnose(source,message);
  }
  Image make_image(const Texture &input,const ShaderTextureState &state) {
    auto native=sampler(state);
    const auto source=prepare_signed_texture(input,state.color_sign);
    if((source.format!=11 && source.format!=native_rgba32f) || source.layers!=1 || source.depth || source.mips.empty())throw ParticleResourceError("particle upload requires original-decoded 2D mips");
    if(state.color_key_op)throw ParticleResourceError("unsupported original particle color-key operation: "+std::to_string(state.color_key_op));
    if(source.width>properties.limits.maxImageDimension2D || source.height>properties.limits.maxImageDimension2D)
      throw ParticleResourceError("particle image exceeds native device dimensions");
    Image image;Buffer staging{};VkCommandBuffer command{};
    try {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;
    ci.format=source.format==native_rgba32f?VK_FORMAT_R32G32B32A32_SFLOAT:VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties format_properties;vkGetPhysicalDeviceFormatProperties(physical,ci.format,&format_properties);
    const auto required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if((format_properties.optimalTilingFeatures&required)!=required)throw ParticleResourceError("particle signed texture filtering unavailable");
    ci.extent={source.width,source.height,1};ci.mipLevels=source.mips.size();ci.arrayLayers=1;
    ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    check(vkCreateImage(device,&ci,nullptr,&image.handle));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,image.handle,&req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&image.memory));check(vkBindImageMemory(device,image.handle,image.memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image.handle;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;
    vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,ci.mipLevels,0,1};check(vkCreateImageView(device,&vi,nullptr,&image.view));
    check(vkCreateSampler(device,&native.info,nullptr,&image.sampler));
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
  VkPipeline make_pipeline(const ParticleMaterialValidation& validated) {
    const auto& raster_state=validated.raster;
    const auto classification=validated.classification;const bool distortion=validated.distortion;
    VkPipelineShaderStageCreateInfo stages[3]{};
    const VkShaderStageFlagBits flags[]{VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_GEOMETRY_BIT,VK_SHADER_STAGE_FRAGMENT_BIT};
    for(unsigned n=0;n<3;++n)stages[n]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,flags[n],shaders[n],"main",nullptr};
    std::ifstream fragment_file(shader_directory/std::string(validated.fragment_kernel),std::ios::binary|std::ios::ate);
    if(!fragment_file || fragment_file.tellg()<20 || fragment_file.tellg()>16*1024*1024 || fragment_file.tellg()%4)
      throw ParticleResourceError("compiled native particle fragment kernel missing");
    std::vector<std::uint32_t> fragment_words(std::size_t(fragment_file.tellg())/4);fragment_file.seekg(0);
    if(!fragment_file.read(reinterpret_cast<char*>(fragment_words.data()),fragment_words.size()*4))
      throw ParticleResourceError("compiled native particle fragment kernel truncated");
    if(fragment_words[0]!=0x07230203u || !fragment_words[3] || fragment_words[4])
      throw ParticleResourceError("compiled native particle fragment kernel has an invalid header");
    VkShaderModuleCreateInfo fragment_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragment_info.codeSize=fragment_words.size()*4;fragment_info.pCode=fragment_words.data();
    VkShaderModule fragment{};const auto shader_result=vkCreateShaderModule(device,&fragment_info,nullptr,&fragment);
    if(shader_result==VK_ERROR_INVALID_SHADER_NV)throw ParticleResourceError("compiled native particle fragment kernel is unsupported by the device");
    check(shader_result);
    struct FragmentLease{VkDevice device;VkShaderModule shader;~FragmentLease(){vkDestroyShaderModule(device,shader,nullptr);}} fragment_lease{device,fragment};
    stages[2].module=fragment;
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
    // D3DCULL names the screen winding to remove (CW=900,CCW=901).
    // This pipeline declares CW front faces; the ordinary world pipeline
    // declares CCW, so its front/back selection is correspondingly reversed.
    rs.cullMode=raster_state.cull==0x900?VK_CULL_MODE_FRONT_BIT:
        raster_state.cull==0x901?VK_CULL_MODE_BACK_BIT:VK_CULL_MODE_NONE;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=raster_state.depth_enable!=0;ds.depthWriteEnable=raster_state.depth_write!=0;ds.depthCompareOp=VkCompareOp(raster_state.depth_function-0x200);
    VkPipelineColorBlendAttachmentState cb{};cb.blendEnable=validated.blend;cb.srcColorBlendFactor=cb.srcAlphaBlendFactor=blend_factor(validated.source_blend);cb.dstColorBlendFactor=cb.dstAlphaBlendFactor=blend_factor(validated.destination_blend);cb.colorBlendOp=cb.alphaBlendOp=VK_BLEND_OP_ADD;
    constexpr unsigned shift[]{16,8,0,24};for(unsigned n=0;n<4;++n)if((raster_state.color_write>>shift[n])&1)cb.colorWriteMask|=1u<<n;
    VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
    const VkDynamicState dynamic[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo di{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};di.dynamicStateCount=2;di.pDynamicStates=dynamic;
    VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=3;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vs;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pDepthStencilState=&ds;pi.pColorBlendState=&bs;pi.pDynamicState=&di;pi.layout=layout;pi.renderPass=pass;
    VkPipeline pipeline{};const auto pipeline_result=vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,nullptr,&pipeline);
    if(pipeline_result!=VK_SUCCESS && pipeline)vkDestroyPipeline(device,pipeline,nullptr);
    if(pipeline_result==VK_ERROR_INVALID_SHADER_NV)throw ParticleResourceError("compiled particle pipeline is unsupported by the device");
    check(pipeline_result);return pipeline;
  }
  MaterialGpu &material(std::shared_ptr<const ParticleMaterial> source) {
    if(!source)throw ParticleResourceError("Particle program has no material owner");
    const auto validated=validate_particle_material(*source);
    if(auto entry=materials.find(source.get());entry!=materials.end()) {
      auto& gpu=entry->second;const auto& old=gpu.validation;
      if(!gpu.pipeline || old.fragment_kernel!=validated.fragment_kernel || old.program_hash!=validated.program_hash ||
         old.raster!=validated.raster || old.blend!=validated.blend || old.source_blend!=validated.source_blend ||
         old.destination_blend!=validated.destination_blend || old.external_scene_texture!=validated.external_scene_texture)
        throw FormatError("Particle material identity changed after native pipeline admission");
      return gpu;
    }
    // Do not make a failed shader/image upload visible to the cache. Its owner
    // may repair or finish loading the same immutable material and retry it.
    MaterialGpu gpu{};
    gpu.source=source;
    gpu.fragment_kernel=validated.fragment_kernel;gpu.validation=validated;
    gpu.external23=validated.external_scene_texture;
    try {
    for(unsigned stage=0;stage<2;++stage) {
      if(stage==1 && gpu.external23) {
        const auto native=sampler(source->pass.texture_states[1]);
        check(vkCreateSampler(device,&native.info,nullptr,&gpu.images[stage].sampler));
        continue;
      }
      const unsigned bound=source->textures[stage]?stage:0;
      gpu.images[stage]=make_image(*source->textures[bound],source->pass.texture_states[bound]);
    }
    gpu.pipeline=make_pipeline(validated);
    return materials.emplace(source.get(),std::move(gpu)).first->second;
    }catch(...){free_material(gpu);throw;}
  }
  RendererParticleProgramConfiguration program(std::shared_ptr<const ParticleMaterial> source) {
    auto &gpu=material(source);unavailable_materials.erase(source.get());const auto& validated=gpu.validation;
    const auto classification=validated.classification;
    const auto shader=classification==1?shaders[5]:classification==8?shaders[3]:classification==9?shaders[4]:
        validated.distortion?shaders[7]:shaders[0];
    return {std::bit_cast<std::uint64_t>(gpu.pipeline),std::bit_cast<std::uint64_t>(shader),
        std::bit_cast<std::uint64_t>(layout),validated.program_hash,0,classification,validated.instructions,
        NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&gpu))}};
  }
  std::optional<RendererParticleProgramConfiguration> try_program(
      std::shared_ptr<const ParticleMaterial> source,std::string& diagnostic) {
    diagnostic.clear();
    if(auto failure=unavailable_materials.find(source.get());failure!=unavailable_materials.end()) {
      diagnostic=failure->second.diagnostic;return std::nullopt;
    }
    try{return program(source);}
    catch(const ParticleResourceError& error){diagnostic=error.what();reject_material(source,diagnostic);return std::nullopt;}
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
  void prepare(std::span<const ParticleDraw> source,ParticleSceneTexture scene,
      std::span<const std::optional<ParticlePreparedViewBounds>> required = {}) {
    if(!required.empty() && required.size()!=source.size())throw FormatError("Prepared particle view bounds do not cover their actual draw bank");
    // The owner waits for its frame fence before prepare. Retain one mapped
    // uniform arena and descriptor bank across frames instead of allocating
    // three Vulkan memory objects per live particle on every upload.
    ++view_revision;draw_count=0;reserve_draws(source.size());
    for(std::size_t index=0;index<source.size();++index) {
      const auto &d=source[index];auto &gpu=draws[index];gpu.active=false;
      gpu.scene_resolved=bool(d.bump_texture);gpu.scene_recorded=false;gpu.view_recorded=false;
      gpu.view_prepared=false;gpu.view_published=false;
      gpu.local_view=0;gpu.foreground=false;
      gpu.required_view=required.empty()?std::nullopt:required[index];
      gpu.model=nullptr;
      if(unavailable_materials.contains(d.material.get()))continue;
      bool admitted_material=false;
      try {
      auto &m=material(d.material);
      admitted_material=true;
      if(m.external23 && !scene.view)throw ParticleResourceError("Particle framebuffer lookup has no current-surface texture owner");
      if(m.external23 && (!std::isfinite(d.vertex.surface_fog[0]) || !std::isfinite(d.vertex.surface_fog[1]) ||
          !(d.vertex.surface_fog[0]>0 && d.vertex.surface_fog[1]>0)))
        throw ParticleResourceError("Particle framebuffer lookup has no completed finite surface dimensions");
      if(d.model) {
        if(d.material->vertex.classification!=1 || d.model_first>d.model->indices.size() ||
            d.model_count>d.model->indices.size()-d.model_first || d.model_count%3)
          throw FormatError("Particle model draw escapes authored triangle bank");
        auto at=models.find(d.model.get());
        if(at==models.end()) {
          if(d.model->vertices.empty() || d.model->indices.empty() ||
             std::any_of(d.model->indices.begin(),d.model->indices.end(),[&](unsigned i){return i>=d.model->vertices.size();}))
            throw ParticleResourceError("Particle model indices have no authored vertex owner");
          ModelGpu staged{};staged.source=d.model;
          try {
            staged.vertices=make_buffer(d.model->vertices.size()*sizeof(d.model->vertices[0]),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,d.model->vertices.data());
            staged.indices=make_buffer(d.model->indices.size()*sizeof(d.model->indices[0]),VK_BUFFER_USAGE_INDEX_BUFFER_BIT,d.model->indices.data());
            at=models.emplace(d.model.get(),std::move(staged)).first;
          }catch(...){free_buffer(staged.vertices);free_buffer(staged.indices);throw;}
        }
        gpu.model=&at->second;gpu.first=d.model_first;gpu.count=d.model_count;
      } else if(d.material->vertex.classification==1)throw ParticleResourceError("Particle model has no authored geometry");
      auto *destination=static_cast<std::byte*>(uniform_data)+index*draw_stride;
      std::memcpy(destination,&d.vertex,sizeof(d.vertex));
      MaterialUniform uniform{native_fragment_uniform_words(d.material->pass.program.words),{0,0x207,0,0},{},{1,1,0,0}};
      if(((d.material->pass.program.words[54]>>5u)&31u)==6u) {
        if(d.bump_texture) {
          uniform.bump_matrix=d.bump_texture->matrix;uniform.coordinate_scale=d.bump_texture->coordinate_scale;
        } else if(!m.external23 || d.material->sampler_writes.empty())
          throw ParticleResourceError("Particle bump lookup has no completed texture-stage state owner");
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
      if(!gpu.material_bindings_ready || gpu.material!=&m || m.external23) {
        for(unsigned stage=0;stage<2;++stage) {
          const auto &image=m.images[stage];VkDescriptorImageInfo ii{image.sampler,
              stage==1 && m.external23?scene.view:image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
          VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=gpu.set;w.dstBinding=stage;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.pImageInfo=&ii;
          vkUpdateDescriptorSets(device,1,&w,0,nullptr);
        }
        gpu.material=&m;
        gpu.material_bindings_ready=true;
      }
      gpu.active=true;
      }catch(const ParticleResourceError& error){
        if(!admitted_material)reject_material(d.material,error.what());else diagnose(d.material,error.what());
      }
    }
    draw_count=source.size();
  }
  void record(VkCommandBuffer command,VkViewport viewport,VkRect2D scissor,std::size_t first,std::size_t count) const {
    if(first>draw_count || count>draw_count-first)throw FormatError("particle draw range outside prepared state");
    if(!count)return;
    for(std::size_t n=first;n<first+count;++n)if(draws[n].active && draws[n].view_prepared)
      throw FormatError("Camera-only prepared particle resources cannot record particle geometry");
    for(std::size_t n=first;n<first+count;++n)if(draws[n].active && draws[n].required_view) {
      const auto &r=*draws[n].required_view;const auto &v=r.viewport;const auto&s=r.scissor;
      if(v.x!=viewport.x || v.y!=viewport.y || v.width!=viewport.width || v.height!=viewport.height ||
         v.minDepth!=viewport.minDepth || v.maxDepth!=viewport.maxDepth ||
         s.offset.x!=scissor.offset.x || s.offset.y!=scissor.offset.y ||
         s.extent.width!=scissor.extent.width || s.extent.height!=scissor.extent.height)
        throw FormatError("Particle draw recording differs from its actual prepared view owner");
    }
    ++view_revision;
    for(std::size_t index=first;index<first+count;++index)if(draws[index].active && draws[index].material->external23) {
      if(!draws[index].scene_resolved || draws[index].scene_recorded)
        throw FormatError("particle external23 requires one completed ordered scene lookup per draw");
      draws[index].scene_recorded=true;
    }
    const VkDeviceSize offset=0;vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
    vkCmdBindVertexBuffers(command,0,1,&corners,&offset);
    for(std::size_t n=first;n<first+count;++n) {
      const auto &d=draws[n];
      if(!d.active)continue;
      d.viewport=viewport;d.scissor=scissor;d.view_recorded=true;
      vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,d.material->pipeline);
      vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&d.set,0,nullptr);
      if(d.model) {
        vkCmdBindVertexBuffers(command,0,1,&d.model->vertices.handle,&offset);
        vkCmdBindIndexBuffer(command,d.model->indices.handle,0,VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command,d.count,1,d.first,0,0);
      } else {vkCmdBindVertexBuffers(command,0,1,&corners,&offset);vkCmdDraw(command,4,1,0,0);}
    }
  }
  void prepare_views(std::span<const RendererParticleViewInput> source,unsigned width,unsigned height) {
    if(source.empty() || source.size()>8 || !width || !height)
      throw FormatError("Particle view preparation requires its actual local-view bank and target extent");
    std::vector<MaterialGpu*> admitted;admitted.reserve(source.size());
    for(const auto &input:source) {
      const auto &r=input.rectangle;
      if(!input.material || r[0]<0 || r[1]<0 || r[2]<=0 || r[3]<=0 ||
         std::uint64_t(r[0])+r[2]>width || std::uint64_t(r[1])+r[3]>height)
        throw FormatError("Prepared particle view escapes its actual target");
      if(input.material->vertex.classification<7 || input.material->vertex.classification>9)
        throw FormatError("Prepared particle camera requires its admitted original world shader interface");
      admitted.push_back(&material(input.material));
    }
    // Only vertex/depth bindings are produced here. Texture/current-surface
    // lookups and material uniforms retain their actual draw-time owners.
    // No command may record the camera-only attribute workspace.
    ++view_revision;draw_count=0;reserve_draws(source.size());
    for(std::size_t n=0;n<source.size();++n) {
      const auto &input=source[n];auto &gpu=draws[n];const auto&r=input.rectangle;
      gpu.material=admitted[n];gpu.material_bindings_ready=false;gpu.model=nullptr;gpu.active=true;
      gpu.scene_recorded=gpu.scene_resolved=gpu.view_recorded=gpu.view_published=false;
      gpu.required_view.reset();gpu.view_prepared=true;
      ParticleVertexState vertex;vertex.constants=input.constants;vertex.clip_range=input.clip_range;vertex.surface_fog=input.surface_fog;
      const ParticleDepthState depth{input.clip_range,input.depth_parameters};
      auto *data=static_cast<std::byte*>(uniform_data)+n*draw_stride;
      std::memcpy(data,&vertex,sizeof(vertex));std::memcpy(data+depth_offset,&depth,sizeof(depth));
      gpu.viewport={0,0,float(width),float(height),0,1};
      gpu.scissor={{r[0],r[1]},{unsigned(r[2]),unsigned(r[3])}};
      gpu.local_view=source[n].local_view;gpu.foreground=source[n].foreground;
    }
    draw_count=source.size();
  }
  void publish_view(std::size_t index,std::span<const ShaderVector,12> camera,ShaderVector clip) {
    if(index>=draw_count || !draws[index].active || !draws[index].view_prepared || draws[index].view_recorded ||
       draws[index].view_published || !uniform_data)
      throw FormatError("Particle view has no unique live camera publication slot");
    for(const auto&v:camera)for(float x:v)if(!std::isfinite(x))throw FormatError("Prepared particle camera contains a nonfinite input");
    for(float x:clip)if(!std::isfinite(x))throw FormatError("Prepared particle camera contains a nonfinite clip plane");
    auto *data=static_cast<std::byte*>(uniform_data)+index*draw_stride;
    ParticleVertexState current;std::memcpy(&current,data,sizeof(current));
    if(std::memcmp(current.constants.data(),camera.data(),12*sizeof(ShaderVector)) ||
       clip[0]!=current.clip_range[0] || clip[1]!=current.clip_range[1] || clip[2]<0 || clip[3]<clip[2] || clip[3]>16777215.f)
      throw FormatError("Original camera publication differs from its actual prepared native inputs");
    std::memcpy(data+offsetof(ParticleVertexState,constants),camera.data(),12*sizeof(ShaderVector));
    std::memcpy(data+offsetof(ParticleVertexState,clip_range),clip.data(),sizeof(clip));
    std::memcpy(data+depth_offset+offsetof(ParticleDepthState,clip_range),clip.data(),sizeof(clip));
    draws[index].view_published=true;
  }
  RendererParticleViewState view_inputs(std::size_t index) const {
    if(index>=draw_count || !draws[index].active || (!draws[index].view_recorded && !draws[index].view_prepared) || !uniform_data)
      throw FormatError("Particle view requires actual prepared or recorded native inputs");
    const auto &draw=draws[index];const auto &v=draw.viewport;const auto &s=draw.scissor;
    if(s.extent.width>INT32_MAX || s.extent.height>INT32_MAX)
      throw FormatError("Particle view scissor exceeds its native observation domain");
    ParticleVertexState vertex;ParticleDepthState depth;
    const auto *data=static_cast<const std::byte*>(uniform_data)+index*draw_stride;
    std::memcpy(&vertex,data,sizeof(vertex));std::memcpy(&depth,data+depth_offset,sizeof(depth));
    RendererParticleViewState result;result.revision=view_revision;result.draw=index;
    result.pipeline=std::bit_cast<std::uint64_t>(draw.material->pipeline);
    result.layout=std::bit_cast<std::uint64_t>(layout);
    result.descriptor=std::bit_cast<std::uint64_t>(draw.set);
    result.uniform_buffer=std::bit_cast<std::uint64_t>(uniforms.handle);
    result.uniform_backing=NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(data))};
    result.vertex_offset=index*draw_stride;result.depth_offset=result.vertex_offset+depth_offset;
    result.viewport={v.x,v.y,v.width,v.height,v.minDepth,v.maxDepth};
    result.scissor={s.offset.x,s.offset.y,std::int32_t(s.extent.width),std::int32_t(s.extent.height)};
    std::copy_n(vertex.constants.begin()+8,4,result.viewport_constants.begin());
    std::copy_n(vertex.constants.begin(),12,result.camera_constants.begin());
    result.vertex_clip=vertex.clip_range;result.depth_clip=depth.clip_range;
    result.depth_parameters=depth.parameters;result.prepared=draw.view_prepared;
    result.published=draw.view_published;result.recorded=draw.view_recorded;
    result.local_view=draw.local_view;result.foreground=draw.foreground;return result;
  }
  void release() {
    for(auto &[key,model]:models){free_buffer(model.vertices);free_buffer(model.indices);}
    clear_draws();for(auto &[key,m]:materials)free_material(m);
    free_buffer({corners,corner_memory});for(auto shader:shaders)if(shader)vkDestroyShaderModule(device,shader,nullptr);
    if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
    if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);
    if(uploads)vkDestroyCommandPool(device,uploads,nullptr);
  }
  ~Impl(){release();}
};
static_assert(sizeof(ParticleVertexState)==3360 && offsetof(ParticleVertexState,constants)==256);
static_assert(sizeof(ParticleDepthState)==32);
ParticleRenderer::ParticleRenderer(VkPhysicalDevice p,VkDevice d,VkQueue q,unsigned family,VkRenderPass rp,const std::filesystem::path &directory,bool mirror_clamp)
  :impl_(std::make_unique<Impl>(p,d,q,family,rp,directory,mirror_clamp)){}
ParticleRenderer::~ParticleRenderer()=default;
RendererParticleProgramConfiguration ParticleRenderer::program(std::shared_ptr<const ParticleMaterial> material){return impl_->program(std::move(material));}
std::optional<RendererParticleProgramConfiguration> ParticleRenderer::try_program(
    std::shared_ptr<const ParticleMaterial> material,std::string& diagnostic){return impl_->try_program(std::move(material),diagnostic);}
std::vector<std::string> ParticleRenderer::take_resource_diagnostics(){std::vector<std::string> out;out.swap(impl_->resource_diagnostics);return out;}
RendererParticleViewState ParticleRenderer::view_inputs(std::size_t index) const{return impl_->view_inputs(index);}
std::uint64_t ParticleRenderer::view_revision() const{return impl_->view_revision;}
void ParticleRenderer::prepare(std::span<const ParticleDraw> d,ParticleSceneTexture scene,
    std::span<const std::optional<ParticlePreparedViewBounds>> required){impl_->prepare(d,scene,required);}
void ParticleRenderer::prepare_views(std::span<const RendererParticleViewInput> v,unsigned w,unsigned h){impl_->prepare_views(v,w,h);}
void ParticleRenderer::publish_view(std::size_t n,std::span<const ShaderVector,12> c,ShaderVector clip){impl_->publish_view(n,c,clip);}
bool ParticleRenderer::reads_scene_texture(std::size_t index) const {
  if(index>=impl_->draw_count)throw FormatError("particle scene texture query outside prepared draws");
  return impl_->draws[index].active && impl_->draws[index].material->external23;
}
void ParticleRenderer::resolve_scene_lookup(std::size_t index,const ParticleBumpTextureState &state) {
  if(index>=impl_->draw_count || !impl_->draws[index].active || !impl_->draws[index].material->external23 || impl_->draws[index].scene_recorded)
    throw FormatError("particle scene lookup resolution escapes its unrecorded draw");
  auto *destination=static_cast<std::byte*>(impl_->uniform_data)+index*impl_->draw_stride+impl_->material_offset;
  std::memcpy(destination+offsetof(Impl::MaterialUniform,bump_matrix),state.matrix.data(),sizeof(state.matrix));
  std::memcpy(destination+offsetof(Impl::MaterialUniform,coordinate_scale),state.coordinate_scale.data(),sizeof(state.coordinate_scale));
  impl_->draws[index].scene_resolved=true;
}
void ParticleRenderer::record(VkCommandBuffer c,VkViewport v,VkRect2D s,std::size_t first,std::size_t count)const{impl_->record(c,v,s,first,count);}
} // namespace halo2
