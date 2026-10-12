#include "halo2/menu_world_source_storage.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/menu_world_fragment.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/world_material_uniform.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#include "menu_world_bitmap_import_support.hpp"
#include "menu_world_fog_import_support.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <vector>

// Actual original1335/1448/1880 source-input raster slices. All three modules and
// descriptors are production code. Default modes use captured texture fixtures;
// optional1335/1880 modes use actual Cache/material imports and frontend storage prep.
// Neither mode claims live renderer admission.
using namespace halo2;
namespace {
namespace fs=std::filesystem;
using Bytes=std::vector<std::byte>;
void require(bool v,const std::string &s){if(!v)throw FormatError(s);}
void check(VkResult r){require(r==VK_SUCCESS,"native fragment Vulkan error "+std::to_string(r));}
Bytes read(const fs::path &p,std::size_t expected=0) {
  std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f),"missing native fragment fixture "+p.string());
  const auto n=f.tellg();require(n>0 && n<64*1024*1024,"invalid native fragment fixture extent");
  Bytes b(static_cast<std::size_t>(n));require(!expected || b.size()==expected,"native fragment fixture extent differs: "+p.string());
  f.seekg(0);require(bool(f.read(reinterpret_cast<char *>(b.data()),n)),"short native fragment fixture");return b;
}
void save(const fs::path &p,const Bytes &b) {
  std::ofstream f(p,std::ios::binary);require(bool(f.write(reinterpret_cast<const char *>(b.data()),b.size())),"cannot save native fragment readback");
}
std::uint32_t word(const Bytes &b,std::size_t at) {
  require(at<=b.size() && 4<=b.size()-at,"native fragment word out of bounds");std::uint32_t v;std::memcpy(&v,b.data()+at,4);return v;
}
std::size_t differences(const Bytes &a,const Bytes &b,bool depth=false) {
  require(a.size()==b.size(),"native fragment readback extent differs");std::size_t n=0;
  if(depth){require(a.size()%4==0,"unaligned D24 readback");for(std::size_t i=0;i<a.size();i+=4)n+=(word(a,i)&0xffffffu)!=(word(b,i)&0xffffffu);}
  else for(std::size_t i=0;i<a.size();++i)n+=a[i]!=b[i];return n;
}
struct State {
  std::array<std::uint32_t,44> w{};
  float f(unsigned i)const{return std::bit_cast<float>(w.at(i));}
  VkViewport viewport()const{return {f(31),f(32),f(33),f(34),f(35),f(36)};}
  VkRect2D scissor()const{return {{std::bit_cast<std::int32_t>(w[37]),std::bit_cast<std::int32_t>(w[38])},{w[39],w[40]}};}
  VkStencilOpState stencil(unsigned i)const{return {VkStencilOp(w[i]),VkStencilOp(w[i+1]),VkStencilOp(w[i+2]),VkCompareOp(w[i+3]),w[i+4],w[i+5],w[i+6]};}
};
struct TextureFixture {
  // Version,stage,width,height,layers,mips; native sampler fields; RGBA view.
  std::array<std::uint32_t,22> w{};
  struct Mip {unsigned layer{},level{},width{},height{};Bytes data;};
  std::vector<Mip> levels;
};
struct Blend {
  std::array<std::uint32_t,16> w{0x4d574231,0,0,0,0,0,0,0,15,0,VK_LOGIC_OP_COPY,0,0,0,0,0};
  float f(unsigned i)const{return std::bit_cast<float>(w.at(i));}
};
struct Fixture {
  unsigned event{},source_index_count{},ordered_index_count{},sampled_stages{};
  State state;Blend blend;WorldMaterialUniform material{};MenuWorldVertexUniforms vertex{};
  MenuWorldFragmentUniforms fragment{};
  std::array<TextureFixture,4> textures;
  Bytes before_color,before_depth,before_stencil,after_color,after_depth,after_stencil;
};
template<class T> void load(const fs::path &p,T &value) {
  const auto b=read(p,sizeof(T));std::memcpy(&value,b.data(),b.size());
}
Fixture fixture(const fs::path &p) {
  std::ifstream meta(p/"manifest.tsv");std::string line;
  require(bool(std::getline(meta,line)),"missing native fragment manifest version");
  const bool old1335=line=="H2_NATIVE_FRAGMENT_RASTER_1335_V2";
  const bool version4=line=="H2_NATIVE_FRAGMENT_RASTER_V4";
  require(old1335 || version4 || line=="H2_NATIVE_FRAGMENT_RASTER_V3","native fragment manifest version differs");
  Fixture r;
  require(bool(std::getline(meta,line)),"missing native fragment scope");
  if(old1335){require(line=="1335 640 480 522 4","native fragment1335 scope differs");r.event=1335;r.source_index_count=r.ordered_index_count=522;r.sampled_stages=4;}
  else {
    std::istringstream scope(line);unsigned width{},height{};std::string extra;
    require(bool(scope>>r.event>>width>>height>>r.ordered_index_count>>r.sampled_stages>>r.source_index_count) && !(scope>>extra) &&
        width==640 && height==480,"native fragment scope differs");
    require((r.event==1335 && r.source_index_count==522 && r.ordered_index_count==522 && r.sampled_stages==4) ||
        (r.event==1448 && r.source_index_count==4379 && r.ordered_index_count==13131 && r.sampled_stages==4) ||
        (r.event==1880 && version4 && r.source_index_count==4379 && r.ordered_index_count==13131 && r.sampled_stages==2),"unsupported source-input raster case");
  }
  require(!std::getline(meta,line),"unexpected native fragment manifest rows");
  load(p/"state.bin",r.state);load(p/"program.bin",r.material.program);
  if(version4)load(p/"blend.bin",r.blend);
  load(p/"alpha.bin",r.material.alpha_test);load(p/"effects.bin",r.material.texture_effects);
  load(p/"bump.bin",r.material.bump_matrices);load(p/"vertex.bin",r.vertex);load(p/"fragment.bin",r.fragment);
  // Expected448B packet comes from original944B UBO slices, independently of
  // this real completed-program CPU binder. Do not upload the fixture bank as
  // a substitute for the native producer whose GPU unpack failed13 words.
  PixelProgram completed;completed.words=r.material.program;
  auto bound=r.fragment;bind_menu_world_fragment_completed_program(bound,completed);
  require(std::memcmp(&bound,&r.fragment,sizeof(bound))==0,"native completed-program binder differs from original448B packet");
  r.fragment=bound;
  const auto &s=r.state.w;
  require(s[0]==0x57444731 && s[1]==640 && s[2]==480 && s[3]==r.ordered_index_count && s[6] && s[7] &&
      s[30]==VK_SAMPLE_COUNT_1_BIT && !s[26] && !s[41],"unsupported original raster inputs");
  const auto &b=r.blend.w;
  require(b[0]==0x4d574231 && b[8]==15 && !b[9] && b[10]==VK_LOGIC_OP_COPY && !b[11],"unsupported original blend/write state");
  if(r.event==1880) {
    require(!s[8] && s[9]==VK_COMPARE_OP_EQUAL && !s[10],"original planar fog depth/stencil policy differs");
    require(b[1]==1 && b[2]==VK_BLEND_FACTOR_CONSTANT_COLOR && b[3]==VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA && b[4]==VK_BLEND_OP_ADD &&
        b[5]==VK_BLEND_FACTOR_CONSTANT_COLOR && b[6]==VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA && b[7]==VK_BLEND_OP_ADD &&
        r.blend.f(12)==1 && r.blend.f(13)==1 && r.blend.f(14)==1 && r.blend.f(15)==0,"original planar fog captured blend differs");
  } else {
    require(s[8] && s[9]==VK_COMPARE_OP_LESS_OR_EQUAL && s[10],"original base depth/stencil policy differs");
    for(unsigned i=1;i<8;++i)require(b[i]==0,"base draw changed original disabled blend");
  }
  require(r.vertex.surface_size==std::array<float,2>{640,480},"original surface extent differs");
  for(unsigned stage=r.sampled_stages;stage<4;++stage)
    require(((r.material.program[54]>>(5*stage))&31u)==0,"missing original sampler stage is consumed");
  for(unsigned stage=0;stage<r.sampled_stages;++stage) {
    auto &t=r.textures[stage];load(p/("stage-"+std::to_string(stage)+".bin"),t.w);
    require(t.w[0]==0x4d575431 && t.w[1]==stage && t.w[2] && t.w[3] && (t.w[4]==1 || t.w[4]==6) && t.w[5],"invalid original texture metadata");
    require((t.w[21]==VK_FORMAT_R8G8B8A8_UNORM || t.w[21]==VK_FORMAT_B8G8R8A8_UNORM) &&
        t.w[6]<=VK_FILTER_LINEAR && t.w[7]<=VK_FILTER_LINEAR && t.w[8]<=VK_SAMPLER_MIPMAP_MODE_LINEAR &&
        t.w[12]<=1,"unsupported original texture format/filter");
    for(unsigned i:{9u,10u,11u})require(t.w[i]==VK_SAMPLER_ADDRESS_MODE_REPEAT || t.w[i]==VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,"unsupported original sampler address");
    for(unsigned i=13;i<=16;++i)require(std::isfinite(std::bit_cast<float>(t.w[i])),"nonfinite original sampler scalar");
    for(unsigned i=17;i<=20;++i)require(t.w[i]>=VK_COMPONENT_SWIZZLE_ZERO && t.w[i]<=VK_COMPONENT_SWIZZLE_A,"unsupported original component mapping");
    std::ifstream mip(p/("stage-"+std::to_string(stage)+"-mips.tsv"));require(bool(mip),"missing original mip manifest");
    while(std::getline(mip,line)) {
      std::istringstream row(line);TextureFixture::Mip m;std::string name,extra;
      require(bool(row>>m.layer>>m.level>>m.width>>m.height>>name) && !(row>>extra),"invalid original mip row");
      require(m.layer<t.w[4] && m.level<t.w[5] && m.width==std::max(1u,t.w[2]>>m.level) && m.height==std::max(1u,t.w[3]>>m.level),"original mip shape differs");
      require(fs::path(name).is_relative() && name.find("..") == std::string::npos,"original mip path escapes fixture");
      m.data=read(p/name,std::size_t(m.width)*m.height*4);t.levels.push_back(std::move(m));
    }
    require(t.levels.size()==std::size_t(t.w[4])*t.w[5],"original texture mip/face coverage incomplete");
    for(unsigned layer=0;layer<t.w[4];++layer)for(unsigned level=0;level<t.w[5];++level)
      require(std::count_if(t.levels.begin(),t.levels.end(),[&](const auto &m){return m.layer==layer && m.level==level;})==1,"duplicate/missing original mip");
  }
  constexpr auto pixels=640u*480u;
  r.before_color=read(p/"before-color.bin",pixels*4);r.before_depth=read(p/"before-depth.bin",pixels*4);r.before_stencil=read(p/"before-stencil.bin",pixels);
  r.after_color=read(p/"after-color.bin",pixels*4);r.after_depth=read(p/"after-depth.bin",pixels*4);r.after_stencil=read(p/"after-stencil.bin",pixels);return r;
}
std::atomic<unsigned> validation_errors{},cleanup_failures{};
VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *data,void *) {
  if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<data->pMessage<<'\n';}return VK_FALSE;
}
struct Result {Bytes color,depth,stencil,initial_color,initial_depth,initial_stencil;};
class Gpu {
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};VkDeviceSize size{};};
  struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkImageAspectFlags aspect{};unsigned layers=1,mips=1;VkImageLayout layout=VK_IMAGE_LAYOUT_UNDEFINED;};
  VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkQueue queue{};unsigned family{};
  VkCommandPool pool{};VkCommandBuffer command{};VkDescriptorPool descriptor_pool{};
  std::array<VkDescriptorSetLayout,2> set_layouts{};std::array<VkDescriptorSet,2> sets{};VkPipelineLayout pipeline_layout{};
  VkRenderPass pass{};VkFramebuffer frame{};VkPipeline pipeline{};
  std::vector<Buffer> buffers;std::vector<Image> images;std::vector<VkShaderModule> modules;std::vector<VkSampler> samplers;
  Image color,depth;std::array<Image,4> texture_images;Image dummy2d,dummycube,dummyvolume;
  std::array<VkSampler,4> texture_samplers{};VkSampler dummy_sampler{};
  Buffer material,vertex,fragment,color_seed,depth_seed,stencil_seed;
  std::array<Buffer,3> before,after;
  unsigned memory_type(unsigned bits,VkMemoryPropertyFlags wanted) {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(unsigned i=0;i<p.memoryTypeCount;++i)if((bits&(1u<<i)) && (p.memoryTypes[i].propertyFlags&wanted)==wanted)return i;
    throw FormatError("native fragment memory type unavailable");
  }
  Buffer buffer(VkDeviceSize size,VkBufferUsageFlags usage) {
    require(size>0,"zero-size native fragment buffer");buffers.emplace_back();auto &r=buffers.back();r.size=size;
    VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=size;b.usage=usage;check(vkCreateBuffer(device,&b,nullptr,&r.handle));
    VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,r.handle,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;
    a.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(device,&a,nullptr,&r.memory));check(vkBindBufferMemory(device,r.handle,r.memory,0));check(vkMapMemory(device,r.memory,0,size,0,&r.mapped));return r;
  }
  Image image(VkFormat fmt,unsigned width,unsigned height,unsigned layers,unsigned mips,VkImageUsageFlags usage,VkImageAspectFlags aspect,
      VkComponentMapping mapping={},bool volume=false) {
    images.emplace_back();auto &r=images.back();r.aspect=aspect;r.layers=layers;r.mips=mips;
    VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.flags=layers==6?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
    i.imageType=volume?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;i.format=fmt;i.extent={width,height,1};i.mipLevels=mips;i.arrayLayers=layers;
    i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=usage;check(vkCreateImage(device,&i,nullptr,&r.handle));
    VkMemoryRequirements req;vkGetImageMemoryRequirements(device,r.handle,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;
    a.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);check(vkAllocateMemory(device,&a,nullptr,&r.memory));check(vkBindImageMemory(device,r.handle,r.memory,0));
    VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=r.handle;v.viewType=volume?VK_IMAGE_VIEW_TYPE_3D:layers==6?VK_IMAGE_VIEW_TYPE_CUBE:VK_IMAGE_VIEW_TYPE_2D;
    v.format=fmt;v.components=mapping;v.subresourceRange={aspect,0,mips,0,layers};check(vkCreateImageView(device,&v,nullptr,&r.view));return r;
  }
  VkShaderModule module(std::span<const std::uint32_t> words) {
    require(words.size()>=5 && words[0]==0x07230203u,"invalid native fragment module");VkShaderModuleCreateInfo c{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};c.codeSize=words.size_bytes();c.pCode=words.data();
    VkShaderModule m{};check(vkCreateShaderModule(device,&c,nullptr,&m));modules.push_back(m);return m;
  }
  VkShaderModule module(const fs::path &p) {
    const auto b=read(p);require(b.size()%4==0,"unaligned native fragment SPIR-V");std::vector<std::uint32_t> words(b.size()/4);std::memcpy(words.data(),b.data(),b.size());return module(words);
  }
  void begin() {check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&b));}
  void submit() {check(vkEndCommandBuffer(command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&s,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));}
  void barrier(Image &i,VkImageLayout next,VkAccessFlags from,VkAccessFlags to,VkPipelineStageFlags a,VkPipelineStageFlags b) {
    VkImageMemoryBarrier v{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};v.image=i.handle;v.oldLayout=i.layout;v.newLayout=next;v.srcAccessMask=from;v.dstAccessMask=to;
    v.srcQueueFamilyIndex=v.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;v.subresourceRange={i.aspect,0,i.mips,0,i.layers};vkCmdPipelineBarrier(command,a,b,0,0,nullptr,0,nullptr,1,&v);i.layout=next;
  }
  void bind_buffer(VkDescriptorSet set,unsigned binding,Buffer b) {
    VkDescriptorBufferInfo i{b.handle,0,b.size};VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=set;w.dstBinding=binding;w.descriptorCount=1;w.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w.pBufferInfo=&i;vkUpdateDescriptorSets(device,1,&w,0,nullptr);
  }
  void bind_images(unsigned binding,std::span<const VkDescriptorImageInfo> i) {
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};w.dstSet=sets[0];w.dstBinding=binding;w.descriptorCount=i.size();w.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;w.pImageInfo=i.data();vkUpdateDescriptorSets(device,1,&w,0,nullptr);
  }
  void copy(Image &i,Buffer b,VkImageAspectFlags aspect,bool upload) {
    VkBufferImageCopy c{};c.imageSubresource={aspect,0,0,1};c.imageExtent={640,480,1};
    if(upload)vkCmdCopyBufferToImage(command,b.handle,i.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&c);
    else vkCmdCopyImageToBuffer(command,i.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,b.handle,1,&c);
  }
  Bytes bytes(Buffer b)const {Bytes r(b.size);std::memcpy(r.data(),b.mapped,r.size());return r;}
