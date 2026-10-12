#pragma once
#include "halo2/render_program.hpp"
#include <vulkan/vulkan.h>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
namespace halo2_test {
inline void vkcheck(VkResult r) {if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan compute fixture failed: "+std::to_string(r));}
struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};};
template<class Fixture,unsigned ResultCount> struct Compute {
  using Vector=halo2::ShaderVector;
  const char *shader_path;
  explicit Compute(const char *path):shader_path(path){}
  VkInstance instance{};VkDevice device{};VkQueue queue{};VkPhysicalDevice physical{};
  VkDescriptorSetLayout set_layout{};VkDescriptorPool descriptors{};
  VkPipelineLayout layout{};VkPipeline pipeline{};VkShaderModule shader{};
  VkCommandPool commands{};Buffer input,output;
  ~Compute() {
    if(device) {
      vkDeviceWaitIdle(device);
      if(commands)vkDestroyCommandPool(device,commands,nullptr);
      if(pipeline)vkDestroyPipeline(device,pipeline,nullptr);
      if(shader)vkDestroyShaderModule(device,shader,nullptr);
      if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
      if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);
      if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);
      for(auto *b:{&input,&output}) {
        if(b->mapped)vkUnmapMemory(device,b->memory);
        if(b->handle)vkDestroyBuffer(device,b->handle,nullptr);
        if(b->memory)vkFreeMemory(device,b->memory,nullptr);
      }
      vkDestroyDevice(device,nullptr);
    }
    if(instance)vkDestroyInstance(instance,nullptr);
  }
  void allocate(Buffer &b,VkDeviceSize size) {
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size=size;info.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkcheck(vkCreateBuffer(device,&info,nullptr,&b.handle));
    VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,b.handle,&req);
    VkPhysicalDeviceMemoryProperties props;vkGetPhysicalDeviceMemoryProperties(physical,&props);
    unsigned memory=props.memoryTypeCount;
    for(unsigned i=0;i<props.memoryTypeCount;++i)
      if((req.memoryTypeBits&(1u<<i)) && (props.memoryTypes[i].propertyFlags&
          (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
          (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){memory=i;break;}
    if(memory==props.memoryTypeCount)throw std::runtime_error("no coherent Vulkan test memory");
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=req.size;allocation.memoryTypeIndex=memory;
    vkcheck(vkAllocateMemory(device,&allocation,nullptr,&b.memory));
    vkcheck(vkBindBufferMemory(device,b.handle,b.memory,0));
    vkcheck(vkMapMemory(device,b.memory,0,size,0,&b.mapped));
  }
  std::vector<Vector> run(const std::vector<Fixture> &fixtures) {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
    vkcheck(vkCreateInstance(&create,nullptr,&instance));
    unsigned count=0;vkcheck(vkEnumeratePhysicalDevices(instance,&count,nullptr));
    std::vector<VkPhysicalDevice> devices(count);vkcheck(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
    unsigned family=0;
    for(auto candidate:devices) {
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);
      std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
      for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_COMPUTE_BIT){physical=candidate;family=i;break;}
      if(physical)break;
    }
    if(!physical)throw std::runtime_error("no Vulkan compute device for fixtures");
    const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
    VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;
    vkcheck(vkCreateDevice(physical,&dc,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    allocate(input,fixtures.size()*sizeof(Fixture));allocate(output,fixtures.size()*ResultCount*sizeof(Vector));
    std::memcpy(input.mapped,fixtures.data(),fixtures.size()*sizeof(Fixture));
    VkDescriptorSetLayoutBinding bindings[2]{};
    for(unsigned i=0;i<2;++i)bindings[i]={i,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
    VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=2;sl.pBindings=bindings;
    vkcheck(vkCreateDescriptorSetLayout(device,&sl,nullptr,&set_layout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&size;
    vkcheck(vkCreateDescriptorPool(device,&pool,nullptr,&descriptors));
    VkDescriptorSetAllocateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ds.descriptorPool=descriptors;ds.descriptorSetCount=1;ds.pSetLayouts=&set_layout;
    VkDescriptorSet set;vkcheck(vkAllocateDescriptorSets(device,&ds,&set));
    VkDescriptorBufferInfo buffers[2]{{input.handle,0,VK_WHOLE_SIZE},{output.handle,0,VK_WHOLE_SIZE}};
    VkWriteDescriptorSet writes[2]{};
    for(unsigned i=0;i<2;++i) {writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=set;
      writes[i].dstBinding=i;writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i].descriptorCount=1;writes[i].pBufferInfo=&buffers[i];}
    vkUpdateDescriptorSets(device,2,writes,0,nullptr);
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&set_layout;
    vkcheck(vkCreatePipelineLayout(device,&pl,nullptr,&layout));
    std::ifstream file(shader_path,std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("compute fixture shader missing");
    std::vector<unsigned> code(std::size_t(file.tellg())/4);file.seekg(0);file.read(reinterpret_cast<char*>(code.data()),code.size()*4);
    VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=code.size()*4;sm.pCode=code.data();
    vkcheck(vkCreateShaderModule(device,&sm,nullptr,&shader));
    VkComputePipelineCreateInfo cp{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};cp.layout=layout;
    cp.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};
    vkcheck(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&cp,nullptr,&pipeline));
    VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;
    vkcheck(vkCreateCommandPool(device,&pc,nullptr,&commands));
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=commands;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
    VkCommandBuffer command;vkcheck(vkAllocateCommandBuffers(device,&ca,&command));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};vkcheck(vkBeginCommandBuffer(command,&begin));
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,nullptr);
    vkCmdDispatch(command,unsigned(fixtures.size()),1,1);
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);
    vkcheck(vkEndCommandBuffer(command));
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    vkcheck(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));vkcheck(vkQueueWaitIdle(queue));
    std::vector<Vector> result(fixtures.size()*ResultCount);std::memcpy(result.data(),output.mapped,result.size()*sizeof(Vector));return result;
  }
};

}
