#include "halo2/pstencil/detail.hpp"
#include "halo2/pstencil/lighting.hpp"
#include "halo2/object_lighting.hpp"
#include "../gpu_compute.hpp"
#include "../scalar_combiners.hpp"
#include <iostream>
#include <random>
#include <set>
using namespace halo2;
namespace {
void require(bool v,const char* why){if(!v)throw FormatError(why);}
struct Fixture {
    std::array<std::uint32_t,60> program;
    std::array<ShaderVector,4> samples;
    ShaderVector facing,glancing;
};
static_assert(sizeof(Fixture)==336);
}
int main(int argc,char** argv){try {
    require(argc==3,"supply Cairo cache and compute shader");Cache cache(argv[1]);
    ForwardRasterState raster;raster.color_write=0x1010101u;raster.depth_function=0x202u;
    const auto light=pstencil::light_accumulation_state(raster);
    require(light.color_write==0x10101u && light.depth_function==raster.depth_function &&
        raster.color_write==0x1010101u,"light accumulation lost its RGB-only mask or changed Original state");
    std::mt19937 random(0x2003de);std::uniform_real_distribution<float> value(0,1);
    std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    std::set<std::string> names;unsigned passes{},different{},negative{};
    for(const auto& tag:cache.tags())if(tag.group=="shad" && tag.name.starts_with("objects\\characters\\elite\\")) {
        const auto authored=cache.load_material(tag.datum);
        for(unsigned lod=0;lod<authored.lods.size();++lod) {
            auto material=authored;material.selected_shader_lod=lod;
            if(!prepare_dynamic_material(cache,material,{},true,0.f))continue;
            const auto stage=pstencil::reference_neutral_detail_stage(material);
            if(!stage)continue;
            names.insert(tag.name);++passes;
            const auto adapted=pstencil::neutral_detail_response(material.forward_program,*stage);
            for(unsigned w=0;w<60;++w)if(w!=*stage && w!=34+*stage && w!=18+*stage)
                require(adapted.words[w]==material.forward_program.words[w],"detail adapter changed unrelated material control");
            auto invalid=material;invalid.name+="_unproven";
            require(!pstencil::reference_neutral_detail_stage(invalid),"detail policy guessed an unproven asset alias");++negative;
            invalid=material;invalid.forward_program.words[45+*stage]^=0x1000u;
            require(!pstencil::reference_neutral_detail_stage(invalid),"detail policy admitted an unknown operation");++negative;
            for(unsigned n=0;n<512;++n) {
                Fixture f{adapted.words,{},{},{}};
                for(auto& s:f.samples)for(auto& c:s)c=value(random);
                for(auto& c:f.facing)c=value(random);
                for(auto& c:f.glancing)c=value(random);
                auto source_samples=f.samples;
                source_samples[1]={128.f/255,128.f/255,128.f/255,1};
                const auto reference=halo2_test::scalar_combiners(material.forward_program.words,source_samples,f.facing,f.glancing);
                const auto actual=halo2_test::scalar_combiners(f.program,f.samples,f.facing,f.glancing);
                require(actual==reference,"neutral detail differs from the original texture response including alpha");
                different+=reference!=halo2_test::scalar_combiners(material.forward_program.words,f.samples,f.facing,f.glancing);
                fixtures.push_back(f);expected.push_back(reference);
            }
        }
    }
    require(names.size()==5 && passes>=5 && different>0,"all five reference Elite surfaces need positive coverage");
    halo2_test::Compute<Fixture,1> gpu(argv[2]);const auto actual=gpu.run(fixtures);float maximum{};
    for(unsigned n=0;n<actual.size();++n)for(unsigned c=0;c<4;++c) {
        const auto error=std::abs(actual[n][c]-expected[n][c]);maximum=std::max(maximum,error);
        require(std::isfinite(actual[n][c]) && error<2e-5f,"GPU neutral detail differs from reference texture sampling");
    }
    std::cout<<"PASS: "<<names.size()<<" exact Elite paths, "<<passes<<" current shader LOD passes, "<<negative
        <<" negative admission controls, "<<fixtures.size()<<" CPU/GPU reference cases including destination alpha; max error="<<maximum<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