public:
  VkPhysicalDevice physical{};VkDevice device{};
  ~Gpu() {
    if(device){if(vkDeviceWaitIdle(device)!=VK_SUCCESS)++cleanup_failures;if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);for(auto m:modules)vkDestroyShaderModule(device,m,nullptr);
      if(frame)vkDestroyFramebuffer(device,frame,nullptr);if(pass)vkDestroyRenderPass(device,pass,nullptr);if(pipeline_layout)vkDestroyPipelineLayout(device,pipeline_layout,nullptr);
      if(descriptor_pool)vkDestroyDescriptorPool(device,descriptor_pool,nullptr);for(auto l:set_layouts)if(l)vkDestroyDescriptorSetLayout(device,l,nullptr);for(auto s:samplers)vkDestroySampler(device,s,nullptr);
      for(auto i:images){if(i.view)vkDestroyImageView(device,i.view,nullptr);if(i.handle)vkDestroyImage(device,i.handle,nullptr);if(i.memory)vkFreeMemory(device,i.memory,nullptr);}
      for(auto b:buffers){if(b.mapped)vkUnmapMemory(device,b.memory);if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);if(b.memory)vkFreeMemory(device,b.memory,nullptr);}if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}
    if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);if(instance)vkDestroyInstance(instance,nullptr);
  }
  bool initialize(const Fixture &f) {
    unsigned n{};check(vkEnumerateInstanceLayerProperties(&n,nullptr));std::vector<VkLayerProperties> layers(n);check(vkEnumerateInstanceLayerProperties(&n,layers.data()));
    if(std::none_of(layers.begin(),layers.end(),[](const auto &l){return std::string(l.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
    const char *layer="VK_LAYER_KHRONOS_validation";const char *extensions[]{VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debug.pfnUserCallback=diagnostic;
    VkValidationFeatureEnableEXT sync=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;VkValidationFeaturesEXT v{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};v.enabledValidationFeatureCount=1;v.pEnabledValidationFeatures=&sync;v.pNext=&debug;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledLayerCount=1;ci.ppEnabledLayerNames=&layer;ci.enabledExtensionCount=2;ci.ppEnabledExtensionNames=extensions;ci.pNext=&v;check(vkCreateInstance(&ci,nullptr,&instance));
    check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
    check(vkEnumeratePhysicalDevices(instance,&n,nullptr));std::vector<VkPhysicalDevice> devices(n);check(vkEnumeratePhysicalDevices(instance,&n,devices.data()));
    for(auto d:devices) {
      VkPhysicalDeviceProperties p;vkGetPhysicalDeviceProperties(d,&p);VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&f13;vkGetPhysicalDeviceFeatures2(d,&features);
      if(p.apiVersion<VK_API_VERSION_1_3 || !features.features.geometryShader || !features.features.shaderTessellationAndGeometryPointSize || !features.features.depthClamp || !features.features.samplerAnisotropy || p.limits.maxSamplerAnisotropy<8 || !f13.shaderDemoteToHelperInvocation)continue;
      bool formats=true;for(auto fmt:{VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_B8G8R8A8_UNORM,VK_FORMAT_D24_UNORM_S8_UINT}) {
        VkFormatProperties fp;vkGetPhysicalDeviceFormatProperties(d,fmt,&fp);const auto need=VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT|
            (fmt!=VK_FORMAT_D24_UNORM_S8_UINT?(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT):VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        formats&=(fp.optimalTilingFeatures&need)==need;
      }
      if(!formats)continue;vkGetPhysicalDeviceQueueFamilyProperties(d,&n,nullptr);std::vector<VkQueueFamilyProperties> queues(n);vkGetPhysicalDeviceQueueFamilyProperties(d,&n,queues.data());
      for(unsigned i=0;i<n;++i)if(queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=d;family=i;break;}if(physical)break;
    }
    if(!physical)return false;
    VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};f13.shaderDemoteToHelperInvocation=VK_TRUE;
    VkPhysicalDeviceFeatures features{};features.geometryShader=features.shaderTessellationAndGeometryPointSize=features.depthClamp=features.samplerAnisotropy=VK_TRUE;
    const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
    VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.pNext=&f13;dc.pEnabledFeatures=&features;dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;check(vkCreateDevice(physical,&dc,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(device,&pc,nullptr,&pool));
    VkCommandBufferAllocateInfo ac{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ac.commandPool=pool;ac.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ac.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ac,&command));
    constexpr auto pixels=640u*480u;
    material=buffer(sizeof(f.material),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);vertex=buffer(sizeof(f.vertex),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);fragment=buffer(sizeof(f.fragment),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    std::memcpy(material.mapped,&f.material,sizeof(f.material));std::memcpy(vertex.mapped,&f.vertex,sizeof(f.vertex));
    color_seed=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);depth_seed=buffer(pixels*4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);stencil_seed=buffer(pixels,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    std::memcpy(color_seed.mapped,f.before_color.data(),f.before_color.size());std::memcpy(depth_seed.mapped,f.before_depth.data(),f.before_depth.size());std::memcpy(stencil_seed.mapped,f.before_stencil.data(),f.before_stencil.size());
    for(auto *set:{&before,&after})for(unsigned i=0;i<3;++i)(*set)[i]=buffer(pixels*(i==2?1:4),VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    // Name() authority pins original Resource8435/view8437 to BGRA8. The
    // retained before/after planes are raw BGRA bytes, never PNG/RGBA copies.
    color=image(VK_FORMAT_B8G8R8A8_UNORM,640,480,1,1,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
    depth=image(VK_FORMAT_D24_UNORM_S8_UINT,640,480,1,1,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT);
    const auto image_usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    dummy2d=image(VK_FORMAT_R8G8B8A8_UNORM,1,1,1,1,image_usage,VK_IMAGE_ASPECT_COLOR_BIT);dummycube=image(VK_FORMAT_R8G8B8A8_UNORM,1,1,6,1,image_usage,VK_IMAGE_ASPECT_COLOR_BIT);
    dummyvolume=image(VK_FORMAT_R8G8B8A8_UNORM,1,1,1,1,image_usage,VK_IMAGE_ASPECT_COLOR_BIT,{},true);
    VkSamplerCreateInfo ds{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ds.magFilter=ds.minFilter=VK_FILTER_NEAREST;ds.addressModeU=ds.addressModeV=ds.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    check(vkCreateSampler(device,&ds,nullptr,&dummy_sampler));samplers.push_back(dummy_sampler);
    begin();
    for(auto *i:{&dummy2d,&dummycube,&dummyvolume}) {
      barrier(*i,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
      VkClearColorValue white{{1,1,1,1}};VkImageSubresourceRange r{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,i->layers};vkCmdClearColorImage(command,i->handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&white,1,&r);
      barrier(*i,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    for(unsigned stage=0;stage<f.sampled_stages;++stage) {
      const auto &t=f.textures[stage];const auto &w=t.w;
      VkComponentMapping sw{VkComponentSwizzle(w[17]),VkComponentSwizzle(w[18]),VkComponentSwizzle(w[19]),VkComponentSwizzle(w[20])};
      texture_images[stage]=image(VkFormat(w[21]),w[2],w[3],w[4],w[5],image_usage,VK_IMAGE_ASPECT_COLOR_BIT,sw);auto &i=texture_images[stage];
      VkSamplerCreateInfo s{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};s.magFilter=VkFilter(w[6]);s.minFilter=VkFilter(w[7]);s.mipmapMode=VkSamplerMipmapMode(w[8]);
      s.addressModeU=VkSamplerAddressMode(w[9]);s.addressModeV=VkSamplerAddressMode(w[10]);s.addressModeW=VkSamplerAddressMode(w[11]);s.anisotropyEnable=w[12];s.maxAnisotropy=std::bit_cast<float>(w[13]);
      s.mipLodBias=std::bit_cast<float>(w[14]);s.minLod=std::bit_cast<float>(w[15]);s.maxLod=std::bit_cast<float>(w[16]);check(vkCreateSampler(device,&s,nullptr,&texture_samplers[stage]));samplers.push_back(texture_samplers[stage]);
      barrier(i,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
      std::size_t bytes=0;for(const auto &m:t.levels)bytes+=m.data.size();const auto upload=buffer(bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::vector<VkBufferImageCopy> copies;std::size_t at=0;
      for(const auto &m:t.levels){std::memcpy(static_cast<std::byte *>(upload.mapped)+at,m.data.data(),m.data.size());VkBufferImageCopy c{};c.bufferOffset=at;c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,m.level,m.layer,1};c.imageExtent={m.width,m.height,1};copies.push_back(c);at+=m.data.size();}
      VkMemoryBarrier uploaded{VK_STRUCTURE_TYPE_MEMORY_BARRIER};uploaded.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;uploaded.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
      vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&uploaded,0,nullptr,0,nullptr);
      vkCmdCopyBufferToImage(command,upload.handle,i.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,copies.size(),copies.data());
      barrier(i,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    submit();
    const std::array<VkDescriptorPoolSize,2> sizes{{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,3},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,12}}};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=2;dp.poolSizeCount=sizes.size();dp.pPoolSizes=sizes.data();check(vkCreateDescriptorPool(device,&dp,nullptr,&descriptor_pool));
    std::vector<VkDescriptorSetLayoutBinding> bindings;for(unsigned i=0;i<4;++i)bindings.push_back({i,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});
    bindings.push_back({4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});bindings.push_back({5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});bindings.push_back({6,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr});
    for(unsigned set=0;set<2;++set) {
      const std::array<VkDescriptorSetLayoutBinding,2> draw_bindings{{{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}}};
      VkDescriptorSetLayoutCreateInfo l{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};l.bindingCount=set?draw_bindings.size():bindings.size();l.pBindings=set?draw_bindings.data():bindings.data();check(vkCreateDescriptorSetLayout(device,&l,nullptr,&set_layouts[set]));
    }
    VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=descriptor_pool;da.descriptorSetCount=2;da.pSetLayouts=set_layouts.data();check(vkAllocateDescriptorSets(device,&da,sets.data()));
    bind_buffer(sets[0],4,material);bind_buffer(sets[1],0,vertex);bind_buffer(sets[1],1,fragment);
    std::array<VkDescriptorImageInfo,4> cubes,volumes;
    for(unsigned stage=0;stage<4;++stage) {
      const bool active=stage<f.sampled_stages,cube=active && f.textures[stage].w[4]==6;
      const VkDescriptorImageInfo flat{active&&!cube?texture_samplers[stage]:dummy_sampler,active&&!cube?texture_images[stage].view:dummy2d.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};bind_images(stage,std::span(&flat,1));
      cubes[stage]={cube?texture_samplers[stage]:dummy_sampler,cube?texture_images[stage].view:dummycube.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};volumes[stage]={dummy_sampler,dummyvolume.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    bind_images(5,cubes);bind_images(6,volumes);
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=2;pl.pSetLayouts=set_layouts.data();check(vkCreatePipelineLayout(device,&pl,nullptr,&pipeline_layout));
    std::array<VkAttachmentDescription,2> attachments{};attachments[0].format=VK_FORMAT_B8G8R8A8_UNORM;attachments[1].format=VK_FORMAT_D24_UNORM_S8_UINT;
    for(unsigned i=0;i<2;++i){auto &a=attachments[i];a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_STORE;a.initialLayout=a.finalLayout=i?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;}
    const VkAttachmentReference cr{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},dr{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&cr;sub.pDepthStencilAttachment=&dr;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=2;rp.pAttachments=attachments.data();rp.subpassCount=1;rp.pSubpasses=&sub;check(vkCreateRenderPass(device,&rp,nullptr,&pass));
    const VkImageView views[]{color.view,depth.view};VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.attachmentCount=2;fb.pAttachments=views;fb.width=640;fb.height=480;fb.layers=1;check(vkCreateFramebuffer(device,&fb,nullptr,&frame));
    return true;
  }
  void prepare(const MenuWorldSourceStoredDraw &draw,const MenuWorldVertexCatalogEntry &entry,const Fixture &f,const fs::path &gs,const fs::path &fs_shader) {
    VkPipelineShaderStageCreateInfo stages[3]{};for(auto &s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=module(entry.spirv);stages[1].stage=VK_SHADER_STAGE_GEOMETRY_BIT;stages[1].module=module(gs);stages[2].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[2].module=module(fs_shader);
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};input.vertexBindingDescriptionCount=draw.vertex_bindings.size();input.pVertexBindingDescriptions=draw.vertex_bindings.data();input.vertexAttributeDescriptionCount=draw.vertex_attributes.size();input.pVertexAttributeDescriptions=draw.vertex_attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=draw.topology;
    const auto vp=f.state.viewport();const auto sc=f.state.scissor();VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;viewport.pViewports=&vp;viewport.pScissors=&sc;
    const auto &w=f.state.w;VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.depthClampEnable=w[6];raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=w[4];raster.frontFace=VkFrontFace(w[5]);raster.lineWidth=1;
    raster.depthBiasEnable=w[26];raster.depthBiasConstantFactor=f.state.f(27);raster.depthBiasClamp=f.state.f(28);raster.depthBiasSlopeFactor=f.state.f(29);
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VkSampleCountFlagBits(w[30]);
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=w[7];ds.depthWriteEnable=w[8];ds.depthCompareOp=VkCompareOp(w[9]);ds.stencilTestEnable=w[10];ds.front=f.state.stencil(11);ds.back=f.state.stencil(18);ds.depthBoundsTestEnable=w[41];ds.minDepthBounds=f.state.f(42);ds.maxDepthBounds=f.state.f(43);
    const auto &bw=f.blend.w;VkPipelineColorBlendAttachmentState color_blend{};
    color_blend.blendEnable=bw[1];color_blend.srcColorBlendFactor=VkBlendFactor(bw[2]);color_blend.dstColorBlendFactor=VkBlendFactor(bw[3]);color_blend.colorBlendOp=VkBlendOp(bw[4]);
    color_blend.srcAlphaBlendFactor=VkBlendFactor(bw[5]);color_blend.dstAlphaBlendFactor=VkBlendFactor(bw[6]);color_blend.alphaBlendOp=VkBlendOp(bw[7]);color_blend.colorWriteMask=bw[8];
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&color_blend;blend.logicOpEnable=bw[9];blend.logicOp=VkLogicOp(bw[10]);
    for(unsigned i=0;i<4;++i)blend.blendConstants[i]=f.blend.f(12+i);
    VkGraphicsPipelineCreateInfo p{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};p.stageCount=3;p.pStages=stages;p.pVertexInputState=&input;p.pInputAssemblyState=&assembly;p.pViewportState=&viewport;p.pRasterizationState=&raster;p.pMultisampleState=&samples;p.pDepthStencilState=&ds;p.pColorBlendState=&blend;p.layout=pipeline_layout;p.renderPass=pass;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&pipeline));
  }
  Result run(const MenuWorldSourceStoredDraw &draw,const MenuWorldFragmentUniforms &values) {
    std::memcpy(fragment.mapped,&values,sizeof(values));begin();VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;
    host.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_INDEX_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
    for(auto *i:{&color,&depth})barrier(*i,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,i->layout==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,i->layout==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    copy(color,color_seed,VK_IMAGE_ASPECT_COLOR_BIT,true);copy(depth,depth_seed,VK_IMAGE_ASPECT_DEPTH_BIT,true);copy(depth,stencil_seed,VK_IMAGE_ASPECT_STENCIL_BIT,true);
    for(auto *i:{&color,&depth})barrier(*i,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    copy(color,before[0],VK_IMAGE_ASPECT_COLOR_BIT,false);copy(depth,before[1],VK_IMAGE_ASPECT_DEPTH_BIT,false);copy(depth,before[2],VK_IMAGE_ASPECT_STENCIL_BIT,false);
    barrier(color,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rp.renderPass=pass;rp.framebuffer=frame;rp.renderArea={{0,0},{640,480}};vkCmdBeginRenderPass(command,&rp,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,2,sets.data(),0,nullptr);vkCmdBindVertexBuffers(command,0,draw.vertex_buffers.size(),draw.vertex_buffers.data(),draw.vertex_offsets.data());
    vkCmdBindIndexBuffer(command,draw.indices.buffer,draw.indices.offset,draw.index_type);vkCmdDrawIndexed(command,draw.index_count,1,0,0,0);vkCmdEndRenderPass(command);
    barrier(color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    barrier(depth,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    copy(color,after[0],VK_IMAGE_ASPECT_COLOR_BIT,false);copy(depth,after[1],VK_IMAGE_ASPECT_DEPTH_BIT,false);copy(depth,after[2],VK_IMAGE_ASPECT_STENCIL_BIT,false);
    VkMemoryBarrier ready{VK_STRUCTURE_TYPE_MEMORY_BARRIER};ready.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;ready.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&ready,0,nullptr,0,nullptr);submit();
    return {bytes(after[0]),bytes(after[1]),bytes(after[2]),bytes(before[0]),bytes(before[1]),bytes(before[2])};
  }
};
void original_inputs(const MenuWorldSourceStoredDraw &draw,const fs::path &fixtures,const Fixture &f) {
  const auto prefix="event-"+std::to_string(f.event);
  require(draw.source.inputs().size()==(f.event==1880?1u:5u),"original source input count differs");
  for(unsigned i=0;i<draw.source.inputs().size();++i) {
    const auto &input=draw.source.inputs()[i];const auto &stream=input.stream;
    const auto raw=read(fixtures/(prefix+"-vertex-"+std::to_string(input.vertex_register)+".bin"));
    const auto bytes=draw.source.resource_bytes(i);
    require(stream.byte_offset<=bytes.size() && raw.size()<=bytes.size()-stream.byte_offset &&
        std::equal(raw.begin(),raw.end(),bytes.begin()+stream.byte_offset),"native1335 raw source stream differs from original capture");
    const auto attribute=std::find_if(draw.vertex_attributes.begin(),draw.vertex_attributes.end(),[&](const auto &a){return a.location==input.vertex_register;});
    require(attribute!=draw.vertex_attributes.end() && attribute->binding<draw.vertex_bindings.size(),"native1335 lost original vertex input location");
    const auto format=stream.format==MenuWorldSourceFormat::float32x3?VK_FORMAT_R32G32B32_SFLOAT:
        stream.format==MenuWorldSourceFormat::float32x2?VK_FORMAT_R32G32_SFLOAT:
        stream.format==MenuWorldSourceFormat::snorm16x3?VK_FORMAT_R16G16B16_SNORM:
        stream.format==MenuWorldSourceFormat::snorm16x2?VK_FORMAT_R16G16_SNORM:VK_FORMAT_R32_SINT;
    require(attribute->format==format && attribute->offset==stream.byte_offset &&
        draw.vertex_bindings[attribute->binding].stride==stream.byte_stride,"native1335 changed original source fetch format/offset/stride");
  }
  const auto indices=read(fixtures/(prefix+"-indices.bin"),f.source_index_count*4u);
  const auto source=draw.source.draw().indices();require(source.size()==f.source_index_count,"native source index count differs");
  for(std::size_t i=0;i<source.size();++i)require(source[i]==word(indices,i*4),"native1335 changed original index order");
  require(draw.topology==VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST && draw.index_type==VK_INDEX_TYPE_UINT16 &&
      draw.index_count==draw.source.draw().triangle_count()*3,"native1335 dropped source ordered triangles");
}
struct Measurements {
  unsigned event{},sampled_stages{};
  bool blend_enabled{};
  bool native_imports{},imports_prepared{};
  std::size_t imported_mip_faces{},imported_bytes{},imported_texel_differences{};
  bool normal{},control{};
  std::size_t color{},depth{},stencil{},seed_color{},seed_depth{},seed_stencil{},negative{};
};
void native_cache_textures(const Cache &cache,const fs::path &expected_path,Fixture &f,
    const fs::path &output,Measurements &m) {
  namespace imported=menu_world_bitmap_import;
  require(f.event==1335 && f.sampled_stages==4,"actual Cache import mode only admits original1335");
  const auto expected=imported::fixture(expected_path);
  auto candidate=imported::imports(cache,expected);
  std::ofstream metadata(output/"native-cache-textures.tsv");
  metadata<<"stage\tbitmap_datum\timage_index\tsource_format\timport_format\tupload_format\tconverted\tlayers\tmips\talpha_one\n";
  for(unsigned stage=0;stage<4;++stage) {
    auto &image=candidate[stage].texture;const auto import_format=image.format;
    const bool converted=prepare_frontend_texture_storage(image);
    const auto &e=expected.stages[stage];auto &t=f.textures[stage];
    require(image.format==11 && !image.depth && image.width==t.w[2] && image.height==t.w[3] &&
        image.layers==t.w[4] && image.mips.size()==std::size_t(t.w[4])*t.w[5],
        "prepared native1335 image shape/storage differs from original view");
    require(t.w[17]==VK_COMPONENT_SWIZZLE_R && t.w[18]==VK_COMPONENT_SWIZZLE_G && t.w[19]==VK_COMPONENT_SWIZZLE_B &&
        t.w[20]==(e.alpha_one?VK_COMPONENT_SWIZZLE_ONE:VK_COMPONENT_SWIZZLE_A),
        "native1335 sampled alpha/component semantics differ from original");
    metadata<<stage<<'\t'<<candidate[stage].bitmap.datum<<'\t'<<candidate[stage].bitmap.index<<'\t'
        <<candidate[stage].source_format<<'\t'<<import_format<<'\t'<<image.format<<'\t'<<converted<<'\t'
        <<image.layers<<'\t'<<t.w[5]<<'\t'<<e.alpha_one<<'\n';
    // The imported bytes are the sole candidate. Original mip payloads remain
    // independent expectations; keep only their shape and sampler/view state.
    t.levels.clear();t.w[21]=VK_FORMAT_R8G8B8A8_UNORM;
    for(unsigned face=0;face<image.layers;++face)for(unsigned level=0;level<t.w[5];++level) {
      const auto ordinal=face*t.w[5]+level;const auto &source=image.mips[ordinal];const auto &expect=e.expected[ordinal];
      require(source.width==expect.width && source.height==expect.height && source.size==std::size_t(source.width)*source.height*4 &&
          source.offset<=image.pixels.size() && source.size<=image.pixels.size()-source.offset,
          "prepared native1335 mip range/extent differs from original");
      Bytes bytes(image.pixels.begin()+source.offset,image.pixels.begin()+source.offset+source.size);
      save(output/("native-stage-"+std::to_string(stage)+"-face-"+std::to_string(face)+"-mip-"+std::to_string(level)+"-rgba.bin"),bytes);
      auto sampled=bytes;
      if(e.alpha_one)for(std::size_t i=3;i<sampled.size();i+=4)sampled[i]=std::byte{255};
      m.imported_texel_differences+=differences(sampled,expect.sampled);
      ++m.imported_mip_faces;m.imported_bytes+=bytes.size();
      t.levels.push_back({face,level,source.width,source.height,std::move(bytes)});
    }
  }
  metadata.flush();require(bool(metadata),"cannot retain native1335 import metadata");
  require(m.imported_mip_faces==91,"prepared native1335 mip/face coverage differs");
  m.imports_prepared=true;
}
void native_cache_fog_textures(const Cache &cache,const fs::path &expected_path,Fixture &f,
    const fs::path &output,Measurements &m) {
  namespace fog=menu_world_fog_import;
  require(f.event==1880 && f.sampled_stages==2,"actual fog Cache import mode only admits original1880");
  const auto expected=fog::fixture(expected_path);
  const auto candidate=fog::imports(cache);
  std::ofstream metadata(output/"native-cache-textures.tsv");
  metadata<<"stage\texternal\tbitmap_datum\timage_index\toriginal_resource\timport_format\tupload_format\tlayers\tmips\n";
  for(unsigned stage=0;stage<2;++stage) {
    const auto &image=candidate[stage].texture;const auto &e=expected[stage];auto &t=f.textures[stage];
    fog::topology(image);
    require(t.w[2]==image.width && t.w[3]==image.height && t.w[4]==1 && t.w[5]==1 &&
        t.w[17]==VK_COMPONENT_SWIZZLE_R && t.w[18]==VK_COMPONENT_SWIZZLE_G &&
        t.w[19]==VK_COMPONENT_SWIZZLE_B && t.w[20]==VK_COMPONENT_SWIZZLE_A &&
        t.w[21]==VK_FORMAT_B8G8R8A8_UNORM && t.levels.size()==1 &&
        t.levels[0].layer==0 && t.levels[0].level==0 && t.levels[0].data==e.original_bgra,
        "native1880 original image/view fixture differs from independent pinned export");
    m.imported_texel_differences+=differences(image.pixels,e.rgba);
    m.imported_bytes+=image.pixels.size();++m.imported_mip_faces;
    metadata<<stage<<'\t'<<candidate[stage].identity.external<<'\t'<<candidate[stage].identity.datum<<'\t'
        <<candidate[stage].identity.index<<'\t'<<e.identity.resource<<'\t'<<image.format<<'\t'
        <<image.format<<"\t1\t1\n";
    save(output/("native-stage-"+std::to_string(stage)+"-face-0-mip-0-rgba.bin"),image.pixels);
    // Only actual Cache bytes are uploaded. Keep the original captured sampler,
    // including its irrelevant 2D W-repeat, as an explicit diagnostic input.
    t.w[21]=VK_FORMAT_R8G8B8A8_UNORM;
    t.levels={{0,0,image.width,image.height,image.pixels}};
  }
  metadata.flush();require(bool(metadata),"cannot retain native1880 import metadata");
  require(m.imported_mip_faces==2 && m.imported_bytes==2048,"native1880 import coverage differs");
  m.imports_prepared=true;
}
std::string escaped(const std::string &s) {
  std::string r;for(unsigned char c:s) {
    if(c=='"' || c=='\\'){r+='\\';r+=char(c);}
    else if(c=='\n')r+="\\n";else if(c=='\r')r+="\\r";else if(c=='\t')r+="\\t";
    else if(c>=32)r+=char(c);
  }return r;
}
void report(const fs::path &output,const Measurements &m,const std::string &status,const std::string &error={}) {
  if(output.empty() || !fs::is_directory(output))return;
  std::ofstream r(output/"measurements.json");r<<"{\n  \"event\":"<<m.event<<",\n  \"status\":\""<<escaped(status)<<"\",\n  \"error\":\""<<escaped(error)<<"\",\n"
      <<"  \"normal_measured\":"<<(m.normal?"true":"false")<<",\n  \"control_measured\":"<<(m.control?"true":"false")<<",\n"
      <<"  \"original_sampled_stages\":"<<m.sampled_stages<<",\n  \"original_blend_enabled\":"<<(m.blend_enabled?"true":"false")<<",\n"
      <<"  \"color_different_bytes\":"<<m.color<<",\n  \"D24_different_pixels\":"<<m.depth<<",\n  \"stencil_different_bytes\":"<<m.stencil<<",\n"
      <<"  \"seed_color_different_bytes\":"<<m.seed_color<<",\n  \"seed_D24_different_pixels\":"<<m.seed_depth<<",\n  \"seed_stencil_different_bytes\":"<<m.seed_stencil<<",\n"
      <<"  \"negative_control_differences\":"<<m.negative<<",\n  \"validation_errors\":"<<validation_errors.load()<<",\n  \"cleanup_failures\":"<<cleanup_failures.load()<<",\n"
      <<"  \"capture_texture_fixture_upload\":"<<(m.native_imports?"false":"true")<<",\n"
      <<"  \"native_cache_candidate_upload_prepared\":"<<(m.imports_prepared?"true":"false")<<",\n"
      <<"  \"native_cache_mip_faces\":"<<m.imported_mip_faces<<",\n  \"native_cache_upload_bytes\":"<<m.imported_bytes<<",\n"
      <<"  \"native_cache_sampled_texel_different_bytes\":"<<m.imported_texel_differences<<",\n"
      <<"  \"color_byte_order\":\"BGRA\",\n  \"native_cache_texture_import_verified\":"
      <<(m.native_imports && status=="passed"?"true":"false")<<",\n  \"live_renderer_admission_verified\":false\n}\n";
  r.flush();require(bool(r),"native fragment report write failed");
}
} // namespace

int main(int argc,char **argv) {fs::path output;Measurements m;try {
  require(argc==6 || argc==8,"native fragment raster test needs mainmenu.map,eventfixture,productionGS.spv,productionFS.spv,fresh-output[,--native-cache-textures,bitmap-import-fixture]");
  const bool native_imports=argc==8;
  m.native_imports=native_imports;
  if(native_imports)require(std::string(argv[6])=="--native-cache-textures","unknown native fragment raster mode");
  for(unsigned i=1;i<5;++i)if(!fs::exists(argv[i]))return 77;
  if(native_imports && !fs::exists(argv[7]))return 77;
  const fs::path p=argv[2];require(!fs::exists(argv[5]),"native fragment output already exists");output=argv[5];fs::create_directories(output);auto f=fixture(p);m.event=f.event;m.sampled_stages=f.sampled_stages;m.blend_enabled=f.blend.w[1];
  // Construct actual imports before GPU allocation; candidate upload data is
  // never selected from the original image fixtures in this optional mode.
  if(native_imports) {
    const Cache cache(argv[1]);
    if(f.event==1880)native_cache_fog_textures(cache,argv[7],f,output,m);
    else native_cache_textures(cache,argv[7],f,output,m);
  }
  {
    Gpu gpu;if(!gpu.initialize(f)){report(output,m,"skipped","required native Vulkan feature/format unavailable");return 77;}
    {
      const Cache cache(argv[1]);std::shared_ptr<const MenuWorldSourceSection> section;MenuWorldSourceOwner owner;
      unsigned part{},vertex_datum{};MenuWorldSourceTopology topology;
      if(f.event==1335) {
        section=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
        const auto placed=load_menu_world_source_bsp_owner(cache,0,58);owner={MenuWorldSourceOwnerKind::bsp_instance,placed.bsp_datum,58,{}};
        part=2;topology=MenuWorldSourceTopology::triangle_list;vertex_datum=0xe1cc0057u;
      } else {
        section=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,0xe1d70062u,2));
        owner={MenuWorldSourceOwnerKind::scenery_placement,0xe1780003u,3,{}};
        part=0;topology=MenuWorldSourceTopology::triangle_strip;vertex_datum=f.event==1880?0xe1ba0045u:0xe1dd0068u;
      }
      std::array<float,12> rows;std::memcpy(rows.data(),f.vertex.constants[50].data(),sizeof(rows));
      const auto binding=menu_world_source_binding(cache,owner,
          menu_world_source_draw(section,part,topology,std::span(&rows,1)),vertex_datum,f.vertex.constants);
      MenuWorldSourceStorage storage({gpu.device,gpu.physical});const auto draws=storage.store(std::span(&binding,1));const auto &draw=draws[0];require(draw.index_count==f.ordered_index_count,"original source ordered triangle count differs");
      original_inputs(draw,p,f);
      const auto &entry=menu_world_vertex_catalog_lookup(generated_menu_world_vertex_catalog::entries,draw.source.program_bytes(),draw.source.declaration());
      gpu.prepare(draw,entry,f,argv[3],argv[4]);const auto actual=gpu.run(draw,f.fragment);
      m.normal=true;m.seed_color=differences(actual.initial_color,f.before_color);m.seed_depth=differences(actual.initial_depth,f.before_depth,true);m.seed_stencil=differences(actual.initial_stencil,f.before_stencil);
      m.color=differences(actual.color,f.after_color);m.depth=differences(actual.depth,f.after_depth,true);m.stencil=differences(actual.stencil,f.after_stencil);
      const auto prefix=std::to_string(f.event);
      save(output/(prefix+"-color.bin"),actual.color);save(output/(prefix+"-depth.bin"),actual.depth);save(output/(prefix+"-stencil.bin"),actual.stencil);
      save(output/(prefix+"-seeded-color.bin"),actual.initial_color);save(output/(prefix+"-seeded-depth.bin"),actual.initial_depth);save(output/(prefix+"-seeded-stencil.bin"),actual.initial_stencil);
      require(m.seed_color==0 && m.seed_depth==0 && m.seed_stencil==0,"actual GPU1335 seed differs from original before inputs");
      auto rejected=f.fragment;rejected.clip_range[2]=1;rejected.clip_range[3]=0;const auto control=gpu.run(draw,rejected);
      m.control=true;m.negative=differences(control.color,actual.color)+differences(control.depth,actual.depth,true)+differences(control.stencil,actual.stencil);
      save(output/(prefix+"-rejected-color.bin"),control.color);save(output/(prefix+"-rejected-depth.bin"),control.depth);save(output/(prefix+"-rejected-stencil.bin"),control.stencil);
      require(differences(control.color,f.before_color)==0 && differences(control.depth,f.before_depth,true)==0 && differences(control.stencil,f.before_stencil)==0,"inverted source clip negative control did not preserve original prior attachments");
    }
  }
  require(validation_errors==0,"native fragment emitted validation errors");require(cleanup_failures==0,"native fragment cleanup failed");require(m.negative>0,"native fragment negative control was insensitive");
  require(m.imported_texel_differences==0,"prepared native Cache texels differ from original sampled mip/faces");
  require(m.color==0 && m.depth==0 && m.stencil==0,"native compact raster differs from original capture");report(output,m,"passed");
  std::cout<<"Native compact"<<m.event<<": productionVS/GS/FS, "<<(m.native_imports?"actual Cache/frontend-prepared":"original fixture")
      <<" texture mip/faces,original views/samplers,BGRA/D24/stencil exact; negative control observable; validation0\n";return 0;
}catch(const std::exception &e){try{report(output,m,"failed",e.what());}catch(const std::exception &r){std::cerr<<r.what()<<'\n';}std::cerr<<e.what()<<'\n';return 1;}}
