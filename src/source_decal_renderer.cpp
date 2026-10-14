#include "halo2/source_decal_renderer.hpp"
#include "halo2/sampler.hpp"
#include <cstring>
#include <fstream>
#include <map>
namespace halo2 {
namespace {
void check(VkResult r){if(r!=VK_SUCCESS)throw FormatError("cached decal Vulkan failure: "+std::to_string(r));}
VkBlendFactor factor(unsigned v){switch(v){case 0:return VK_BLEND_FACTOR_ZERO;case 1:return VK_BLEND_FACTOR_ONE;case 0x300:return VK_BLEND_FACTOR_SRC_COLOR;case 0x303:return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;case 0x306:return VK_BLEND_FACTOR_DST_COLOR;default:throw FormatError("Unsupported source decal blend factor");}}
}
struct SourceDecalRenderer::Impl {
    VkPhysicalDevice physical;VkDevice device;VkQueue queue;VkRenderPass pass;
    VkPhysicalDeviceProperties properties{};VkCommandPool uploads{};
    VkDescriptorSetLayout set_layout{};VkPipelineLayout layout{};
    std::array<VkShaderModule,3> shaders{};
    struct Buffer{VkBuffer handle{};VkDeviceMemory memory{};};
    struct Image{VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkSampler sampler{};};
    struct Uniform{std::array<ShaderVector,192> constants;ShaderVector color,viewport,clip,depth;std::array<std::uint32_t,60> program;};
    struct Vertex{Vec3 position;std::uint32_t projection;};
    struct Draw{VkDescriptorSet set{};VkPipeline pipeline{};std::uint32_t first{},count{},layer{};};
    std::map<const Texture*,Image> images;std::map<unsigned,VkPipeline> pipelines;
    Buffer vertices,uniforms;VkDescriptorPool descriptors{};std::vector<Draw> draws;
    Impl(VkPhysicalDevice p,VkDevice d,VkQueue q,unsigned family,VkRenderPass rp,const std::filesystem::path&directory):physical(p),device(d),queue(q),pass(rp){
        try{
        vkGetPhysicalDeviceProperties(physical,&properties);
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;check(vkCreateCommandPool(device,&pool,nullptr,&uploads));
        const std::array<VkDescriptorSetLayoutBinding,2> bindings{{{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},{2,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}}};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set.bindingCount=bindings.size();set.pBindings=bindings.data();check(vkCreateDescriptorSetLayout(device,&set,nullptr,&set_layout));
        VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};li.setLayoutCount=1;li.pSetLayouts=&set_layout;check(vkCreatePipelineLayout(device,&li,nullptr,&layout));
        const char*names[]{"source_decal.vert.spv","source_decal.geom.spv","source_decal.frag.spv"};
        for(unsigned n=0;n<3;++n){std::ifstream f(directory/names[n],std::ios::binary|std::ios::ate);if(!f||f.tellg()<=0||f.tellg()%4)throw FormatError("Compiled source decal shader missing");std::vector<std::uint32_t> code(std::size_t(f.tellg())/4);f.seekg(0);if(!f.read(reinterpret_cast<char*>(code.data()),code.size()*4))throw FormatError("Truncated source decal shader");VkShaderModuleCreateInfo si{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};si.codeSize=code.size()*4;si.pCode=code.data();check(vkCreateShaderModule(device,&si,nullptr,&shaders[n]));}
        }catch(...){release();throw;}
    }
  unsigned memory_type(unsigned mask,VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(unsigned n=0;n<p.memoryTypeCount;++n)if((mask&(1u<<n)) && (p.memoryTypes[n].propertyFlags&flags)==flags)return n;
    throw FormatError("cached decal graphics memory unavailable");
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
  Image make_image(const Texture &source,const ShaderTextureState &state) {
    if((source.format!=11 && source.format!=14 && source.format!=15 && source.format!=16) || source.layers!=1 || source.depth || source.mips.empty())throw FormatError("cached decal upload requires original-decoded RGBA8 mips");
    if(state.alpha_kill || state.color_key_op || state.color_sign)throw FormatError("unsupported original cached decal texture operation");
    auto sampler=native_sampler(state,properties.limits,false);
    if(sampler.custom_border)throw FormatError("cached decal custom sampler border requires original device support");
    Image image;Buffer staging{};VkCommandBuffer command{};
    try {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;
    ci.format=source.format==14?VK_FORMAT_BC1_RGBA_UNORM_BLOCK:source.format==15?VK_FORMAT_BC2_UNORM_BLOCK:source.format==16?VK_FORMAT_BC3_UNORM_BLOCK:VK_FORMAT_R8G8B8A8_UNORM;ci.extent={source.width,source.height,1};ci.mipLevels=source.mips.size();ci.arrayLayers=1;
    ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    check(vkCreateImage(device,&ci,nullptr,&image.handle));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,image.handle,&req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(vkAllocateMemory(device,&ai,nullptr,&image.memory));check(vkBindImageMemory(device,image.handle,image.memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image.handle;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;
    vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,ci.mipLevels,0,1};check(vkCreateImageView(device,&vi,nullptr,&image.view));
    check(vkCreateSampler(device,&sampler.info,nullptr,&image.sampler));
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
    VkPipeline pipeline(unsigned layer){
        if(auto i=pipelines.find(layer);i!=pipelines.end())return i->second;
        const auto state=source_decal_layer_state_tu15(layer);
        const VkPipelineShaderStageCreateInfo stages[]{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,shaders[0],"main",nullptr},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_GEOMETRY_BIT,shaders[1],"main",nullptr},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,shaders[2],"main",nullptr}};
        VkVertexInputBindingDescription binding{0,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[]{{0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex,position)},{1,0,VK_FORMAT_R32_UINT,offsetof(Vertex,projection)}};
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&binding;vi.vertexAttributeDescriptionCount=2;vi.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo vs{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vs.viewportCount=vs.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.depthClampEnable=VK_TRUE;rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.cullMode=state.cull==0x901?VK_CULL_MODE_FRONT_BIT:VK_CULL_MODE_BACK_BIT;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=VK_TRUE;ds.depthWriteEnable=VK_FALSE;ds.depthCompareOp=VkCompareOp(state.depth_function-0x200);
        VkPipelineColorBlendAttachmentState cb{};cb.blendEnable=VK_TRUE;cb.srcColorBlendFactor=cb.srcAlphaBlendFactor=factor(state.source_blend);cb.dstColorBlendFactor=cb.dstAlphaBlendFactor=factor(state.destination_blend);cb.colorBlendOp=cb.alphaBlendOp=state.blend_equation==0x8008?VK_BLEND_OP_MAX:VK_BLEND_OP_ADD;constexpr unsigned shift[]{16,8,0,24};for(unsigned c=0;c<4;++c)if((state.color_mask>>shift[c])&1u)cb.colorWriteMask|=1u<<c;
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
        const VkDynamicState dynamic[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo di{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};di.dynamicStateCount=2;di.pDynamicStates=dynamic;
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=3;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vs;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pDepthStencilState=&ds;pi.pColorBlendState=&bs;pi.pDynamicState=&di;pi.layout=layout;pi.renderPass=pass;VkPipeline result{};check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,nullptr,&result));pipelines.emplace(layer,result);return result;
    }
    void clear(){free_buffer(vertices);free_buffer(uniforms);vertices={};uniforms={};draws.clear();if(descriptors)vkDestroyDescriptorPool(device,descriptors,nullptr);descriptors=VK_NULL_HANDLE;}
    void prepare(std::span<const SourceDecalDraw> source){
        clear();if(source.empty())return;
        const auto alignment=std::max<VkDeviceSize>(1,properties.limits.minUniformBufferOffsetAlignment);const auto stride=(sizeof(Uniform)+alignment-1)/alignment*alignment;
        std::vector<std::byte> constants(stride*source.size());std::vector<Vertex> geometry;draws.resize(source.size());
        const std::array<VkDescriptorPoolSize,2> sizes{{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,std::uint32_t(source.size())},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,std::uint32_t(source.size())}}};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=source.size();pool.poolSizeCount=2;pool.pPoolSizes=sizes.data();check(vkCreateDescriptorPool(device,&pool,nullptr,&descriptors));
        std::vector<VkDescriptorSetLayout> layouts(source.size(),set_layout);std::vector<VkDescriptorSet> sets(source.size());VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ai.descriptorPool=descriptors;ai.descriptorSetCount=sets.size();ai.pSetLayouts=layouts.data();check(vkAllocateDescriptorSets(device,&ai,sets.data()));
        for(std::size_t n=0;n<source.size();++n){const auto &d=source[n];if(!d.texture)throw FormatError("Source decal texture owner missing");auto [at,created]=images.try_emplace(d.texture);if(created){ShaderTextureState sampler;sampler.address_u=sampler.address_v=sampler.address_w=3;at->second=make_image(*d.texture,sampler);}auto &g=draws[n];g.set=sets[n];g.layer=d.layer;g.pipeline=pipeline(d.layer);g.first=geometry.size();for(const auto &quad:d.quads)for(unsigned i:{0u,1u,2u,0u,2u,3u})geometry.push_back({quad[i].position,quad[i].packed_projection});g.count=geometry.size()-g.first;Uniform uniform{d.constants,d.color,d.viewport,d.clip_range,{0,d.state.depth_bias,1,1},d.state.pixel.words};std::memcpy(constants.data()+stride*n,&uniform,sizeof(uniform));}
        if(geometry.empty())return;
        vertices=make_buffer(geometry.size()*sizeof(Vertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,geometry.data());uniforms=make_buffer(constants.size(),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,constants.data());
        for(std::size_t n=0;n<source.size();++n){const auto &image=images.at(source[n].texture);VkDescriptorImageInfo ii{image.sampler,image.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};VkDescriptorBufferInfo bi{uniforms.handle,n*stride,sizeof(Uniform)};const std::array<VkWriteDescriptorSet,2> writes{{{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,sets[n],0,0,1,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,&ii,nullptr,nullptr},{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,nullptr,sets[n],2,0,1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,nullptr,&bi,nullptr}}};vkUpdateDescriptorSets(device,writes.size(),writes.data(),0,nullptr);}
    }
    void record(VkCommandBuffer command,VkViewport viewport,VkRect2D scissor,std::size_t first,std::size_t count,std::uint32_t layers)const{
        if(first>draws.size()||count>draws.size()-first)throw FormatError("Cached decal draw range exceeds prepared frame");if(!count||!vertices.handle)return;const VkDeviceSize zero=0;vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);vkCmdBindVertexBuffers(command,0,1,&vertices.handle,&zero);for(std::size_t n=first;n<first+count;++n){const auto &d=draws[n];if(!d.count || !(layers&(1u<<d.layer)))continue;vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,d.pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&d.set,0,nullptr);vkCmdDraw(command,d.count,1,d.first,0);}
    }
    void release(){clear();for(auto [p,i]:images){if(i.sampler)vkDestroySampler(device,i.sampler,nullptr);if(i.view)vkDestroyImageView(device,i.view,nullptr);if(i.handle)vkDestroyImage(device,i.handle,nullptr);if(i.memory)vkFreeMemory(device,i.memory,nullptr);}for(auto [n,p]:pipelines)if(p)vkDestroyPipeline(device,p,nullptr);for(auto s:shaders)if(s)vkDestroyShaderModule(device,s,nullptr);if(layout)vkDestroyPipelineLayout(device,layout,nullptr);if(set_layout)vkDestroyDescriptorSetLayout(device,set_layout,nullptr);if(uploads)vkDestroyCommandPool(device,uploads,nullptr);}
    ~Impl(){release();}
};
SourceDecalRenderer::SourceDecalRenderer(VkPhysicalDevice p,VkDevice d,VkQueue q,unsigned f,VkRenderPass r,const std::filesystem::path&s):impl_(std::make_unique<Impl>(p,d,q,f,r,s)){}
SourceDecalRenderer::~SourceDecalRenderer()=default;
void SourceDecalRenderer::prepare(std::span<const SourceDecalDraw>d){impl_->prepare(d);}
void SourceDecalRenderer::record(VkCommandBuffer c,VkViewport v,VkRect2D s,std::size_t f,std::size_t n,std::uint32_t m)const{impl_->record(c,v,s,f,n,m);}
}
