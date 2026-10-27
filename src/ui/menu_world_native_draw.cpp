#include "halo2/menu_world_native_draw.hpp"
#include "halo2/menu_world_vertex_source.hpp"
#include "halo2/menu_world_triangle_storage.hpp"
#include "halo2/world_material_uniform.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <type_traits>

#include "halo2/macos/bundle.hpp"
#ifndef HALO2_SHADER_DIR
#error HALO2_SHADER_DIR must name the compiled production shader directory
#endif

namespace halo2 {
namespace {
void check(VkResult result,const char *operation) {
  if(result!=VK_SUCCESS)
    throw FormatError(std::string(operation)+": Vulkan result "+std::to_string(result));
}
VkDeviceSize aligned(VkDeviceSize value,VkDeviceSize alignment) {
  if(!alignment || value>std::numeric_limits<VkDeviceSize>::max()-(alignment-1))
    throw FormatError("raw world uniform alignment overflow");
  return (value+alignment-1)/alignment*alignment;
}
VkDeviceSize multiplied(VkDeviceSize value,std::size_t count) {
  if(count>std::numeric_limits<VkDeviceSize>::max()/value)
    throw FormatError("raw world uniform allocation overflow");
  return value*count;
}
std::vector<std::uint32_t> shader(const std::filesystem::path &path) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  const auto size=input.tellg();
  if(!input || size<20 || size%4 || size>16*1024*1024)
    throw FormatError("invalid raw world production shader "+path.string());
  std::vector<std::uint32_t> result(static_cast<std::size_t>(size)/4);
  input.seekg(0);
  if(!input.read(reinterpret_cast<char *>(result.data()),size) || result[0]!=0x07230203u)
    throw FormatError("invalid raw world production SPIR-V "+path.string());
  return result;
}
template<class T> std::uint64_t handle_word(T value) {
  if constexpr(std::is_pointer_v<T>)return reinterpret_cast<std::uintptr_t>(value);
  else return value;
}
VkCompareOp comparison(std::uint32_t source) {
  if(source<0x200 || source>0x207)
    throw FormatError("unsupported raw world source comparison");
  return VkCompareOp(source-0x200);
}
VkStencilOp stencil_operation(std::uint32_t source) {
  switch(source) {
  case 0:return VK_STENCIL_OP_ZERO;
  case 0x1e00:return VK_STENCIL_OP_KEEP;
  case 0x1e01:return VK_STENCIL_OP_REPLACE;
  case 0x1e02:return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
  case 0x1e03:return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
  case 0x150a:return VK_STENCIL_OP_INVERT;
  case 0x8507:return VK_STENCIL_OP_INCREMENT_AND_WRAP;
  case 0x8508:return VK_STENCIL_OP_DECREMENT_AND_WRAP;
  default:throw FormatError("unsupported raw world source stencil operation");
  }
}
struct FixedState {
  VkCullModeFlags cull{};
  VkBool32 depth_test{},depth_write{},stencil_test{};
  VkCompareOp depth_compare{};
  VkStencilOpState stencil{};
  VkPipelineColorBlendAttachmentState blend{};
  std::array<float,4> blend_constants{};
};
FixedState fixed_state(const MenuWorldNativeDrawInput &input) {
  const auto &source=input.forward;
  if(source.depth_enable>2 || source.depth_write>1 ||
      source.color_write&~0x01010101u || source.alpha_test>1 || source.alpha_reference>255)
    throw FormatError("unsupported raw world source forward state");
  (void)comparison(source.alpha_function);
  FixedState result{};
  result.depth_test=source.depth_enable!=0;result.depth_write=source.depth_write!=0;
  result.depth_compare=comparison(source.depth_function);
  // D3D source0x900 culls clockwise screen triangles. Raw pipelines use a
  // positive viewport and CW frontFace, unlike the packed CPU pipeline's CCW.
  switch(source.cull) {
  case 0:result.cull=VK_CULL_MODE_NONE;break;
  case 0x900:result.cull=VK_CULL_MODE_FRONT_BIT;break;
  case 0x901:result.cull=VK_CULL_MODE_BACK_BIT;break;
  default:throw FormatError("unsupported raw world source cull state");
  }
  if(input.raster.source_stencil_mode>2)
    throw FormatError("unsupported raw world source stencil mode");
  result.stencil_test=input.raster.stencil.enabled;
  if(result.stencil_test) {
    if(!input.raster.stencil.values)
      throw FormatError("enabled raw world stencil has unknown retained fields");
    const auto &s=*input.raster.stencil.values;
    if(s.reference>255 || s.read_mask>255 || s.write_mask>255)
      throw FormatError("raw world stencil values exceed D24S8 fields");
    result.stencil.failOp=stencil_operation(s.fail);
    result.stencil.depthFailOp=stencil_operation(s.zfail);
    result.stencil.passOp=stencil_operation(s.pass);
    result.stencil.compareOp=comparison(s.function);
    result.stencil.reference=s.reference;
    result.stencil.compareMask=s.read_mask;result.stencil.writeMask=s.write_mask;
  }
  // Disabled retained stencil fields are deliberately ignored: Vulkan's
  // disabled placeholders are not reconstructed original hardware values.
  constexpr unsigned shifts[]{16,8,0,24};
  for(unsigned channel=0;channel<4;++channel)
    if((source.color_write>>shifts[channel])&1u)result.blend.colorWriteMask|=1u<<channel;
  result.blend.colorBlendOp=result.blend.alphaBlendOp=VK_BLEND_OP_ADD;
  if(input.forward_blend==0) {
    result.blend.blendEnable=VK_FALSE;
  } else if(input.forward_blend==1) {
    // Original layer21: SRC_ALPHA/INV_SRC_ALPHA and ADD. D3D uses the
    // source alpha factor for alpha too (As*As + Ad*(1-As)).
    result.blend.blendEnable=VK_TRUE;
    result.blend.srcColorBlendFactor=result.blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
    result.blend.dstColorBlendFactor=result.blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  } else if(input.forward_blend==3) {
    // Authored projected-light passes use ONE/ONE for both RGB and alpha.
    result.blend.blendEnable=VK_TRUE;
    result.blend.srcColorBlendFactor=result.blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
    result.blend.dstColorBlendFactor=result.blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
  } else if(input.forward_blend==5) {
    result.blend.blendEnable=VK_TRUE;
    result.blend.srcColorBlendFactor=result.blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_CONSTANT_COLOR;
    result.blend.dstColorBlendFactor=result.blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    result.blend_constants={1,1,1,0};
  } else if(input.forward_blend==9) {
    // Authored shadow Apply: source0x306 (DST_COLOR), destination0,
    // operation0x8006 (ADD). D3D's color factor uses destination alpha for
    // alpha; the actual shadow pass independently masks alpha writes off.
    result.blend.blendEnable=VK_TRUE;
    result.blend.srcColorBlendFactor=VK_BLEND_FACTOR_DST_COLOR;
    result.blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_DST_ALPHA;
    result.blend.dstColorBlendFactor=result.blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
  } else if(input.forward_blend==8) {
    // Combined projected light: sourceDST_ALPHA + destinationONE.
    result.blend.blendEnable=VK_TRUE;
    result.blend.srcColorBlendFactor=result.blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_DST_ALPHA;
    result.blend.dstColorBlendFactor=result.blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
  } else if(input.forward_blend==10) {
    // Sphere diffuse: ONE + CONSTANT_COLOR, literal00FFFFFF. The alpha
    // destination factor is the same constant's zero alpha component.
    result.blend.blendEnable=VK_TRUE;
    result.blend.srcColorBlendFactor=result.blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
    result.blend.dstColorBlendFactor=VK_BLEND_FACTOR_CONSTANT_COLOR;
    result.blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_CONSTANT_ALPHA;
    result.blend_constants={1,1,1,0};
  } else throw FormatError("raw world blend shorthand has no admitted exact source factor mapping");
  return result;
}
struct Buffer {
  explicit Buffer(VkDevice owner):device(owner){}
  ~Buffer() {
    if(mapped)vkUnmapMemory(device,memory);
    if(handle)vkDestroyBuffer(device,handle,nullptr);
    if(memory)vkFreeMemory(device,memory,nullptr);
  }
  Buffer(const Buffer &)=delete;
  Buffer &operator=(const Buffer &)=delete;
  VkDevice device{};VkBuffer handle{};VkDeviceMemory memory{};
  VkDeviceSize size{},allocation_size{};void *mapped{};bool coherent{};
};
struct PreparedDraw {
  MenuWorldSourceStoredDraw geometry;
  VkPipeline pipeline{};
  VkDescriptorSet uniforms{},material{};
  std::uint32_t material_offset{};
};
struct Frame {
  explicit Frame(VkDevice owner):device(owner){}
  ~Frame() {if(pool)vkDestroyDescriptorPool(device,pool,nullptr);}
  VkDevice device{};VkDescriptorPool pool{};
  std::unique_ptr<Buffer> vertex,fragment;
  std::vector<std::unique_ptr<Buffer>> triangles;
  std::vector<PreparedDraw> draws;
};
}

