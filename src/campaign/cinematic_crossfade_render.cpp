#include "halo2/cinematic_crossfade_render.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <vector>
namespace halo2 {
namespace {
void check(VkResult result,const char *what) {
  if(result!=VK_SUCCESS)throw FormatError(std::string(what)+": Vulkan "+std::to_string(result));
}
}
struct CinematicCrossfadeRender::Impl {
  CinematicCrossfadeRenderContext context;
  CinematicCrossfadeFrame frame;
  VkImage history{};VkDeviceMemory image_memory{};VkImageView view{};
  VkBuffer uniform{};VkDeviceMemory uniform_memory{};
  VkSampler sampler{};VkDescriptorSetLayout set_layout{};VkPipelineLayout layout{};
  VkDescriptorPool pool{};VkDescriptorSet descriptor{};VkPipeline pipeline{};
  bool initialized{};
  VkExtent2D capture_extent{};
  unsigned memory_type(unsigned bits,VkMemoryPropertyFlags flags)const {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(context.physical,&p);
    for(unsigned i=0;i<p.memoryTypeCount;++i)
      if((bits&(1u<<i)) && (p.memoryTypes[i].propertyFlags&flags)==flags)return i;
    throw FormatError("Cinematic capture memory type unavailable");
  }
  VkShaderModule shader(const char *name)const {
    const auto path=context.shader_directory/name;std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file)throw FormatError("Missing cinematic crossfade shader "+path.string());
    const auto size=file.tellg();if(size<=0 || size%4)throw FormatError("Invalid cinematic crossfade shader");
    std::vector<std::uint32_t> words(std::size_t(size)/4);file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(words.data()),size))throw FormatError("Short cinematic shader");
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=std::size_t(size);info.pCode=words.data();
    VkShaderModule module{};check(vkCreateShaderModule(context.device,&info,nullptr,&module),"Cinematic shader module");return module;
  }
  void size_history(VkExtent2D extent) {
    if(history && extent.width==capture_extent.width && extent.height==capture_extent.height)return;
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(context.physical,&properties);
    if(!extent.width || !extent.height || extent.width>properties.limits.maxImageDimension2D ||
       extent.height>properties.limits.maxImageDimension2D)
      throw FormatError("Cinematic native capture extent unavailable");
    VkImage replacement{};VkDeviceMemory memory{};VkImageView image_view{};
    try {
      VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};image.imageType=VK_IMAGE_TYPE_2D;image.format=VK_FORMAT_B8G8R8A8_UNORM;
      image.extent={extent.width,extent.height,1};image.mipLevels=image.arrayLayers=1;image.samples=VK_SAMPLE_COUNT_1_BIT;image.tiling=VK_IMAGE_TILING_OPTIMAL;
      image.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
      check(vkCreateImage(context.device,&image,nullptr,&replacement),"Cinematic history image");
      VkMemoryRequirements r;vkGetImageMemoryRequirements(context.device,replacement,&r);
      VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=r.size;allocation.memoryTypeIndex=memory_type(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      check(vkAllocateMemory(context.device,&allocation,nullptr,&memory),"Cinematic history memory");check(vkBindImageMemory(context.device,replacement,memory,0),"Cinematic history binding");
      VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=replacement;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=image.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
      check(vkCreateImageView(context.device,&vi,nullptr,&image_view),"Cinematic history view");
    } catch(...) {
      if(image_view)vkDestroyImageView(context.device,image_view,nullptr);
      if(replacement)vkDestroyImage(context.device,replacement,nullptr);
      if(memory)vkFreeMemory(context.device,memory,nullptr);
      throw;
    }
    if(view)vkDestroyImageView(context.device,view,nullptr);
    if(history)vkDestroyImage(context.device,history,nullptr);
    if(image_memory)vkFreeMemory(context.device,image_memory,nullptr);
    history=replacement;image_memory=memory;view=image_view;capture_extent=extent;initialized=false;
    if(descriptor) {
      const VkDescriptorImageInfo image{sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=0;write.descriptorCount=1;
      write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
      vkUpdateDescriptorSets(context.device,1,&write,0,nullptr);
    }
  }
  void make_pipeline(VkFormat format,VkRenderPass pass) {
    if(format!=VK_FORMAT_B8G8R8A8_UNORM && format!=VK_FORMAT_R8G8B8A8_UNORM)
      throw FormatError("Original cinematic capture requires an unorm ARGB32 scene target");
    VkFormatProperties source,capture;vkGetPhysicalDeviceFormatProperties(context.physical,format,&source);
    vkGetPhysicalDeviceFormatProperties(context.physical,VK_FORMAT_B8G8R8A8_UNORM,&capture);
    if(!(source.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
       !(capture.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_DST_BIT) ||
       !(capture.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
      throw FormatError("Original cinematic capture format cannot be copied/sampled");
    const auto source_program=cinematic_crossfade_program(1.f,1.f);
    if(!source_program)throw FormatError("Original cinematic crossfade kernel identity unavailable");
    const std::string native_path(native_fragment_kernel_path("cinematic-crossfade",source_program->words));
    const auto vertex=shader("bloom.vert.spv");VkShaderModule fragment{};
    try{fragment=shader(native_path.c_str());}
    catch(...){vkDestroyShaderModule(context.device,vertex,nullptr);throw;}
    const std::array<VkPipelineShaderStageCreateInfo,2> stages{{
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment,"main",nullptr}}};
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=0xf;attachment.blendEnable=VK_TRUE;
    // Original142D0 mode7: ONE/INV_SRC_ALPHA/ADD. The actual PS emits
    // premultiplied sample RGB and its quantized final-combiner alpha.
    attachment.srcColorBlendFactor=attachment.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
    attachment.dstColorBlendFactor=attachment.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp=attachment.alphaBlendOp=VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
    const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};info.stageCount=2;info.pStages=stages.data();
    info.pVertexInputState=&input;info.pInputAssemblyState=&assembly;info.pViewportState=&viewport;info.pRasterizationState=&raster;
    info.pMultisampleState=&samples;info.pDepthStencilState=&depth;info.pColorBlendState=&blend;info.pDynamicState=&dynamic;info.layout=layout;info.renderPass=pass;
    VkPipeline replacement{};const auto result=vkCreateGraphicsPipelines(context.device,VK_NULL_HANDLE,1,&info,nullptr,&replacement);
    vkDestroyShaderModule(context.device,vertex,nullptr);vkDestroyShaderModule(context.device,fragment,nullptr);
    check(result,"Cinematic crossfade pipeline");
    if(pipeline)vkDestroyPipeline(context.device,pipeline,nullptr);pipeline=replacement;context.color_format=format;
  }
  explicit Impl(CinematicCrossfadeRenderContext c):context(std::move(c)) {
    try {
    size_history(context.extent);
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};si.magFilter=si.minFilter=VK_FILTER_NEAREST;si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    check(vkCreateSampler(context.device,&si,nullptr,&sampler),"Cinematic point/clamp sampler");
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=240;bi.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryRequirements r;VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    check(vkCreateBuffer(context.device,&bi,nullptr,&uniform),"Cinematic program buffer");vkGetBufferMemoryRequirements(context.device,uniform,&r);
    allocation.allocationSize=r.size;allocation.memoryTypeIndex=memory_type(r.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check(vkAllocateMemory(context.device,&allocation,nullptr,&uniform_memory),"Cinematic program memory");check(vkBindBufferMemory(context.device,uniform,uniform_memory,0),"Cinematic program binding");
    const VkDescriptorSetLayoutBinding bindings[]{{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
      {1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};li.bindingCount=2;li.pBindings=bindings;
    check(vkCreateDescriptorSetLayout(context.device,&li,nullptr,&set_layout),"Cinematic descriptor layout");
    const VkPushConstantRange dimensions{VK_SHADER_STAGE_FRAGMENT_BIT,0,8};
    VkPipelineLayoutCreateInfo pi{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pi.setLayoutCount=1;pi.pSetLayouts=&set_layout;
    pi.pushConstantRangeCount=1;pi.pPushConstantRanges=&dimensions;
    check(vkCreatePipelineLayout(context.device,&pi,nullptr,&layout),"Cinematic pipeline layout");
    const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1}};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool_info.maxSets=1;pool_info.poolSizeCount=2;pool_info.pPoolSizes=sizes;
    check(vkCreateDescriptorPool(context.device,&pool_info,nullptr,&pool),"Cinematic descriptor pool");
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=pool;ai.descriptorSetCount=1;ai.pSetLayouts=&set_layout;
    check(vkAllocateDescriptorSets(context.device,&ai,&descriptor),"Cinematic descriptor");
    VkDescriptorImageInfo image_info{sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkDescriptorBufferInfo buffer_info{uniform,0,240};
    std::array<VkWriteDescriptorSet,2> writes{};
    for(unsigned i=0;i<2;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=descriptor;writes[i].dstBinding=i;writes[i].descriptorCount=1;}
    writes[0].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[0].pImageInfo=&image_info;
    writes[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;writes[1].pBufferInfo=&buffer_info;
    vkUpdateDescriptorSets(context.device,2,writes.data(),0,nullptr);make_pipeline(context.color_format,context.color_pass);
    }catch(...){release();throw;}
  }
  void release() {
    for(auto handle:{pipeline})if(handle)vkDestroyPipeline(context.device,handle,nullptr);
    if(pool)vkDestroyDescriptorPool(context.device,pool,nullptr);
    if(layout)vkDestroyPipelineLayout(context.device,layout,nullptr);
    if(set_layout)vkDestroyDescriptorSetLayout(context.device,set_layout,nullptr);
    if(sampler)vkDestroySampler(context.device,sampler,nullptr);
    if(uniform)vkDestroyBuffer(context.device,uniform,nullptr);
    if(uniform_memory)vkFreeMemory(context.device,uniform_memory,nullptr);
    if(view)vkDestroyImageView(context.device,view,nullptr);
    if(history)vkDestroyImage(context.device,history,nullptr);
    if(image_memory)vkFreeMemory(context.device,image_memory,nullptr);
  }
  ~Impl(){release();}
  bool record(const CinematicCrossfadeRenderTarget &target) {
    if(!frame.capture && !frame.composite)return false;
    if(frame.composite && !initialized && !frame.capture)throw FormatError("Cinematic composite has no captured original scene");
    const auto command=target.command;vkCmdEndRenderPass(command);
    if(frame.capture) {
      // Startup preallocates the native scene size. Resize only on a new
      // capture; a window change during a fade must retain the outgoing scene.
      size_history(target.extent);
      std::array<VkImageMemoryBarrier,2> b{};
      for(auto &barrier:b){barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
      b[0].image=target.color;b[0].oldLayout=target.layout;b[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      b[0].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;b[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
      b[1].image=history;b[1].oldLayout=initialized?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
      b[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b[1].srcAccessMask=initialized?VK_ACCESS_SHADER_READ_BIT:0;b[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
      vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,b.data());
      VkImageBlit blit{};blit.srcSubresource=blit.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
      blit.srcOffsets[1]={int(target.extent.width),int(target.extent.height),1};
      blit.dstOffsets[1]={int(capture_extent.width),int(capture_extent.height),1};
      vkCmdBlitImage(command,target.color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,history,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_NEAREST);
      b[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b[0].newLayout=target.layout;b[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      b[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b[1].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;b[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,2,b.data());initialized=true;
    }
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderArea={{0,0},target.extent};
    const VkViewport viewport{0,0,float(target.extent.width),float(target.extent.height),0,1};const VkRect2D rectangle{{0,0},target.extent};
    if(frame.composite) {
      pass.renderPass=target.color_pass;pass.framebuffer=target.color_framebuffer;vkCmdBeginRenderPass(command,&pass,VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&rectangle);
      vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);
      const std::array<std::int32_t,2> dimensions{std::int32_t(target.extent.width),std::int32_t(target.extent.height)};
      vkCmdPushConstants(command,layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(dimensions),dimensions.data());
      vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
    }
    pass.renderPass=target.continuation_pass;pass.framebuffer=target.continuation_framebuffer;vkCmdBeginRenderPass(command,&pass,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&rectangle);return frame.capture;
  }
};
CinematicCrossfadeRender::CinematicCrossfadeRender(CinematicCrossfadeRenderContext context):impl_(std::make_unique<Impl>(std::move(context))){}
CinematicCrossfadeRender::~CinematicCrossfadeRender()=default;
void CinematicCrossfadeRender::reconfigure(VkFormat format,VkRenderPass pass){if(format!=impl_->context.color_format)impl_->make_pipeline(format,pass);}
void CinematicCrossfadeRender::prepare(const CinematicCrossfadeFrame &frame) {
  if(!frame.composite){impl_->frame=frame;return;}
  (void)native_fragment_kernel_path("cinematic-crossfade",frame.composite->words);
  const auto colors=native_fragment_uniform_words(frame.composite->words);
  void *memory{};check(vkMapMemory(impl_->context.device,impl_->uniform_memory,0,240,0,&memory),"Cinematic program map");
  std::memcpy(memory,colors.data(),240);vkUnmapMemory(impl_->context.device,impl_->uniform_memory);
  impl_->frame=frame;
}
bool CinematicCrossfadeRender::record(const CinematicCrossfadeRenderTarget &target){return impl_->record(target);}
}
