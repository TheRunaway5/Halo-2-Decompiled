#include "halo2/menu_world_source_storage.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/effect_light.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

// An isolated real graphics-input/descriptor test. Retained source inputs and
// expected original GS banks are fixtures, never live renderer constants.
using namespace halo2;
namespace {
using Bytes=std::vector<std::byte>;
namespace fs=std::filesystem;
void require(bool value,const std::string &message){if(!value)throw FormatError(message);}
void check(VkResult result){require(result==VK_SUCCESS,"catalog GPU Vulkan error "+std::to_string(result));}
Bytes read(const fs::path &path,std::size_t expected=0) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"missing catalog GPU fixture "+path.string());
  const auto size=file.tellg();require(size>=0 && size<64*1024*1024,"invalid catalog GPU fixture extent");
  Bytes result(static_cast<std::size_t>(size));require(!expected || result.size()==expected,"catalog GPU fixture length differs");
  file.seekg(0);require(bool(file.read(reinterpret_cast<char *>(result.data()),size)),"short catalog GPU fixture");return result;
}
void save(const fs::path &path,const Bytes &bytes) {
  std::ofstream file(path,std::ios::binary);require(bool(file.write(reinterpret_cast<const char *>(bytes.data()),bytes.size())),"cannot retain catalog GPU output");
}
std::uint32_t word(const Bytes &bytes,std::size_t offset) {
  require(offset+4<=bytes.size(),"catalog GPU word escapes fixture");std::uint32_t value;
  std::memcpy(&value,bytes.data()+offset,4);return value;
}
std::atomic<unsigned> validation_errors{};
VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void *) {
  if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<data->pMessage<<'\n';}return VK_FALSE;
}
class Gpu {
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};VkDeviceSize size{};};
  VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkQueue queue{};
  VkCommandPool command_pool{};VkCommandBuffer command{};VkDescriptorPool descriptors{};
  std::array<VkDescriptorSetLayout,3> set_layouts{};VkPipelineLayout pipeline_layout{};
  VkRenderPass render_pass{};VkFramebuffer framebuffer{};VkShaderModule capture_shader{};
  std::vector<Buffer> buffers;std::vector<VkPipeline> pipelines;std::vector<VkShaderModule> modules;
  std::map<const MenuWorldVertexCatalogEntry *,VkShaderModule> vertex_modules;
  Buffer buffer(VkDeviceSize size,VkBufferUsageFlags usage) {
    require(size>0,"catalog GPU buffer is empty");Buffer result;result.size=size;
    VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};create.size=size;create.usage=usage;
    check(vkCreateBuffer(device,&create,nullptr,&result.handle));buffers.push_back(result);auto &owned=buffers.back();
    VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device,result.handle,&requirements);
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
    unsigned type=properties.memoryTypeCount;
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((requirements.memoryTypeBits&(1u<<i)) &&
        (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
    require(type<properties.memoryTypeCount,"catalog GPU coherent memory is unavailable");
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
    check(vkAllocateMemory(device,&allocation,nullptr,&owned.memory));check(vkBindBufferMemory(device,owned.handle,owned.memory,0));
    check(vkMapMemory(device,owned.memory,0,size,0,&owned.mapped));return owned;
  }
  VkShaderModule module(std::span<const std::uint32_t> words) {
    require(words.size()>=5 && words[0]==0x07230203u,"catalog GPU module is invalid");
    VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};create.codeSize=words.size_bytes();create.pCode=words.data();
    VkShaderModule result;check(vkCreateShaderModule(device,&create,nullptr,&result));modules.push_back(result);return result;
  }
  VkDescriptorSet descriptor(unsigned set,const Buffer &buffer,VkDescriptorType type) {
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=descriptors;
    allocate.descriptorSetCount=1;allocate.pSetLayouts=&set_layouts[set];VkDescriptorSet result;
    check(vkAllocateDescriptorSets(device,&allocate,&result));VkDescriptorBufferInfo info{buffer.handle,0,buffer.size};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=result;write.dstBinding=0;
    write.descriptorCount=1;write.descriptorType=type;write.pBufferInfo=&info;vkUpdateDescriptorSets(device,1,&write,0,nullptr);return result;
  }
  VkPipeline pipeline(const MenuWorldSourceStoredDraw &draw,const MenuWorldVertexCatalogEntry &entry) {
    auto [found,inserted]=vertex_modules.try_emplace(&entry,VK_NULL_HANDLE);
    if(inserted)found->second=module(entry.spirv);
    VkPipelineShaderStageCreateInfo stages[2]{};
    for(auto &stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=found->second;
    stages[1].stage=VK_SHADER_STAGE_GEOMETRY_BIT;stages[1].module=capture_shader;
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    input.vertexBindingDescriptionCount=draw.vertex_bindings.size();input.pVertexBindingDescriptions=draw.vertex_bindings.data();
    input.vertexAttributeDescriptionCount=draw.vertex_attributes.size();input.pVertexAttributeDescriptions=draw.vertex_attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=draw.topology;
    const VkViewport viewport{0,0,640,480,0,1};const VkRect2D scissor{{0,0},{640,480}};
    VkPipelineViewportStateCreateInfo view{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};view.viewportCount=view.scissorCount=1;view.pViewports=&viewport;view.pScissors=&scissor;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.rasterizerDiscardEnable=VK_TRUE;raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};create.stageCount=2;create.pStages=stages;
    create.pVertexInputState=&input;create.pInputAssemblyState=&assembly;create.pViewportState=&view;
    create.pRasterizationState=&raster;create.pMultisampleState=&samples;create.layout=pipeline_layout;create.renderPass=render_pass;
    VkPipeline result;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&create,nullptr,&result));pipelines.push_back(result);return result;
  }
