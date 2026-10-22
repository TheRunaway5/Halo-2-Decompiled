#include "../depth_storage/gpu.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <nlohmann/json.hpp>
using nlohmann::json;
namespace {
constexpr VkExtent2D extent{256,256};
constexpr unsigned count=extent.width*extent.height;
constexpr unsigned sentinel=0xdeadbeefu;
struct Format {const char *name;VkFormat native,padded;unsigned components,bytes;bool integer;};
const std::array<Format,9> formats{{
    {"float32x3",VK_FORMAT_R32G32B32_SFLOAT,VK_FORMAT_R32G32B32A32_SFLOAT,3,4,false},
    {"float32x2",VK_FORMAT_R32G32_SFLOAT,VK_FORMAT_R32G32B32A32_SFLOAT,2,4,false},
    {"snorm16x3",VK_FORMAT_R16G16B16_SNORM,VK_FORMAT_R16G16B16A16_SNORM,3,2,false},
    {"snorm16x2",VK_FORMAT_R16G16_SNORM,VK_FORMAT_R16G16B16A16_SNORM,2,2,false},
    {"packed_signed_11_11_10",VK_FORMAT_R32_SINT,VK_FORMAT_R32G32B32A32_SINT,1,4,true},
    {"unorm8x1",VK_FORMAT_R8_UNORM,VK_FORMAT_R8G8B8A8_UNORM,1,1,false},
    {"unorm8x2",VK_FORMAT_R8G8_UNORM,VK_FORMAT_R8G8B8A8_UNORM,2,1,false},
    {"unorm8x3",VK_FORMAT_R8G8B8_UNORM,VK_FORMAT_R8G8B8A8_UNORM,3,1,false},
    {"unorm8x4",VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R8G8B8A8_UNORM,4,1,false}
}};
void word(std::uint8_t *data,unsigned size,unsigned value){for(unsigned i=0;i<size;++i)data[i]=value>>(i*8);}
unsigned component(unsigned format,unsigned vertex,unsigned lane){
    if(format<2){const std::array special{0u,0x80000000u,1u,0x007fffffu,0x00800000u,0x3f800000u,0x7f7fffffu,0x7f800000u,0xff800000u,0x7fc00000u,0x7f800001u,0xffc01234u};if(vertex<special.size())return special[(vertex+lane)%special.size()];return (vertex*0x9e3779b9u)^(lane*0x85ebca6bu);}
    if(format<4)return (vertex+lane*7919u)&65535u;
    if(format==4)return (vertex*0x9e3779b9u)^(vertex<<16);
    return (vertex+lane*71u)&255u;
}
struct Pass {
    Gpu &g;std::array<Gpu::Image,2> images{};std::array<VkImageView,2> views{};std::array<Gpu::Buffer,2> output{};
    VkRenderPass pass{};VkFramebuffer frame{};VkDescriptorSetLayout descriptor_layout{};VkPipelineLayout layout{};VkDescriptorPool pool{};VkDescriptorSet descriptor{};
    std::array<VkShaderModule,3> modules{};bool initialized{};
    explicit Pass(Gpu &gpu):g(gpu){}
    void initialize(){
        VkFormatProperties supported{};vkGetPhysicalDeviceFormatProperties(g.physical,VK_FORMAT_R32G32B32A32_UINT,&supported);require(supported.optimalTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT,"RGBA32 integer color unsupported");
        std::array<VkAttachmentDescription,2> attachments{};std::array<VkAttachmentReference,2> refs{};
        for(unsigned i=0;i<2;++i){images[i]=g.image(extent,VK_FORMAT_R32G32B32A32_UINT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);output[i]=g.buffer(count*16);VkImageViewCreateInfo vc{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vc.image=images[i].handle;vc.viewType=VK_IMAGE_VIEW_TYPE_2D;vc.format=VK_FORMAT_R32G32B32A32_UINT;vc.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};check(vkCreateImageView(g.device,&vc,nullptr,&views[i]));auto &a=attachments[i];a.format=vc.format;a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a.initialLayout=a.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;refs[i]={i,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};}
        VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=2;sub.pColorAttachments=refs.data();VkRenderPassCreateInfo rc{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rc.attachmentCount=2;rc.pAttachments=attachments.data();rc.subpassCount=1;rc.pSubpasses=&sub;check(vkCreateRenderPass(g.device,&rc,nullptr,&pass));
        VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fc.renderPass=pass;fc.attachmentCount=2;fc.pAttachments=views.data();fc.width=extent.width;fc.height=extent.height;fc.layers=1;check(vkCreateFramebuffer(g.device,&fc,nullptr,&frame));
        const VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr};VkDescriptorSetLayoutCreateInfo dc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dc.bindingCount=1;dc.pBindings=&binding;check(vkCreateDescriptorSetLayout(g.device,&dc,nullptr,&descriptor_layout));
        const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT,0,8};VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.setLayoutCount=1;lc.pSetLayouts=&descriptor_layout;lc.pushConstantRangeCount=1;lc.pPushConstantRanges=&push;check(vkCreatePipelineLayout(g.device,&lc,nullptr,&layout));
        const VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.maxSets=1;pc.poolSizeCount=1;pc.pPoolSizes=&ps;check(vkCreateDescriptorPool(g.device,&pc,nullptr,&pool));VkDescriptorSetAllocateInfo ac{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ac.descriptorPool=pool;ac.descriptorSetCount=1;ac.pSetLayouts=&descriptor_layout;check(vkAllocateDescriptorSets(g.device,&ac,&descriptor));
        modules={g.shader("fetch.vert.spv"),g.shader("fetch_integer.vert.spv"),g.shader("fetch.frag.spv")};
    }
    VkPipeline pipeline(unsigned format,VkFormat native,unsigned stride){
        const VkSpecializationMapEntry entry{0,0,4};const VkSpecializationInfo info{1,&entry,4,&format};
        const std::array<VkPipelineShaderStageCreateInfo,2> stages{{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,modules[formats[format].integer?1:0],"main",&info},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,modules[2],"main",nullptr}}};
        const VkVertexInputBindingDescription vb{0,stride,VK_VERTEX_INPUT_RATE_VERTEX};const VkVertexInputAttributeDescription va{0,0,native,0};VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=vi.vertexAttributeDescriptionCount=1;vi.pVertexBindingDescriptions=&vb;vi.pVertexAttributeDescriptions=&va;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_POINT_LIST;const VkViewport viewport{0,0,256,256,0,1};const VkRect2D scissor{{0,0},extent};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        std::array<VkPipelineColorBlendAttachmentState,2> blend_attachments{};for(auto &b:blend_attachments)b.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=2;blend.pAttachments=blend_attachments.data();
        VkGraphicsPipelineCreateInfo pc{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pc.stageCount=2;pc.pStages=stages.data();pc.pVertexInputState=&vi;pc.pInputAssemblyState=&ia;pc.pViewportState=&vp;pc.pRasterizationState=&raster;pc.pMultisampleState=&ms;pc.pColorBlendState=&blend;pc.layout=layout;pc.renderPass=pass;VkPipeline pipeline{};check(vkCreateGraphicsPipelines(g.device,{},1,&pc,nullptr,&pipeline));return pipeline;
    }
    json run(unsigned format,unsigned prefix){
        const auto &f=formats[format];VkFormatProperties properties{};vkGetPhysicalDeviceFormatProperties(g.physical,f.native,&properties);const bool direct=properties.bufferFeatures&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;const auto native=direct?f.native:f.padded;vkGetPhysicalDeviceFormatProperties(g.physical,native,&properties);require(properties.bufferFeatures&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,"Neither source nor padded native fetch control is supported");
        const auto native_stride=((direct?f.components:4u)*f.bytes+3u)&~3u;const auto stride=f.components*f.bytes;const auto raw_bytes=(prefix+count*stride+7u)&~3u;
        const auto vertex=g.buffer(count*native_stride,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),raw=g.buffer(raw_bytes,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);std::memset(vertex.mapped,0,count*native_stride);std::memset(raw.mapped,0,raw_bytes);
        for(unsigned i=0;i<count;++i){auto *v=static_cast<std::uint8_t*>(vertex.mapped)+i*native_stride;auto *r=static_cast<std::uint8_t*>(raw.mapped)+prefix+i*stride;for(unsigned lane=0;lane<f.components;++lane){const auto value=component(format,i,lane);word(v+lane*f.bytes,f.bytes,value);word(r+lane*f.bytes,f.bytes,value);}if(!direct)for(unsigned lane=f.components;lane<4;++lane)word(v+lane*f.bytes,f.bytes,lane==3?(format<2?0x3f800000u:format<4?32767u:format==4?1u:255u):0u);}
        const VkDescriptorBufferInfo buffer{raw.handle,0,raw_bytes};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&buffer;vkUpdateDescriptorSets(g.device,1,&write,0,nullptr);
        const auto graphics=pipeline(format,native,native_stride);
        try{
            g.begin();for(auto image:images)barrier(g.command,image.handle,VK_IMAGE_ASPECT_COLOR_BIT,initialized?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT|VK_ACCESS_SHADER_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
            std::array<VkClearValue,2> clear{};for(auto &c:clear)for(auto &v:c.color.uint32)v=sentinel;VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=frame;begin.renderArea={{0,0},extent};begin.clearValueCount=2;begin.pClearValues=clear.data();vkCmdBeginRenderPass(g.command,&begin,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,graphics);vkCmdBindDescriptorSets(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);const std::array<unsigned,2> push{prefix,stride};vkCmdPushConstants(g.command,layout,VK_SHADER_STAGE_VERTEX_BIT,0,8,push.data());const VkDeviceSize zero=0;vkCmdBindVertexBuffers(g.command,0,1,&vertex.handle,&zero);vkCmdDraw(g.command,count,1,0,0);vkCmdEndRenderPass(g.command);
            for(unsigned i=0;i<2;++i){barrier(g.command,images[i].handle,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);auto copy=region(extent,VK_IMAGE_ASPECT_COLOR_BIT);vkCmdCopyImageToBuffer(g.command,images[i].handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,output[i].handle,1,&copy);}host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);check(vkEndCommandBuffer(g.command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&submit,{}));check(vkQueueWaitIdle(g.queue));initialized=true;
        }catch(...){vkDeviceWaitIdle(g.device);vkDestroyPipeline(g.device,graphics,nullptr);throw;}
        vkDestroyPipeline(g.device,graphics,nullptr);
        const auto *native_bits=static_cast<const std::uint32_t*>(output[0].mapped),*fetch_bits=static_cast<const std::uint32_t*>(output[1].mapped);unsigned differences=0,uncovered=0;json first=json::array();
        for(unsigned i=0;i<count;++i){uncovered+=native_bits[4*i+3]==sentinel;for(unsigned lane=0;lane<4;++lane)if(native_bits[4*i+lane]!=fetch_bits[4*i+lane]){++differences;if(first.size()<12)first.push_back({{"vertex",i},{"lane",lane},{"input",lane<f.components?component(format,i,lane):0u},{"native_bits",native_bits[4*i+lane]},{"fetched_bits",fetch_bits[4*i+lane]}});}}
        return {{"format",f.name},{"format_code",format},{"source_stride",stride},{"native_stride",native_stride},{"prefix",prefix},{"native_format",native},{"native_source_format_supported",direct},{"vertices",count},{"uncovered_pixels",uncovered},{"different_lanes",differences},{"first_differences",first}};
    }
    ~Pass(){if(frame)vkDestroyFramebuffer(g.device,frame,nullptr);if(pass)vkDestroyRenderPass(g.device,pass,nullptr);if(pool)vkDestroyDescriptorPool(g.device,pool,nullptr);if(layout)vkDestroyPipelineLayout(g.device,layout,nullptr);if(descriptor_layout)vkDestroyDescriptorSetLayout(g.device,descriptor_layout,nullptr);for(auto view:views)if(view)vkDestroyImageView(g.device,view,nullptr);for(auto module:modules)if(module)vkDestroyShaderModule(g.device,module,nullptr);}
};
}
int main(int argc,char **argv){json report;report["schema"]="H2_RAW_VERTEX_FETCH_V1";report["cases"]=json::array();int result=0;try{require(argc==2,"Expected small report path");{Gpu g;g.initialize();report["device"]=g.properties.deviceName;report["validation_layer_active"]=g.validation;report["synchronization_validation_active"]=g.synchronization_validation;Pass pass(g);pass.initialize();for(unsigned format=0;format<formats.size();++format)for(unsigned prefix=0;prefix<4;++prefix)report["cases"].push_back(pass.run(format,prefix));}unsigned differences=0;for(auto &row:report["cases"])differences+=row["different_lanes"].get<unsigned>()+row["uncovered_pixels"].get<unsigned>();report["differences"]=differences;require(!differences,"Raw vertex fetch differs from native vertex input");require(!validation_errors,"Vulkan validation errors");report["passed"]=true;}catch(const std::exception &e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';result=1;}report["validation_errors"]=validation_errors;if(argc==2){std::ofstream file(argv[1]);file<<report.dump(2)<<'\n';if(!file)result=1;}std::cout<<"Raw vertex fetch cases: "<<report["cases"].size()<<", passed: "<<report["passed"]<<'\n';return result;}
