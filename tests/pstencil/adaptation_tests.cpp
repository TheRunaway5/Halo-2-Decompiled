#include "halo2/pstencil/adaptation.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "../gpu_compute.hpp"
#include "../scalar_combiners.hpp"
#include <iostream>
#include <random>

using namespace halo2;
namespace {
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
struct Fixture {
    std::array<std::uint32_t,60> program;
    std::array<ShaderVector,4> samples;
    ShaderVector facing,glancing;
};
static_assert(sizeof(Fixture)==336);
Material find(const Cache& cache,std::string_view path) {
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& t){
        return t.group=="shad" && t.name==path;
    });
    if(tag==cache.tags().end())throw FormatError("required current armor material is absent: "+std::string(path));
    return cache.load_material(tag->datum);
}
const Material* combined(const Material& material) {
    if(pstencil::supports_combined_environment_strength(material))return &material;
    if(material.environment_pass)if(auto* result=combined(*material.environment_pass))return result;
    for(const auto& m:material.world_overlays_before)if(auto* result=combined(*m))return result;
    for(const auto& m:material.world_overlays_after)if(auto* result=combined(*m))return result;
    return nullptr;
}
}
int main(int argc,char** argv){try {
    require(argc==3,"supply current Cairo Station map and GPU compute shader");
    Cache cache(argv[1]);Mesh mesh;pstencil::MaterialBank bank(cache,mesh);
    EffectLightUniformInput input;input.position={1,2,3};input.forward={1,0,0};
    input.up={0,0,1};input.left={0,1,0};input.radius=2;
    input.diffuse=input.specular={1,1,1};input.brightness=1;
    const auto uniforms=source_effect_light_uniforms(input);
    const std::array<std::string_view,7> names{
        "objects\\characters\\dervish\\shaders\\dervish_armor",
        "objects\\characters\\dervish\\shaders\\dervish_helmet",
        "objects\\characters\\dervish\\shaders\\dervish_cloth",
        "objects\\characters\\dervish\\shaders\\dervish_flesh",
        "objects\\characters\\elite\\shaders\\torso",
        "objects\\characters\\masterchief\\shaders\\masterchief",
        "objects\\characters\\masterchief\\shaders\\masterchief_visor"};
    std::mt19937 random(0x2003ab);std::uniform_real_distribution<float> unorm(0,1);
    std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    unsigned polished{},ordinary{},combined_count{},cube_zero{},color_diffuse{},cube_live{},mask_cases{};
    for(const auto name:names) {
        auto authored=find(cache,name);
        const auto source_spec=std::find_if(authored.passes.begin(),authored.passes.end(),[](const auto& p){
            return p.vertex_shader.ends_with("\\stencil_light_specular_tangentspace_sphere");
        });
        require(source_spec!=authored.passes.end(),"armor mask test lacks its actual specular pass");
        const auto source_material=pstencil::bind_light_pass(cache,authored,unsigned(source_spec-authored.passes.begin()),uniforms);
        require(bool(source_material),"source specular mask input did not bind");
        const auto masked=pstencil::texture_masked_specular(source_material->forward_program);
        for(unsigned n=0;n<512;++n) {
            Fixture f{masked.words,{},{},{}};
            for(auto& s:f.samples)for(auto& c:s)c=unorm(random);
            for(auto& c:f.facing)c=unorm(random);
            for(auto& c:f.glancing)c=unorm(random);
            auto old=halo2_test::scalar_combiners(source_material->forward_program.words,f.samples,f.facing,f.glancing);
            for(unsigned c=0;c<3;++c)old[c]*=f.samples[0][3];
            require(masked.words==source_material->forward_program.words,"mask transfer changed the authored combiner program");
            f.program[59]|=0x80000000u; // Compute-fixture tag, not a runtime shader word.
            fixtures.push_back(f);expected.push_back(old);++mask_cases;
        }
        bank.preload(authored,0,{});
        const auto draws=bank.complete(authored.source_shader_datum,authored.selected_shader_lod,0,uniforms);
        require(bool(draws) && !draws->empty(),"authored armor specular layer was not bound");
        const bool metal=pstencil::arbiter_metal(name);
        require(draws->size()==(metal?1u+pstencil::arbiter_specular_copies:1u),"polish escaped the two Arbiter metal materials");
        if(metal) {
            ++polished;const auto& full=draws->at(0);const auto& extra=draws->at(1);
            require(full.material_slot==extra.material_slot,"polish changed texture/pass residency");
            require(full.vertex_bank.range==extra.vertex_bank.range,"polish changed vertex bank range");
            for(unsigned n=0;n<60;++n)if(!(n>=10 && n<=25) && n!=43 && n!=44)
                require(full.program.words[n]==extra.program.words[n],"polish changed an authored combiner/mask control");
            // C31..39 geometry, normal and gel inputs remain exact. Only the
            // light's C38 specular RGB is reduced for the additional pass.
            for(unsigned n=0;n<158;++n) {
                const auto source=n+full.vertex_bank.range[0];
                for(unsigned c=0;c<4;++c) {
                    const auto value=full.vertex_bank.constants[n][c];
                    require(extra.vertex_bank.constants[n][c]==
                        (source==38 && c<3?value*pstencil::arbiter_specular_companion:value),"polish changed a position, mask or palette input");
                }
            }
            bool contributes{};
            for(unsigned n=0;n<256;++n) {
                Fixture f{extra.program.words,{},{},{}};
                for(auto& sample:f.samples)for(auto& value:sample)value=unorm(random);
                for(auto& value:f.facing)value=unorm(random);
                for(auto& value:f.glancing)value=unorm(random);
                const auto result=halo2_test::scalar_combiners(f.program,f.samples,f.facing,f.glancing);
                contributes|=result[0]>0 || result[1]>0 || result[2]>0;
                expected.push_back(result);fixtures.push_back(f);
            }
            require(contributes,"polished authored specular pass makes no contribution");
        } else ++ordinary;
        require(prepare_dynamic_material(cache,authored,{},true,0.f),"current dynamic material did not complete");
        const auto* pass=combined(authored);
        if(!pass)continue;
        ++combined_count;
        const auto* reference=pstencil::reference_environment_parameters(name);
        auto adapted=pass->forward_program;
        pstencil::adapt_combined_environment_strength(adapted,reference,metal);
        for(unsigned n=0;n<60;++n)if(n!=17 && n!=25)
            require(adapted.words[n]==pass->forward_program.words[n],"combined adapter changed diffuse tint/control");
        if(!metal && !reference)require(adapted.words==pass->forward_program.words,"ordinary cloth/flesh changed");
        if(metal) {
            require((adapted.words[17]>>24)>(pass->forward_program.words[17]>>24) &&
                (adapted.words[25]>>24)>(pass->forward_program.words[25]>>24),"Arbiter metal reflection did not increase");
            require(adapted.words[17]>>24==204 && adapted.words[25]>>24==255,
                "Arbiter metal did not reach its stronger facing/glancing response");
        }
        if(name.ends_with("masterchief_visor")) {
            require(reference && adapted.words[17]>>24==102 && adapted.words[25]>>24==51,
                "visor did not receive the original 0.4/0.2 reflection strengths");
        }
        auto invalid=*pass;invalid.forward_program.words[34]^=1;
        require(!pstencil::supports_combined_environment_strength(invalid),"unknown combined shader was admitted");
        invalid=*pass;invalid.forward_blend=8;
        require(!pstencil::supports_combined_environment_strength(invalid),"combined destination-alpha ownership changed");
        for(unsigned n=0;n<512;++n) {
            Fixture f{adapted.words,{},{},{}};
            // Real control words, independently varied completed constants and
            // texture samples expose the consumer of each packed field.
            for(unsigned c=10;c<=25;++c)f.program[c]=random();
            for(auto& sample:f.samples)for(auto& value:sample)value=unorm(random);
            for(auto& value:f.facing)value=unorm(random);
            for(auto& value:f.glancing)value=unorm(random);
            const auto live=halo2_test::scalar_combiners(f.program,f.samples,f.facing,f.glancing);
            auto zero=f;zero.program[17]&=0xffffffu;zero.program[25]&=0xffffffu;
            const auto no_env=halo2_test::scalar_combiners(zero.program,zero.samples,zero.facing,zero.glancing);
            auto changed_cube=zero;for(auto& v:changed_cube.samples[1])v=unorm(random);
            require(no_env==halo2_test::scalar_combiners(changed_cube.program,changed_cube.samples,zero.facing,zero.glancing),
                "combined strength zero retained cube contribution");++cube_zero;
            require(live[3]==no_env[3],"reflection strength changed destination alpha");
            changed_cube.program=f.program;
            cube_live+=live!=halo2_test::scalar_combiners(f.program,changed_cube.samples,f.facing,f.glancing);
            auto tint=zero.program;tint[13]^=0xffffffu;tint[21]^=0xffffffu;
            color_diffuse+=no_env!=halo2_test::scalar_combiners(tint,zero.samples,zero.facing,zero.glancing);
            expected.push_back(live);fixtures.push_back(f);
            expected.push_back(no_env);fixtures.push_back(zero);
        }
    }
    require(polished==2 && ordinary==5 && combined_count>=3,"current material coverage is incomplete");
    require(color_diffuse>0 && cube_live>0,"combined input discrimination has no positive controls");
    halo2_test::Compute<Fixture,1> gpu(argv[2]);const auto actual=gpu.run(fixtures);float maximum{};
    for(unsigned n=0;n<actual.size();++n)for(unsigned c=0;c<4;++c) {
        const auto error=std::abs(actual[n][c]-expected[n][c]);maximum=std::max(maximum,error);
        require(std::isfinite(actual[n][c]) && error<2e-5f,"GPU current armor/visor consumer differs from scalar interpreter");
    }
    std::cout<<"PASS: "<<polished<<" polished Arbiter metal materials, "<<ordinary<<" ordinary armor/cloth/flesh materials, "
        <<combined_count<<" real combined passes, "<<cube_zero<<" zero-reflection controls, "<<mask_cases<<" material-mask cases, "<<fixtures.size()
        <<" GPU cases, maximum error="<<maximum<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
