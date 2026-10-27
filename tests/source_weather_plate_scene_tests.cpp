#include "halo2/source_weather_plate_render.hpp"
#include "source_weather_plate_gpu_fixture.hpp"
#include <cmath>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
using namespace halo2_weather_plate_fixture;
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
void upload_texture(Gpu &g,Image image,const std::filesystem::path &inputs,unsigned stage,unsigned size) {
    std::vector<std::byte> raw;std::vector<VkBufferImageCopy> regions;
    for(unsigned i=0;i<image.mip_levels;++i){const auto level=read(inputs/("stage-"+std::to_string(stage)+"-mip-"+std::to_string(i)+".rgba"));const unsigned n=std::max(1u,size>>i);const unsigned h=stage? n:480u;
        require(level.size()==std::size_t(n)*h*4,"actual plate mip byte count changed");VkBufferImageCopy r{};r.bufferOffset=raw.size();r.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i,0,1};r.imageExtent={n,h,1};regions.push_back(r);raw.insert(raw.end(),level.begin(),level.end());}
    auto b=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,raw.data(),raw.size());g.begin();
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,regions.size(),regions.data());
    barrier(g,image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);g.end();
}
void report(std::span<const std::byte> before,std::span<const std::byte> original,std::span<const std::byte> native,bool exact) {
    unsigned source_changed=0,native_changed=0,different=0,max_channel=0,only_source=0,only_native=0;std::uint64_t absolute=0;
    for(unsigned i=0;i<before.size();i+=4){const bool s=!std::equal(before.begin()+i,before.begin()+i+4,original.begin()+i),n=!std::equal(before.begin()+i,before.begin()+i+4,native.begin()+i);
        source_changed+=s;native_changed+=n;only_source+=s&&!n;only_native+=n&&!s;
        different+=!std::equal(original.begin()+i,original.begin()+i+4,native.begin()+i);
        require(native[i+3]==before[i+3],"RGB-only weather changed original target alpha");
        for(unsigned c=0;c<3;++c){const auto d=unsigned(std::abs(int(std::to_integer<unsigned>(native[i+c]))-int(std::to_integer<unsigned>(original[i+c]))));absolute+=d;max_channel=std::max(max_channel,d);}
    }
    require(source_changed==181676&&native_changed>0,"actual source plate scene response missing");
    std::cout<<"Actual event215778: source changed "<<source_changed<<", native changed "<<native_changed<<", differing pixels "<<different<<", source-only "<<only_source<<", native-only "<<only_native<<", max channel "<<max_channel<<", total RGB absolute "<<absolute<<'\n';
    if(exact)require(different==0,"native weather differs from the retained original OpenGL plate scene");
    else require(max_channel<=1,"actual source/native plate scene exceeds the observed one-byte comparison boundary");
}
}
int main(int argc,char **argv){try{
    require(argc==1||(argc==2&&(std::string(argv[1])=="--gpu"||std::string(argv[1])=="--gpu-baseline"||std::string(argv[1])=="--gpu-exact")),"optional --gpu, --gpu-exact or --gpu-baseline expected");
    const bool exact=argc==2&&std::string(argv[1])=="--gpu-exact";
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR),inputs=root/"analysis/multiplayer-shading/weather-plate-original-reference/event-215778";
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"weather scene test requires owned scratch");
    const auto output=std::filesystem::path(scratch)/"weather-plate-scene";
    std::filesystem::create_directories(output);
    const auto code=read(root/"analysis/multiplayer-shading/weather-gpu-assets/lockout-global21-class0.vp");const auto vertices=decode<SourceWeatherPlateVertex>(read(inputs/"actual-four-vertices.bin"));require(vertices.size()==4,"actual source four-vertex strip changed");
    const auto scroll=decode<ShaderVector>(read(inputs/"actual-scroll-c21-c23.bin")),clip=decode<ShaderVector>(read(inputs/"actual-clip-range.bin"));require(scroll.size()==3&&clip.size()==1,"actual source plate scroll/clip bank changed");
    const auto regions=decode<std::array<std::int32_t,4>>(read(inputs/"actual-clip-regions.bin"));require(regions.size()==8,"actual source clip regions changed");
    const auto ps=decode<std::uint32_t>(read(inputs/"source-mode3-pixel-definition.bin"));require(ps.size()==60,"complete original mode3 definition missing");
    const auto before=read(inputs/"before-scene-color.rgba"),after=read(inputs/"after-scene-color.rgba"),source_depth=read(inputs/"before-scene-depth.raw"),alias=read(inputs/"stage-0-mip-0.rgba");
    require(source_depth.size()==640*480*4,"source plate depth size changed");
    auto low24=source_depth;
    for(std::size_t at=0;at<low24.size();at+=4) {
        std::uint32_t word;std::memcpy(&word,source_depth.data()+at,4);word>>=8;
        std::memcpy(low24.data()+at,&word,4);
    }
    require(before.size()==640*480*4&&after.size()==before.size()&&alias.size()==before.size(),"source scene color/alias dimensions changed");
    for(unsigned i=0;i<640*480;++i){require(alias[i*4]==source_depth[i*4+2]&&alias[i*4+1]==source_depth[i*4+1]&&alias[i*4+2]==source_depth[i*4]&&alias[i*4+3]==source_depth[i*4+3],"actual depth-alias byte packing changed");}
    const auto program=decode_vertex_program(code);require(program.count==5,"literal VS21 changed");
    for(const auto &v:vertices){VertexProgramState state;std::copy(v.registers.begin(),v.registers.end(),state.attributes.begin());std::copy(scroll.begin(),scroll.end(),state.constants.begin()+21);execute_vertex_program(program,state);require(state.outputs[0]==v.registers[0]&&state.outputs[9]==v.registers[1],"literal VS21 immediate position/depth alias differs");for(unsigned i=0;i<3;++i)for(unsigned xy=0;xy<2;++xy)require(state.outputs[10+i][xy]==v.registers[2+i][xy]+scroll[i][xy],"literal VS21 masked scroll differs");}
    std::cout<<"Actual event215778: four original VS21 vertices, complete60-word3A5C0 program, three scroll rows, eight clip regions; all307200 alias/depth pixels exact\n";
    if(argc==1)return 0;
    Gpu g;if(!g.initialize())return 77;if(std::string(argv[1])=="--gpu-baseline")return 0;VkFormatProperties properties;vkGetPhysicalDeviceFormatProperties(g.physical,VK_FORMAT_D24_UNORM_S8_UINT,&properties);require(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,"actual D24S8 attachment unavailable");
    const VkExtent2D extent{640,480};const auto color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_UNDEFINED);upload_depth(g,depth,low24,source_depth);verify_depth(g,depth,low24,source_depth);
    const auto pass=g.scene_pass(false,VK_FORMAT_D24_UNORM_S8_UINT);const auto frame=g.framebuffer(pass,extent,color,depth);
    SourceWeatherPlateRender renderer({g.physical,g.device,pass,HALO2_WEATHER_PLATE_SHADER_DIR,true});SourceWeatherPlatePacket packet;packet.vertex_program=code;std::copy(vertices.begin(),vertices.end(),packet.vertices.begin());std::copy(scroll.begin(),scroll.end(),packet.scroll.begin());std::copy(ps.begin(),ps.end(),packet.pixel.words.begin());std::copy(regions.begin(),regions.end(),packet.clip_regions.begin());packet.clip_range=clip[0];packet.surface={640,480,0,0};packet.depth_clamp=true;packet.depth_clipping=true;
    for(unsigned i=0;i<4;++i){const VkExtent2D size=i?VkExtent2D{256,256}:extent;const unsigned mips=i?7:1;const auto texture=g.image(size,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,mips);upload_texture(g,texture,inputs,i,size.width);VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ci.magFilter=ci.minFilter=i?VK_FILTER_LINEAR:VK_FILTER_NEAREST;ci.mipmapMode=i?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;ci.addressModeU=ci.addressModeV=ci.addressModeW=i?VK_SAMPLER_ADDRESS_MODE_REPEAT:VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;ci.minLod=-1000;ci.maxLod=1000;VkSampler sampler;check(vkCreateSampler(g.device,&ci,nullptr,&sampler));g.samplers.push_back(sampler);packet.textures[i]={texture.view,sampler,VK_FORMAT_R8G8B8A8_UNORM,size,mips};}
    unsigned record_rejected=0;const auto render=[&](bool prepare,bool record_controls=false){if(prepare)renderer.prepare(packet);auto out=g.buffer(before.size(),VK_BUFFER_USAGE_TRANSFER_DST_BIT);g.begin();VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},extent};vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);if(record_controls){const auto reject_record=[&](VkViewport v,VkRect2D r){try{renderer.record(g.command,v,r);}catch(const FormatError&){++record_rejected;return;}throw FormatError("unproved plate viewport/scissor admitted");};
        reject_record({1,0,640,480,0,1},{{0,0},extent});reject_record({0,0,639,480,0,1},{{0,0},extent});reject_record({0,0,640,480,.1f,1},{{0,0},extent});reject_record({0,0,640,480,0,1},{{-1,0},extent});reject_record({0,0,640,480,0,1},{{0,0},{641,480}});}
        renderer.record(g.command,{0,0,640,480,0,1},{{0,0},extent});vkCmdEndRenderPass(g.command);g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,out);g.end();std::vector<std::byte> result(out.size);std::memcpy(result.data(),out.mapped,out.size);return result;};
    const auto native=render(true);save(output/"native-after-scene-color.rgba",native);verify_depth(g,depth,low24,source_depth);
    unsigned rejected=0;const auto reject=[&](SourceWeatherPlatePacket bad){try{renderer.prepare(bad);}catch(const FormatError&){++rejected;return;}throw FormatError("unproved source plate packet admitted");};
    auto bad=packet;bad.pixel.words[54]^=1;reject(bad);bad=packet;bad.vertices[1].registers[0][3]+=1;reject(bad);bad=packet;bad.textures[0].format=VK_FORMAT_D32_SFLOAT;reject(bad);bad=packet;bad.textures[0].mips=2;reject(bad);bad=packet;bad.surface[0]=640.5f;reject(bad);bad=packet;bad.vertices[0].registers[0][0]=std::numeric_limits<float>::max();reject(bad);bad=packet;bad.vertices[0].registers[2][0]=std::numeric_limits<float>::max();bad.scroll[0][0]=std::numeric_limits<float>::max();reject(bad);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(render(false,true)==native,"rejected prepare mutated retained actual plate bindings");verify_depth(g,depth,low24,source_depth);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(render(true)==native,"reused actual plate scene bindings differ");verify_depth(g,depth,low24,source_depth);require(rejected==7&&record_rejected==5,"source admission/record controls incomplete");require(validation_errors==0,"weather plate scene validation diagnostics");
    std::cout<<"All307200 source D24 integers/stencil bytes exact before and after; alpha unchanged, reused bindings exact, seven unproved packets and five viewport/scissor controls rejected; retained bindings exact after rejection\n";
    report(before,after,native,exact);
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
