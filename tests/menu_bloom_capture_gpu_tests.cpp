#include "gpu_postprocess.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <iostream>

namespace {
std::vector<std::byte> read(const std::filesystem::path &path,std::size_t expected) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || std::size_t(file.tellg())!=expected)throw std::runtime_error("bloom capture fixture size differs: "+path.string());
    std::vector<std::byte> bytes(expected);file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    if(!file)throw std::runtime_error("short bloom capture fixture");return bytes;
}
unsigned compare(const std::vector<std::byte> &pixels,const std::vector<std::byte> &original,unsigned event) {
    unsigned maximum=0;std::size_t different=0,above_one=0;std::uint64_t sum=0;
    for(std::size_t i=0;i<pixels.size();++i) {
        const unsigned error=unsigned(std::abs(int(std::to_integer<unsigned>(pixels[i]))-int(std::to_integer<unsigned>(original[i]))));
        maximum=std::max(maximum,error);different+=error!=0;above_one+=error>1;sum+=error;
    }
    std::cout<<"original "<<event<<": "<<pixels.size()/4<<" pixels, maximum "<<maximum<<" byte, "<<above_one
             <<" channels above one byte, "<<different<<" changed channels, mean "<<double(sum)/(pixels.size()/4*4)<<'\n';
    return different;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=4)throw std::runtime_error("bloom fixture/output directories and original map required");
        const auto directory=std::filesystem::path(argv[1]),output=std::filesystem::path(argv[2]);
        std::filesystem::create_directories(output);
        const auto shaders=std::filesystem::path(HALO2_CAPTURE_SHADER_DIR);
        const std::array events{8048u,8062u,8076u,8090u,8104u,8118u,8132u};
        const auto filter=halo2::Cache(argv[3]).load_menu_scene_filter();
        const auto captured_filter=read(directory/"8132-uniform.bin",272);
        if(std::memcmp(filter.words.data(),captured_filter.data(),sizeof(filter.words)))
            throw std::runtime_error("authored title filter program differs from original captured program");
        unsigned source_width=640,source_height=480,failures=0;
        auto source=read(directory/"8034.rgba",source_width*source_height*4);
        for(unsigned index=0;index<events.size();++index) {
            const auto event=events[index];const unsigned width=index>=5?640:160,height=index>=5?480:120;
            const halo2_test::PostImage sampled{0,source_width,source_height,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_LINEAR,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,source};
            halo2_test::Postprocess renderer;
            const auto previous=index==5?read(directory/"8034.rgba",640*480*4):std::vector<std::byte>{};
            source=renderer.render(shaders/"bloom.vert.spv",shaders/"bloom.frag.spv",1,
                read(directory/(std::to_string(event)+"-uniform.bin"),272),{&sampled,1},width,height,
                index==5?halo2_test::PostBlend::Screen:halo2_test::PostBlend::Disabled,previous);
            failures+=compare(source,read(directory/(std::to_string(event)+".rgba"),width*height*4),event);
            std::ofstream file(output/(std::to_string(event)+".rgba"),std::ios::binary);
            file.write(reinterpret_cast<const char*>(source.data()),source.size());
            if(!file)throw std::runtime_error("cannot write captured bloom result");
            source_width=width;source_height=height;
        }
        if(failures)throw std::runtime_error("production bloom shader chain differs from captured original pixels");
        std::cout<<"710400 captured original bloom/filter RGBA pixels checked through production shader chain; complete native scene parity remains unverified\n";
    }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
