#include "halo2/render_program.hpp"
#include "gpu_postprocess.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>

namespace {
struct PatchyUniform {
    std::array<std::uint32_t,60> program;
    std::array<halo2::ShaderVector,8> rows{};
    halo2::ShaderVector controls,lookup;
};
static_assert(sizeof(PatchyUniform)==400);
// TU1.5 Colossus direct fog draw: 40..50 range, captured 27500 quad W
// 655359.9375, ZENABLE=2, LESSEQUAL, ZWRITEENABLE=0. This exercises the
// production fragment shader's depth admission, independently of fog motion.
constexpr std::array<std::uint32_t,60> program{
    0,0x04209140,0x18201140,0xdd202df1,0x18011120,0x0d200d20,0x1d120c02,0x1d0d0d20,
    0x1d010000,0x1d00,0xff0000,0,0x0a000000,0x01000000,0,0,
    0,0,0xff00,0,0,0,0,0x80000000,
    0xcc000033,0,0,0xc00,0xd00,0xd00,0xc00,0x20d00,
    0x10d00,0x4d00,0x08010802,0x04201140,0x2020,0xcc202d20,0x1d201d20,0x39111912,
    0,0,0,0x9a5a34,0,0x3045,0xc00,0x4d00,
    0xc00,0x20d00,0xd00,0,0,0x11108,0x21,0,
    0,0,0,0};
}
int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("production shader directory required");
        using namespace halo2_test;
        constexpr std::array<std::uint32_t,6> depths{104741,655358,655359,655360,0xffffff,0};
        constexpr std::array<std::byte,4> backdrop{std::byte{13},std::byte{27},std::byte{41},std::byte{83}};
        std::vector<std::byte> previous(depths.size()*4),sampled_depth(depths.size()*4);
        for(unsigned i=0;i<depths.size();++i) {
            std::memcpy(previous.data()+i*4,backdrop.data(),4);
            const float depth=float(depths[i])/16777215.f;
            std::memcpy(sampled_depth.data()+i*4,&depth,4);
        }
        std::vector<PostImage> sampled{{0,unsigned(depths.size()),1,VK_FORMAT_R32_SFLOAT,
            VK_FILTER_NEAREST,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,sampled_depth},
            {1,1,1,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_NEAREST,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                std::vector<std::byte>(4,std::byte{255})}};
        for(unsigned i=0;i<4;++i) {
            PostImage noise{2,1,1,VK_FORMAT_R8G8B8A8_UNORM,VK_FILTER_NEAREST,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,std::vector<std::byte>(4,std::byte{231})};
            noise.array_element=i;sampled.push_back(noise);
        }
        const auto shaders=std::filesystem::path(argv[1]);
        for(bool enabled:{true,false}) {
            const PatchyUniform uniform{program,{},
                {1,float(depths.size()),1,enabled?655359.9375f:0.f},{102.4f,0,-4,0}};
            Postprocess gpu;
            const auto pixels=gpu.render(shaders/"bloom.vert.spv",shaders/"bsp_patchy_composite.frag.spv",
                3,std::as_bytes(std::span{&uniform,1}),sampled,unsigned(depths.size()),1,
                PostBlend::Disabled,previous);
            for(unsigned i=0;i<depths.size();++i) {
                const bool preserved=std::equal(backdrop.begin(),backdrop.end(),pixels.begin()+i*4);
                const bool rejected=enabled && (i==0 || i==1 || i==5);
                if(preserved!=rejected)throw std::runtime_error("patchy fog depth admission differs at case "+
                    std::to_string(i)+(enabled?" with LESSEQUAL":" with Z disabled"));
            }
        }
        std::cout<<"production patchy fog: near surfaces rejected, equal/far depths admitted, disabled-depth control passed\n";
    } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
