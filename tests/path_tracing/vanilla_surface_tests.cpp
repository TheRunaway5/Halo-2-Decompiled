#include "halo2/path_tracing/surface.hpp"
#include "../scalar_combiners.hpp"
#include <cmath>
#include <iostream>
#include <random>
using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
// Literal current retail material programs: Arbiter armor, Elite torso, Chief.
// Captured source controls are independent of the production family recognizer.
constexpr std::array<std::uint32_t,60> programs[]{
    {
        0x18190000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x0c010000u,0x00001c80u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000000c0u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x08090000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00ffffffu,0x00000000u,0x000100c0u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00011101u,0x00000021u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    },
    {
        0x00000000u,0x00000000u,0x00000000u,0x18190000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x0c0f0000u,0x04051c80u,
        0x00ff0000u,0x00ffffffu,0x00ffffffu,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x0000ff00u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000000c0u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x010b020bu,0x2c200c01u,0x2d200d01u,0x08090000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000030cdu,0x00000400u,0x00000500u,0x000100c0u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00011104u,0x00008021u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    },
    {
        0x0b200000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x18190000u,0x00000000u,0x0c0f0000u,0x04051c80u,
        0x00ff0000u,0x00ffffffu,0x00ffffffu,0x00ffffffu,0x00ffffffu,0x00000000u,0x00000000u,0x00000000u,0x0000ff00u,0x00000000u,
        0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000000c0u,0x00000000u,0x00000000u,0x00000000u,
        0x00000000u,0x00000000u,0x000000c0u,0x00000000u,0x010b020bu,0x2c200c01u,0x2d200d01u,0x3c201c01u,0x3b201b01u,0x04050c0du,
        0x08090000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000030cdu,0x00000400u,0x00000500u,0x00000c00u,0x00000d00u,
        0x00000045u,0x000100c0u,0x00000000u,0x00011107u,0x00008021u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    },
};
void require(bool value,const char* why){if(!value)throw FormatError(why);}
Material source(unsigned family) {
    Material m;m.forward_pass=true;m.forward_vertex=3;m.forward_pass_index=0;
    m.passes.resize(1);auto& pass=m.passes[0];
    pass.name="shaders\\shader_passes\\texture_mapping\\";
    pass.name+=family==0?"default_texture_zwrite":family==1?"default_texture_two_change_color_zwrite":
        "default_texture_four_change_color_zwrite";
    pass.vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_texture";
    for(const auto word:pt::unlit_uv_detail::texture_words)for(unsigned byte=0;byte<4;++byte)
        m.vertex_program.push_back(std::byte(word>>(byte*8)));
    pass.vertex_code=m.vertex_program;m.forward_program.words=programs[family];
    m.diffuse=m.detail=m.auxiliary={1,1,11,{std::byte{128},std::byte{128},std::byte{128},std::byte{255}}};
    m.forward_transforms[0]={1,1,0,0};m.forward_transforms[1]={2,3,.125f,.25f};
    m.forward_transforms[2]={4,5,6,7};
    return m;
}
}
int main()try {
    std::mt19937 random(0x2a17c0u);std::uniform_real_distribution<float> value(0,1);
    unsigned cases{};float maximum{};
    for(unsigned family=0;family<3;++family) {
        auto material=source(family);
        const auto surface=pt::qualified_surface(material);
        require(surface && unsigned(surface->family)==8+family,
            "Vanilla armor source texture/detail/change-color program fell back to T0 only");
        require(surface->stages==(family?11u:3u),"Vanilla armor uses the wrong source texture stages");
        for(unsigned n=0;n<4096;++n) {
            for(unsigned word=10;word<26;++word)material.forward_program.words[word]=random();
            // The two mask selectors are part of this exact admitted source family.
            material.forward_program.words[10]=0x00ff0000u;material.forward_program.words[18]=0x0000ff00u;
            material.forward_program.words[43]=random();material.forward_program.words[44]=random();
            std::array<ShaderVector,4> samples;for(auto& sample:samples)for(auto& c:sample)c=value(random);
            const auto expected=halo2_test::scalar_combiners(material.forward_program.words,samples);
            const auto actual=pt::surface_color(surface->family,pt::surface_constants(material,surface->family),samples);
            for(unsigned c=0;c<4;++c) {
                maximum=std::max(maximum,std::abs(actual[c]-expected[c]));
                require(std::abs(actual[c]-expected[c])<2.e-6f,
                    "Vanilla unlit albedo/mask differs from the complete native combiner");
            }
            ++cases;
        }
        material=source(family);
        for(unsigned word=0;word<60;++word) {
            const bool color=(word>=10 && word<=25) || word==43 || word==44;
            if(color)continue;
            auto changed=material;changed.forward_program.words[word]^=1u;
            require(!pt::qualified_surface(changed),"Foreign texture controls were admitted by vanilla albedo family");
        }
        if(family) {
            for(unsigned selector:{10u,18u}) {
                auto changed=material;changed.forward_program.words[selector]^=0x101u;
                require(!pt::qualified_surface(changed),"Unknown color-mask channel selection was admitted");
            }
        }
        auto changed=material;changed.vertex_program[0]^=std::byte{1};
        require(!pt::qualified_surface(changed),"Unknown native UV body was admitted for vanilla armor");
    }
    std::cout<<"PASS vanilla armor native albedo/detail/change-color: "<<cases<<" source cases, maximum error="<<maximum<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
