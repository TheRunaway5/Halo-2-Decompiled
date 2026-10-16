#include "halo2/cinematic_crossfade_render.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <vector>
using namespace halo2;
namespace {
void check(VkResult r){if(r!=VK_SUCCESS)throw FormatError("Crossfade GPU test Vulkan "+std::to_string(r));}
struct Gpu {
  VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
  VkCommandPool pool{};VkCommandBuffer command{};VkFence fence{};
  VkImage image{};VkDeviceMemory image_memory{};VkImageView image_view{};VkRenderPass pass{};VkFramebuffer framebuffer{};
  VkBuffer buffer{};VkDeviceMemory buffer_memory{};VkExtent2D extent{};bool initialized{};
  std::uint64_t pixels_checked{};
  unsigned type(unsigned mask,VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(unsigned i=0;i<p.memoryTypeCount;++i)if((mask&(1u<<i)) && (p.memoryTypes[i].propertyFlags&flags)==flags)return i;
    throw FormatError("Crossfade GPU test memory unavailable");
  }
  Gpu() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Original cinematic crossfade verification";app.apiVersion=VK_API_VERSION_1_0;
    VkInstanceCreateInfo ii{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ii.pApplicationInfo=&app;
    check(vkCreateInstance(&ii,nullptr,&instance));unsigned count{};check(vkEnumeratePhysicalDevices(instance,&count,nullptr));
    if(!count)throw FormatError("No Vulkan device for cinematic capture verification");
    std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));physical=devices.front();
    for(auto p:devices){VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(p,&properties);if(properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){physical=p;break;}}
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);std::cout<<properties.deviceName<<'\n';
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,families.data());
    unsigned family=0;while(family<count && !(families[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))++family;
    if(family==count)throw FormatError("No graphics queue for cinematic capture verification");
    const float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;
    check(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pi.queueFamilyIndex=family;pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device,&pi,nullptr,&pool));VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;
    check(vkAllocateCommandBuffers(device,&ai,&command));VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&fi,nullptr,&fence));
  }
  void release_target() {
    if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
    if(pass)vkDestroyRenderPass(device,pass,nullptr);
    if(image_view)vkDestroyImageView(device,image_view,nullptr);
    if(image)vkDestroyImage(device,image,nullptr);
    if(image_memory)vkFreeMemory(device,image_memory,nullptr);
    if(buffer)vkDestroyBuffer(device,buffer,nullptr);
    if(buffer_memory)vkFreeMemory(device,buffer_memory,nullptr);
    framebuffer={};pass={};image_view={};image={};image_memory={};buffer={};buffer_memory={};initialized=false;
  }
  ~Gpu() {
    if(device){vkDeviceWaitIdle(device);release_target();if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}
    if(instance)vkDestroyInstance(instance,nullptr);
  }
  void resize(unsigned width,unsigned height) {
    check(vkDeviceWaitIdle(device));release_target();extent={width,height};
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=VK_FORMAT_B8G8R8A8_UNORM;ci.extent={width,height,1};
    ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    check(vkCreateImage(device,&ci,nullptr,&image));VkMemoryRequirements r;vkGetImageMemoryRequirements(device,image,&r);
    VkMemoryAllocateInfo mi{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};mi.allocationSize=r.size;mi.memoryTypeIndex=type(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&mi,nullptr,&image_memory));check(vkBindImageMemory(device,image,image_memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(device,&vi,nullptr,&image_view));
    VkAttachmentDescription attachment{};attachment.format=ci.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout=attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
    const VkSubpassDependency dependencies[]{{VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT},
      {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,VK_DEPENDENCY_BY_REGION_BIT}};
    VkRenderPassCreateInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ri.attachmentCount=1;ri.pAttachments=&attachment;ri.subpassCount=1;ri.pSubpasses=&subpass;ri.dependencyCount=2;ri.pDependencies=dependencies;
    check(vkCreateRenderPass(device,&ri,nullptr,&pass));VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.attachmentCount=1;fb.pAttachments=&image_view;fb.width=width;fb.height=height;fb.layers=1;
    check(vkCreateFramebuffer(device,&fb,nullptr,&framebuffer));VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=std::uint64_t(width)*height*4;bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    check(vkCreateBuffer(device,&bi,nullptr,&buffer));vkGetBufferMemoryRequirements(device,buffer,&r);mi.allocationSize=r.size;mi.memoryTypeIndex=type(r.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(device,&mi,nullptr,&buffer_memory));check(vkBindBufferMemory(device,buffer,buffer_memory,0));
  }
  void frame(CinematicCrossfadeRender &renderer,const CinematicCrossfadeFrame &frame,
      std::array<float,4> scene,std::array<unsigned char,4> expected,
      bool pattern_scene=false,bool pattern_expected=false) {
    renderer.reconfigure(VK_FORMAT_B8G8R8A8_UNORM,pass);renderer.prepare(frame);
    check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=image;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    if(!initialized){barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);initialized=true;}
    VkRenderPassBeginInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass_info.renderPass=pass;pass_info.framebuffer=framebuffer;pass_info.renderArea={{0,0},extent};
    vkCmdBeginRenderPass(command,&pass_info,VK_SUBPASS_CONTENTS_INLINE);VkClearAttachment clear{};clear.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;std::copy(scene.begin(),scene.end(),clear.clearValue.color.float32);
    const VkClearRect rect{{{0,0},extent},0,1};vkCmdClearAttachments(command,1,&clear,1,&rect);
    if(pattern_scene) {
      if(extent.width!=640 || extent.height!=480)
        throw FormatError("Pattern source must exercise the original capture dimensions");
      constexpr std::array<std::array<float,4>,4> colors{{
        {1,0,0,1},{0,1,0,1},{0,0,1,1},{1,1,1,1}}};
      for(unsigned quadrant=0;quadrant<4;++quadrant) {
        std::copy(colors[quadrant].begin(),colors[quadrant].end(),clear.clearValue.color.float32);
        const VkClearRect part{{{int(quadrant%2)*320,int(quadrant/2)*240},{320,240}},0,1};
        vkCmdClearAttachments(command,1,&clear,1,&part);
      }
    }
    const auto captured=renderer.record({command,image,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,pass,pass,framebuffer,framebuffer});
    if(captured!=frame.capture)throw FormatError("Production capture scheduling differs from requested frame");
    vkCmdEndRenderPass(command);barrier.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(command,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.buffer=buffer;host.size=VK_WHOLE_SIZE;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,0,nullptr);
    check(vkEndCommandBuffer(command));check(vkResetFences(device,1,&fence));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    check(vkQueueSubmit(queue,1,&submit,fence));check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX));
    void *mapped{};check(vkMapMemory(device,buffer_memory,0,VK_WHOLE_SIZE,0,&mapped));const auto *pixels=static_cast<const unsigned char*>(mapped);
    unsigned failures=0;
    constexpr std::array<std::array<unsigned char,4>,4> pattern{{
      {0,0,255,255},{0,255,0,255},{255,0,0,255},{255,255,255,255}}};
    for(unsigned y=0;y<extent.height;++y)for(unsigned x=0;x<extent.width;++x) {
      const auto sample_x=(std::uint64_t(2*x+1)*640)/(2*extent.width);
      const auto sample_y=(std::uint64_t(2*y+1)*480)/(2*extent.height);
      const auto color=pattern_expected?pattern[(sample_y>=240)*2+(sample_x>=320)]:expected;
      const auto i=(std::uint64_t(y)*extent.width+x)*4;
      for(unsigned c=0;c<4;++c)failures+=pixels[i+c]!=color[c];
    }
    vkUnmapMemory(device,buffer_memory);if(failures)throw FormatError("Production captured-frame blend differs in "+std::to_string(failures)+" channels");
    pixels_checked+=std::uint64_t(extent.width)*extent.height;
  }
};
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Production cinematic shader directory required");
    Gpu gpu;gpu.resize(640,480);
    CinematicCrossfadeRender renderer({gpu.device,gpu.physical,VK_FORMAT_B8G8R8A8_UNORM,gpu.pass,argv[1]});
    gpu.frame(renderer,{true,cinematic_crossfade_program(1,1)},{1,0,0,1},{0,0,255,255});
    gpu.frame(renderer,{false,cinematic_crossfade_program(.5f,1)},{0,0,1,1},{127,0,128,255});
    gpu.frame(renderer,{false,cinematic_crossfade_program(.25f,1)},{0,1,0,1},{0,191,64,255});
    gpu.frame(renderer,{true,cinematic_crossfade_program(1,1)},{0,0,1,1},{255,0,0,255});
    gpu.resize(853,479);
    gpu.frame(renderer,{false,cinematic_crossfade_program(.5f,1)},{1,0,0,1},{128,0,127,255});
    gpu.frame(renderer,{true,std::nullopt},{0,1,0,1},{0,255,0,255});
    gpu.frame(renderer,{}, {1,0,0,1},{0,0,255,255});
    gpu.resize(640,480);
    gpu.frame(renderer,{true,cinematic_crossfade_program(1,1)},{0,0,0,1},{},true,true);
    gpu.frame(renderer,{false,cinematic_crossfade_program(1,1)},{1,1,1,1},{},false,true);
    gpu.resize(853,479);
    gpu.frame(renderer,{false,cinematic_crossfade_program(1,1)},{0,0,0,1},{},false,true);
    std::cout<<gpu.pixels_checked<<" exact BGRA8 pixels: capture/reuse/recapture, source-alpha quantization, spatial orientation, point sampling, resize history and inactive control passed\n";
  } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
