int water_reflection_gpu(const std::filesystem::path& shaders) {
    constexpr VkExtent2D extent{width,height};
    Fixture gpu;gpu.initialize(shaders,extent);
    gpu.renderer->prepare_water_reflection(gpu.pass);
    gpu.renderer->prepare_environment(VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_D24_UNORM_S8_UINT,1,false);
    const std::array materials{white_material(),white_material()};
    std::vector<Vertex> geometry;
    quad(geometry,-4,-4,4,4,-1,{0,0,1},1);
    quad(geometry,.3f,-.5f,1.3f,.5f,1,{0,0,-1},0);
    const auto make_scene=[&] {
        auto scene=pt::build_scene(geometry,materials);
        scene.materials[0].emission={.8f,0,0,0};
        scene.materials[1].emission={0,.8f,0,0};
        return scene;
    };
    const pt::WaterReflectionView view{{1,0,0,0},{0,1,0,0},{0,0,1,0},
        {0,0,2,0},{float(width),float(height),10,0}};
    const auto draw=[&](const pt::Scene& scene,const pt::WaterReflectionView& camera) {
        gpu.renderer->upload(scene);
        gpu.begin();
        for(unsigned face=0;face<6;++face) {
            gpu.renderer->begin_environment(gpu.command,face,{0,0,0,1});
            gpu.renderer->end_environment(gpu.command);
        }
        constexpr std::array<VkClearValue,2> clear{{{.color={{0,0,.25f,.5f}}},{.depthStencil={1,0}}}};
        const VkRect2D scissor{{0,0},extent};
        const VkViewport viewport{0,0,float(width),float(height),0,1};
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        render.renderPass=gpu.pass;render.framebuffer=gpu.framebuffer;render.renderArea=scissor;
        render.clearValueCount=clear.size();render.pClearValues=clear.data();
        vkCmdBeginRenderPass(gpu.command,&render,VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(gpu.command,0,1,&viewport);vkCmdSetScissor(gpu.command,0,1,&scissor);
        gpu.renderer->draw_water_reflection(gpu.command,camera);
        vkCmdEndRenderPass(gpu.command);
        const VkBufferImageCopy copy{0,0,0,{VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},{0,0,0},{width,height,1}};
        vkCmdCopyImageToBuffer(gpu.command,gpu.color.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,gpu.readback.handle,1,&copy);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(gpu.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
        gpu.submit();
        const auto* bytes=static_cast<const std::uint8_t*>(gpu.readback.mapped);
        return std::vector<std::uint8_t>(bytes,bytes+width*height*4);
    };
    const auto pixel=[](const auto& image,unsigned x,unsigned y) {
        const auto p=image.data()+(y*width+x)*4;
        return std::array{p[0],p[1],p[2],p[3]};
    };
    constexpr std::array<std::uint8_t,4> sky{0,0,64,128},red{204,0,0,128};
    const auto scene=make_scene();
    const auto image=draw(scene,view);
    require(pixel(image,40,32)==red,"Water reflection lost its projected geometry hit or changed target alpha");
    require(pixel(image,24,32)==sky && pixel(image,2,2)==sky,
        "Water reflection replaced the native sky miss or hit submerged geometry");
    auto moved=view;moved.eye_plane[0]=.8f;
    const auto shifted=draw(scene,moved);
    require(pixel(shifted,32,32)==red && pixel(shifted,42,32)==sky,
        "Water reflection lost its camera-relative parallax");
    geometry.resize(6);
    const auto submerged=draw(make_scene(),view);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)
        require(pixel(submerged,x,y)==sky,"Water reflection rendered geometry behind the water plane");
    require(gpu.validation_errors.load()==0,"Water reflection fixture reported Vulkan validation errors");
    std::cout<<"Path Tracing water reflection matched projected geometry, plane clipping, camera parallax, and native sky/alpha\n";
    return 0;
}
