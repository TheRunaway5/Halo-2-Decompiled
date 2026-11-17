#include "halo2/path_tracing/surface.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/vertex_program.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <random>

using namespace halo2;
namespace pt=halo2::path_tracing;
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply prepared introduction cache and small report path");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);std::mt19937 random(0x24d37a1u);
    std::uniform_real_distribution<float> channel(0,1),coordinate(-3,3);
    nlohmann::json rows=nlohmann::json::array();bool admitted=true,controls=true;
    unsigned cases{},uv_cases{};float rgb_error{},alpha_error{},uv_error{},fallback_error{};
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad" || (!tag.name.ends_with("\\capital_ship_a") && !tag.name.ends_with("\\capital_ship_b")))continue;
        auto material=cache.load_material(tag.datum);material.selected_shader_lod=0;
        if(!prepare_dynamic_material(cache,material,{},true,0.f))throw FormatError("Actual cruiser source did not prepare");
        const auto qualified=pt::qualified_surface(material);admitted&=qualified && unsigned(qualified->family)==11;
        admitted&=pt::qualified_unlit_white_tint(material);
        const auto program=decode_vertex_program(material.vertex_program);
        for(unsigned sample=0;sample<8192;++sample) {
            auto words=material.forward_program.words;
            for(unsigned n=10;n<=25;++n)words[n]=random();words[43]=random();words[44]=random();
            std::array<ShaderVector,4> textures;ShaderVector v0,v1;
            for(auto& texture:textures)for(auto& c:texture)c=channel(random);
            for(auto& c:v0)c=channel(random);for(auto& c:v1)c=channel(random);
            const auto full=halo2_test::scalar_combiners(words,textures,v0,v1);
            // Native stage1 writes T0.a*T1.a; stage6 writes twice C1.a
            // (word24) times that product to v0.a; final G selects v0.a.
            const float alpha=std::clamp(2.f*float(words[24]>>24)/255.f*textures[0][3]*textures[1][3],0.f,1.f);
            alpha_error=std::max(alpha_error,std::abs(full[3]-alpha));
            // Isolate diffuse while preserving every native combiner stage:
            // signed bump.T3 dot=.5, diffuse C0=white/C1=black; suppress
            // only the independent specular and ambient RGB constants.
            auto isolated=words;isolated[12]|=0xffffffu;
            for(unsigned word:{14u,15u,20u})isolated[word]&=0xff000000u;
            auto t=textures;t[2]={.5f,.5f,1.f,textures[2][3]};t[3]={.5f,.5f,.75f,textures[3][3]};
            const auto unlit=halo2_test::scalar_combiners(isolated,t,v0,v1);
            ShaderVector expected{};
            for(unsigned c=0;c<3;++c) {
                expected[c]=std::clamp(2.f*textures[0][c]*textures[1][c],0.f,1.f);
                rgb_error=std::max(rgb_error,std::abs(unlit[c]-expected[c]));
                fallback_error=std::max(fallback_error,std::abs(textures[0][c]-expected[c]));
            }
            expected[3]=alpha;
            if(qualified) {
                auto changed=material;changed.forward_program.words=words;
                const auto fixed=pt::surface_color(qualified->family,pt::surface_constants(changed,qualified->family),textures);
                for(unsigned c=0;c<3;++c)rgb_error=std::max(rgb_error,std::abs(fixed[c]-expected[c]));
                alpha_error=std::max(alpha_error,std::abs(fixed[3]-expected[3]));
            }
            ++cases;
        }
        for(unsigned sample=0;sample<256;++sample) {
            VertexProgramState state;
            for(auto& attr:state.attributes)for(auto& c:attr)c=coordinate(random);
            for(auto& constant:state.constants)for(auto& c:constant)c=coordinate(random);
            state.constants[172]={1,1,0,0};
            const Vec2 raw{coordinate(random),coordinate(random)};state.attributes[1]={raw.x,raw.y,0,1};
            auto changed=material;
            for(unsigned n=0;n<3;++n)for(unsigned c=0;c<4;++c)
                changed.forward_transforms[n][c]=state.constants[18+n][c];
            execute_vertex_program(program,state);
            const auto& base=changed.forward_transforms[0];const Vec2 uv{raw.x*base[0]+base[2],raw.y*base[1]+base[3]};
            for(unsigned stage=0;stage<2;++stage) {
                const auto transform=stage?changed.forward_transforms[stage]:ShaderVector{1,1,0,0};
                const Vec2 expected{uv.x*transform[0]+transform[2],uv.y*transform[1]+transform[3]};
                const auto& actual=state.outputs[9+stage];
                uv_error=std::max({uv_error,std::abs(expected.x-actual[0]),std::abs(expected.y-actual[1]),std::abs(actual[3]-1.f)});
                if(const auto model=pt::qualified_unlit_uv(changed)) {
                    const auto fixed=pt::unlit_uv(*model,raw)[stage];
                    uv_error=std::max({uv_error,std::abs(fixed.x-actual[0]),std::abs(fixed.y-actual[1])});
                }
            }
            ++uv_cases;
        }
        if(qualified)for(unsigned word=0;word<60;++word) {
            auto changed=material;changed.forward_program.words[word]^=1u;
            const bool color=(word>=10 && word<=25)||word==43||word==44;
            controls&=bool(pt::qualified_surface(changed))==color;
        }
        auto changed=material;changed.vertex_program.back()^=std::byte{1};controls&=!pt::qualified_surface(changed);
        rows.push_back({{"name",tag.name},{"admitted",bool(qualified)},{"white_tint",pt::qualified_unlit_white_tint(material)},
            {"vertex_shader",material.passes.at(material.forward_pass_index).vertex_shader},
            {"source_program",material.forward_program.words},{"source_reflection_mask_color",material.forward_program.words[24]}});
    }
    const bool passed=rows.size()==2 && admitted && controls && rgb_error<2.e-6f && alpha_error<2.e-6f && uv_error==0 && fallback_error>.2f;
    nlohmann::json report{{"passed",passed},{"materials",rows},{"scalar_cases",cases},{"vertex_cases",uv_cases},
        {"maximum_rgb_error",rgb_error},{"maximum_alpha_error",alpha_error},{"maximum_uv_error",uv_error},
        {"maximum_T0_fallback_error",fallback_error},{"control_mutations_passed",controls}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return passed?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