public:
  VkPhysicalDevice physical{};VkDevice device{};
  ~Gpu() {
    if(device) {
      vkDeviceWaitIdle(device);for(auto pipeline:pipelines)vkDestroyPipeline(device,pipeline,nullptr);
      for(auto shader:modules)vkDestroyShaderModule(device,shader,nullptr);
      if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);if(render_pass)vkDestroyRenderPass(device,render_pass,nullptr);
      if(pipeline_layout)vkDestroyPipelineLayout(device,pipeline_layout,nullptr);if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
      for(auto layout:set_layouts)if(layout)vkDestroyDescriptorSetLayout(device,layout,nullptr);
      for(auto buffer:buffers){if(buffer.mapped)vkUnmapMemory(device,buffer.memory);vkDestroyBuffer(device,buffer.handle,nullptr);if(buffer.memory)vkFreeMemory(device,buffer.memory,nullptr);}
      if(command_pool)vkDestroyCommandPool(device,command_pool,nullptr);vkDestroyDevice(device,nullptr);
    }
    if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
    if(instance)vkDestroyInstance(instance,nullptr);
  }
  bool initialize(const fs::path &shader) {
    unsigned count{};check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);
    check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
    if(std::none_of(layers.begin(),layers.end(),[](const auto &layer){return std::string(layer.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
    const char *layer="VK_LAYER_KHRONOS_validation";
    const char *extensions[]{VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback=diagnostic;
    VkValidationFeatureEnableEXT synchronization=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validation{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};validation.enabledValidationFeatureCount=1;
    validation.pEnabledValidationFeatures=&synchronization;validation.pNext=&debug;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;create.enabledLayerCount=1;
    create.ppEnabledLayerNames=&layer;create.enabledExtensionCount=2;create.ppEnabledExtensionNames=extensions;create.pNext=&validation;
    check(vkCreateInstance(&create,nullptr,&instance));
    check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
    check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);
    check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));unsigned family{};
    for(auto candidate:devices) {
      VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(candidate,&properties);
      VkPhysicalDeviceFeatures features;vkGetPhysicalDeviceFeatures(candidate,&features);
      if(properties.apiVersion<VK_API_VERSION_1_3 || !features.geometryShader ||
          !features.shaderTessellationAndGeometryPointSize || !features.vertexPipelineStoresAndAtomics)continue;
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
      for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=i;break;}
      if(physical)break;
    }
    if(!physical)return false;
    const float priority=1;VkDeviceQueueCreateInfo queue_create{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_create.queueFamilyIndex=family;queue_create.queueCount=1;queue_create.pQueuePriorities=&priority;
    VkPhysicalDeviceFeatures features{};features.geometryShader=features.shaderTessellationAndGeometryPointSize=features.vertexPipelineStoresAndAtomics=VK_TRUE;
    VkDeviceCreateInfo device_create{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};device_create.queueCreateInfoCount=1;
    device_create.pQueueCreateInfos=&queue_create;device_create.pEnabledFeatures=&features;
    check(vkCreateDevice(physical,&device_create,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;pool.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device,&pool,nullptr,&command_pool));VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate.commandPool=command_pool;allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;allocate.commandBufferCount=1;
    check(vkAllocateCommandBuffers(device,&allocate,&command));
    const std::array<VkDescriptorPoolSize,2> sizes{{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,256},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,256}}};
    VkDescriptorPoolCreateInfo descriptor_pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};descriptor_pool.maxSets=512;
    descriptor_pool.poolSizeCount=sizes.size();descriptor_pool.pPoolSizes=sizes.data();check(vkCreateDescriptorPool(device,&descriptor_pool,nullptr,&descriptors));
    for(unsigned i=0;i<3;++i) {
      VkDescriptorSetLayoutBinding binding{0,i==1?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,
          i==1?VK_SHADER_STAGE_VERTEX_BIT:VK_SHADER_STAGE_GEOMETRY_BIT,nullptr};
      VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
      if(i){set.bindingCount=1;set.pBindings=&binding;}check(vkCreateDescriptorSetLayout(device,&set,nullptr,&set_layouts[i]));
    }
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=set_layouts.size();layout.pSetLayouts=set_layouts.data();
    check(vkCreatePipelineLayout(device,&layout,nullptr,&pipeline_layout));
    VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.subpassCount=1;pass.pSubpasses=&subpass;
    check(vkCreateRenderPass(device,&pass,nullptr,&render_pass));VkFramebufferCreateInfo frame{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    frame.renderPass=render_pass;frame.width=640;frame.height=480;frame.layers=1;check(vkCreateFramebuffer(device,&frame,nullptr,&framebuffer));
    const auto raw=read(shader);require(raw.size()%4==0,"capture GS module extent is invalid");
    std::vector<std::uint32_t> words(raw.size()/4);std::memcpy(words.data(),raw.data(),raw.size());capture_shader=module(words);
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);
    std::cout<<"Compact raw vertex catalog device: "<<properties.deviceName<<'\n';return true;
  }
  Bytes run(const MenuWorldSourceStoredDraw &draw,const MenuWorldVertexCatalogEntry &entry,
      const MenuWorldVertexUniforms &values) {
    require(draw.index_count && draw.index_count%3==0,"catalog GPU requires original ordered triangle list");
    const auto uniform=buffer(sizeof(values),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    const auto output=buffer(VkDeviceSize(draw.index_count)*204,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    std::memcpy(uniform.mapped,&values,sizeof(values));std::memset(output.mapped,0xcd,output.size);
    const std::array sets{descriptor(1,uniform,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),descriptor(2,output,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)};
    const auto selected=pipeline(draw,entry);check(vkResetCommandBuffer(command,0));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;
    host.dstAccessMask=VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_INDEX_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
        0,1,&host,0,nullptr,0,nullptr);
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderPass=render_pass;pass.framebuffer=framebuffer;pass.renderArea={{0,0},{640,480}};
    vkCmdBeginRenderPass(command,&pass,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,selected);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,1,sets.size(),sets.data(),0,nullptr);
    vkCmdBindVertexBuffers(command,0,draw.vertex_buffers.size(),draw.vertex_buffers.data(),draw.vertex_offsets.data());
    vkCmdBindIndexBuffer(command,draw.indices.buffer,draw.indices.offset,draw.index_type);vkCmdDrawIndexed(command,draw.index_count,1,0,0,0);
    vkCmdEndRenderPass(command);VkMemoryBarrier readback{VK_STRUCTURE_TYPE_MEMORY_BARRIER};readback.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;readback.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&readback,0,nullptr,0,nullptr);
    check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));Bytes result(output.size);
    std::memcpy(result.data(),output.mapped,result.size());return result;
  }
};
MenuWorldSourceBinding binding(const Cache &cache,const fs::path &fixtures,unsigned event,
    std::shared_ptr<const MenuWorldSourceSection> section,unsigned part,MenuWorldSourceTopology topology,
    MenuWorldSourceOwner owner,std::uint32_t vertex) {
  const auto raw=read(fixtures/("event-"+std::to_string(event)+"-constants192.bin"),sizeof(MenuWorldSourceConstants));
  MenuWorldSourceConstants bank;std::memcpy(bank.data(),raw.data(),raw.size());std::array<float,12> rows;
  std::memcpy(rows.data(),bank[50].data(),sizeof(rows));
  return menu_world_source_binding(cache,owner,menu_world_source_draw(section,part,topology,std::span(&rows,1)),vertex,bank);
}
void original_inputs(const MenuWorldSourceStoredDraw &draw,const fs::path &fixtures,unsigned event) {
  const auto prefix="event-"+std::to_string(event);
  for(unsigned i=0;i<draw.source.inputs().size();++i) {
    const auto &input=draw.source.inputs()[i];const auto &stream=input.stream;
    const auto raw=read(fixtures/(prefix+"-vertex-"+std::to_string(input.vertex_register)+".bin"));
    const auto bytes=draw.source.resource_bytes(i);
    require(stream.byte_offset<=bytes.size() && raw.size()<=bytes.size()-stream.byte_offset &&
        std::equal(raw.begin(),raw.end(),bytes.begin()+stream.byte_offset),"catalog GPU input differs from original retained raw stream");
    const auto attribute=std::find_if(draw.vertex_attributes.begin(),draw.vertex_attributes.end(),[&](const auto &a){return a.location==input.vertex_register;});
    require(attribute!=draw.vertex_attributes.end() && attribute->binding<draw.vertex_bindings.size(),"catalog GPU lost a source input location");
    const auto format=stream.format==MenuWorldSourceFormat::float32x3?VK_FORMAT_R32G32B32_SFLOAT:
        stream.format==MenuWorldSourceFormat::float32x2?VK_FORMAT_R32G32_SFLOAT:
        stream.format==MenuWorldSourceFormat::snorm16x3?VK_FORMAT_R16G16B16_SNORM:
        stream.format==MenuWorldSourceFormat::snorm16x2?VK_FORMAT_R16G16_SNORM:VK_FORMAT_R32_SINT;
    require(attribute->format==format && attribute->offset==stream.byte_offset &&
        draw.vertex_bindings[attribute->binding].stride==stream.byte_stride,
        "catalog GPU changed original raw fetch format/offset/stride");
  }
  const auto indices=read(fixtures/(prefix+"-indices.bin"));require(indices.size()%4==0,"catalog captured index bank is invalid");
  const auto source=draw.source.draw().indices();require(source.size()==indices.size()/4,"catalog source part index count differs");
  for(std::size_t i=0;i<source.size();++i)require(source[i]==word(indices,i*4),"catalog source part changed original index order");
  require(draw.topology==VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST && draw.index_type==VK_INDEX_TYPE_UINT16 &&
      draw.index_count==draw.source.draw().triangle_count()*3,"catalog did not retain every source ordered triangle");
}
std::size_t compare(const Bytes &actual,const Bytes &expected,unsigned event,bool control=false) {
  require(actual.size()==expected.size(),"catalog GS output size differs at event "+std::to_string(event));
  std::size_t count=0,first=actual.size();
  for(std::size_t i=0;i<actual.size();i+=4)if(word(actual,i)!=word(expected,i)){if(first==actual.size())first=i;++count;}
  if(count)std::cerr<<(control?"Expected uniform control: ":"Unexpected catalog output: ")
      <<"event"<<event<<" first GS difference record"<<first/204<<" offset"<<first%204
      <<" expected0x"<<std::hex<<word(expected,first)<<" actual0x"<<word(actual,first)<<std::dec<<"; differing words "<<count<<'\n';
  return count;
}
int sphere(int argc,char** argv) {
  require(argc==7,"sphere catalog GPU needs --sphere Lockout.map bindings references capture-GS output");
  const fs::path fixtures=argv[3],references=argv[4],output=argv[6];
  require(!fs::exists(output),"sphere catalog GPU output already exists");fs::create_directories(output);
  struct Reference {std::vector<std::uint32_t> words;MenuWorldVertexCatalogEntry entry;};
  struct Input {unsigned command;std::string key;MenuWorldSourceBinding binding;};
  std::size_t words=0,records=0,differences=0,controls=0;unsigned views=0;
  std::ofstream report(output/"measurements.tsv");report<<"width\theight\tview\tcommand\tclass\trecords\tdiffering_words\n";
  {
    Gpu gpu;if(!gpu.initialize(argv[5]))return 77;const Cache cache(argv[2]);
    std::map<std::string,Reference> shaders;std::vector<Input> inputs;
    std::ifstream manifest(fixtures/"manifest.tsv");std::string line;
    require(bool(std::getline(manifest,line)) && line=="H2_SPHERE_RAW_BINDINGS_V1","sphere GPU fixture schema");
    while(std::getline(manifest,line)) {
      unsigned command,kind,section,instance,part,shader,lod,implementation,vertex,classification;std::string key,extra;std::istringstream row(line);
      require(bool(row>>command>>kind>>section>>instance>>part>>shader>>lod>>implementation>>vertex>>classification>>key) && !(row>>extra),"sphere GPU fixture row");
      const auto geometry=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,section));
      MenuWorldSourceConstants bank{};for(unsigned i=0;i<192;++i)bank[i]={1,1,0,0};
      MenuWorldSourceOwner owner;MenuWorldSourceDraw source;
      if(kind) {const auto actual=load_menu_world_source_bsp_owner(cache,0,instance);owner={MenuWorldSourceOwnerKind::bsp_instance,actual.bsp_datum,instance,{}};source=menu_world_source_draw(geometry,part,MenuWorldSourceTopology::triangle_list,std::span(&actual.rows,1));}
      else {owner={MenuWorldSourceOwnerKind::bsp_cluster,geometry->owner_datum(),section,{}};source=menu_world_source_cluster_draw(geometry,part,MenuWorldSourceTopology::triangle_list);}
      auto bound=menu_world_source_binding_class(cache,owner,source,vertex,classification,bank);
      const auto& entry=menu_world_vertex_catalog_lookup(generated_menu_world_vertex_catalog::entries,bound.program_bytes(),bound.declaration());
      if(!shaders.contains(key)) {const auto raw=read(references/(key+".spv"));require(raw.size()%4==0,"sphere reference SPIR-V extent");auto& ref=shaders[key];ref.words.resize(raw.size()/4);std::memcpy(ref.words.data(),raw.data(),raw.size());ref.entry=entry;ref.entry.spirv=ref.words;}
      inputs.push_back({command,key,std::move(bound)});
    }
    require(inputs.size()==13 && shaders.size()==3,"sphere GPU actual draw/program scope differs");
    MenuWorldSourceStorage storage({gpu.device,gpu.physical});std::vector<MenuWorldSourceBinding> bindings;for(const auto& in:inputs)bindings.push_back(in.binding);const auto draws=storage.store(bindings);
    for(const auto layout:std::array<std::array<unsigned,3>,4>{{{640,480,1},{1280,720,1},{1920,1080,1},{1280,720,4}}})for(unsigned view=0;view<layout[2];++view) {
      const auto width=layout[0],height=layout[1];const auto count=layout[2];++views;
      const RenderViewport viewport{count==4?float(view%2*width/2):0,count==4?float(view/2*height/2):0,float(count==4?width/2:width),float(count==4?height/2:height)};
      SourceFrustumProjectionInput projection;projection.projection_angle=1.2217305f;projection.outer=projection.safe={std::int16_t(viewport.top),std::int16_t(viewport.left),std::int16_t(viewport.top+viewport.height),std::int16_t(viewport.left+viewport.width)};projection.scenario_present=true;projection.scenario_type=1;
      const auto frustum=source_frustum_projection(projection);const auto camera=source_view_projection({{0,0,3},{1,0,0},{0,0,1},.015625f,1024.f,{},frustum.projection_program});const auto view_bank=source_view_shader_bank(camera,1024.f,{0,0,3},viewport,false);
      EffectLightUniformInput light;light.position={-2,-1,3};light.forward={1,0,0};light.up={0,0,1};light.left={0,-1,0};light.radius=2;light.diffuse={1,.5f,.25f};light.specular={.25f,.125f,.0625f};light.brightness=1;
      const auto light_bank=source_effect_light_uniforms(light);
      for(unsigned i=0;i<draws.size();++i) {
        const auto& draw=draws[i];const auto& input=inputs[i];const auto& entry=menu_world_vertex_catalog_lookup(generated_menu_world_vertex_catalog::entries,draw.source.program_bytes(),draw.source.declaration());
        MenuWorldVertexUniforms values;values.constants=draw.source.constants();std::copy(view_bank.begin(),view_bank.end(),values.constants.begin());std::copy(light_bank.constants.begin(),light_bank.constants.end(),values.constants.begin()+31);values.clip_range={.015625f,1024.f,.015625f,1024.f};values.fog_parameters={1,1};values.surface_size={float(width),float(height)};
        const auto original=gpu.run(draw,shaders.at(input.key).entry,values),actual=gpu.run(draw,entry,values);const auto n=compare(actual,original,input.command);differences+=n;records+=actual.size()/204;words+=actual.size()/4;
        const auto name=std::to_string(width)+"x"+std::to_string(height)+"-view"+std::to_string(view)+"-command"+std::to_string(input.command);save(output/(name+"-native.bin"),actual);save(output/(name+"-original-emitter.bin"),original);
        report<<width<<'\t'<<height<<'\t'<<view<<'\t'<<input.command<<'\t'<<draw.source.declaration().class_index<<'\t'<<actual.size()/204<<'\t'<<n<<'\n';
        if(i==0){values.constants[35][3]+=.125f;const auto wrong=gpu.run(draw,entry,values);require(compare(wrong,original,input.command,true)>0,"sphere c35 binding negative control insensitive");++controls;}
      }
    }
  }
  report.flush();require(bool(report),"sphere GPU report write failed");require(views==7 && controls==7 && words>0 && validation_errors==0 && differences==0,"sphere original-emitter GPU comparison failed");
  std::cout<<"PASS sphere raw catalogue / 13 sorted source parts / 3 original-emitter programs / "<<views<<" resolution/split views / "<<records<<" GS records / "<<words<<" exact words / "<<controls<<" c35 controls / validation0; isolated shader proof, no original gameplay framebuffer\n";return 0;
}
}
int main(int argc,char **argv) {try {
  if(argc>1 && std::string_view(argv[1])=="--sphere")return sphere(argc,argv);
  require(argc==6,"catalog GPU test needs mainmenu.map, Stage3 fixtures, attachment fixtures, capture GS.spv, fresh output directory");
  for(unsigned i=1;i<5;++i)if(!fs::exists(argv[i]))return 77;
  const fs::path fixtures=argv[2],attachments=argv[3],output=argv[5];
  require(!fs::exists(output),"catalog GPU output directory already exists");fs::create_directories(output);
  std::size_t total_records=0,total_words=0,total_differences=0,controls=0;
  std::ofstream report(output/"measurements.tsv");require(bool(report),"cannot write catalog GPU report");
  report<<"event\trecords\twords\tdiffering_words\twrong_surface_differing_words\n";
  {
    Gpu gpu;if(!gpu.initialize(argv[4]))return 77;
    {
    const Cache cache(argv[1]);
    const auto bsp52=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
    const auto bsp0=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,0));
    const auto carrier=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,0xe1d70062u,2));
    const auto owner58=load_menu_world_source_bsp_owner(cache,0,58),owner0=load_menu_world_source_bsp_owner(cache,0,0);
    const MenuWorldSourceOwner carrier_owner{MenuWorldSourceOwnerKind::scenery_placement,0xe1780003u,3,{}};
    std::vector<MenuWorldSourceBinding> bindings;
    bindings.push_back(binding(cache,fixtures,1335,bsp52,2,MenuWorldSourceTopology::triangle_list,{MenuWorldSourceOwnerKind::bsp_instance,owner58.bsp_datum,58,{}},0xe1cc0057u));
    bindings.push_back(binding(cache,fixtures,1448,carrier,0,MenuWorldSourceTopology::triangle_strip,carrier_owner,0xe1dd0068u));
    bindings.push_back(binding(cache,fixtures,1880,carrier,0,MenuWorldSourceTopology::triangle_strip,carrier_owner,0xe1ba0045u));
    bindings.push_back(binding(cache,fixtures,2409,bsp0,0,MenuWorldSourceTopology::triangle_list,{MenuWorldSourceOwnerKind::bsp_instance,owner0.bsp_datum,0,{}},0xe1ba0045u));
    MenuWorldSourceStorage storage({gpu.device,gpu.physical});const auto draws=storage.store(bindings);
    const std::array events{1335u,1448u,1880u,2409u};
    for(unsigned i=0;i<events.size();++i) {
      const auto event=events[i];const auto &draw=draws[i];
      original_inputs(draw,fixtures,event);
      const auto raw=read(fixtures/("event-"+std::to_string(event)+"-uniforms.bin"),5344);
      MenuWorldVertexUniforms values;std::memcpy(&values,raw.data(),3096);
      std::memcpy(values.surface_size.data(),raw.data()+5336,8);
      require(std::memcmp(values.constants.data(),draw.source.constants().data(),sizeof(values.constants))==0,
          "catalog fixture binding changed retained c192 words");
      require(values.surface_size==std::array<float,2>{640,480},"catalog original surface extent differs");
      const auto &entry=menu_world_vertex_catalog_lookup(generated_menu_world_vertex_catalog::entries,
          draw.source.program_bytes(),draw.source.declaration());
      const auto expected=read(attachments/(std::to_string(event)+"-records.bin"),draw.index_count*204u);
      const auto actual=gpu.run(draw,entry,values);save(output/(std::to_string(event)+"-catalog-records.bin"),actual);
      const auto differences=compare(actual,expected,event);total_differences+=differences;
      // Actual set1 buffer changes, with unchanged embedded module and raw
      // inputs. Surface location3096 must affect raster position, not c192.
      values.surface_size[0]+=1;
      const auto wrong=gpu.run(draw,entry,values);save(output/(std::to_string(event)+"-wrong-surface-records.bin"),wrong);
      const auto wrong_differences=compare(wrong,expected,event,true);
      require(wrong_differences>0,"catalog descriptor surface-size negative control was insensitive");++controls;
      const auto records=actual.size()/204,words=actual.size()/4;total_records+=records;total_words+=words;
      report<<event<<'\t'<<records<<'\t'<<words<<'\t'<<differences<<'\t'<<wrong_differences<<'\n';
    }
    } // Raw storage completes/destroys before its VkDevice.
  } // Device/debug messenger cleanup also participates in validation count.
  report.flush();require(bool(report),"catalog GPU measurement write failed");
  require(validation_errors==0,"catalog GPU emitted Vulkan validation errors");
  require(total_records==27945 && total_words==1425195 && controls==4,"catalog GPU original input scope is incomplete");
  require(total_differences==0,"compact catalog ABI did not match original GS records exactly");
  std::cout<<"Compact embedded catalog: "<<total_records<<" original GS records / "<<total_words
      <<" words exact; 4 wrong-surface controls; validation0; no fragment/depth-image or live-renderer admission\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