struct MenuWorldNativeDraw::Impl {
  MenuWorldNativeDrawContext context;
  MenuWorldSourceStorage geometry;
  VkPhysicalDeviceProperties properties{};
  VkPhysicalDeviceMemoryProperties memory_properties{};
  VkDescriptorSetLayout draw_layout{};
  VkPipelineLayout layout{};
  VkShaderModule geometry_shader{};
  std::map<std::string_view,VkShaderModule> fragment_modules;
  VkDeviceSize vertex_stride{},fragment_stride{};
  std::map<std::vector<std::uint32_t>,VkShaderModule> vertex_modules;
  std::map<std::vector<std::uint32_t>,VkPipeline> pipelines;
  std::unique_ptr<Frame> frame;
  explicit Impl(MenuWorldNativeDrawContext input):context(input),geometry({input.device,input.physical,input.triangle_vertices}) {
    if(context.shader_directory.empty())context.shader_directory=runtime_shader_directory(HALO2_SHADER_DIR);
    else std::cout<<"Pinned raw world shaders: "<<context.shader_directory.string()<<'\n';
    try {initialize();}catch(...) {destroy();throw;}
  }
  ~Impl() {destroy();}
  void destroy() noexcept {
    frame.reset();
    for(const auto &[key,pipeline]:pipelines)vkDestroyPipeline(context.device,pipeline,nullptr);
    pipelines.clear();
    for(const auto &[key,module]:vertex_modules)vkDestroyShaderModule(context.device,module,nullptr);
    vertex_modules.clear();
    if(geometry_shader)vkDestroyShaderModule(context.device,geometry_shader,nullptr);
    for(const auto &[path,handle]:fragment_modules)vkDestroyShaderModule(context.device,handle,nullptr);
    fragment_modules.clear();
    if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);
    if(draw_layout)vkDestroyDescriptorSetLayout(context.device,draw_layout,nullptr);
    geometry_shader=VK_NULL_HANDLE;layout=VK_NULL_HANDLE;draw_layout=VK_NULL_HANDLE;
  }
  VkShaderModule module(std::span<const std::uint32_t> words) {
    if(words.size()<5 || words[0]!=0x07230203u)
      throw FormatError("invalid raw world catalog SPIR-V");
    VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    create.codeSize=words.size_bytes();create.pCode=words.data();VkShaderModule result{};
    check(vkCreateShaderModule(context.device,&create,nullptr,&result),"create raw world shader module");
    return result;
  }
  void initialize() {
#ifdef __APPLE__
    if(context.triangle_vertices&&!context.strict_shader_math_enabled)
      throw FormatError("raw world triangle vertices require the actual strict MoltenVK compiler policy");
#endif
    if(!context.device || !context.physical || !context.render_pass || !context.material_layout ||
        context.api_version<VK_API_VERSION_1_3 ||
        (!context.triangle_vertices&&(!context.geometry_shader_enabled||!context.geometry_point_size_enabled)) || !context.depth_clamp_enabled || !context.demote_to_helper_enabled)
      throw FormatError("raw world draw context lacks actual required device/render state");
    vkGetPhysicalDeviceProperties(context.physical,&properties);
    vkGetPhysicalDeviceMemoryProperties(context.physical,&memory_properties);
    VkPhysicalDeviceVulkan13Features vk13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&vk13;
    vkGetPhysicalDeviceFeatures2(context.physical,&features);
    const auto &limits=properties.limits;
    if(properties.apiVersion<VK_API_VERSION_1_3 ||
        (!context.triangle_vertices&&(!features.features.geometryShader||!features.features.shaderTessellationAndGeometryPointSize)) || !features.features.depthClamp ||
        !vk13.shaderDemoteToHelperInvocation || limits.maxBoundDescriptorSets<2 ||
        limits.maxPushConstantsSize<112 || limits.maxUniformBufferRange<sizeof(WorldMaterialUniform) ||
        limits.maxUniformBufferRange<sizeof(MenuWorldVertexAttributeUniforms) ||
        limits.maxUniformBufferRange<sizeof(MenuWorldFragmentUniforms) ||
        (context.triangle_vertices&&limits.maxPerStageDescriptorStorageBuffers<2))
      throw FormatError("raw world draw device does not support the production shader/layout contract");
    if(context.depth_encoding!=SourceDepthStorageEncoding::native_d24&&context.depth_encoding!=SourceDepthStorageEncoding::d32_integer_d24)
      throw FormatError("raw world draw source depth encoding invalid");
    vertex_stride=aligned(sizeof(MenuWorldVertexAttributeUniforms),limits.minUniformBufferOffsetAlignment);
    fragment_stride=aligned(sizeof(MenuWorldFragmentUniforms),limits.minUniformBufferOffsetAlignment);
    const VkDescriptorSetLayoutBinding bindings[]{
        {0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
        {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
        {2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
        {3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set.bindingCount=context.triangle_vertices?4:2;set.pBindings=bindings;
    check(vkCreateDescriptorSetLayout(context.device,&set,nullptr,&draw_layout),"create raw world draw set layout");
    const VkDescriptorSetLayout sets[]{context.material_layout,draw_layout};
    const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT,0,112};
    VkPipelineLayoutCreateInfo create{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    create.setLayoutCount=2;create.pSetLayouts=sets;create.pushConstantRangeCount=1;create.pPushConstantRanges=&push;
    check(vkCreatePipelineLayout(context.device,&create,nullptr,&layout),"create raw world pipeline layout");
    if(!context.triangle_vertices)geometry_shader=module(shader(context.shader_directory/"menu_world.geom.spv"));
  }
  std::unique_ptr<Buffer> buffer(VkDeviceSize size,VkBufferUsageFlags usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
    auto result=std::make_unique<Buffer>(context.device);result->size=size;
    VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    create.size=size;create.usage=usage;
    create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(context.device,&create,nullptr,&result->handle),"create raw world uniform buffer");
    VkMemoryRequirements requirements{};vkGetBufferMemoryRequirements(context.device,result->handle,&requirements);
    unsigned selected=memory_properties.memoryTypeCount;
    for(unsigned i=0;i<memory_properties.memoryTypeCount;++i) {
      const auto flags=memory_properties.memoryTypes[i].propertyFlags;
      if(!(requirements.memoryTypeBits&(1u<<i)) || !(flags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))continue;
      if(selected==memory_properties.memoryTypeCount)selected=i;
      if(flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){selected=i;break;}
    }
    if(selected==memory_properties.memoryTypeCount)
      throw FormatError("raw world uniforms require host-visible memory");
    result->coherent=memory_properties.memoryTypes[selected].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    result->allocation_size=requirements.size;
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=selected;
    check(vkAllocateMemory(context.device,&allocation,nullptr,&result->memory),"allocate raw world uniforms");
    check(vkBindBufferMemory(context.device,result->handle,result->memory,0),"bind raw world uniforms");
    check(vkMapMemory(context.device,result->memory,0,VK_WHOLE_SIZE,0,&result->mapped),"map raw world uniforms");
    return result;
  }
  void flush(const Buffer &buffer) {
    if(buffer.coherent)return;
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    range.memory=buffer.memory;range.offset=0;range.size=VK_WHOLE_SIZE;
    check(vkFlushMappedMemoryRanges(context.device,1,&range),"flush raw world uniforms");
  }
  void validate(const MenuWorldNativeDrawInput &input) const {
    (void)native_fragment_kernel_path("menu",input.fragment_program);
    if(input.w_clip_mode!=MenuWorldWClipMode::cull && input.w_clip_mode!=MenuWorldWClipMode::clamp)
      throw FormatError("raw world draw has an unadmitted source W clip mode");
    if(input.w_clip_mode==MenuWorldWClipMode::clamp && input.fragment.clip_range[2]>input.fragment.clip_range[3])
      throw FormatError("raw world draw clamp interval is inverted");
    if(!input.vertex_shader || !input.material_set ||
        input.material_offset%properties.limits.minUniformBufferOffsetAlignment)
      throw FormatError("raw world draw has no catalog/material or an unaligned material offset");
    (void)menu_world_vertex_catalog_lookup(std::span(input.vertex_shader,1),
        input.source.program_bytes(),input.source.declaration());
    if(context.triangle_vertices&&(input.vertex_shader->triangle_abi!=1||input.vertex_shader->triangle_spirv.size()<5||
        input.vertex_shader->triangle_spirv[0]!=0x07230203u||input.vertex_shader->triangle_spirv[1]!=0x00010600u||input.vertex_shader->triangle_spirv[4]))
      throw FormatError("raw world triangle vertex catalog is absent or incompatible");
    if(!input.source.external_attribute_registers().empty()) {
      if(input.vertex_shader->abi!=menu_world_vertex_attribute_catalog_abi && input.vertex_shader->abi!=menu_world_renderer_vertex_catalog_abi)
        throw FormatError("raw world draw constant attributes require their native attribute ABI");
      const auto& reads=menu_world_vertex_metadata_lookup(input.source.program_bytes(),input.source.declaration()).consumed_input_components;
      for(const auto reg:input.source.external_attribute_registers())
        if((input.vertex_attribute_components[reg]&reads[reg])!=reads[reg])
          throw FormatError("raw world draw consumes unbound external attribute components");
    }
    if(!std::ranges::equal(input.vertex_shader->consumed_inputs,input.source.consumed_registers()) ||
        !std::ranges::equal(input.vertex_shader->consumed_constants,input.source.consumed_constants()))
      throw FormatError("raw world catalog read set differs from actual source binding");
    for(const auto constant:input.source.consumed_constants())
      if(constant>=input.vertex.constants.size() ||
          std::memcmp(&input.vertex.constants[constant],&input.source.constants()[constant],sizeof(ShaderVector)))
        throw FormatError("raw world vertex packet differs from its consumed owner/view bank");
    for(const auto value:input.vertex.surface_size)if(!std::isfinite(value) || value<=0)
      throw FormatError("raw world source surface extent is not positive and finite");
    for(const auto value:input.vertex.fog_parameters)if(!std::isfinite(value))
      throw FormatError("raw world hardware fog state is nonfinite");
    for(const auto value:input.vertex.clip_range)if(!std::isfinite(value))
      throw FormatError("raw world vertex clip packet is nonfinite");
    if(input.raster.window_exclusive ||
        std::memcmp(input.vertex.clip_range.data(),input.fragment.clip_range.data(),sizeof(input.vertex.clip_range)) ||
        input.raster.regions!=input.fragment.regions || input.raster.surface_scale!=input.fragment.surface_scale ||
        std::memcmp(input.raster.depth_factor_offset.data(),input.fragment.depth_factor_offset.data(),sizeof(input.raster.depth_factor_offset)))
      throw FormatError("raw world draw shader/raster packets do not describe the same inclusive source state");
    for(const auto value:input.fragment.depth_factor_offset)if(!std::isfinite(value))
      throw FormatError("raw world source polygon offset is nonfinite");
    for(const auto value:input.fragment.surface_scale)if(value<=0)
      throw FormatError("raw world source surface scale is not positive");
    for(const auto &color:input.fragment.combiner_constants)for(const auto value:color)
      if(!std::isfinite(value))throw FormatError("raw world completed combiner float bank is missing");
    (void)fixed_state(input);
  }
  std::vector<std::uint32_t> key(const MenuWorldSourceStoredDraw &draw,
      const MenuWorldVertexCatalogEntry &entry,const FixedState &state) const {
    const auto pass=handle_word(context.render_pass);
    std::vector<std::uint32_t> result{
        std::uint32_t(pass),std::uint32_t(pass>>32),0,entry.abi,entry.class_index,
        std::uint32_t(draw.topology),0, // Restart disabled; source strips already expanded.
        VK_POLYGON_MODE_FILL,state.cull,VK_FRONT_FACE_CLOCKWISE,VK_TRUE,VK_FALSE,
        std::bit_cast<std::uint32_t>(1.0f),VK_SAMPLE_COUNT_1_BIT,VK_FALSE,VK_FALSE,
        state.depth_test,state.depth_write,std::uint32_t(state.depth_compare),VK_FALSE,state.stencil_test,
        std::uint32_t(state.stencil.failOp),std::uint32_t(state.stencil.passOp),
        std::uint32_t(state.stencil.depthFailOp),std::uint32_t(state.stencil.compareOp),
        state.stencil.compareMask,state.stencil.writeMask,state.stencil.reference,
        state.blend.blendEnable,std::uint32_t(state.blend.srcColorBlendFactor),std::uint32_t(state.blend.dstColorBlendFactor),
        std::uint32_t(state.blend.colorBlendOp),std::uint32_t(state.blend.srcAlphaBlendFactor),std::uint32_t(state.blend.dstAlphaBlendFactor),
        std::uint32_t(state.blend.alphaBlendOp),state.blend.colorWriteMask,VK_FALSE,VK_LOGIC_OP_COPY};
    for(const auto value:state.blend_constants)result.push_back(std::bit_cast<std::uint32_t>(value));
    result.push_back(std::uint32_t(draw.vertex_bindings.size()));
    for(const auto &binding:draw.vertex_bindings)
      result.insert(result.end(),{binding.binding,binding.stride,std::uint32_t(binding.inputRate)});
    result.push_back(std::uint32_t(draw.vertex_attributes.size()));
    for(const auto &attribute:draw.vertex_attributes)
      result.insert(result.end(),{attribute.location,attribute.binding,std::uint32_t(attribute.format),attribute.offset});
    result.push_back(std::uint32_t(entry.semantics.size()));
    for(const auto value:entry.semantics)result.push_back(value);
    result.push_back(std::uint32_t(entry.spirv.size()));
    result.insert(result.end(),entry.spirv.begin(),entry.spirv.end());
    if(context.triangle_vertices)result.insert(result.end(),entry.triangle_spirv.begin(),entry.triangle_spirv.end());
    return result;
  }
  VkPipeline pipeline(const MenuWorldSourceStoredDraw &draw,const MenuWorldNativeDrawInput &input) {
    if(draw.topology!=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST || draw.index_type!=VK_INDEX_TYPE_UINT16 ||
        !draw.index_count || !draw.indices.buffer || draw.vertex_buffers.empty())
      throw FormatError("raw world draw is not a nonempty source-ordered ushort triangle list");
    const auto default_texture_w=menu_world_vertex_metadata_lookup(
        input.source.program_bytes(),input.source.declaration()).default_texture_w_mask;
    const auto state=fixed_state(input);auto identity=key(draw,*input.vertex_shader,state);
    identity.push_back(default_texture_w);
    identity.push_back(std::uint32_t(input.w_clip_mode));
    identity.push_back(std::uint32_t(input.fog_alpha_bound));
    const auto fragment_path=native_fragment_kernel_path("menu",input.fragment_program);
    for(unsigned i=0;i<60;++i)
      identity.push_back((i>=10 && i<26) || i==43 || i==44?0:input.fragment_program[i]);
    if(const auto found=pipelines.find(identity);found!=pipelines.end())return found->second;
    auto fragment=fragment_modules.find(fragment_path);
    if(fragment==fragment_modules.end()) {
      const auto handle=module(shader(context.shader_directory/fragment_path));
      try {fragment=fragment_modules.emplace(fragment_path,handle).first;}
      catch(...) {vkDestroyShaderModule(context.device,handle,nullptr);throw;}
    }
    const auto selected_vertex=context.triangle_vertices?input.vertex_shader->triangle_spirv:input.vertex_shader->spirv;
    std::vector<std::uint32_t> words(selected_vertex.begin(),selected_vertex.end());
    auto found=vertex_modules.find(words);
    if(found==vertex_modules.end()) {
      const auto handle=module(words);
      try {found=vertex_modules.emplace(std::move(words),handle).first;}
      catch(...) {vkDestroyShaderModule(context.device,handle,nullptr);throw;}
    }
    const std::array<std::uint32_t,4> specialization_values{default_texture_w,std::uint32_t(input.w_clip_mode),std::uint32_t(input.fog_alpha_bound),std::uint32_t(context.depth_encoding)};
    const VkSpecializationMapEntry specialization_entries[]{{0,0,sizeof(std::uint32_t)},
        {1,sizeof(std::uint32_t),sizeof(std::uint32_t)},
        {2,2*sizeof(std::uint32_t),sizeof(std::uint32_t)},
        {61,3*sizeof(std::uint32_t),sizeof(std::uint32_t)}};
    const VkSpecializationInfo projection{4,specialization_entries,sizeof(specialization_values),specialization_values.data()};
    const VkPipelineShaderStageCreateInfo stages[]{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,found->second,"main",nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,geometry_shader,"main",nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment->second,"main",&projection}};
    VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex.vertexBindingDescriptionCount=context.triangle_vertices?0:std::uint32_t(draw.vertex_bindings.size());vertex.pVertexBindingDescriptions=draw.vertex_bindings.data();
    vertex.vertexAttributeDescriptionCount=context.triangle_vertices?0:std::uint32_t(draw.vertex_attributes.size());vertex.pVertexAttributeDescriptions=draw.vertex_attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=draw.topology;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.depthClampEnable=VK_TRUE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=state.cull;
    raster.frontFace=VK_FRONT_FACE_CLOCKWISE;raster.lineWidth=1; // Offset already belongs to FS packet.
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable=state.depth_test;depth.depthWriteEnable=state.depth_write;depth.depthCompareOp=state.depth_compare;
    depth.stencilTestEnable=state.stencil_test;depth.front=depth.back=state.stencil;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount=1;blend.pAttachments=&state.blend;blend.logicOp=VK_LOGIC_OP_COPY;
    std::copy(state.blend_constants.begin(),state.blend_constants.end(),blend.blendConstants);
    const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
    VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    const VkPipelineShaderStageCreateInfo triangle_stages[]{stages[0],stages[2]};
    create.stageCount=context.triangle_vertices?2:3;create.pStages=context.triangle_vertices?triangle_stages:stages;create.pVertexInputState=&vertex;create.pInputAssemblyState=&assembly;
    create.pViewportState=&viewport;create.pRasterizationState=&raster;create.pMultisampleState=&samples;
    create.pDepthStencilState=&depth;create.pColorBlendState=&blend;create.pDynamicState=&dynamic;
    create.layout=layout;create.renderPass=context.render_pass;
    VkPipeline result{};const auto status=vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&create,nullptr,&result);
    if(status!=VK_SUCCESS) {
      if(result)vkDestroyPipeline(context.device,result,nullptr);
      check(status,"create raw world production pipeline");
    }
    try {pipelines.emplace(std::move(identity),result);}
    catch(...) {vkDestroyPipeline(context.device,result,nullptr);throw;}
    return result;
  }
  void prepare(std::span<const MenuWorldNativeDrawInput> inputs) {
    // Caller has completed all uses. Clearing first prevents recording any
    // stale prepared state after validation/allocation/pipeline failure.
    frame.reset();
    if(inputs.empty())return;
    if(inputs.size()>std::numeric_limits<std::uint32_t>::max()/2u)
      throw FormatError("raw world draw descriptor count overflow");
    std::vector<MenuWorldSourceBinding> bindings;bindings.reserve(inputs.size());
    for(const auto &input:inputs){validate(input);bindings.push_back(input.source);}
    const auto stored=geometry.store(bindings);
    auto next=std::make_unique<Frame>(context.device);
    next->vertex=buffer(multiplied(vertex_stride,inputs.size()));
    next->fragment=buffer(multiplied(fragment_stride,inputs.size()));
    const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,std::uint32_t(inputs.size()*2)},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,std::uint32_t(inputs.size()*2)}};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets=std::uint32_t(inputs.size());pool.poolSizeCount=context.triangle_vertices?2:1;pool.pPoolSizes=sizes;
    check(vkCreateDescriptorPool(context.device,&pool,nullptr,&next->pool),"create raw world per-draw descriptor pool");
    std::vector<VkDescriptorSetLayout> layouts(inputs.size(),draw_layout);
    std::vector<VkDescriptorSet> sets(inputs.size());
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool=next->pool;allocate.descriptorSetCount=std::uint32_t(sets.size());allocate.pSetLayouts=layouts.data();
    check(vkAllocateDescriptorSets(context.device,&allocate,sets.data()),"allocate raw world per-draw descriptors");
    next->draws.reserve(inputs.size());
    for(std::size_t i=0;i<inputs.size();++i) {
      const auto &input=inputs[i];
      std::memcpy(static_cast<std::byte *>(next->vertex->mapped)+i*vertex_stride,&input.vertex,sizeof(input.vertex));
      std::memcpy(static_cast<std::byte *>(next->vertex->mapped)+i*vertex_stride+sizeof(input.vertex),
          input.vertex_attributes.data(),sizeof(input.vertex_attributes));
      std::memcpy(static_cast<std::byte *>(next->fragment->mapped)+i*fragment_stride,&input.fragment,sizeof(input.fragment));
      std::array<VkDescriptorBufferInfo,4> buffers{{
          {next->vertex->handle,i*vertex_stride,(input.vertex_shader->abi==menu_world_vertex_attribute_catalog_abi ||
              input.vertex_shader->abi==menu_world_renderer_vertex_catalog_abi)?
              sizeof(MenuWorldVertexAttributeUniforms):sizeof(MenuWorldVertexUniforms)},
          {next->fragment->handle,i*fragment_stride,sizeof(MenuWorldFragmentUniforms)}}};
      if(context.triangle_vertices) {
        const auto words=menu_world_triangle_storage(stored[i]);
        const auto bytes=words.size()*sizeof(std::uint32_t);
        if(bytes>properties.limits.maxStorageBufferRange)throw FormatError("world triangle metadata exceeds storage buffer device range");
        auto metadata=buffer(bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        std::memcpy(metadata->mapped,words.data(),bytes);flush(*metadata);
        const auto &raw=stored[i].raw_storage;
        buffers[2]={raw.buffer,raw.offset,raw.size};buffers[3]={metadata->handle,0,bytes};
        next->triangles.push_back(std::move(metadata));
      }
      const auto descriptor_count=context.triangle_vertices?4u:2u;
      VkWriteDescriptorSet writes[4]{};
      for(unsigned binding=0;binding<descriptor_count;++binding) {
        writes[binding]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[binding].dstSet=sets[i];writes[binding].dstBinding=binding;
        writes[binding].descriptorCount=1;writes[binding].descriptorType=binding<2?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[binding].pBufferInfo=&buffers[binding];
      }
      vkUpdateDescriptorSets(context.device,descriptor_count,writes,0,nullptr);
      next->draws.push_back({stored[i],pipeline(stored[i],input),sets[i],input.material_set,input.material_offset});
    }
    flush(*next->vertex);flush(*next->fragment);frame=std::move(next);
  }
  void record(VkCommandBuffer command,std::size_t index) const {
    if(!command || !frame || index>=frame->draws.size())
      throw FormatError("raw world record has no matching prepared draw");
    const auto &draw=frame->draws[index];const auto &geometry=draw.geometry;
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.pipeline);
    const VkDescriptorSet sets[]{draw.material,draw.uniforms};
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,2,sets,1,&draw.material_offset);
    if(context.triangle_vertices)vkCmdDraw(command,geometry.index_count,1,0,0);
    else {
      vkCmdBindVertexBuffers(command,0,std::uint32_t(geometry.vertex_buffers.size()),geometry.vertex_buffers.data(),geometry.vertex_offsets.data());
      vkCmdBindIndexBuffer(command,geometry.indices.buffer,geometry.indices.offset,geometry.index_type);
      vkCmdDrawIndexed(command,geometry.index_count,1,0,0,0);
    }
  }
};

MenuWorldNativeDraw::MenuWorldNativeDraw(MenuWorldNativeDrawContext context):impl_(std::make_unique<Impl>(context)){}
MenuWorldNativeDraw::~MenuWorldNativeDraw()=default;
void MenuWorldNativeDraw::prepare(std::span<const MenuWorldNativeDrawInput> inputs){impl_->prepare(inputs);}
void MenuWorldNativeDraw::record(VkCommandBuffer command,std::size_t index) const{impl_->record(command,index);}
std::size_t MenuWorldNativeDraw::size() const{return impl_->frame?impl_->frame->draws.size():0;}
VkPipelineLayout MenuWorldNativeDraw::pipeline_layout() const{return impl_->layout;}
} // namespace halo2
