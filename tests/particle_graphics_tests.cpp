#include "halo2/particle_renderer.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
void check(VkResult result) {
  if(result!=VK_SUCCESS)throw FormatError("particle capture Vulkan failure: "+std::to_string(result));
}
std::vector<std::byte> read(const std::filesystem::path &path,std::size_t size) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  if(!file || std::size_t(file.tellg())!=size)throw FormatError("particle fixture size differs: "+path.string());
  std::vector<std::byte> data(size);file.seekg(0);
  if(!file.read(reinterpret_cast<char*>(data.data()),size))throw FormatError("short particle capture fixture");
  return data;
}
void save(const std::filesystem::path &path,std::span<const std::byte> data) {
  std::ofstream file(path,std::ios::binary);
  if(!file.write(reinterpret_cast<const char*>(data.data()),data.size()))throw FormatError("cannot save particle capture result");
}
template<class T> T at(std::span<const std::byte> data,std::size_t offset) {
  if(offset>data.size() || sizeof(T)>data.size()-offset)throw FormatError("particle fixture field outside buffer");
  T value;std::memcpy(&value,data.data()+offset,sizeof(value));return value;
}

// This fixture invokes the actual production subrenderer. Captured uniform
// arrays and prior framebuffer contents are test inputs, never runtime state.
class CaptureGpu {
  struct Buffer {VkBuffer handle{};VkDeviceMemory memory{};void *mapped{};};
  struct Image {VkImage handle{};VkDeviceMemory memory{};VkImageView view{};};
  VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
  unsigned family{},width{},height{};VkFormat depth_format{};
  VkCommandPool commands{};VkCommandBuffer command{};VkRenderPass pass{};VkFramebuffer framebuffer{};
  Image color,depth;Buffer color_input,depth_input,color_output,depth_output;
  std::unique_ptr<ParticleRenderer> renderer;bool initialized{};
  unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(unsigned n=0;n<p.memoryTypeCount;++n)
      if((bits&(1u<<n)) && (p.memoryTypes[n].propertyFlags&flags)==flags)return n;
    throw FormatError("particle capture memory unavailable");
  }
  void buffer(Buffer &b,VkBufferUsageFlags usage) {
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=std::size_t(width)*height*4;ci.usage=usage;
    check(vkCreateBuffer(device,&ci,nullptr,&b.handle));VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,b.handle,&req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;
    ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&b.memory));check(vkBindBufferMemory(device,b.handle,b.memory,0));
    check(vkMapMemory(device,b.memory,0,ci.size,0,&b.mapped));
  }
  void image(Image &i,VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect) {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=format;
    ci.extent={width,height,1};ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;
    ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    check(vkCreateImage(device,&ci,nullptr,&i.handle));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,i.handle,&req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;
    ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&i.memory));check(vkBindImageMemory(device,i.handle,i.memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=i.handle;vi.format=format;
    vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.subresourceRange={aspect,0,1,0,1};check(vkCreateImageView(device,&vi,nullptr,&i.view));
  }
  void barrier(Image image,VkImageAspectFlags aspect,VkImageLayout before,VkImageLayout after,
               VkAccessFlags source,VkAccessFlags destination,VkPipelineStageFlags first,VkPipelineStageFlags last) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image.handle;b.subresourceRange={aspect,0,1,0,1};
    b.oldLayout=before;b.newLayout=after;b.srcAccessMask=source;b.dstAccessMask=destination;
    b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    vkCmdPipelineBarrier(command,first,last,0,0,nullptr,0,nullptr,1,&b);
  }
  void release() {
    if(device)vkDeviceWaitIdle(device);renderer.reset();
    if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
    if(pass)vkDestroyRenderPass(device,pass,nullptr);
    for(auto b:{color_input,depth_input,color_output,depth_output}) {
      if(b.mapped)vkUnmapMemory(device,b.memory);
      if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);
      if(b.memory)vkFreeMemory(device,b.memory,nullptr);
    }
    for(auto i:{color,depth}) {
      if(i.view)vkDestroyImageView(device,i.view,nullptr);
      if(i.handle)vkDestroyImage(device,i.handle,nullptr);
      if(i.memory)vkFreeMemory(device,i.memory,nullptr);
    }
    if(commands)vkDestroyCommandPool(device,commands,nullptr);
    if(device)vkDestroyDevice(device,nullptr);
    if(instance)vkDestroyInstance(instance,nullptr);
  }
