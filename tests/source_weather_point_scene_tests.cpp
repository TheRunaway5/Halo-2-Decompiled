#include "halo2/source_weather_point_render.hpp"
#include "source_weather_point_gpu_fixture.hpp"
#include <cmath>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
using namespace halo2_weather_point_fixture;
std::vector<std::byte> read(const std::filesystem::path &p) {
    std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f)&&f.tellg()>0,"source scene artifact missing");
    std::vector<std::byte> b(std::size_t(f.tellg()));f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(b.data()),b.size())),"short source scene artifact");return b;
}
template<class T>std::vector<T> decode(const std::vector<std::byte>&b) {
    require(b.size()%sizeof(T)==0,"source scene artifact alignment");std::vector<T> v(b.size()/sizeof(T));std::memcpy(v.data(),b.data(),b.size());return v;
}
void save(const std::filesystem::path &p,std::span<const std::byte> b) {
    std::ofstream f(p,std::ios::binary);require(bool(f.write(reinterpret_cast<const char*>(b.data()),b.size())),"scene readback write failed");
}
void barrier(Gpu &g,Image image,VkImageAspectFlags aspect,VkImageLayout from,VkImageLayout to,
             VkAccessFlags src,VkAccessFlags dst,VkPipelineStageFlags before,VkPipelineStageFlags after) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.image=image.handle;b.subresourceRange={aspect,0,image.mip_levels,0,1};
    b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.oldLayout=from;b.newLayout=to;b.srcAccessMask=src;b.dstAccessMask=dst;
    vkCmdPipelineBarrier(g.command,before,after,0,0,nullptr,0,nullptr,1,&b);
}
void upload_color(Gpu &g,Image image,std::span<const std::byte> raw,VkImageLayout from) {
    auto b=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,raw.data(),raw.size());
    g.begin();barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,from,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        from==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,
        from==VK_IMAGE_LAYOUT_UNDEFINED?VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r{};r.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};r.imageExtent={640,480,1};
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&r);
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);g.end();
}
void upload_depth(Gpu &g,Image image,std::span<const std::byte> low24,std::span<const std::byte> source) {
    require(low24.size()==640*480*4&&source.size()==low24.size(),"actual source D24 surface dimensions");
    auto b=g.buffer(low24.size()+640*480,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,low24.data(),low24.size());
    auto *stencil=static_cast<std::byte*>(b.mapped)+low24.size();
    for(unsigned i=0;i<640*480;++i){
        std::uint32_t original,native;std::memcpy(&original,source.data()+i*4,4);std::memcpy(&native,low24.data()+i*4,4);
        require(native==original>>8,"D24 upload did not preserve the source integer");stencil[i]=std::byte(original&255);
    }
    g.begin();barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r[2]{};r[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};r[1].bufferOffset=low24.size();r[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};
    for(auto &copy:r)copy.imageExtent={640,480,1};
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,2,r);
    barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);g.end();
}
void verify_depth(Gpu &g,Image image,std::span<const std::byte> low24,std::span<const std::byte> source) {
    auto out=g.buffer(low24.size()+640*480,VK_BUFFER_USAGE_TRANSFER_DST_BIT);g.begin();
    barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r[2]{};r[0].imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};r[1].bufferOffset=low24.size();r[1].imageSubresource={VK_IMAGE_ASPECT_STENCIL_BIT,0,0,1};
    for(auto &copy:r)copy.imageExtent={640,480,1};
    vkCmdCopyImageToBuffer(g.command,image.handle,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,out.handle,2,r);
    barrier(g,image,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);g.end();
    const auto *raw=static_cast<std::byte*>(out.mapped);
    for(unsigned i=0;i<640*480;++i){std::uint32_t original,native;std::memcpy(&original,source.data()+i*4,4);std::memcpy(&native,raw+i*4,4);
        require((native&0xffffff)==original>>8,"native D24 upload/readback changed an original depth integer");require(raw[low24.size()+i]==std::byte(original&255),"native stencil upload/readback changed an original byte");}
}
void upload_sprite(Gpu &g,Image image,std::span<const std::byte> raw) {
    require(raw.size()==1408,"actual snow sprite partition size");auto b=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,raw.data(),raw.size());g.begin();
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r[4]{};const unsigned offsets[]{0,1024,1280,1344};for(unsigned i=0;i<4;++i){r[i].bufferOffset=offsets[i];r[i].imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i,0,1};r[i].imageExtent={32u>>i,32u>>i,1};}
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,4,r);
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);g.end();
}
void report(std::span<const std::byte> before,std::span<const std::byte> original,std::span<const std::byte> native) {
    unsigned source_changed=0,native_changed=0,different=0,max_channel=0,only_source=0,only_native=0;std::uint64_t absolute=0;
    for(unsigned i=0;i<before.size();i+=4){const bool s=!std::equal(before.begin()+i,before.begin()+i+4,original.begin()+i),n=!std::equal(before.begin()+i,before.begin()+i+4,native.begin()+i);
        source_changed+=s;native_changed+=n;only_source+=s&&!n;only_native+=n&&!s;
        different+=!std::equal(original.begin()+i,original.begin()+i+4,native.begin()+i);
        require(native[i+3]==before[i+3],"RGB-only weather changed original target alpha");
        for(unsigned c=0;c<3;++c){const auto d=unsigned(std::abs(int(std::to_integer<unsigned>(native[i+c]))-int(std::to_integer<unsigned>(original[i+c]))));absolute+=d;max_channel=std::max(max_channel,d);}
    }
    require(source_changed==1255&&native_changed>0,"actual source point scene response missing");
    std::cout<<"Actual event215873: source changed "<<source_changed<<", native changed "<<native_changed<<", differing pixels "<<different<<", source-only "<<only_source<<", native-only "<<only_native<<", max channel "<<max_channel<<", total RGB absolute "<<absolute<<'\n';
}
}
int main(int argc,char **argv){try{
    require(argc==1||(argc==2&&std::string(argv[1])=="--gpu"),"optional --gpu expected");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR),packet_dir=root/"analysis/multiplayer-shading/weather-point-original-reference-event215873";
    const auto original_dir=root/"analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873",output=root/"analysis/multiplayer-shading/weather-point-scene-prototype";
    std::filesystem::create_directories(output);
    const auto code=read(packet_dir/"source-vs22-class0.vp");const auto points=decode<SourceWeatherPointInput>(read(packet_dir/"points-v0-logical-argb.bin"));
    require(points.size()==1015,"actual source event range changed");const auto camera_raw=decode<ShaderVector>(read(packet_dir/"vertex_constants_c0_c11.bin"));require(camera_raw.size()==12,"full source camera missing");
    std::array<ShaderVector,12> camera;std::copy(camera_raw.begin(),camera_raw.end(),camera.begin());const auto program=decode_vertex_program(code);require(program.count==9,"literal VS22 changed");
    float min_size=std::numeric_limits<float>::max(),max_size=0;std::vector<ShaderVector> cpu_outputs;
    for(const auto &p:points){VertexProgramState s;std::copy(camera.begin(),camera.end(),s.constants.begin());s.attributes[0]=p.position;
        s.attributes[1]={float((p.color>>16)&255)/255,float((p.color>>8)&255)/255,float(p.color&255)/255,float(p.color>>24)/255};
        execute_vertex_program(program,s);require(s.outputs[0][3]>0&&std::isfinite(s.outputs[6][0])&&s.outputs[6][0]>0,"actual source event has unsupported output");min_size=std::min(min_size,s.outputs[6][0]);max_size=std::max(max_size,s.outputs[6][0]);cpu_outputs.insert(cpu_outputs.end(),s.outputs.begin(),s.outputs.end());}
    save(output/"cpu-program-outputs.bin",std::as_bytes(std::span(cpu_outputs)));
    const auto before=read(packet_dir/"before-scene-color.rgba"),after=read(packet_dir/"after-scene-color.rgba"),low24=read(packet_dir/"before-depth-d24-low24.bin"),source_depth=read(original_dir/"before-scene-depth.raw");
    require(before.size()==640*480*4&&after.size()==before.size(),"source scene color size changed");
    std::cout<<"Frozen actual1015 points, all positive W; source oPts range "<<min_size<<".."<<max_size<<"; actual original pre/post scene color and D24 integers\n";
    if(argc==1)return 0;
    Gpu g;if(!g.initialize())return 77;
    VkFormatProperties properties;vkGetPhysicalDeviceFormatProperties(g.physical,VK_FORMAT_D24_UNORM_S8_UINT,&properties);require(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,"actual D24S8 attachment unavailable");
    const VkExtent2D extent{640,480};const auto color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_UNDEFINED);upload_depth(g,depth,low24,source_depth);verify_depth(g,depth,low24,source_depth);
    const auto sprite=g.image({32,32},VK_FORMAT_BC3_UNORM_BLOCK,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,4);upload_sprite(g,sprite,read(packet_dir/"sprite-resident-physical-partition.bin"));
    VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ci.magFilter=ci.minFilter=VK_FILTER_LINEAR;ci.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;ci.addressModeU=ci.addressModeV=ci.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;ci.maxLod=1000;
    VkSampler sampler;check(vkCreateSampler(g.device,&ci,nullptr,&sampler));g.samplers.push_back(sampler);
    const auto ps=decode<std::uint32_t>(read(packet_dir/"source-compatible-pixel-definition-39780.bin"));require(ps.size()==60,"source-compatible complete pixel definition missing");PixelProgram pixel;std::copy(ps.begin(),ps.end(),pixel.words.begin());
    const auto clip_raw=decode<ShaderVector>(read(packet_dir/"clip_range.bin"));require(clip_raw.size()==1,"source clip bank missing");
    const auto pass=g.scene_pass(false,VK_FORMAT_D24_UNORM_S8_UINT);const auto frame=g.framebuffer(pass,extent,color,depth);
    SourceWeatherPointRender renderer({g.physical,g.device,pass,HALO2_WEATHER_POINT_SHADER_DIR,true,true});
    SourceWeatherPointPacket packet{code,camera,pixel,points,sprite.view,sampler,VK_FORMAT_BC3_UNORM_BLOCK,{32,32},4,{clip_raw[0],{640,480,0,0},0,0,7,true,true,1,1,false,SourceWeatherPointSpriteCoordinates::OriginalGlRawRowsUpperLeft}};
    const auto render=[&](){renderer.prepare(packet);auto out=g.buffer(before.size(),VK_BUFFER_USAGE_TRANSFER_DST_BIT);g.begin();VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},extent};vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);renderer.record(g.command,{0,0,640,480,0,1},{{0,0},extent});vkCmdEndRenderPass(g.command);g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,out);g.end();std::vector<std::byte> result(out.size);std::memcpy(result.data(),out.mapped,out.size);return result;};
    const auto native=render();report(before,after,native);save(output/"native-after-scene-color.rgba",native);verify_depth(g,depth,low24,source_depth);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(render()==native,"reused actual point scene bindings differ");verify_depth(g,depth,low24,source_depth);
    require(validation_errors==0,"weather scene validation diagnostics");
    std::cout<<"All307200 source D24 integers/stencil bytes exact before and after; original alpha unchanged, reused bindings exact; raw GL rows preserved\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
