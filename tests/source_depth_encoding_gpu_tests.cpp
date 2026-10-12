#include "source_weather_plate_gpu_fixture.hpp"
#include <bit>
#include <cmath>
#include <filesystem>
#include <map>
#include <span>
#include <sstream>

namespace {
using namespace halo2_weather_plate_fixture;
constexpr std::uint32_t domain=1u<<24,maximum=domain-1;
const std::filesystem::path shader_dir=HALO2_DEPTH_ENCODING_DIR;
struct Push {std::uint32_t width,base,input_mode,expression,source_control=1,w_mode=0x3f800000,count,unused=0;};
static_assert(sizeof(Push)==32);
struct Storage {VkDeviceSize color,depth,stencil;std::uint32_t pixels;};
Storage storage(VkExtent2D extent) {
    require(extent.width&&extent.height,"depth diagnostic empty extent");
    const auto pixels=std::uint64_t(extent.width)*extent.height;
    require(pixels<=domain,"depth diagnostic extent exceeds exhaustive domain");
    return {pixels*4,pixels*4,pixels,static_cast<std::uint32_t>(pixels)};
}
std::vector<std::uint32_t> words(const std::filesystem::path &path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    require(bool(file)&&file.tellg()>0&&std::uint64_t(file.tellg())%4==0,"depth diagnostic input unavailable");
    std::vector<std::uint32_t> result(std::size_t(file.tellg())/4);file.seekg(0);
    require(bool(file.read(reinterpret_cast<char*>(result.data()),result.size()*4)),"depth diagnostic input incomplete");return result;
}
void save(const std::filesystem::path &path,const void *data,std::size_t bytes,bool append=false) {
    std::ofstream file(path,std::ios::binary|(append?std::ios::app:std::ios::trunc));
    require(bool(file.write(static_cast<const char*>(data),bytes)),"depth diagnostic output failed");
}
VkShaderModule module(Gpu &g,const char *name) {
    const auto code=words(shader_dir/name);VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize=code.size()*4;ci.pCode=code.data();VkShaderModule result{};
    check(vkCreateShaderModule(g.device,&ci,nullptr,&result));g.modules.push_back(result);return result;
}
VkRenderPass pass(Gpu &g,VkFormat format) {
    VkAttachmentDescription attachments[2]{};
    for(unsigned i=0;i<2;++i){auto &a=attachments[i];a.format=i?VK_FORMAT_D24_UNORM_S8_UINT:format;a.samples=VK_SAMPLE_COUNT_1_BIT;
        a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp=i?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.stencilStoreOp=i?VK_ATTACHMENT_STORE_OP_STORE:VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;a.finalLayout=i?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;}
    const VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;sub.pDepthStencilAttachment=&depth;
    const VkSubpassDependency deps[]{
        {VK_SUBPASS_EXTERNAL,0,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
         VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_DEPENDENCY_BY_REGION_BIT},
        {0,VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
         VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,0}};
    VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ci.attachmentCount=2;ci.pAttachments=attachments;ci.subpassCount=1;ci.pSubpasses=&sub;ci.dependencyCount=2;ci.pDependencies=deps;
    VkRenderPass result{};check(vkCreateRenderPass(g.device,&ci,nullptr,&result));g.passes.push_back(result);return result;
}
VkPipeline pipeline(Gpu &g,VkRenderPass render_pass,VkShaderModule vertex,VkShaderModule fragment,VkCompareOp compare,bool writes,bool rgb_only) {
    const VkPipelineShaderStageCreateInfo stages[]{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vertex,"main",nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,fragment,"main",nullptr}};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=VK_TRUE;ds.depthWriteEnable=writes;ds.depthCompareOp=compare;
    // Stencil is native initialized state; this encoding test never writes it.
    ds.stencilTestEnable=VK_FALSE;
    VkPipelineColorBlendAttachmentState color{};color.colorWriteMask=rgb_only?7u:15u;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&color;
    const VkDynamicState dynamic[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dyn.dynamicStateCount=2;dyn.pDynamicStates=dynamic;
    VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};ci.stageCount=2;ci.pStages=stages;ci.pVertexInputState=&vi;ci.pInputAssemblyState=&ia;ci.pViewportState=&vp;ci.pRasterizationState=&raster;ci.pMultisampleState=&ms;ci.pDepthStencilState=&ds;ci.pColorBlendState=&blend;ci.pDynamicState=&dyn;ci.layout=g.layout;ci.renderPass=render_pass;
    VkPipeline result{};check(vkCreateGraphicsPipelines(g.device,{},1,&ci,nullptr,&result));g.pipelines.push_back(result);return result;
}
struct Readback {Buffer color,depth,stencil;};
Readback buffers(Gpu &g,VkExtent2D extent) {
    const auto size=storage(extent);return {g.buffer(size.color,VK_BUFFER_USAGE_TRANSFER_DST_BIT),g.buffer(size.depth,VK_BUFFER_USAGE_TRANSFER_DST_BIT),g.buffer(size.stencil,VK_BUFFER_USAGE_TRANSFER_DST_BIT)};
}
void copied(Gpu &g,Image color,Image depth,VkExtent2D extent,const Readback &out) {
    VkImageMemoryBarrier image[2]{};
    for(auto &b:image){b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;}
    image[0].image=color.handle;image[0].oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;image[0].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;image[0].subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    image[1].image=depth.handle;image[1].oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;image[1].srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;image[1].subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,0,1,0,1};
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,image);
    const VkImageAspectFlags aspects[]{VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_ASPECT_STENCIL_BIT};
    const Buffer destination[]{out.color,out.depth,out.stencil};
    for(unsigned i=0;i<3;++i){VkBufferImageCopy region{};region.imageSubresource={aspects[i],0,0,1};region.imageExtent={extent.width,extent.height,1};vkCmdCopyImageToBuffer(g.command,i?depth.handle:color.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,destination[i].handle,1,&region);}
    for(auto &b:image){std::swap(b.oldLayout,b.newLayout);b.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;}
    image[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;image[1].dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,nullptr,0,nullptr,2,image);
    VkBufferMemoryBarrier host[3]{};for(unsigned i=0;i<3;++i){auto &b=host[i];b.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_HOST_READ_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.buffer=destination[i].handle;b.size=destination[i].size;}
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,3,host,0,nullptr);
}
void submitted(Gpu &g) {
    check(vkEndCommandBuffer(g.command));VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence fence{};check(vkCreateFence(g.device,&ci,nullptr,&fence));
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;
    const auto result=vkQueueSubmit(g.queue,1,&submit,fence);
    if(result!=VK_SUCCESS){vkDestroyFence(g.device,fence,nullptr);check(result);}
    const auto waited=vkWaitForFences(g.device,1,&fence,VK_TRUE,std::numeric_limits<std::uint64_t>::max());vkDestroyFence(g.device,fence,nullptr);check(waited);
}
void render(Gpu &g,VkRenderPass render_pass,VkFramebuffer frame,VkPipeline selected,VkDescriptorSet descriptors,
            Image color,Image depth,VkExtent2D extent,Push push,const Readback &out,float clear_depth,bool integer_color,bool draw=true) {
    storage(extent);require(push.width==extent.width&&push.source_control==1&&push.w_mode==0x3f800000,"depth diagnostic source integer branch required");
    g.begin();VkClearValue clear[2]{};
    if(integer_color)clear[0].color.uint32[0]=0xbaadf00d;
    else {clear[0].color.float32[0]=17.f/255;clear[0].color.float32[1]=34.f/255;clear[0].color.float32[2]=68.f/255;clear[0].color.float32[3]=127.f/255;}
    clear[1].depthStencil={clear_depth,0x5a};
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=render_pass;begin.framebuffer=frame;begin.renderArea={{0,0},extent};begin.clearValueCount=2;begin.pClearValues=clear;
    vkCmdBeginRenderPass(g.command,&begin,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,selected);
    const VkViewport vp{0,0,float(extent.width),float(extent.height),0,1};const VkRect2D scissor{{0,0},extent};vkCmdSetViewport(g.command,0,1,&vp);vkCmdSetScissor(g.command,0,1,&scissor);
    vkCmdBindDescriptorSets(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,g.layout,0,1,&descriptors,0,nullptr);vkCmdPushConstants(g.command,g.layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push),&push);if(draw)vkCmdDraw(g.command,3,1,0,0);vkCmdEndRenderPass(g.command);
    copied(g,color,depth,extent,out);submitted(g);
    const auto *stencil=static_cast<const std::uint8_t*>(out.stencil.mapped);
    require(std::all_of(stencil,stencil+storage(extent).pixels,[](auto v){return v==0x5a;}),"depth diagnostic changed initialized stencil");
}
struct Capabilities {VkExtent2D exhaustive;VkPhysicalDeviceProperties properties;};
Capabilities capabilities(Gpu &g) {
    Capabilities c{};vkGetPhysicalDeviceProperties(g.physical,&c.properties);
    require(c.properties.apiVersion>=VK_API_VERSION_1_1,"depth diagnostic requires Vulkan1.1");
    require(c.properties.limits.maxPushConstantsSize>=sizeof(Push),"depth diagnostic push limit");
    unsigned width=4096,height=4096;
    for(const auto format:{VK_FORMAT_D24_UNORM_S8_UINT,VK_FORMAT_R32_UINT,VK_FORMAT_R8G8B8A8_UNORM}) {
        const bool depth=format==VK_FORMAT_D24_UNORM_S8_UINT;
        const VkImageUsageFlags usage=(depth?VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkFormatProperties props{};vkGetPhysicalDeviceFormatProperties(g.physical,format,&props);
        const VkFormatFeatureFlags features=(depth?VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT:VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        require((props.optimalTilingFeatures&features)==features,"depth diagnostic required format features absent");
        VkImageFormatProperties image{};check(vkGetPhysicalDeviceImageFormatProperties(g.physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&image));
        require((image.sampleCounts&VK_SAMPLE_COUNT_1_BIT)&&image.maxMipLevels&&image.maxArrayLayers,"depth diagnostic image shape unsupported");
        if(format!=VK_FORMAT_R8G8B8A8_UNORM){width=std::min(width,image.maxExtent.width);height=std::min(height,image.maxExtent.height);
            while(std::uint64_t(width)*height*4>image.maxResourceSize){require(height>1,"depth diagnostic resource size unavailable");height/=2;}}
    }
    width=std::min({width,c.properties.limits.maxFramebufferWidth,c.properties.limits.maxViewportDimensions[0]});height=std::min({height,c.properties.limits.maxFramebufferHeight,c.properties.limits.maxViewportDimensions[1]});
    require(width&&height,"depth diagnostic image/viewport limits empty");
    c.exhaustive={std::bit_floor(width),std::bit_floor(height)};require(domain%storage(c.exhaustive).pixels==0,"depth diagnostic tiled coverage not exact");
    return c;
}
VkDescriptorSet descriptors(Gpu &g,Buffer cases) {
    VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
    VkDescriptorSetLayoutCreateInfo l{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};l.bindingCount=1;l.pBindings=&binding;check(vkCreateDescriptorSetLayout(g.device,&l,nullptr,&g.descriptor_layout));
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(Push)};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&g.descriptor_layout;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;check(vkCreatePipelineLayout(g.device,&pl,nullptr,&g.layout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&size;check(vkCreateDescriptorPool(g.device,&pool,nullptr,&g.descriptors));
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=g.descriptors;allocate.descriptorSetCount=1;allocate.pSetLayouts=&g.descriptor_layout;VkDescriptorSet set{};check(vkAllocateDescriptorSets(g.device,&allocate,&set));
    const VkDescriptorBufferInfo info{cases.handle,0,cases.size};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=set;write.dstBinding=0;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&info;vkUpdateDescriptorSets(g.device,1,&write,0,nullptr);return set;
}
struct Summary {std::uint64_t tested{},depth_mismatches{},float_mismatches{},padding_nonzero{};std::map<int,std::uint64_t> depth_delta;};
void observed(Summary &s,const Readback &out,std::uint32_t base,unsigned count,unsigned expression) {
    const auto *depth=static_cast<const std::uint32_t*>(out.depth.mapped),*floating=static_cast<const std::uint32_t*>(out.color.mapped);
    for(unsigned i=0;i<count;++i){const auto n=base+i;require(floating[i]!=0xbaadf00d,"depth diagnostic pixel not covered");
        const auto actual=depth[i]&maximum;const float w=float(n);
        const auto expected=expression?std::bit_cast<std::uint32_t>(w/16777216.f)+1:std::bit_cast<std::uint32_t>(w/16777215.f);
        ++s.tested;s.depth_mismatches+=actual!=n;s.float_mismatches+=floating[i]!=expected;s.padding_nonzero+=(depth[i]>>24)!=0;++s.depth_delta[int(actual)-int(n)];}
}
void summary(std::ofstream &file,const char *name,const Summary &s) {
    file<<name<<'\t'<<s.tested<<'\t'<<s.depth_mismatches<<'\t'<<s.float_mismatches<<'\t'<<s.padding_nonzero<<'\n';
    for(auto [delta,count]:s.depth_delta)file<<name<<"-delta\t"<<delta<<'\t'<<count<<'\n';
}
void cpu() {
    const auto values=words(shader_dir/"fractional-w.bin"),compare=words(shader_dir/"compare-w.bin");
    require(values.size()==30&&compare.size()==8,"depth diagnostic controlled corpus changed");
    for(auto word:values)require(std::isfinite(std::bit_cast<float>(word)),"depth diagnostic nonfinite input");
    require(storage({4096,4096}).pixels==domain,"depth diagnostic exhaustive domain incomplete");
    unsigned rejects=0;for(const auto bad:{VkExtent2D{0,1},VkExtent2D{1,0},VkExtent2D{4097,4096},VkExtent2D{0xffffffffu,0xffffffffu}}){try{storage(bad);}catch(const std::exception&){++rejects;}}
    require(rejects==4,"depth diagnostic extent guards incomplete");std::cout<<"CPU:30 finite fractional/clamp inputs,8 comparison inputs,16777216 integer domain,4 extent guards; no Vulkan calls\n";
}
void run(Gpu &g,const std::filesystem::path &output) {
    const auto caps=capabilities(g);const auto extent=caps.exhaustive;const auto pixels=storage(extent).pixels;
    std::filesystem::create_directories(output);std::ofstream report(output/"measurements.tsv");require(bool(report),"depth diagnostic report unavailable");
    report<<"device\t"<<caps.properties.deviceName<<"\ndriver\t"<<caps.properties.driverVersion<<"\napi\t"<<caps.properties.apiVersion<<"\nextent\t"<<extent.width<<'\t'<<extent.height<<"\nnative-stencil-clear\t90\n";
    const auto fractional=words(shader_dir/"fractional-w.bin"),compare=words(shader_dir/"compare-w.bin");
    const auto input=g.buffer(std::max(fractional.size(),compare.size())*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);std::memcpy(input.mapped,fractional.data(),fractional.size()*4);
    const auto set=descriptors(g,input);const auto vertex=module(g,"source_depth_encoding.vert.spv"),fragment=module(g,"source_depth_encoding.frag.spv"),color_fragment=module(g,"source_depth_encoding_color.frag.spv");
    const auto integer_pass=pass(g,VK_FORMAT_R32_UINT),rgba_pass=pass(g,VK_FORMAT_R8G8B8A8_UNORM);
    const auto integer_pipeline=pipeline(g,integer_pass,vertex,fragment,VK_COMPARE_OP_ALWAYS,true,false);
    const auto color=g.image(extent,VK_FORMAT_R32_UINT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const auto frame=g.framebuffer(integer_pass,extent,color,depth);const auto out=buffers(g,extent);Summary results[2];
    for(unsigned expression=0;expression<2;++expression) {
        const std::string name=expression?"source":"current";
        for(std::uint32_t base=0;base<domain;base+=pixels){render(g,integer_pass,frame,integer_pipeline,set,color,depth,extent,{extent.width,base,0,expression,1,0x3f800000,pixels},out,0.f,true);observed(results[expression],out,base,pixels,expression);
            std::vector<std::uint32_t> low(pixels);const auto *raw=static_cast<const std::uint32_t*>(out.depth.mapped);for(unsigned i=0;i<pixels;++i)low[i]=raw[i]&maximum;
            save(output/(name+"-depth-low24.bin"),low.data(),low.size()*4,base!=0);save(output/(name+"-emitted-float-words.bin"),out.color.mapped,pixels*4,base!=0);}
        summary(report,name.c_str(),results[expression]);require(results[expression].tested==domain,"depth diagnostic coverage incomplete");
    }
    const VkExtent2D small{static_cast<unsigned>(fractional.size()),1};
    const auto small_color=g.image(small,VK_FORMAT_R32_UINT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),small_depth=g.image(small,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const auto small_frame=g.framebuffer(integer_pass,small,small_color,small_depth);const auto small_out=buffers(g,small);
    for(unsigned expression=0;expression<2;++expression){render(g,integer_pass,small_frame,integer_pipeline,set,small_color,small_depth,small,{small.width,0,1,expression,1,0x3f800000,small.width},small_out,0.f,true);
        const auto *d=static_cast<const std::uint32_t*>(small_out.depth.mapped),*f=static_cast<const std::uint32_t*>(small_out.color.mapped);
        for(unsigned i=0;i<small.width;++i)report<<"fractional\t"<<expression<<'\t'<<i<<'\t'<<fractional[i]<<'\t'<<(d[i]&maximum)<<'\t'<<f[i]<<'\n';}
    std::memcpy(input.mapped,compare.data(),compare.size()*4);
    const VkExtent2D ce{8,1};const auto cc=g.image(ce,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),cd=g.image(ce,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    const auto cf=g.framebuffer(rgba_pass,ce,cc,cd);const auto co=buffers(g,ce);
    const VkCompareOp comparators[]{VK_COMPARE_OP_ALWAYS,VK_COMPARE_OP_NEVER,VK_COMPARE_OP_EQUAL,VK_COMPARE_OP_LESS_OR_EQUAL};
    for(auto comparator:comparators){const auto p=pipeline(g,rgba_pass,vertex,color_fragment,comparator,false,true);
        const float clear=comparator==VK_COMPARE_OP_LESS_OR_EQUAL?.5f:1.f;
        render(g,rgba_pass,cf,p,set,cc,cd,ce,{8,0,1,0,1,0x3f800000,8},co,clear,false,false);
        std::array<std::uint32_t,8> reference_depth{};
        const auto *cleared=static_cast<const std::uint32_t*>(co.depth.mapped);
        for(unsigned i=0;i<8;++i)reference_depth[i]=cleared[i]&maximum;
        for(unsigned expression=0;expression<2;++expression){const float clear=comparator==VK_COMPARE_OP_LESS_OR_EQUAL?.5f:1.f;
            render(g,rgba_pass,cf,p,set,cc,cd,ce,{8,0,1,expression,1,0x3f800000,8},co,clear,false);
            const auto *rgba=static_cast<const std::uint8_t*>(co.color.mapped);const auto *d=static_cast<const std::uint32_t*>(co.depth.mapped);unsigned mask=0;
            for(unsigned i=0;i<8;++i){require(rgba[4*i+3]==127,"depth comparator changed alpha");
                const bool changed=rgba[4*i]==51&&rgba[4*i+1]==102&&rgba[4*i+2]==153;
                require(changed||(rgba[4*i]==17&&rgba[4*i+1]==34&&rgba[4*i+2]==68),"depth comparator unexpected RGB");mask|=unsigned(changed)<<i;
                require((d[i]&maximum)==reference_depth[i],"no-write depth comparator changed separately cleared reference depth");}
            if(comparator==VK_COMPARE_OP_ALWAYS)require(mask==255,"ALWAYS comparator did not pass all");
            if(comparator==VK_COMPARE_OP_NEVER)require(mask==0,"NEVER comparator passed");
            if(comparator==VK_COMPARE_OP_EQUAL)require((mask&0xa3)==0xa0,"EQUAL endpoint controls failed");
            if(comparator==VK_COMPARE_OP_LESS_OR_EQUAL)require((mask&0xf7)==0x47,"LEQUAL separated endpoint controls failed");
            report<<"compare\t"<<unsigned(comparator)<<'\t'<<expression<<'\t'<<mask<<'\t'<<(d[0]&maximum)<<'\n';}
    }
    report<<"validation-errors\t"<<validation_errors<<'\n';report.flush();require(validation_errors==0,"depth diagnostic SDK errors");
    require(results[1].depth_mismatches==0,"pinned encoding did not preserve all D24 integers; diagnostic outputs retained");
    std::cout<<"GPU measured every D24 integer twice; source depth mismatches="<<results[1].depth_mismatches<<" current="<<results[0].depth_mismatches<<"; R32 float companion,30 fractional/clamp cases,8 actual comparator masks, native initialized stencil/alpha preserved; SDK errors0\n";
}
}
int main(int argc,char **argv){try {
    require((argc==2&&std::string(argv[1])=="--cpu")||(argc==3&&std::string(argv[1])=="--gpu"),"depth diagnostic --cpu or --gpu output_directory expected");cpu();if(argc==2)return 0;
    Gpu g;if(!g.initialize())return 77;run(g,argv[2]);return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
