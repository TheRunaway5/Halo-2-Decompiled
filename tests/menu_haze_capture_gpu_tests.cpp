#include "halo2/atmosphere.hpp"
#include "gpu_postprocess.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
std::vector<std::byte> read(const std::filesystem::path &path,std::size_t expected) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || std::size_t(file.tellg())!=expected)throw std::runtime_error("haze capture fixture size differs: "+path.string());
    std::vector<std::byte> bytes(expected);file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    if(!file)throw std::runtime_error("short haze capture fixture");return bytes;
}
std::vector<std::byte> uniform(const std::filesystem::path &path) {
    auto raw=read(path,sizeof(halo2::PatchyFogUniform));
    halo2::PixelProgram program;
    std::memcpy(program.words.data(),raw.data(),sizeof(program.words));
    const auto native=halo2::patchy_fog_combiner_constants(program);
    if(std::memcmp(native.data(),raw.data()+offsetof(halo2::PatchyFogUniform,combiner_constants),sizeof(native)))
        throw std::runtime_error("native haze float constants differ from captured original upload");
    std::memcpy(raw.data()+offsetof(halo2::PatchyFogUniform,combiner_constants),native.data(),sizeof(native));
    return raw;
}
void save(const std::filesystem::path &path,const std::vector<std::byte> &pixels) {
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
    if(!file)throw std::runtime_error("cannot write haze capture result");
}
unsigned compare(const std::vector<std::byte> &pixels,const std::vector<std::byte> &original,
                 const std::string &label,unsigned channels,bool exact=false) {
    unsigned maximum=0;std::size_t different=0,above_one=0;std::uint64_t sum=0;
    for(std::size_t i=0;i<pixels.size();++i)if(i%4<channels) {
        const unsigned error=unsigned(std::abs(int(std::to_integer<unsigned>(pixels[i]))-int(std::to_integer<unsigned>(original[i]))));
        maximum=std::max(maximum,error);different+=error!=0;above_one+=error>1;sum+=error;
    }
    std::cout<<label<<": "<<pixels.size()/4<<" pixels, maximum "<<maximum<<" byte, "<<above_one
             <<" channels above one byte, "<<different<<" changed channels, mean "<<double(sum)/(pixels.size()/4*channels)<<'\n';
    return exact?different:above_one;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=4)throw std::runtime_error("original map, haze fixture and output directories required");
        const auto fog=halo2::Cache(argv[1]).load_menu_atmosphere();
        const auto directory=std::filesystem::path(argv[2]),output=std::filesystem::path(argv[3]);
        std::filesystem::create_directories(output);
        const auto bitmap=read(directory/"bitmap.rgba",256*256*4),gradient=read(directory/"gradient.rgba",16*16*4);
        const auto &native_bitmap=fog.patchy->noise.at(0);
        std::cout<<"native noise bitmap "<<native_bitmap.width<<'x'<<native_bitmap.height<<" format "<<native_bitmap.format
                 <<" pixels "<<native_bitmap.pixels.size()<<'\n';
        save(output/"native-bitmap.bin",native_bitmap.pixels);
        if(native_bitmap.width!=256 || native_bitmap.height!=256 || native_bitmap.format!=11 ||
           native_bitmap.pixels.size()<bitmap.size() ||
           !std::equal(bitmap.begin(),bitmap.end(),native_bitmap.pixels.begin()))
            throw std::runtime_error("native decoded noise bitmap differs from original sampled texture");
        if(fog.gradient.width!=16 || fog.gradient.height!=16 || fog.gradient.pixels!=gradient)
            throw std::runtime_error("native gradient differs from original sampled texture");
        using namespace halo2_test;
        const auto shaders=std::filesystem::path(HALO2_CAPTURE_SHADER_DIR);
        const PostImage noise_image{2,256,256,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_LINEAR,VK_SAMPLER_ADDRESS_MODE_REPEAT,bitmap};
        Postprocess noise;
        const auto quad=read(directory/"noise-quad.bin",352);
        const auto noise_pixels=noise.render(shaders/"patchy_noise.vert.spv",shaders/"patchy_noise.frag.spv",3,
            uniform(directory/"noise-uniform.bin"),{&noise_image,1},320,240,PostBlend::Disabled,{},quad,6);
        save(output/"noise.rgba",noise_pixels);
        unsigned failures=compare(noise_pixels,read(directory/"noise-output.rgba",320*240*4),"original 8005 noise",4,true);
        const PostImage native_noise_image{2,256,256,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_LINEAR,VK_SAMPLER_ADDRESS_MODE_REPEAT,
            std::vector<std::byte>(native_bitmap.pixels.begin(),native_bitmap.pixels.begin()+bitmap.size())};
        Postprocess native_noise;
        const auto native_noise_pixels=native_noise.render(shaders/"patchy_noise.vert.spv",shaders/"patchy_noise.frag.spv",3,
            uniform(directory/"noise-uniform.bin"),{&native_noise_image,1},320,240,PostBlend::Disabled,{},quad,6);
        save(output/"native-decoded-noise.rgba",native_noise_pixels);
        failures+=compare(native_noise_pixels,read(directory/"noise-output.rgba",320*240*4),"native decoded upload 8005 noise",4,true);
        // Use the actual original noise target for this isolated composition
        // check; a failure in noise generation must not hide composite errors.
        const std::array<PostImage,3> sampled{{
            {0,640,480,VK_FORMAT_R32_SFLOAT,VK_FILTER_NEAREST,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,read(directory/"depth-r32.bin",640*480*4)},
            {1,16,16,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_LINEAR,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,gradient},
            {2,320,240,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_LINEAR,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,read(directory/"sampled-noise.rgba",320*240*4)}}};
        Postprocess composite;
        const auto composite_pixels=composite.render(shaders/"patchy_noise.vert.spv",shaders/"patchy_composite.frag.spv",3,
            uniform(directory/"composite-uniform.bin"),sampled,640,480,PostBlend::Premultiplied,
            read(directory/"before-output.rgba",640*480*4),read(directory/"composite-quad.bin",352),6);
        save(output/"composite.rgba",composite_pixels);
        failures+=compare(composite_pixels,read(directory/"composite-output.rgba",640*480*4),"original 8034 direct haze",4,true);
        if(failures)throw std::runtime_error("production haze shaders differ from captured original pixels");
        std::cout<<"original captured haze sampled passes verified; complete native scene parity remains unverified\n";
    }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