public:
  CaptureGpu(unsigned w,unsigned h,VkFormat format,const std::filesystem::path &shaders)
    :width(w),height(h),depth_format(format) {
    try {
      VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
      VkInstanceCreateInfo ii{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ii.pApplicationInfo=&app;check(vkCreateInstance(&ii,nullptr,&instance));
      unsigned count{};check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);
      check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));VkPhysicalDeviceFeatures features{};
      for(auto candidate:devices) {
        vkGetPhysicalDeviceFeatures(candidate,&features);
        if(!features.geometryShader || !features.shaderTessellationAndGeometryPointSize || !features.depthClamp)continue;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
        for(unsigned n=0;n<count;++n)if(families[n].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=n;break;}
        if(physical)break;
      }
      if(!physical)throw FormatError("original particle graphics device unavailable");
      VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(physical,&props);std::cout<<"Particle graphics GPU: "<<props.deviceName<<'\n';
      VkFormatProperties fp;vkGetPhysicalDeviceFormatProperties(physical,format,&fp);
      const VkFormatFeatureFlags required=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
      if((fp.optimalTilingFeatures&required)!=required)throw FormatError("particle capture depth format unavailable");
      float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
      VkPhysicalDeviceFeatures enabled{};enabled.geometryShader=enabled.shaderTessellationAndGeometryPointSize=enabled.depthClamp=VK_TRUE;
      VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;di.pEnabledFeatures=&enabled;
      check(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
      VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pi.queueFamilyIndex=family;pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      check(vkCreateCommandPool(device,&pi,nullptr,&commands));VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=commands;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
      check(vkAllocateCommandBuffers(device,&ca,&command));
      buffer(color_input,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);buffer(depth_input,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
      buffer(color_output,VK_BUFFER_USAGE_TRANSFER_DST_BIT);buffer(depth_output,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
      const auto aspect=VK_IMAGE_ASPECT_DEPTH_BIT|(format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_STENCIL_BIT:0);
      image(color,VK_FORMAT_B8G8R8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
      image(depth,format,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,aspect);
      VkAttachmentDescription attachments[2]{};
      for(auto &a:attachments){a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;}
      attachments[0].format=VK_FORMAT_B8G8R8A8_UNORM;attachments[0].initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachments[1].format=format;attachments[1].initialLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      VkAttachmentReference cr{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},dr{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
      VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&cr;sub.pDepthStencilAttachment=&dr;
      const auto stages=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      const auto access=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      VkSubpassDependency deps[]{{VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_TRANSFER_BIT,stages,VK_ACCESS_TRANSFER_WRITE_BIT,access,0},
        {0,VK_SUBPASS_EXTERNAL,stages,VK_PIPELINE_STAGE_TRANSFER_BIT,access,VK_ACCESS_TRANSFER_READ_BIT,0}};
      VkRenderPassCreateInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ri.attachmentCount=2;ri.pAttachments=attachments;ri.subpassCount=1;ri.pSubpasses=&sub;ri.dependencyCount=2;ri.pDependencies=deps;
      check(vkCreateRenderPass(device,&ri,nullptr,&pass));VkImageView views[]{color.view,depth.view};
      VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fi.renderPass=pass;fi.attachmentCount=2;fi.pAttachments=views;fi.width=w;fi.height=h;fi.layers=1;
      check(vkCreateFramebuffer(device,&fi,nullptr,&framebuffer));renderer=std::make_unique<ParticleRenderer>(physical,device,queue,family,pass,shaders);
    }catch(...){release();throw;}
  }
  ~CaptureGpu(){release();}
  void run(const ParticleDraw &draw,std::span<const std::byte> background,std::span<const std::byte> d24,std::size_t padding=0) {
    const auto bytes=std::size_t(width)*height*4;if(background.size()!=bytes || d24.size()!=bytes)throw FormatError("particle capture target size differs");
    std::memcpy(color_input.mapped,background.data(),bytes);
    for(std::size_t n=0;n<bytes;n+=4) {
      const auto z=at<unsigned>(d24,n)&0xffffffu;
      if(depth_format==VK_FORMAT_D32_SFLOAT) {const float f=float(z)/16777215.f;std::memcpy(static_cast<std::byte*>(depth_input.mapped)+n,&f,4);}
      else std::memcpy(static_cast<std::byte*>(depth_input.mapped)+n,&z,4);
    }
    std::vector<ParticleDraw> draws(padding+1,draw);
    // Extra submissions exercise arena growth and draw offsets while the
    // original fragment clip rejects their pixels. The retained first draw
    // must still produce exactly the captured color/depth after each resize.
    for(std::size_t n=1;n<draws.size();++n)draws[n].depth.clip_range[2]=draw.depth.clip_range[3]+1.f;
    renderer->prepare(draws);check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    const auto aspect=VK_IMAGE_ASPECT_DEPTH_BIT|(depth_format==VK_FORMAT_D24_UNORM_S8_UINT?VK_IMAGE_ASPECT_STENCIL_BIT:0);
    const auto old=initialized?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
    const auto access=initialized?VK_ACCESS_TRANSFER_READ_BIT:0;
    const auto stage=initialized?VK_PIPELINE_STAGE_TRANSFER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    barrier(color,VK_IMAGE_ASPECT_COLOR_BIT,old,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,access,VK_ACCESS_TRANSFER_WRITE_BIT,stage,VK_PIPELINE_STAGE_TRANSFER_BIT);
    barrier(depth,aspect,old,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,access,VK_ACCESS_TRANSFER_WRITE_BIT,stage,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};
    vkCmdCopyBufferToImage(command,color_input.handle,color.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;vkCmdCopyBufferToImage(command,depth_input.handle,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    barrier(color,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    barrier(depth,aspect,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rb.renderPass=pass;rb.framebuffer=framebuffer;rb.renderArea={{0,0},{width,height}};
    vkCmdBeginRenderPass(command,&rb,VK_SUBPASS_CONTENTS_INLINE);renderer->record(command,{0,0,float(width),float(height),0,1},{{0,0},{width,height}},0,draws.size());vkCmdEndRenderPass(command);
    copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;vkCmdCopyImageToBuffer(command,color.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,color_output.handle,1,&copy);
    copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;vkCmdCopyImageToBuffer(command,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,depth_output.handle,1,&copy);
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
    check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));initialized=true;
  }
  std::span<const std::byte> color_bytes()const{return {static_cast<const std::byte*>(color_output.mapped),std::size_t(width)*height*4};}
  std::span<const std::byte> depth_bytes()const{return {static_cast<const std::byte*>(depth_output.mapped),std::size_t(width)*height*4};}
  std::span<const std::byte> initial_depth_bytes()const{return {static_cast<const std::byte*>(depth_input.mapped),std::size_t(width)*height*4};}
};
}
int main(int argc,char **argv) {
  try {
    if(argc!=4)throw FormatError("original map, particle fixture and output directories required");
    const std::filesystem::path directory=argv[2],output=argv[3];
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(directory/"draws.bin"))return 77;
    Cache cache(argv[1]);std::filesystem::create_directories(output);
    std::ifstream records(directory/"draws.bin",std::ios::binary);unsigned count{};
    if(!records.read(reinterpret_cast<char*>(&count),4) || count==0 || count>1000)throw FormatError("invalid particle capture record count");
    std::vector<std::array<unsigned,7>> rows(count);
    if(!records.read(reinterpret_cast<char*>(rows.data()),rows.size()*sizeof(rows[0])) || records.peek()!=std::char_traits<char>::eof())throw FormatError("invalid particle capture records");
    unsigned failures{};
    for(auto format:{VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_D32_SFLOAT}) {
      const auto label=format==VK_FORMAT_D24_UNORM_S8_UINT?"d24":"d32";
      CaptureGpu gpu(rows[0][5],rows[0][6],format,HALO2_PARTICLE_GRAPHICS_SHADER_DIR);
      for(const auto &r:rows) {
        if(r[5]!=rows[0][5] || r[6]!=rows[0][6])throw FormatError("mixed particle fixture target sizes");
        const auto prefix=std::to_string(r[0]);const auto bytes=std::size_t(r[5])*r[6]*4;
        const auto vertex=read(directory/(prefix+"-vertex-0.bin"),5344),attributes=read(directory/(prefix+"-vertex-1.bin"),240);
        const auto fragment=read(directory/(prefix+"-fragment.bin"),944);
        ParticleDraw draw;draw.material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(r[1],r[2],r[3],r[4]));
        std::memcpy(draw.vertex.attributes.data()+1,attributes.data(),240);std::memcpy(draw.vertex.constants.data(),vertex.data(),3072);
        draw.vertex.clip_range=at<ShaderVector>(vertex,3072);
        draw.vertex.surface_fog={at<float>(vertex,5336),at<float>(vertex,5340),at<float>(vertex,3088),at<float>(vertex,3092)};
        draw.depth.clip_range=at<ShaderVector>(fragment,272);
        draw.depth.parameters={at<float>(fragment,832),at<float>(fragment,836),float(at<int>(fragment,864)),float(at<int>(fragment,868))};
        const auto region=at<std::array<int,4>>(fragment,288);
        if(region[0]>0 || region[1]>0 || region[2]<int(r[5]) || region[3]<int(r[6]))
          throw FormatError("particle fixture has additional fragment clipping");
        const auto before=read(directory/(prefix+"-before-color.bin"),bytes),expected=read(directory/(prefix+"-after-color.bin"),bytes);
        const auto depth_before=read(directory/(prefix+"-before-depth.bin"),bytes+bytes/4),depth_after=read(directory/(prefix+"-after-depth.bin"),bytes+bytes/4);
        if(depth_before!=depth_after)throw FormatError("captured particle fixture writes depth; extend expected-depth conversion first");
        gpu.run(draw,before,{depth_before.data(),bytes});
        if(r==rows.front()) {
          const std::vector<std::byte> color(gpu.color_bytes().begin(),gpu.color_bytes().end()),depth(gpu.depth_bytes().begin(),gpu.depth_bytes().end());
          for(const auto padding:{64u,128u,0u}) {
            gpu.run(draw,before,{depth_before.data(),bytes},padding);
            if(!std::equal(color.begin(),color.end(),gpu.color_bytes().begin()) ||
               !std::equal(depth.begin(),depth.end(),gpu.depth_bytes().begin()))
              throw FormatError("Particle uniform arena growth/shrink changed color or depth");
          }
          std::cout<<"Particle uniform arena retained exact output through 1/65/129/1 draws "<<label<<'\n';
        }
        save(output/(prefix+"-"+label+"-color.bgra"),gpu.color_bytes());save(output/(prefix+"-"+label+"-depth.bin"),gpu.depth_bytes());
        std::size_t color_differences{},depth_differences{};unsigned maximum{};
        for(std::size_t n=0;n<bytes;++n) {
          const auto difference=unsigned(std::abs(int(std::to_integer<unsigned>(gpu.color_bytes()[n]))-int(std::to_integer<unsigned>(expected[n]))));
          color_differences+=difference!=0;maximum=std::max(maximum,difference);
          if(format==VK_FORMAT_D32_SFLOAT)depth_differences+=gpu.depth_bytes()[n]!=gpu.initial_depth_bytes()[n];
        }
        if(format==VK_FORMAT_D24_UNORM_S8_UINT)for(std::size_t n=0;n<bytes;n+=4)
          depth_differences+=(at<unsigned>(gpu.depth_bytes(),n)&0xffffffu)!=(at<unsigned>(depth_after,n)&0xffffffu);
        failures+=color_differences!=0 || depth_differences!=0;
        std::cout<<"event "<<r[0]<<' '<<label<<": "<<color_differences<<" color bytes differ, maximum "<<maximum<<", "<<depth_differences<<" depth differences\n";
      }
    }
    if(failures)throw FormatError("native particle draw pipeline differs from original captured draws");
    std::cout<<count<<" original draws exact through the native pipeline with both depth formats; live simulation remains unverified\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
