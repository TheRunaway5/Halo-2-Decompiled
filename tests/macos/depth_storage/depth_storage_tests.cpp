#include "halo2/source_weather_depth_alias.hpp"
#include "gpu.hpp"
#include <array>
#include <bit>
#include <nlohmann/json.hpp>
using namespace halo2;
using nlohmann::json;
namespace {
constexpr VkImageAspectFlags aspects=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
constexpr VkImageUsageFlags usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
constexpr VkExtent2D extent{1024,1024};
constexpr unsigned pixels=extent.width*extent.height;
struct Fence {VkDevice device;VkFence handle{};explicit Fence(VkDevice d):device(d){VkFenceCreateInfo c{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(d,&c,nullptr,&handle));}~Fence(){vkDestroyFence(device,handle,nullptr);}};
struct Submission {
    Gpu &g;SourceWeatherDepthAlias &alias;VkFence fence;bool submitted{};
    void finish(){check(vkEndCommandBuffer(g.command));VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&s,fence));submitted=true;check(vkWaitForFences(g.device,1,&fence,VK_TRUE,UINT64_MAX));alias.retire(fence);}
    ~Submission(){if(alias.has_pending_submission()){if(submitted){vkDeviceWaitIdle(g.device);try{alias.retire(fence);}catch(...){}}else{vkResetCommandBuffer(g.command,0);try{alias.discard_unsubmitted(g.command);}catch(...){}}}}
};
template<class Action> void rejects(Action action,const char *message) {bool failed=false;try{action();}catch(const std::exception&){failed=true;}require(failed,message);}
struct ColorDraw {
    Gpu &g;VkRenderPass pass{};VkFramebuffer framebuffer{};VkPipelineLayout layout{};VkPipeline pipeline{};std::array<VkShaderModule,2> modules{};
    explicit ColorDraw(Gpu &gpu):g(gpu){}
    void initialize(VkImageView view) {
        VkAttachmentDescription attachment{};attachment.format=VK_FORMAT_R8G8B8A8_UNORM;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.initialLayout=attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&reference;
        VkRenderPassCreateInfo rc{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rc.attachmentCount=1;rc.pAttachments=&attachment;rc.subpassCount=1;rc.pSubpasses=&subpass;check(vkCreateRenderPass(g.device,&rc,nullptr,&pass));
        VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fc.renderPass=pass;fc.attachmentCount=1;fc.pAttachments=&view;fc.width=extent.width;fc.height=extent.height;fc.layers=1;check(vkCreateFramebuffer(g.device,&fc,nullptr,&framebuffer));
        const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT,0,8};VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.pushConstantRangeCount=1;lc.pPushConstantRanges=&push;check(vkCreatePipelineLayout(g.device,&lc,nullptr,&layout));
        modules={g.shader("depth_color.vert.spv"),g.shader("depth_color.frag.spv")};
        std::array<VkPipelineShaderStageCreateInfo,2> stages{};for(unsigned i=0;i<2;++i){stages[i]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};stages[i].stage=i?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT;stages[i].module=modules[i];stages[i].pName="main";}
        VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        const VkViewport viewport{0,0,float(extent.width),float(extent.height),0,1};const VkRect2D scissor{{0,0},extent};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState attachment_blend{};attachment_blend.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment_blend;
        VkGraphicsPipelineCreateInfo pc{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pc.stageCount=stages.size();pc.pStages=stages.data();pc.pVertexInputState=&vertices;pc.pInputAssemblyState=&ia;pc.pViewportState=&vp;pc.pRasterizationState=&raster;pc.pMultisampleState=&ms;pc.pColorBlendState=&blend;pc.layout=layout;pc.renderPass=pass;check(vkCreateGraphicsPipelines(g.device,{},1,&pc,nullptr,&pipeline));
    }
    void record(unsigned base){VkRenderPassBeginInfo bc{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bc.renderPass=pass;bc.framebuffer=framebuffer;bc.renderArea={{0,0},extent};vkCmdBeginRenderPass(g.command,&bc,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);const std::array<unsigned,2> push{extent.width,base};vkCmdPushConstants(g.command,layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(push),push.data());vkCmdDraw(g.command,3,1,0,0);vkCmdEndRenderPass(g.command);}
    ~ColorDraw(){if(pipeline)vkDestroyPipeline(g.device,pipeline,nullptr);if(layout)vkDestroyPipelineLayout(g.device,layout,nullptr);if(framebuffer)vkDestroyFramebuffer(g.device,framebuffer,nullptr);if(pass)vkDestroyRenderPass(g.device,pass,nullptr);for(auto module:modules)if(module)vkDestroyShaderModule(g.device,module,nullptr);}
};
json cpu_contract() {
    std::uint32_t previous=0;
    for(std::uint32_t n=0;n<=source_d24_maximum;++n){const auto word=source_d24_carrier_word(n);require(source_d24_carrier_integer(word)==n,"CPU depth integer round trip");require(!n||word>previous,"CPU carrier lost ordering");require(std::bit_cast<float>(word)==float(n)/16777216.0f,"CPU carrier disagrees with exact binary32 value");previous=word;}
    for(auto n:{0x01000000u,0xffffffffu})rejects([&]{source_d24_carrier_word(n);},"Out-of-domain integer admitted");
    const std::array invalid{0x80000000u,0x3f800000u,0x7fc00000u,0x7f800000u,0x33800001u,0x33000000u,0x00000001u};
    for(auto word:invalid)rejects([&]{source_d24_carrier_integer(word);},"Noncanonical CPU carrier admitted");
    return {{"integers",16777216},{"invalid_integers",2},{"invalid_carriers",invalid.size()},{"binary32_identity",true},{"strict_order",true}};
}
json gpu_contract(Gpu &g,VkFormat format,SourceDepthStorageEncoding encoding) {
    const bool carrier=encoding==SourceDepthStorageEncoding::d32_integer_d24;
    VkImageFormatProperties support{};const auto status=vkGetPhysicalDeviceImageFormatProperties(g.physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,usage,0,&support);
    if(status==VK_ERROR_FORMAT_NOT_SUPPORTED&&!carrier)return {{"supported",false}};
    check(status);require(support.maxExtent.width>=extent.width&&support.maxExtent.height>=extent.height,"Depth extent unsupported");
    const auto image=g.image(extent,format,usage);
    const auto upload_d=g.buffer(pixels*4),upload_s=g.buffer(pixels),read_d=g.buffer(pixels*4),read_s=g.buffer(pixels),read_alias=g.buffer(pixels*4);
    auto *input_d=static_cast<std::uint32_t*>(upload_d.mapped);auto *input_s=static_cast<std::uint8_t*>(upload_s.mapped);
    Fence fence(g.device);SourceWeatherDepthAlias alias({g.physical,g.device,g.family,DEPTH_SHADER_DIR});alias.prepare(extent);
    ColorDraw draw(g);draw.initialize(alias.prepared_binding().view);
    SourceWeatherDepthAliasRecord record{};record.source={g.device,image.handle,format,extent,usage,VK_IMAGE_LAYOUT_GENERAL,VK_SAMPLE_COUNT_1_BIT,1,1,g.family,VK_SHARING_MODE_EXCLUSIVE};record.source.depth_encoding=encoding;
    record.command_owner=g.device;record.command=g.command;record.completion=fence.handle;record.command_queue_family=g.family;record.outside_render_pass=record.exclusive_access=true;
    bool initialized=false;
    const auto upload=[&]{barrier(g.command,image.handle,aspects,initialized?VK_IMAGE_LAYOUT_GENERAL:VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);auto depth=region(extent,VK_IMAGE_ASPECT_DEPTH_BIT),stencil=region(extent,VK_IMAGE_ASPECT_STENCIL_BIT);vkCmdCopyBufferToImage(g.command,upload_d.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&depth);vkCmdCopyBufferToImage(g.command,upload_s.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&stencil);barrier(g.command,image.handle,aspects,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);};
    const auto readback=[&]{const auto binding=alias.binding();barrier(g.command,image.handle,aspects,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);auto depth=region(extent,VK_IMAGE_ASPECT_DEPTH_BIT),stencil=region(extent,VK_IMAGE_ASPECT_STENCIL_BIT);vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,read_d.handle,1,&depth);vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,read_s.handle,1,&stencil);barrier(g.command,image.handle,aspects,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);barrier(g.command,binding.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);auto color=region(extent,VK_IMAGE_ASPECT_COLOR_BIT);vkCmdCopyImageToBuffer(g.command,binding.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,read_alias.handle,1,&color);barrier(g.command,binding.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);VkMemoryBarrier memory{VK_STRUCTURE_TYPE_MEMORY_BARRIER};memory.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;memory.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&memory,0,nullptr,0,nullptr);};
    const auto verify=[&](unsigned base,bool modified){const auto *depth=static_cast<const std::uint32_t*>(read_d.mapped);const auto *stencil=static_cast<const std::uint8_t*>(read_s.mapped);const auto *color=static_cast<const std::uint8_t*>(read_alias.mapped);for(unsigned i=0;i<pixels;++i){const auto n=modified?((base+i)^source_d24_maximum):(base+i);const auto s=modified?((n^(n>>8))&255u):(n&255u);const auto expected=carrier?source_d24_carrier_word(n):n;if((carrier?depth[i]:depth[i]&source_d24_maximum)!=expected||stencil[i]!=s||color[4*i]!=((n>>8)&255)||color[4*i+1]!=(n&255)||color[4*i+2]!=s||color[4*i+3]!=(n>>16))throw std::runtime_error("Physical depth/stencil/alias mismatch at "+std::to_string(base+i)+(modified?" after color writeback":" after forward alias"));}};
    unsigned negative_metadata=0;
    {g.begin();Submission submission{g,alias,fence.handle};auto bad=record;bad.source.depth_encoding=carrier?SourceDepthStorageEncoding::native_d24:SourceDepthStorageEncoding::d32_integer_d24;rejects([&]{alias.record(bad);},"Mismatched physical depth encoding admitted");++negative_metadata;check(vkResetCommandBuffer(g.command,0));}
    for(unsigned base=0;base<=source_d24_maximum;base+=pixels) {
        for(unsigned i=0;i<pixels;++i){const auto n=base+i;input_d[i]=carrier?source_d24_carrier_word(n):n;input_s[i]=n&255;}
        check(vkResetFences(g.device,1,&fence.handle));g.begin();{Submission submission{g,alias,fence.handle};upload();alias.record(record);readback();submission.finish();}initialized=true;verify(base,false);
        check(vkResetFences(g.device,1,&fence.handle));g.begin();{Submission submission{g,alias,fence.handle};alias.begin_color(record);draw.record(base);alias.end_color(g.command);readback();submission.finish();}verify(base,true);
    }
    unsigned rejected_physical=0;
    if(carrier) {
        for(auto word:{0x80000000u,0x3f800000u,0x7fc00000u,0x7f800000u,0x33800001u,0x33000000u,0x00000001u}) {
            input_d[0]=word;check(vkResetFences(g.device,1,&fence.handle));g.begin();
            {Submission submission{g,alias,fence.handle};upload();alias.record(record);bool failed=false;try{submission.finish();}catch(const FormatError&){failed=true;}require(failed&&!alias.has_pending_submission(),"Noncanonical physical depth was accepted or stranded fence");rejects([&]{alias.binding();},"Invalid carrier left a ready alias");++rejected_physical;}
            input_d[0]=source_d24_carrier_word(source_d24_maximum-pixels+1);check(vkResetFences(g.device,1,&fence.handle));g.begin();{Submission submission{g,alias,fence.handle};upload();alias.record(record);readback();submission.finish();}verify(source_d24_maximum-pixels+1,false);
        }
    }
    return {{"supported",true},{"forward_values",16777216},{"source_words_preserved",16777216},{"color_writeback_values",16777216},{"stencil_byte_patterns",256},{"metadata_rejections",negative_metadata},{"noncanonical_physical_rejections",rejected_physical},{"recovery_after_rejection",true},{"tile_extent",{extent.width,extent.height}}};
}
}
int main(int argc,char **argv) {
    json report;int result=0;
    try {require(argc==2,"Expected bounded report path");report["schema"]="H2_D24_STORAGE_VALIDATION_V1";report["cpu"]=cpu_contract();
        {Gpu g;g.initialize();report["device"]=g.properties.deviceName;report["driver_version"]=g.properties.driverVersion;report["api_version"]=g.properties.apiVersion;report["validation_layer_active"]=g.validation;report["synchronization_validation_active"]=g.synchronization_validation;report["carrier"]=gpu_contract(g,VK_FORMAT_D32_SFLOAT_S8_UINT,SourceDepthStorageEncoding::d32_integer_d24);report["native_d24"]=gpu_contract(g,VK_FORMAT_D24_UNORM_S8_UINT,SourceDepthStorageEncoding::native_d24);}
        require(validation_errors==0,"Vulkan validation errors: "+std::to_string(validation_errors));report["passed"]=true;
    }catch(const std::exception &error){report["passed"]=false;report["error"]=error.what();std::cerr<<error.what()<<'\n';result=1;}
    report["validation_errors"]=validation_errors;if(argc==2){std::ofstream file(argv[1]);file<<report.dump(2)<<'\n';if(!file)result=1;}std::cout<<report.dump()<<'\n';return result;
}
