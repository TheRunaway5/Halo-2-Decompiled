#include "gpu_postprocess.hpp"
#include <iostream>
#include <cstdint>

namespace {
std::vector<std::byte> read(const std::filesystem::path &path,std::size_t size) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f || std::size_t(f.tellg())!=size)throw std::runtime_error("Original armor fixture size differs: "+path.string());
    std::vector<std::byte> data(size);f.seekg(0);f.read(reinterpret_cast<char*>(data.data()),size);
    if(!f)throw std::runtime_error("Short original armor fixture");return data;
}
}
int main(int argc,char **argv)try {
    if(argc!=4 && argc!=5)throw std::runtime_error("Supply original armor fixtures, shaders, output directory and optional fragment shader");
    const std::filesystem::path fixtures=argv[1],shaders=argv[2],output=argv[3];
    std::filesystem::create_directories(output);unsigned failures=0;
    for(const auto species:{"spartan","elite"}) {
        const auto p=fixtures/species;std::ifstream count_file(p/"count.txt");unsigned count;
        if(!(count_file>>count) || count>256)throw std::runtime_error("Original armor vertex count differs");
        std::vector<halo2_test::PostImage> sampled{
            {0,128,128,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_LINEAR,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,read(p/"texture.rgba",87380),8}};
        // Bind valid inert resources for the unused production texture stages.
        for(unsigned binding=1;binding<4;++binding)
            sampled.push_back({binding,1,1,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_NEAREST,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,std::vector<std::byte>(4)});
        for(unsigned i=0;i<4;++i) {
            sampled.push_back({5,1,1,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_NEAREST,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,std::vector<std::byte>(24),1,VK_IMAGE_VIEW_TYPE_CUBE,i});
            sampled.push_back({6,1,1,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_NEAREST,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,std::vector<std::byte>(4),1,VK_IMAGE_VIEW_TYPE_3D,i});
        }
        const auto before=read(p/"before.rgba",1228800),expected=read(p/"after.rgba",1228800);
        const auto material=read(p/"material.bin",3712),program=read(p/"program.bin",240);
        if(!std::equal(program.begin(),program.end(),material.begin()))throw std::runtime_error("Captured native material changed its original emblem program");
        halo2_test::Postprocess renderer;
        // Only the vertex input adapter belongs to the test. Both later stages
        // are the production shaders, and depth compares against real D24S8.
        const auto pixels=renderer.render(shaders/"player_armor_emblem_capture.vert.spv",shaders/(argc==5?argv[4]:"world_emblem.frag.spv"),
            4,material,sampled,640,480,halo2_test::PostBlend::AlphaRGB,before,
            read(p/"vertices.bin",12288),count,7,VK_CULL_MODE_FRONT_BIT,VK_FRONT_FACE_CLOCKWISE,
            shaders/"world_emblem.geom.spv",read(p/"depth.d24",1228800));
        std::size_t different=0,changed=0;unsigned maximum=0;std::uint64_t sum=0;
        for(std::size_t i=0;i<pixels.size();++i) {
            const auto error=unsigned(std::abs(int(std::to_integer<unsigned>(pixels[i]))-int(std::to_integer<unsigned>(expected[i]))));
            different+=error!=0;maximum=std::max(maximum,error);sum+=error;changed+=expected[i]!=before[i];
        }
        std::ofstream f(output/(std::string(species)+".rgba"),std::ios::binary);f.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
        std::cout<<species<<": "<<changed<<" original changed channels, "<<different<<" differing framebuffer channels, max "<<maximum<<", sum "<<sum<<std::endl;
        failures+=different!=0 || changed==0;
    }
    if(failures)throw std::runtime_error("Native armor emblem pass differs from original framebuffer");
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
