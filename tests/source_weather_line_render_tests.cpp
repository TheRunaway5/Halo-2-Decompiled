#include "halo2/source_weather_line_render.hpp"
#include "halo2/source_weather_assets.hpp"
#include "source_weather_point_gpu_fixture.hpp"
#include <bit>
#include <cmath>

using namespace halo2;
using namespace halo2_weather_point_fixture;
int main(){try{
    // Controlled renderer checks, using the unmodified retail VS22 and rain
    // combiner. These endpoints are test inputs, not a captured retail scene.
    const auto path=std::filesystem::path(HALO2_SOURCE_DIR)/
        "analysis/multiplayer-shading/weather-particle-packet/source-vs22-class0.vp";
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    require(bool(in)&&in.tellg()>0,"original VS22 missing");
    std::vector<std::byte> code(std::size_t(in.tellg()));in.seekg(0);
    require(bool(in.read(reinterpret_cast<char*>(code.data()),code.size())),"short original VS22");
    Gpu g;if(!g.initialize(true))return 77;
    const VkExtent2D extent{640,480};
    const auto pass=g.scene_pass(true);
    SourceWeatherLineRender renderer({g.physical,g.device,pass,HALO2_WEATHER_POINT_SHADER_DIR,true,true,true});
    std::array<ShaderVector,12> camera{};
    camera[0]={1,0,0,0};camera[1]={0,1,0,0};camera[3]={0,0,1,0};
    camera[10]={320,-240,16777215.f,0};camera[11]={320.5f,240.5f,0,0};
    std::array<SourceWeatherLineInput,2> vertices{{{{-204.8f,0,256,1},0xffffffff},{{409.6f,0,512,1},0xffffffff}}};
    const auto program=decode_vertex_program(code);
    for(unsigned i=0;i<2;++i){VertexProgramState s;std::copy(camera.begin(),camera.end(),s.constants.begin());
        s.attributes[0]=vertices[i].position;execute_vertex_program(program,s);
        require(std::abs(s.outputs[0][0]-(i?576.5f:64.5f))<.001f && s.outputs[0][1]==240.5f &&
            s.outputs[0][3]==(i?512.f:256.f),"controlled source projection changed");}
    SourceWeatherLinePacket packet{code,camera,source_weather_pixel_program(true),vertices,
        {{0,16777215.f,0,16777215.f},{640,480,0,0},0,0,1,7,true,true,false}};
    const auto reject=[&](SourceWeatherLinePacket p){bool failed=false;
        try{renderer.prepare(p);}catch(const FormatError&){failed=true;}
        require(failed,"unsupported rain packet admitted");};
    const auto render=[&](SourceWeatherLinePacket p,float clear_depth,bool retained=false){
        const auto color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        const auto depth=g.image(extent,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        const auto frame=g.framebuffer(pass,extent,color,depth);
        auto out=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        auto depth_out=g.buffer(640*480*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        renderer.prepare(p);
        if(retained){auto bad=p;bad.vertices=std::span(vertices).first(1);reject(bad);
            bad=p;bad.raster.line_width=0;reject(bad);bad=p;bad.pixel=source_weather_pixel_program(false);reject(bad);
            bad=p;bad.raster.depth_factor=1;reject(bad);bad=p;bad.raster.stencil_enabled=true;reject(bad);}
        g.begin();VkClearValue clear[2]{};clear[0].color={{.0627451f,.09411765f,.1254902f,.25f}};
        clear[1].depthStencil={clear_depth,0};VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},extent};bi.clearValueCount=2;bi.pClearValues=clear;
        vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);
        renderer.record(g.command,{0,0,640,480,0,1},{{0,0},extent});vkCmdEndRenderPass(g.command);
        g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,out);
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=depth.handle;
        barrier.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};copy.imageExtent={640,480,1};
        vkCmdCopyImageToBuffer(g.command,depth.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,depth_out.handle,1,&copy);g.end();
        for(unsigned i=0;i<640*480;++i)require(static_cast<float*>(depth_out.mapped)[i]==clear_depth,"rain wrote world depth");
        std::vector<std::byte> result(out.size);std::memcpy(result.data(),out.mapped,out.size);return result;
    };
    const auto visible=render(packet,1,true),hidden=render(packet,0);
    const auto count=[&](const auto &image){unsigned n=0;for(unsigned i=0;i<image.size();i+=4){
        require(image[i+3]==hidden[i+3],"rain changed target alpha");
        n+=!std::equal(image.begin()+i,image.begin()+i+3,hidden.begin()+i);}return n;};
    const auto full=count(visible);require(full>400,"rain lines have no visible coverage");
    const float middle=std::bit_cast<float>(std::bit_cast<unsigned>(384.f/16777216.f)+1u);
    const auto partial=count(render(packet,middle));require(partial>100&&partial<full-100,"rain lost perspective W-depth occlusion");
    auto wide=packet;wide.raster.line_width=2;require(count(render(wide,1))>full,"authored wide rain has no coverage response");
    auto clipped=packet;clipped.raster.clip_range[2]=1024;require(render(clipped,1)==hidden,"rain bypassed source W clip");
    auto transparent=vertices;for(auto &v:transparent)v.color&=0xffffff;
    auto zero=packet;zero.vertices=transparent;require(render(zero,1)==hidden,"rain zero alpha blend changed color");
    auto masked=packet;masked.raster.color_write_mask=0;require(render(masked,1)==hidden,"rain ignored color-write mask");
    require(validation_errors==0,"rain Vulkan validation errors");
    std::cout<<"Rain: "<<full<<" visible, "<<partial<<" W-occluded pixels; width, clipping, RGB-only, no depth writes, retained admission passed\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
