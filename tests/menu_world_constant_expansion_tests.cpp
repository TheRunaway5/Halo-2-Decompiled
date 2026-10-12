#include "halo2/world_material_uniform.hpp"
#include "halo2/menu_world_fragment.hpp"
#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Real production Material-prefix descriptor/argb compute gate. Original
// expected words are unchanged captured FS inputs, never native expectations.
namespace {
namespace fs=std::filesystem;
using Bytes=std::vector<std::byte>;
using Words=std::array<std::uint32_t,72>;
std::atomic<unsigned> validation_errors{},cleanup_errors{};
void require(bool yes,const std::string &message){if(!yes)throw std::runtime_error(message);}
void check(VkResult result){require(result==VK_SUCCESS,"constant GPU Vulkan error "+std::to_string(result));}
VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void *) {
  if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){++validation_errors;std::cerr<<data->pMessage<<'\n';}
  return VK_FALSE;
}
Bytes read(const fs::path &path,std::size_t expected=0) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file),"missing constant GPU input "+path.string());
  const auto extent=file.tellg();require(extent>0 && extent<1024*1024,"invalid constant GPU input length");
  Bytes result(static_cast<std::size_t>(extent));require(!expected || result.size()==expected,"constant input extent changed");
  file.seekg(0);require(bool(file.read(reinterpret_cast<char *>(result.data()),extent)),"truncated constant GPU input");
  return result;
}
void save(const fs::path &path,const void *data,std::size_t bytes) {
  std::ofstream file(path,std::ios::binary);require(bool(file.write(static_cast<const char *>(data),bytes)),"cannot retain constant GPU output");
}
unsigned number(const std::string &text) {
  std::size_t end{};const auto value=std::stoul(text,&end);
  require(end==text.size() && value<=1000000,"invalid constant fixture number");return unsigned(value);
}
fs::path child(const fs::path &base,const std::string &name) {
  const fs::path path(name);require(path.has_filename() && !path.has_parent_path(),"constant fixture path escapes manifest");return base/path;
}
Words words(const Bytes &bytes){require(bytes.size()==sizeof(Words),"expected original18vec4 words");Words result;std::memcpy(result.data(),bytes.data(),bytes.size());return result;}
unsigned differences(const Words &a,const Words &b){unsigned result=0;for(unsigned i=0;i<a.size();++i)result+=a[i]!=b[i];return result;}
struct Row {
  unsigned event{},expected_control_differences{};std::string family;
  halo2::WorldMaterialUniform material;halo2::MenuWorldFragmentUniforms fragment;
  Words expected{},control{};
};
std::vector<Row> fixtures(const fs::path &manifest) {
  std::ifstream input(manifest);std::string line;require(bool(std::getline(input,line)) && line=="H2_MENU_WORLD_CONSTANT_EXPANSION_V2","invalid constant fixture manifest");
  std::vector<Row> result;std::set<unsigned> events;unsigned controls{};
  while(std::getline(input,line)) {
    if(line.empty())continue;
    std::istringstream fields(line);std::vector<std::string> values;std::string value;
    while(std::getline(fields,value,'\t'))values.push_back(value);
    require(values.size()==7,"invalid constant fixture columns");Row row;
    row.event=number(values[0]);row.family=values[1];row.expected_control_differences=number(values[6]);
    require(events.insert(row.event).second,"duplicate original constant event");
    const auto packed=read(child(manifest.parent_path(),values[2]),240);
    std::memcpy(row.material.program.data(),packed.data(),packed.size());
    row.expected=words(read(child(manifest.parent_path(),values[3]),288));
    row.control=words(read(child(manifest.parent_path(),values[4]),288));
    const auto original_packet=read(child(manifest.parent_path(),values[5]),448);
    // The candidate uses the real CPU binder and completed program. Original
    // float words are ONLY the independent expectation, never the candidate.
    std::memcpy(&row.fragment,original_packet.data(),160);
    halo2::PixelProgram completed;completed.words=row.material.program;
    halo2::bind_menu_world_fragment_completed_program(row.fragment,completed);
    require(std::memcmp(&row.fragment,original_packet.data(),448)==0,"completed CPU float packet differs from independent original448 bytes");
    require(differences(row.expected,row.control)==row.expected_control_differences,"independent reciprocal control count changed");
    controls+=row.expected_control_differences;result.push_back(std::move(row));
  }
  require(result.size()==8 && controls==13,"original constant fixture coverage changed");return result;
}
class Gpu {
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};VkDeviceSize size{};};
  VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};
  VkDevice device{};VkQueue queue{};VkCommandPool command_pool{};VkCommandBuffer command{};
  std::array<VkDescriptorSetLayout,2> set_layouts{};std::array<VkDescriptorSet,2> sets{};
  VkDescriptorPool descriptors{};VkPipelineLayout pipeline_layout{};VkPipeline pipeline{};VkShaderModule shader{};
  Buffer material{},output{},fragment{};
  void allocate(Buffer &buffer,VkDeviceSize size,VkBufferUsageFlags usage) {
    buffer.size=size;VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};create.size=size;create.usage=usage;
    check(vkCreateBuffer(device,&create,nullptr,&buffer.handle));VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device,buffer.handle,&requirements);VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(physical,&properties);unsigned type=properties.memoryTypeCount;
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((requirements.memoryTypeBits&(1u<<i)) &&
        (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
    require(type<properties.memoryTypeCount,"constant GPU requires coherent host memory");
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
    check(vkAllocateMemory(device,&allocation,nullptr,&buffer.memory));check(vkBindBufferMemory(device,buffer.handle,buffer.memory,0));
    check(vkMapMemory(device,buffer.memory,0,size,0,&buffer.mapped));
  }
public:
  std::string device_name;
  ~Gpu() {
    if(device) {
      if(vkDeviceWaitIdle(device)!=VK_SUCCESS){++cleanup_errors;std::cerr<<"constant GPU device wait failed during cleanup\n";}
      if(command_pool)vkDestroyCommandPool(device,command_pool,nullptr);
      if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
      if(shader)vkDestroyShaderModule(device,shader,nullptr);
      if(pipeline_layout)vkDestroyPipelineLayout(device,pipeline_layout,nullptr);
      if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
      for(auto layout:set_layouts)if(layout)vkDestroyDescriptorSetLayout(device,layout,nullptr);
      for(auto *buffer:{&material,&output,&fragment}) {
        if(buffer->mapped)vkUnmapMemory(device,buffer->memory);
        if(buffer->handle)vkDestroyBuffer(device,buffer->handle,nullptr);
        if(buffer->memory)vkFreeMemory(device,buffer->memory,nullptr);
      }
      vkDestroyDevice(device,nullptr);
    }
    if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
    if(instance)vkDestroyInstance(instance,nullptr);
  }
  bool initialize(const fs::path &shader_path) {
    unsigned count{};check(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> layers(count);
    check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
    if(std::none_of(layers.begin(),layers.end(),[](const auto &layer){return std::string(layer.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
    const char *layer="VK_LAYER_KHRONOS_validation";
    const char *extensions[]{VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_3;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;debug.pfnUserCallback=diagnostic;
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
      if(properties.apiVersion<VK_API_VERSION_1_3 || properties.limits.maxUniformBufferRange<sizeof(halo2::WorldMaterialUniform))continue;
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
      for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_COMPUTE_BIT){physical=candidate;family=i;device_name=properties.deviceName;break;}
      if(physical)break;
    }
    if(!physical)return false;
    const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
    VkDeviceCreateInfo d{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};d.queueCreateInfoCount=1;d.pQueueCreateInfos=&q;
    check(vkCreateDevice(physical,&d,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    allocate(material,sizeof(halo2::WorldMaterialUniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    allocate(fragment,sizeof(halo2::MenuWorldFragmentUniforms),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    allocate(output,864,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    for(unsigned set=0;set<2;++set) {
      const VkDescriptorSetLayoutBinding bindings[]{
        {set?0u:4u,set?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},
        {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
      VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};info.bindingCount=set?2:1;info.pBindings=bindings;
      check(vkCreateDescriptorSetLayout(device,&info,nullptr,&set_layouts[set]));
    }
    const std::array<VkDescriptorPoolSize,2> sizes{{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}}};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2;pool.poolSizeCount=sizes.size();pool.pPoolSizes=sizes.data();
    check(vkCreateDescriptorPool(device,&pool,nullptr,&descriptors));
    VkDescriptorSetAllocateInfo sets_allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};sets_allocate.descriptorPool=descriptors;
    sets_allocate.descriptorSetCount=2;sets_allocate.pSetLayouts=set_layouts.data();check(vkAllocateDescriptorSets(device,&sets_allocate,sets.data()));
    const std::array<VkDescriptorBufferInfo,3> buffers{{{material.handle,0,sizeof(halo2::WorldMaterialUniform)},
      {output.handle,0,864},{fragment.handle,0,sizeof(halo2::MenuWorldFragmentUniforms)}}};
    VkWriteDescriptorSet writes[3]{};
    for(unsigned i=0;i<3;++i) {
      writes[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};writes[i].dstSet=sets[i?1:0];writes[i].dstBinding=i?(i-1):4;
      writes[i].descriptorCount=1;writes[i].descriptorType=i==1?VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[i].pBufferInfo=&buffers[i];
    }
    vkUpdateDescriptorSets(device,3,writes,0,nullptr);
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=2;layout.pSetLayouts=set_layouts.data();
    check(vkCreatePipelineLayout(device,&layout,nullptr,&pipeline_layout));const auto bytes=read(shader_path);
    require(bytes.size()>=20 && bytes.size()%4==0,"invalid constant GPU SPIR-V length");
    std::vector<std::uint32_t> code(bytes.size()/4);std::memcpy(code.data(),bytes.data(),bytes.size());require(code[0]==0x07230203,"invalid constant GPU SPIR-V magic");
    VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};module.codeSize=bytes.size();module.pCode=code.data();
    check(vkCreateShaderModule(device,&module,nullptr,&shader));
    VkComputePipelineCreateInfo p{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};p.layout=pipeline_layout;
    p.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};
    check(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&pipeline));
    VkCommandPoolCreateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};command_info.queueFamilyIndex=family;command_info.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device,&command_info,nullptr,&command_pool));
    VkCommandBufferAllocateInfo command_allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};command_allocate.commandPool=command_pool;
    command_allocate.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;command_allocate.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&command_allocate,&command));
    return true;
  }
  std::array<Words,3> run(const halo2::WorldMaterialUniform &values,const halo2::MenuWorldFragmentUniforms &packet) {
    std::memcpy(material.mapped,&values,sizeof(values));std::memcpy(fragment.mapped,&packet,sizeof(packet));
    std::memset(output.mapped,0xcd,864);
    check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;
    host.dstAccessMask=VK_ACCESS_UNIFORM_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline_layout,0,sets.size(),sets.data(),0,nullptr);
    vkCmdDispatch(command,1,1,1);VkMemoryBarrier result{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    result.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;result.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&result,0,nullptr,0,nullptr);
    check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));
    std::array<Words,3> data;static_assert(sizeof(data)==864);std::memcpy(data.data(),output.mapped,sizeof(data));return data;
  }
};
struct Measurement {unsigned event{},production_differences{},control_differences{},control_expected_differences{},float_differences{};};
}
int main(int argc,char **argv) {
  std::vector<Measurement> measurements;std::string failure,device;unsigned actual_words{},actual_control_differences{},old_unpack_differences{};
  bool output_created=false,available=false;
  try {
    require(argc==4,"expected constant fixture manifest, compiled compute SPIR-V and fresh output directory");
    if(!fs::exists(argv[1]) || !fs::exists(argv[2]))return 77;
    const auto rows=fixtures(argv[1]);const fs::path out=argv[3];
    require(!fs::exists(out),"constant GPU output directory must be fresh");fs::create_directories(out);output_created=true;
    {
      Gpu gpu;available=gpu.initialize(argv[2]);
      if(available) {
      device=gpu.device_name;std::cout<<"Constant expansion device: "<<device<<'\n';
      for(const auto &row:rows) {
        const auto actual=gpu.run(row.material,row.fragment);const auto prefix="event-"+std::to_string(row.event);
        save(out/(prefix+"-production.bin"),actual[0].data(),sizeof(Words));
        save(out/(prefix+"-reciprocal-control.bin"),actual[1].data(),sizeof(Words));
        save(out/(prefix+"-float-packet.bin"),actual[2].data(),sizeof(Words));
        const auto production=differences(actual[0],row.expected),control=differences(actual[1],row.control);
        const auto control_original=differences(actual[1],row.expected);
        const auto floating=differences(actual[2],row.expected);
        measurements.push_back({row.event,production,control,control_original,floating});actual_words+=72;
        actual_control_differences+=control_original;old_unpack_differences+=production;
        std::cout<<"event "<<row.event<<": completed float packet "<<floating<<" differing words; old unpack observed diagnostic "
            <<production<<"; reciprocal control "<<control_original<<" vs original, "<<control<<" vs exact control\n";
      }
      }
    } // Validation/cleanup errors are counted after all Vulkan destruction.
    if(!available && !validation_errors && !cleanup_errors){std::cerr<<"SKIP: required validated Vulkan1.3 compute device unavailable\n";return 77;}
  } catch(const std::exception &error){failure=error.what();std::cerr<<failure<<'\n';}
  // The production packed divide is a driver-dependent diagnostic. A driver
  // may divide exactly rather than lower it to reciprocal multiplication.
  // Only the explicit reciprocal control and explicit float packet gate pass.
  bool exact=measurements.size()==8 && actual_words==576 && actual_control_differences==13 && failure.empty() && !validation_errors && !cleanup_errors;
  for(const auto &row:measurements)exact&=row.float_differences==0 && row.control_differences==0;
  if(output_created) {
    std::ofstream report(fs::path(argv[3])/"report.json");
    report<<"{\n  \"status\": \""<<(exact?"passed":"failed")<<"\",\n  \"device\": "<<std::quoted(device)
        <<",\n  \"original_words\": "<<actual_words<<",\n  \"reciprocal_control_different_original_words\": "<<actual_control_differences
        <<",\n  \"old_unpack_observed_different_words\": "<<old_unpack_differences
        <<",\n  \"validation_errors_after_cleanup\": "<<validation_errors.load()<<",\n  \"cleanup_errors\": "<<cleanup_errors.load()
        <<",\n  \"failure\": "<<std::quoted(failure)<<",\n  \"measurements\": [\n";
    for(std::size_t i=0;i<measurements.size();++i) {
      const auto &m=measurements[i];report<<"    {\"event\": "<<m.event<<", \"old_unpack_observed_differing_words\": "<<m.production_differences
          <<", \"control_differing_words\": "<<m.control_differences<<", \"control_different_original_words\": "<<m.control_expected_differences
          <<", \"completed_float_packet_differing_words\": "<<m.float_differences<<"}"
          <<(i+1==measurements.size()?"\n":",\n");
    }
    report<<"  ],\n  \"scope\": \"Actual completed-program CPU binder plus448-byte float packet GPU passthrough, with explicit reciprocal negative control and driver-dependent old combiner.glsl argb unpack diagnostic; no fragment compiler context, raster, dynamic live inputs or raw fragment admission.\"\n}\n";
    if(!report){std::cerr<<"cannot retain constant GPU report\n";return 1;}
  }
  if(!exact)return 1;
  std::cout<<"576 completed float packet words exact; reciprocal13-word negative control; old unpack observed "
      <<old_unpack_differences<<" differing words (diagnostic only); validation/cleanup0\n";return 0;
}
