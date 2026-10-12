#include "halo2/render_program.hpp"
#include "halo2/render_view.hpp"
#include <vulkan/vulkan.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Vector=halo2::ShaderVector;
struct Fixture {std::array<unsigned,4> control{};std::array<Vector,4> coordinates{};Vector normal{};
  std::array<Vector,4> bump_matrices{};};
static_assert(sizeof(Fixture)==160);
struct Check {unsigned fixture,stage;Vector expected;const char *name;};
unsigned modes(std::array<unsigned,4> stages) {
  unsigned bits=0;for(unsigned i=0;i<4;++i)bits|=stages[i]<<(5*i);return bits;
}
void vkcheck(VkResult r) {if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan addressing test failed: "+std::to_string(r));}
struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};};
struct Compute {
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
    if(!physical)throw std::runtime_error("no Vulkan compute device for addressing test");
    const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
    VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;
    vkcheck(vkCreateDevice(physical,&dc,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    allocate(input,fixtures.size()*sizeof(Fixture));allocate(output,fixtures.size()*4*sizeof(Vector));
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
    std::ifstream file(HALO2_TEXTURE_TEST_SHADER,std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("addressing shader missing");
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
    std::vector<Vector> result(fixtures.size()*4);std::memcpy(result.data(),output.mapped,result.size()*sizeof(Vector));return result;
  }
};
}
int main() {
  try {
    std::vector<Fixture> fixtures;std::vector<Check> checks;
    // Real Cairo bumped environment definitions use [PROJECT2D,DOTPRODUCT,
    // DOT_RFLCT_DIFF,DOT_RFLCT_SPEC] with dot mappings 0x111.
    Fixture reflection;reflection.control={modes({1,17,11,12}),0x111,0,0};
    reflection.normal={128.f/255,128.f/255,1,1};
    reflection.coordinates={Vector{0,0,0,1},Vector{1,0,0,.4f},Vector{0,1,0,.2f},Vector{0,0,1,1}};
    fixtures.push_back(reflection);
    checks.push_back({0,2,{0,0,1,1},"Cairo dependent diffuse cube address"});
    checks.push_back({0,3,{-.4f,-.2f,1,1},"Cairo dependent reflected cube address"});
    checks.push_back({0,1,{0,0,0,0},"DOTPRODUCT color result"});
    auto passthrough=reflection;passthrough.control={modes({4,0,0,0}),0,0,0};
    passthrough.coordinates[0]={-1,.5f,2,.25f};fixtures.push_back(passthrough);
    checks.push_back({1,0,{0,.5f,1,.25f},"PASSTHRU clamp"});
    auto dependent=reflection;dependent.control={modes({4,4,0,15}),0,1u<<20,0};
    dependent.coordinates[1]={.2f,.3f,.4f,.5f};fixtures.push_back(dependent);
    checks.push_back({2,3,{.5f,.2f,0,1},"dependent AR stage selector"});
    dependent.control[0]=modes({4,4,0,16});fixtures.push_back(dependent);
    checks.push_back({3,3,{.3f,.4f,0,1},"dependent GB stage selector"});
    auto dot=reflection;dot.control[0]=modes({1,17,9,0});dot.coordinates[2]={0,0,1,1};fixtures.push_back(dot);
    checks.push_back({4,2,{0,1,0,1},"dependent dot ST coordinates"});
    for(const unsigned green:{0u,31u,127u,128u,255u})
    for(const unsigned blue:{0u,31u,127u,128u,255u}) {
      Fixture water;water.control={modes({1,6,6,6}),0,0,0};
      water.normal={.8f,float(green)/255.f,float(blue)/255.f,1};
      water.coordinates[0]={.1f,.2f,0,1};
      for(unsigned i=1;i<4;++i) {
        water.coordinates[i]={.25f,.75f,0,2}; // BUMPENVMAP ignores q.
        water.bump_matrices[i]={.2f,.3f,-.4f,.5f};
      }
      const auto signed_byte=[](unsigned b){return float(b<128?int(b):int(b)-256)/127.f;};
      const float ds=signed_byte(blue),dt=signed_byte(green);
      const Vector expected{.25f+.2f*ds-.4f*dt,.75f+.3f*ds+.5f*dt,0,1};
      const auto index=unsigned(fixtures.size());fixtures.push_back(water);
      for(unsigned i=1;i<4;++i)checks.push_back({index,i,expected,"original water signed B/G bump matrix address"});
      // Signed texture storage is byte/128, independently of PS sign3.
      water.control[3]=2;water.normal[1]=dt*127.f/128.f;water.normal[2]=ds*127.f/128.f;
      const Vector signed_expected{.25f+.2f*water.normal[2]-.4f*water.normal[1],
        .75f+.3f*water.normal[2]+.5f*water.normal[1],0,1};
      const auto signed_index=unsigned(fixtures.size());fixtures.push_back(water);
      for(unsigned i=1;i<4;++i)checks.push_back({signed_index,i,signed_expected,"signed image water bump without duplicate conversion"});
    }
    {
      Fixture filtered;filtered.control={modes({1,6,6,6}),0,0,2};
      filtered.normal={0,(127.f/128.f-1.f)*.5f,(31.f/128.f-1.f/128.f)*.5f,1};
      for(unsigned i=1;i<4;++i) {
        filtered.coordinates[i]={.25f,.75f,0,2};filtered.bump_matrices[i]={.2f,.3f,-.4f,.5f};
      }
      const Vector expected{.25f+.2f*filtered.normal[2]-.4f*filtered.normal[1],
        .75f+.3f*filtered.normal[2]+.5f*filtered.normal[1],0,1};
      const auto index=unsigned(fixtures.size());fixtures.push_back(filtered);
      for(unsigned i=1;i<4;++i)checks.push_back({index,i,expected,"water signed texels interpolate across sign boundary"});
    }
    for(const float size:{480.f,720.f,1080.f,2160.f}) {
      Fixture projected;projected.control[0]=modes({0,1,0,0});
      projected.coordinates[1]={size*.25f,size*.75f,0,size};
      const unsigned index=unsigned(fixtures.size());fixtures.push_back(projected);
      checks.push_back({index,1,{.25f,.75f,0,1},"projected lookup resolution invariance"});
    }
    // Original Elite plasma mode word=0x8420, PS flags word59=0. The
    // texture type does not rewrite PROJECT2D to PROJECT3D. xemu's primary
    // NV2A translator supplies R=0 for PROJECT2D with a volume binding.
    for(const float q:{.125f,1.f,2.f,64.f})
    for(const float authored_r:{-4.f,0.f,.75f,8.f}) {
      Fixture volume;volume.control={0x8420,0,0,0x100u|(1u<<1)|(1u<<2)};
      volume.coordinates[1]={q*.25f,q*.75f,authored_r,q};
      volume.coordinates[2]={q*.375f,q*.625f,authored_r,q};
      volume.coordinates[3]={q*.125f,q*.875f,authored_r,q};
      const unsigned index=unsigned(fixtures.size());fixtures.push_back(volume);
      checks.push_back({index,1,{.25f,.75f,0,1},"Elite volume PROJECT2D stage1 address"});
      checks.push_back({index,2,{.375f,.625f,0,1},"Elite volume PROJECT2D stage2 address"});
      checks.push_back({index,3,{.125f,.875f,0,1},"Elite mixed 2D stage3 address"});
    }
    // Original frag attenuation uses PROJECT3D, retaining all three authored
    // coordinates and their common homogeneous divisor. Stage2 remains a
    // mixed 2D control, and the same ratios survive resolution-scaled q.
    for(const float q:{.125f,1.f,2.f,64.f,480.f,720.f,1080.f,2160.f})
    for(const float r:{-.25f,0.f,.375f,1.25f}) {
      Fixture volume;volume.control={modes({0,2,2,2}),0,0,0x100u|(1u<<1)|(1u<<3)};
      volume.coordinates[1]={q*.25f,q*.75f,q*r,q};
      volume.coordinates[2]={q*.375f,q*.625f,q*r,q};
      volume.coordinates[3]={q*.125f,q*.875f,q*r,q};
      const unsigned index=unsigned(fixtures.size());fixtures.push_back(volume);
      checks.push_back({index,1,{.25f,.75f,r,1},"frag PROJECT3D stage1 address"});
      checks.push_back({index,2,{.375f,.625f,0,1},"mixed PROJECT3D 2D image address"});
      checks.push_back({index,3,{.125f,.875f,r,1},"frag PROJECT3D stage3 address/resolution invariance"});
    }
    for(const auto dimensions:{std::array{640.f,480.f},{1280.f,960.f},
                               {1280.f,720.f},{1920.f,1080.f},{3840.f,2160.f}})
    for(const float scale:{.01f,.0625f,.5f,1.f,2.f})
    for(unsigned flags=0;flags<4;++flags)
    for(const unsigned mode:{0u,2u}) {
      Fixture viewport;viewport.control[3]=1;
      viewport.coordinates[0]={13,27,dimensions[0],dimensions[1]};
      viewport.coordinates[1]={scale,float(flags&1),float(flags&2),float(mode)};
      const unsigned index=unsigned(fixtures.size());fixtures.push_back(viewport);
      // The CPU reference is compared bit-for-bit with original 00012FA0 by
      // differential_viewport.py; here verify the actual shared GPU function.
      const auto expected=halo2::viewport_constants({13,27,dimensions[0],dimensions[1],
          scale,bool(flags&1),bool(flags&2),mode});
      for(unsigned i=0;i<4;++i)checks.push_back({index,i,expected[i],"native GPU retail viewport constant"});
    }
    Compute compute;const auto results=compute.run(fixtures);
    for(const auto &c:checks)for(unsigned axis=0;axis<4;++axis)
      if(!std::isfinite(results[c.fixture*4+c.stage][axis]) ||
         std::abs(results[c.fixture*4+c.stage][axis]-c.expected[axis])>2e-5f)
        throw std::runtime_error(std::string(c.name)+" failed, component "+std::to_string(axis)+
          ": got "+std::to_string(results[c.fixture*4+c.stage][axis])+", expected "+std::to_string(c.expected[axis]));
    std::cout << checks.size() << " native GPU texture addressing checks passed\n";return 0;
  } catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
