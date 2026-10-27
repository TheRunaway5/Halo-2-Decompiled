#include "halo2/source_weather_point_render.hpp"
#include "halo2/source_weather_sprite_profile.hpp"
#include <nlohmann/json.hpp>
#include "source_weather_point_gpu_fixture.hpp"
#include "gpu_compute.hpp"
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iomanip>

using namespace halo2;
namespace {
using halo2_weather_point_fixture::require;
std::vector<std::byte> read(const std::filesystem::path &p) {
    std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f)&&f.tellg()>0,"weather point artifact missing");
    std::vector<std::byte>b(std::size_t(f.tellg()));f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(b.data()),b.size())),"short weather point artifact");return b;
}
template<class T>std::vector<T> decode(const std::vector<std::byte>&b){require(b.size()%sizeof(T)==0,"weather point artifact alignment");std::vector<T>out(b.size()/sizeof(T));std::memcpy(out.data(),b.data(),b.size());return out;}
void save(const std::filesystem::path&p,std::span<const std::byte>b){std::ofstream f(p,std::ios::binary);require(bool(f.write(reinterpret_cast<const char*>(b.data()),b.size())),"weather point output write failed");}
std::array<ShaderVector,12> target_camera(std::array<ShaderVector,12> c,VkExtent2D e) {
    // Literal separately proved12FA0,391F0 caller flag0: no resolution-size
    // multiplier is applied to oPts. Captured projection remains explicit.
    c[10]={float(e.width)*.5f,float(e.height)*-.5f,16777215.f,0};
    c[11]={float(e.width)*.5f+.5f,float(e.height)*.5f+.5f,0,0};return c;
}
struct Probe {std::array<VertexInstruction,136>instructions;std::array<ShaderVector,12>camera;std::array<std::uint32_t,4>flags;ShaderVector position;std::array<std::uint32_t,4>color;};
static_assert(sizeof(Probe)==2416);
std::uint32_t distance(float a,float b){auto x=std::bit_cast<std::uint32_t>(a),y=std::bit_cast<std::uint32_t>(b);return x>y?x-y:y-x;}
void compare(std::span<const ShaderVector>a,std::span<const ShaderVector>b,bool gpu){
    require(a.size()==b.size(),"weather point probe count");unsigned different=0;std::uint32_t max_ulp=0;std::array<float,4>position_error{};
    for(unsigned i=0;i<a.size();++i)for(unsigned c=0;c<4;++c){require(std::isfinite(a[i][c]),"nonfinite weather point output");
        if(std::bit_cast<unsigned>(a[i][c])!=std::bit_cast<unsigned>(b[i][c])){
            ++different;auto ulp=distance(a[i][c],b[i][c]);max_ulp=std::max(ulp,max_ulp);
            const auto o=i%16;
            // Subtracting the viewport translation can magnify ULP distance
            // near zero. Bound only the reciprocal consumers by their native
            // float operation scale; all other source outputs remain exact.
            // This consistency check does not assign NV2A reciprocal precision.
            const auto extent=i/(16*238);const float translation=c==0?std::array<float,4>{320.5f,321.f,960.5f,1920.5f}[extent]:c==1?std::array<float,4>{240.5f,241.f,540.5f,1080.5f}[extent]:0.f;
            const float scale=std::abs(b[i][c]-translation)+std::abs(translation)+1.f;
            const bool division=gpu&&((o==0&&c<3&&std::abs(a[i][c]-b[i][c])<=scale*5.f*std::numeric_limits<float>::epsilon())||(o==6&&c==0&&ulp<=2));
            if(o==0)position_error[c]=std::max(position_error[c],std::abs(a[i][c]-b[i][c]));
            if(!division)throw FormatError("weather point analytic mismatch at output"+std::to_string(i)+" component"+std::to_string(c)+" ULP"+std::to_string(ulp));}}
    std::cout<<(gpu?"GPU":"CPU")<<" analytic952 cases/60928 words: "<<different<<" differ, maxULP"<<max_ulp<<'\n';
    std::cout<<"  position max absolute";for(auto e:position_error)std::cout<<' '<<e;std::cout<<'\n';
}
void upload(halo2_weather_point_fixture::Gpu &g,halo2_weather_point_fixture::Image image,std::span<const std::byte> raw) {
    require(raw.size()==1408,"actual snow sprite partition size");auto b=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(b.mapped,raw.data(),raw.size());g.begin();
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=image.handle;barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,4,0,1};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    VkBufferImageCopy regions[4]{};const unsigned offsets[]{0,1024,1280,1344};for(unsigned i=0;i<4;++i){regions[i].bufferOffset=offsets[i];regions[i].imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i,0,1};regions[i].imageExtent={32u>>i,32u>>i,1};}
    vkCmdCopyBufferToImage(g.command,b.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,4,regions);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);g.end();
}
void upload64(halo2_weather_point_fixture::Gpu &g,halo2_weather_point_fixture::Image image,std::span<const std::byte> raw) {
    require(raw.size()==5456,"actual flood-spore mip payload size");
    auto buffer=g.buffer(raw.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT);std::memcpy(buffer.mapped,raw.data(),raw.size());g.begin();
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.image=image.handle;
    barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,5,0,1};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    VkBufferImageCopy regions[5]{};const unsigned offsets[]{0,4096,5120,5376,5440};
    for(unsigned i=0;i<5;++i){regions[i].bufferOffset=offsets[i];regions[i].imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,i,0,1};regions[i].imageExtent={64u>>i,64u>>i,1};}
    vkCmdCopyBufferToImage(g.command,buffer.handle,image.handle,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,5,regions);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);g.end();
}
}
int main(int argc,char**argv){try{
    require(argc==1||(argc==2&&std::string(argv[1])=="--gpu")||
        (argc==4&&std::string(argv[1])=="--gpu"&&std::string(argv[2])=="--sprite-fixture"),
        "optional--gpu [--sprite-fixture owned-source.json] expected");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR),dir=root/"analysis/multiplayer-shading/weather-particle-packet",reference=root/"analysis/multiplayer-shading/weather-point-prototype";
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"weather test requires owned scratch");
    const auto output=std::filesystem::path(scratch)/"weather-point";std::filesystem::create_directories(output);
    const auto code=read(dir/"source-vs22-class0.vp");const auto program=decode_vertex_program(code);require(program.count==9,"sourceVS22 changed");
    auto inputs=decode<SourceWeatherPointInput>(read(dir/"emitted-source-v0-logical-color.bin"));require(inputs.size()==238,"actual point count changed");
    const auto commands=decode<std::uint32_t>(read(dir/"appended-command-span.bin"));
    unsigned alpha_function_writes=0,alpha_reference_writes=0;
    for(std::size_t i=0;i<commands.size();){
        const auto n=(commands[i]>>18)&0x7ff,method=commands[i]&0x1ffc;
        require(n<=commands.size()-i-1,"short original weather command packet");
        if(method==0x33c){require(n==1&&commands[i+1]==0x206,"source alpha function is not GEQUAL");++alpha_function_writes;}
        if(method==0x340){require(n==1&&commands[i+1]==0,"source alpha reference is not zero");++alpha_reference_writes;}
        i+=n+1;
    }
    require(alpha_function_writes==1&&alpha_reference_writes==1,"original alpha state packet missing");
    auto bank=decode<ShaderVector>(read(dir/"vertex_constants_c0_c11.bin"));require(bank.size()==12,"actual fullcamera missing");std::array<ShaderVector,12>camera;std::copy(bank.begin(),bank.end(),camera.begin());
    require(target_camera(camera,{640,480})==camera,"source12FA0 half-pixel target bank differs from actual draw");
    const std::array<VkExtent2D,4>extents{{{640,480},{641,481},{1920,1080},{3840,2160}}};
    auto expected=decode<ShaderVector>(read(reference/"vertex-analytic.bin"));std::vector<ShaderVector>cpu;std::vector<Probe>probes;
    for(auto e:extents)for(const auto&p:inputs){VertexProgramState s;const auto c=target_camera(camera,e);std::copy(c.begin(),c.end(),s.constants.begin());s.attributes[0]=p.position;s.attributes[1]={float((p.color>>16)&255)/255,float((p.color>>8)&255)/255,float(p.color&255)/255,float(p.color>>24)/255};execute_vertex_program(program,s);cpu.insert(cpu.end(),s.outputs.begin(),s.outputs.end());probes.push_back({program.instructions,c,{0,0,0,program.count},p.position,{p.color,0,0,0}});}
    compare(cpu,expected,false);if(argc==1)return 0;
    {
        halo2_test::Compute<Probe,16>probe(HALO2_WEATHER_POINT_PROBE_SHADER);const auto gpu=probe.run(probes);compare(gpu,expected,true);
        save(output/"vertex-gpu.bin",std::as_bytes(std::span(gpu)));
    }
    halo2_weather_point_fixture::Gpu g;if(!g.initialize())return 77;
    VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(g.physical,&props);
    std::cout<<"Host pointSizeRange="<<props.limits.pointSizeRange[0]<<","<<props.limits.pointSizeRange[1]<<" granularity="<<props.limits.pointSizeGranularity<<'\n';
    const auto sprite=g.image({32,32},VK_FORMAT_BC3_UNORM_BLOCK,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,4);upload(g,sprite,read(dir/"sprite-resident-physical-partition.bin"));
    // Actual unchanged SDK stage3 cache404F50: WRAP,linearMIN/MAG/MIP,
    // bias0/maxMip0. No uncaptured PGRAPH live-state assertion is made.
    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sci.minFilter=sci.magFilter=VK_FILTER_LINEAR;sci.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;sci.addressModeU=sci.addressModeV=sci.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;sci.maxLod=VK_LOD_CLAMP_NONE;
    VkSampler sampler;halo2_weather_point_fixture::check(vkCreateSampler(g.device,&sci,nullptr,&sampler));g.samplers.push_back(sampler);
    auto ps=decode<std::uint32_t>(read(dir/"pixel_definition_39780.bin"));require(ps.size()==60,"complete39780 missing");PixelProgram pixel;std::copy(ps.begin(),ps.end(),pixel.words.begin());
    const auto pass=g.scene_pass(true);SourceWeatherPointRender renderer({g.physical,g.device,pass,HALO2_WEATHER_POINT_SHADER_DIR,true,true});
    SourceWeatherPointPacket packet{code,camera,pixel,inputs,sprite.view,sampler,VK_FORMAT_BC3_UNORM_BLOCK,{32,32},4,{{0,16777215.f,std::bit_cast<float>(0x437fffffu),16777215.f},{640,480,0,0},0,0,7,true,true,1,1,false,SourceWeatherPointSpriteCoordinates::OriginalGlRawRowsUpperLeft}};
    const auto reject=[&](SourceWeatherPointPacket p){bool failed=false;try{renderer.prepare(p);}catch(const FormatError&){failed=true;}require(failed,"unproved weather packet admitted");};
    auto wrong=packet;wrong.sprite_mips=6;reject(wrong);wrong=packet;wrong.raster.point_parameters_enabled=0;reject(wrong);wrong=packet;wrong.pixel.words[0]^=1;reject(wrong);
    auto altered_code=code;altered_code[8]^=std::byte{1};wrong=packet;wrong.vertex_program=altered_code;reject(wrong);
    wrong=packet;wrong.raster.stencil_enabled.reset();reject(wrong);wrong=packet;wrong.raster.stencil_enabled=true;reject(wrong);wrong=packet;wrong.raster.depth_offset=1;reject(wrong);
    wrong=packet;wrong.raster.sprite_coordinates.reset();reject(wrong);
    auto badpoints=inputs;badpoints[0].position[3]=0;wrong=packet;wrong.points=badpoints;reject(wrong);
    badpoints[0].position[3]=-1;reject(wrong);
    wrong=packet;wrong.camera[3]={0,0,0,0};reject(wrong);wrong.camera[3]={0,0,0,-1};reject(wrong);
    wrong=packet;wrong.camera[3]={0,0,std::numeric_limits<float>::max(),0};reject(wrong);
    VertexProgramState size_reference;std::copy(camera.begin(),camera.end(),size_reference.constants.begin());size_reference.attributes[0]=inputs[0].position;execute_vertex_program(program,size_reference);
    badpoints=inputs;badpoints[0].position[3]=(props.limits.pointSizeRange[1]+16.f)*size_reference.outputs[0][3];wrong=packet;wrong.points=badpoints;reject(wrong);
    badpoints[0].position[3]=props.limits.pointSizeRange[0]*.5f*size_reference.outputs[0][3];reject(wrong);
    const auto render=[&](VkExtent2D e,SourceWeatherPointPacket p,float clear_depth,bool retained_control=false,bool world_admission=false){
        const auto color=g.image(e,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),depth=g.image(e,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);const auto frame=g.framebuffer(pass,e,color,depth);auto out=g.buffer(std::size_t(e.width)*e.height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        p.camera=target_camera(p.camera,e);p.raster.surface={float(e.width),float(e.height),0,0};
        if(world_admission)renderer.prepare_world(p);else renderer.prepare(p);
        if(retained_control){auto invalid=inputs;invalid[0].position[3]=0;auto bad=p;bad.points=invalid;reject(bad);}
        g.begin();
        VkClearValue clear[2]{};clear[0].color={{.0627451f,.09411765f,.1254902f,.25f}};clear[1].depthStencil={clear_depth,0};VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},e};bi.clearValueCount=2;bi.pClearValues=clear;
        vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);
        if(retained_control){
            const auto reject_record=[&](VkViewport v,VkRect2D s){bool failed=false;try{renderer.record(g.command,v,s);}catch(const FormatError&){failed=true;}require(failed,"unproved point viewport/scissor admitted");};
            reject_record({1,0,float(e.width),float(e.height),0,1},{{0,0},e});
            reject_record({0,0,float(e.width)*.5f,float(e.height),0,1},{{0,0},e});
            reject_record({0,0,float(e.width),float(e.height),.25f,1},{{0,0},e});
            reject_record({0,0,float(e.width),float(e.height),0,1},{{-1,0},e});
            reject_record({0,0,float(e.width),float(e.height),0,1},{{1,0},e});
        }
        renderer.record(g.command,{0,0,float(e.width),float(e.height),0,1},{{0,0},e});vkCmdEndRenderPass(g.command);g.readback(color.handle,e,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,out);g.end();
        std::vector<std::byte>result(out.size);std::memcpy(result.data(),out.mapped,out.size);return result;
    };
    for(auto e:extents){const auto actual=render(e,packet,1),occluded=render(e,packet,0);require(actual!=occluded,"actual points have no raster response");
        auto noalpha=inputs;for(auto &p:noalpha)p.color&=0xffffff;auto zero=packet;zero.points=noalpha;require(render(e,zero,1)==occluded,"zero-alpha neutral blending control failed");
        auto nowrite=packet;nowrite.raster.color_write_mask=0;require(render(e,nowrite,1)==occluded,"colorwrite negativecontrol failed");
        auto clipped=packet;clipped.raster.clip_range[2]=16777215.f;require(render(e,clipped,1)==occluded,"sourceWclip negativecontrol failed");
        auto resized=inputs;for(auto &p:resized)p.position[3]*=2.f;auto doubled=packet;doubled.points=resized;require(render(e,doubled,1)!=actual,"authoredoPts point-size control failed");
        auto tinted=inputs;for(auto &p:tinted)p.color=0xff0040ff;auto tint=packet;tint.points=tinted;require(render(e,tint,1)!=actual,"immediate logicalARGB color control failed");
        require(render(e,packet,1)==actual,"reused weather point bindings differ");
        require(render(e,packet,1,false,true)==actual,"world point admission changed visible source packet");
        auto behind=packet;behind.camera[3]={0,0,0,-1};
        require(render(e,behind,1,false,true)==occluded,"negative homogeneous W entered world point raster");
        require(render(e,clipped,1,false,true)==occluded,"world point admission bypassed source W clip");
        auto subpixel=inputs;
        for(auto &point:subpixel) {
            VertexProgramState s;std::copy(camera.begin(),camera.end(),s.constants.begin());
            s.attributes[0]=point.position;execute_vertex_program(program,s);
            point.position[3]=s.outputs[0][3]*(props.limits.pointSizeRange[0]*.5f);
        }
        auto small=packet;small.points=subpixel;
        require(render(e,small,1,false,true)!=occluded,"native programmable point range lost subpixel snow");
        require(render(e,packet,1,true)==actual,"rejected packet/view mutation destroyed the retained valid binding");
        unsigned changed=0;for(unsigned i=0;i<actual.size();i+=4){require(actual[i+3]==occluded[i+3],"inheritedRGB-only state changed alpha");if(!std::equal(actual.begin()+i,actual.begin()+i+3,occluded.begin()+i))++changed;}
        std::cout<<e.width<<"x"<<e.height<<": "<<changed<<" changed pixels; depth/alpha/color/Wclip/oPts controls passed\n";
        save(output/("controlled-"+std::to_string(e.width)+"x"+std::to_string(e.height)+".rgba"),actual);
    }
    if(argc==4) {
        std::ifstream stream(argv[3]);require(bool(stream),"missing actual64-square sprite fixture");
        const auto source=nlohmann::json::parse(stream);
        require(source.at("schema")=="H2_WEATHER_SPRITE_RESOURCE_FIXTURE_V1"&&source.at("cases").size()==2,
            "actual original/prepared weather resource census changed");
        std::vector<std::byte> previous;
        for(const auto& owner:source.at("cases")) {
            const auto& d=owner.at("descriptor");require(d.at("width")==64&&d.at("height")==64&&d.at("format")==16&&d.at("type")==0&&d.at("depth")==1,
                "actual64-square weather resource dimensions changed");
            const auto hex=owner.at("raw_resource_hex").get<std::string>();require(hex.size()%2==0,"incomplete weather resource hex");
            std::vector<std::byte> raw;for(std::size_t i=0;i<hex.size();i+=2)raw.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
            require(raw.size()>=5456,"short actual weather resource");
            const auto flood=g.image({64,64},VK_FORMAT_BC3_UNORM_BLOCK,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,5);
            upload64(g,flood,std::span(raw).first(5456));
            auto packet64=packet;packet64.sprite_view=flood.view;packet64.sprite_extent={64,64};packet64.sprite_mips=5;
            const auto actual=render({640,480},packet64,1),hidden=render({640,480},packet64,0);
            require(actual!=hidden,"actual64-square sprite has no GPU effect response");
            require(render({640,480},packet64,1)==actual,"actual64-square sprite binding reuse differs");
            if(previous.empty())previous=actual;else require(previous==actual,"original/prepared64-square sprite GPU resource bindings differ");
            auto wrong64=packet64;wrong64.sprite_mips=4;reject(wrong64);wrong64.sprite_mips=6;reject(wrong64);
            wrong64=packet64;wrong64.sprite_extent={32,32};reject(wrong64);
            wrong64=packet64;wrong64.sprite_extent={32,64};reject(wrong64);
            wrong64=packet64;wrong64.sprite_extent={64,32};reject(wrong64);
            wrong64=packet64;wrong64.sprite_format=VK_FORMAT_BC1_RGBA_UNORM_BLOCK;reject(wrong64);
            require(render({640,480},packet64,1)==actual,"rejected64-square profile mutation damaged valid bindings");
        }
        std::cout<<"Actual original/prepared64-square/five-mip resource GPU bindings/reuse match; retained238-point component input, not live flood-spore or original64-square raster parity\n";
    }
    require(halo2_weather_point_fixture::validation_errors==0,"weather point validation diagnostics");
    std::cout<<"Isolated actual238-point packet over CONTROLLED clear color/depth; no original scene or physicalXbox coverage claim\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
