// Reuse the actual integer D24S8 upload/readback and sampling fixture. Its
// standalone entry is renamed and never supplies this test's result.
#define main halo2_depth_sampled_view_fixture_entry
#include "source_weather_depth_alias_tests.cpp"
#undef main

namespace {
struct Edit { VkRect2D rectangle;std::array<std::byte,4> rgba; };
unsigned color_guards{},color_cases{};
std::uint64_t checked_pixels{},edited_pixels{};
VkFramebuffer color_frame(Gpu &g,SourceWeatherDepthAliasBinding target,VkRenderPass &pass,Image source_depth={}) {
    std::array<VkAttachmentDescription,2> a{};a[0].format=target.format;a[0].samples=VK_SAMPLE_COUNT_1_BIT;
    a[0].loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    a[0].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a[0].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    a[0].initialLayout=a[0].finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    a[1].format=VK_FORMAT_D24_UNORM_S8_UINT;a[1].samples=VK_SAMPLE_COUNT_1_BIT;
    a[1].loadOp=a[1].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_LOAD;a[1].storeOp=a[1].stencilStoreOp=VK_ATTACHMENT_STORE_OP_STORE;
    a[1].initialLayout=a[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    const VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    const VkAttachmentReference depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
    if(source_depth.handle)sub.pDepthStencilAttachment=&depth;
    VkRenderPassCreateInfo pc{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pc.attachmentCount=source_depth.handle?2:1;pc.pAttachments=a.data();pc.subpassCount=1;pc.pSubpasses=&sub;
    check(vkCreateRenderPass(g.device,&pc,nullptr,&pass));g.passes.push_back(pass);
    const VkImageView views[]{target.view,source_depth.view};
    VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fc.renderPass=pass;fc.attachmentCount=pc.attachmentCount;fc.pAttachments=views;
    fc.width=target.extent.width;fc.height=target.extent.height;fc.layers=1;
    VkFramebuffer frame{};check(vkCreateFramebuffer(g.device,&fc,nullptr,&frame));g.frames.push_back(frame);return frame;
}
Input edited(Input input,std::span<const Edit> edits) {
    for(const auto &edit:edits)for(unsigned y=0;y<edit.rectangle.extent.height;++y)for(unsigned x=0;x<edit.rectangle.extent.width;++x) {
        const auto i=(y+edit.rectangle.offset.y)*input.extent.width+x+edit.rectangle.offset.x;
        std::copy(edit.rgba.begin(),edit.rgba.end(),input.expected.begin()+i*4);
        const auto c=[&](unsigned at){return std::to_integer<std::uint32_t>(edit.rgba[at]);};
        const auto n=(c(3)<<16)|(c(0)<<8)|c(1);std::memcpy(input.depth.data()+i*4,&n,4);input.stencil[i]=edit.rgba[2];
    }
    return input;
}
void color_case(Gpu &g,SourceWeatherDepthAlias &alias,SampleProbe &normal_probe,SampleProbe &alternate_probe,
                const Input &input,VkImageLayout layout,std::span<const Edit> edits) {
    const auto depth=g.image(input.extent,VK_FORMAT_D24_UNORM_S8_UINT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    upload(g,depth,input,VK_IMAGE_LAYOUT_UNDEFINED,layout);alias.prepare(input.extent);
    const auto expected=edited(input,edits);const auto n=source_weather_depth_alias_storage(input.extent).pixels;
    Fence fence(g);auto p=packet(g,depth,input,layout,fence.value);
    reject([&]{alias.end_color(g.command);},color_guards);
    auto invalid=p;invalid.source.usage&=~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    reject([&]{alias.begin_color(invalid);},color_guards);
    g.begin();alias.begin_color(p);
    reject([&]{alias.binding();},color_guards);
    reject([&]{alias.binding(SourceWeatherDepthAliasFormat::linear_a8b8g8r8);},color_guards);
    reject([&]{alias.end_color({});},color_guards);
    reject([&]{alias.retire(fence.value);},color_guards);
    reject([&]{alias.prepare(input.extent);},color_guards);
    reject([&]{alias.begin_color(p);},color_guards);
    check(vkResetCommandBuffer(g.command,0));alias.discard_unsubmitted(g.command);
    for(unsigned repeat=0;repeat<2;++repeat) {
        if(repeat)check(vkResetFences(g.device,1,&fence.value));
        const auto normal=g.buffer(std::uint64_t(n+2)*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
            alternate=g.buffer(std::uint64_t(n+2)*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        g.begin();const auto target=alias.begin_color(p);bool submitted=false,retired=false;
        const auto submit=[&]{check(vkEndCommandBuffer(g.command));VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&si,fence.value));submitted=true;};
        const auto retire=[&]{check(vkWaitForFences(g.device,1,&fence.value,VK_TRUE,UINT64_MAX));alias.retire(fence.value);retired=true;};
        try {
            require(target.layout==VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL&&target.view==alias.prepared_binding().view,
                "color phase returned a different target or wrong attachment layout");
            VkRenderPass pass{};const auto frame=color_frame(g,target,pass);
            VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},input.extent};
            vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);
            if(!repeat)for(const auto &edit:edits) {
                VkClearAttachment clear{};clear.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
                for(unsigned c=0;c<4;++c)clear.clearValue.color.float32[c]=std::to_integer<unsigned>(edit.rgba[c])/255.f;
                const VkClearRect rectangle{edit.rectangle,0,1};vkCmdClearAttachments(g.command,1,&clear,1,&rectangle);
            }
            vkCmdEndRenderPass(g.command);alias.end_color(g.command);
            reject([&]{alias.end_color(g.command);},color_guards);
            normal_probe.record(alias.binding(),normal);alternate_probe.record(alias.binding(SourceWeatherDepthAliasFormat::linear_a8b8g8r8),alternate);
            submit();retire();
        } catch(...) {
            if(!submitted){check(vkResetCommandBuffer(g.command,0));alias.discard_unsubmitted(g.command);}
            else if(!retired)retire();
            throw;
        }
        require(std::memcmp(normal.mapped,expected.expected.data(),n*4)==0,"renderable depth color alias normal sampled bytes differ");
        auto swapped=expected.expected;for(unsigned i=0;i<n;++i)std::swap(swapped[i*4],swapped[i*4+2]);
        require(std::memcmp(alternate.mapped,swapped.data(),n*4)==0,"renderable depth color alias alternate sampled bytes differ");
        verify_source(g,depth,expected,layout);checked_pixels+=n;
    }
    for(const auto &edit:edits)edited_pixels+=std::uint64_t(edit.rectangle.extent.width)*edit.rectangle.extent.height;
    ++color_cases;
}
void blue_mask_case(Gpu &g,SourceWeatherDepthAlias &alias,SampleProbe &normal_probe,SampleProbe &alternate_probe) {
    auto input=control({17,17},13),expected=input;constexpr std::byte stencil{169};
    for(unsigned i=0;i<289;++i){expected.stencil[i]=stencil;expected.expected[i*4+2]=stencil;}
    const auto depth=g.image(input.extent,VK_FORMAT_D24_UNORM_S8_UINT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    upload(g,depth,input,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);alias.prepare(input.extent);
    const auto shader=[&](const char *name) {
        const auto bytes=read(std::filesystem::path(HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR)/name);
        std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
        VkShaderModuleCreateInfo sc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sc.codeSize=bytes.size();sc.pCode=words.data();
        VkShaderModule result{};check(vkCreateShaderModule(g.device,&sc,nullptr,&result));g.modules.push_back(result);return result;
    };
    const auto vertex=shader("source_depth_color_alias_test.vert.spv"),fragment=shader("source_depth_color_alias_test.frag.spv");
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT,0,16};VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};lc.pushConstantRangeCount=1;lc.pPushConstantRanges=&push;
    check(vkCreatePipelineLayout(g.device,&lc,nullptr,&g.layout));
    Fence fence(g);auto record=packet(g,depth,input,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,fence.value);
    const auto normal=g.buffer(291*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),alternate=g.buffer(291*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    g.begin();const auto target=alias.begin_color(record);bool submitted=false,retired=false;
    try {
        VkRenderPass pass{};const auto frame=color_frame(g,target,pass,depth);
        VkPipelineShaderStageCreateInfo stages[2]{};
        for(unsigned i=0;i<2;++i){stages[i]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};stages[i].stage=i?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT;stages[i].module=i?fragment:vertex;stages[i].pName="main";}
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        const VkViewport viewport{0,0,17,17,0,1};const VkRect2D scissor{{0,0},{17,17}};
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.lineWidth=1;rs.cullMode=VK_CULL_MODE_NONE;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;ds.depthWriteEnable=VK_FALSE;
        VkPipelineColorBlendAttachmentState cb{};cb.colorWriteMask=VK_COLOR_COMPONENT_B_BIT;cb.blendEnable=VK_FALSE;
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&cb;
        VkGraphicsPipelineCreateInfo pc{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pc.stageCount=2;pc.pStages=stages;pc.pVertexInputState=&vi;pc.pInputAssemblyState=&ia;pc.pViewportState=&vp;pc.pRasterizationState=&rs;pc.pMultisampleState=&ms;pc.pDepthStencilState=&ds;pc.pColorBlendState=&bs;pc.layout=g.layout;pc.renderPass=pass;
        VkPipeline pipeline{};check(vkCreateGraphicsPipelines(g.device,{},1,&pc,nullptr,&pipeline));g.pipelines.push_back(pipeline);
        VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea=scissor;
        vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        const std::array<float,4> rgba{.1f,.2f,169.f/255,.9f};vkCmdPushConstants(g.command,g.layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,16,rgba.data());vkCmdDraw(g.command,3,1,0,0);vkCmdEndRenderPass(g.command);
        alias.end_color(g.command);normal_probe.record(alias.binding(),normal);alternate_probe.record(alias.binding(SourceWeatherDepthAliasFormat::linear_a8b8g8r8),alternate);
        check(vkEndCommandBuffer(g.command));VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&g.command;check(vkQueueSubmit(g.queue,1,&si,fence.value));submitted=true;
        check(vkWaitForFences(g.device,1,&fence.value,VK_TRUE,UINT64_MAX));alias.retire(fence.value);retired=true;
    } catch(...) {
        if(!submitted){check(vkResetCommandBuffer(g.command,0));alias.discard_unsubmitted(g.command);}
        else if(!retired){check(vkWaitForFences(g.device,1,&fence.value,VK_TRUE,UINT64_MAX));alias.retire(fence.value);}
        throw;
    }
    require(std::memcmp(normal.mapped,expected.expected.data(),expected.expected.size())==0,"original blue-only mask modified non-stencil channels");
    auto swapped=expected.expected;for(unsigned i=0;i<289;++i)std::swap(swapped[i*4],swapped[i*4+2]);
    require(std::memcmp(alternate.mapped,swapped.data(),swapped.size())==0,"blue-only stencil draw alternate view differs");
    verify_source(g,depth,expected,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    std::cout<<"Original blue-only color mask and disabled blend: actual draw changed 289 stencil bytes, preserved every D24 bit, with the actual source depth attachment bound for read-only depth testing\n";
}
}
int main(){try {
    Gpu g;if(!g.initialize())return 77;
    SourceWeatherDepthAlias alias({g.physical,g.device,g.family,HALO2_WEATHER_DEPTH_ALIAS_SHADER_DIR});SampleProbe normal(g),alternate(g);
    const auto original=original_input();
    color_case(g,alias,normal,alternate,original,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,{});
    const std::array<Edit,1> patch{{{{{13,17},{87,65}},{std::byte{0x13},std::byte{0x47},std::byte{0x96},std::byte{0xe1}}}}};
    color_case(g,alias,normal,alternate,original,VK_IMAGE_LAYOUT_GENERAL,patch);
    std::vector<Edit> all_bytes;
    for(unsigned i=0;i<256;++i)all_bytes.push_back({{{int(i%17),int(i/17)},{1,1}},
        {std::byte(i),std::byte(i*17+3),std::byte(i*37+7),std::byte(i*53+9)}});
    color_case(g,alias,normal,alternate,control({17,17},41),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,all_bytes);
    all_bytes.resize(15);
    for(unsigned i=0;i<15;++i)all_bytes[i].rectangle.offset={int(i%3),int(i/3)};
    color_case(g,alias,normal,alternate,control({3,5},91),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,all_bytes);
    for(unsigned value:{0u,255u}) {
        const std::array<Edit,1> edit{{{{{0,0},{1,1}},{std::byte(value),std::byte(value),std::byte(value),std::byte(value)}}}};
        color_case(g,alias,normal,alternate,control({1,1},255-value),VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,edit);
    }
    require(color_cases==6&&color_guards==60&&checked_pixels==1229412&&edited_pixels==5928,"depth color alias coverage counts changed");
    blue_mask_case(g,alias,normal,alternate);
    require(validation_errors==0,"depth color alias Vulkan validation errors");
    std::cout<<"Six color attachment cases and reuse: 1229412 exact D24/stencil pixels, both sampled views, 5928 edited pixels, all byte values, partial untouched regions, five source layouts, odd tails and 60 phase guards; zero Vulkan validation errors\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
