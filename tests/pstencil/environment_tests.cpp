#include "halo2/pstencil/environment.hpp"
#include "../gpu_compute.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <random>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
struct Fixture {
    std::array<std::uint32_t,60> program;
    std::array<ShaderVector,4> samples;
    ShaderVector facing,glancing;
};
static_assert(sizeof(Fixture)==336);
}
int main(int argc,char** argv){try {
    require(argc==5,"supply original construction JSON, GPU compute shader, resident default program and parameter evidence");
    nlohmann::json original;std::ifstream(argv[1])>>original;
    unsigned source_cases{};
    for(const auto& fixture:original.at("fixtures")) {
        const auto v=fixture.at("window_vector_f32").get<std::array<float,3>>();
        const auto program=pstencil::fallback_environment_response({v[0],v[1],v[2]});
        auto expected=original.at("program_words").get<std::array<std::uint32_t,60>>();
        expected[10]=std::stoul(fixture.at("c0_argb").get<std::string>(),nullptr,16);
        require(program.words==expected,
            "native environment construction differs from unchanged original instructions");++source_cases;
    }
    std::mt19937 random(0x1cf010);std::uniform_real_distribution<float> unorm(0,1);
    std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    for(unsigned n=0;n<4096;++n) {
        const auto program=pstencil::fallback_environment_response({2*unorm(random)-1,2*unorm(random)-1,2*unorm(random)-1});
        Fixture fixture{program.words,{},{},{}};
        for(auto& sample:fixture.samples)for(auto& value:sample)value=unorm(random);
        for(auto& value:fixture.facing)value=unorm(random);
        for(auto& value:fixture.glancing)value=unorm(random);
        if(n==0)fixture.samples[2]={1,.5f,.5f,1};
        if(n==1)fixture.samples[2]={0,.5f,.5f,1};
        const auto packed=program.words[10];
        const std::array<float,3> c{2*float(packed>>16&255)/255-1,
            2*float(packed>>8&255)/255-1,2*float(packed&255)/255-1};
        float d=0;for(unsigned i=0;i<3;++i)d+=(2*fixture.samples[2][i]-1)*c[i];
        const float f=std::pow(std::clamp(d,0.f,1.f),2.f);
        const float brightness=f*fixture.facing[3]+(1-f)*fixture.glancing[3];
        ShaderVector analytic{};
        for(unsigned i=0;i<3;++i) {
            const auto tint=f*fixture.facing[i]+(1-f)*fixture.glancing[i];
            const auto env=fixture.samples[3][i];
            analytic[i]=brightness*(tint*env+(1-tint)*std::pow(env,8.f));
        }
        const auto scalar=halo2_test::scalar_combiners(program.words,fixture.samples,fixture.facing,fixture.glancing);
        for(unsigned i=0;i<4;++i)require(std::abs(scalar[i]-analytic[i])<2e-6f,
            "original environment word interpretation differs from saturated response formula");
        expected.push_back(analytic);fixtures.push_back(fixture);
    }
    halo2_test::Compute<Fixture,1> gpu(argv[2]);const auto actual=gpu.run(fixtures);
    float maximum{};for(unsigned n=0;n<actual.size();++n)for(unsigned c=0;c<4;++c) {
        const auto error=std::abs(actual[n][c]-expected[n][c]);maximum=std::max(maximum,error);
        require(std::isfinite(actual[n][c]) && error<2e-5f,
            "GPU environment response differs from exact original words/analytic formula");
    }
    Material pass;pass.forward_pass=true;pass.forward_vertex=4;pass.forward_blend=8;
    pass.forward_pass_index=0;
    // Exact supported current pass control shape; constants are dynamic.
    pass.forward_program.words={0x00000000u,0x0c200000u,0x1c113c11u,0x3c151c14u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x0b0f0000u,0x0c1c0080u,0x00000000u,0x00000000u,0x00000000u,0x00787387u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000000c0u,0x00014d00u,0x00000c00u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x6a410000u,0x00000000u,0x3c051c04u,0x3d0c1d01u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x000020c0u,0x00000000u,0x00000c00u,0x00000c00u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00011104u,0x00062e21u,0x00000111u,0x00000000u,0x00000000u,0x00000000u,0x00000000u};
    pass.passes.emplace_back();pass.passes[0].name="shaders\\shader_passes\\environment_mapping\\default_environment_map_tangent_space";
    pass.passes[0].vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_env_bumped_tangentspace";
    require(pstencil::supports_environment_response(pass),"bounded tangent-space environment response was not selected");
    pass.forward_vertex=6;require(!pstencil::supports_environment_response(pass),"combined lightmap pass was replaced by analogy");
    pass.forward_vertex=4;pass.forward_blend=3;require(!pstencil::supports_environment_response(pass),"destination-alpha ownership was discarded");
    std::array<std::uint32_t,60> resident{};
    std::ifstream bytes(argv[3],std::ios::binary);bytes.read(reinterpret_cast<char*>(resident.data()),240);
    require(bool(bytes),"primary resident environment program is truncated");
    fixtures.clear();expected.clear();
    for(unsigned n=0;n<4096;++n) {
        auto current=pass.forward_program;
        current.words[10]=random()&0xffffffu;current.words[12]=random()&0xff000000u;
        current.words[13]=random()&0xffffffu;
        const auto adapted=pstencil::adapt_environment_response(current);
        auto exact=resident;exact[10]=current.words[10];exact[13]=current.words[12]&0xff000000u;
        exact[14]=current.words[13]&0xffffffu;
        require(adapted.words==exact,"adapted response changed primary resident control words");
        Fixture fixture{adapted.words,{},{},{}};
        for(auto& sample:fixture.samples)for(auto& value:sample)value=unorm(random);
        for(auto& value:fixture.facing)value=unorm(random);
        for(auto& value:fixture.glancing)value=unorm(random);
        const auto packed=current.words[10];
        const std::array<float,3> c{2*float(packed>>16&255)/255-1,
            2*float(packed>>8&255)/255-1,2*float(packed&255)/255-1};
        float d=0;for(unsigned i=0;i<3;++i)d+=(1-2*fixture.samples[0][i])*c[i];
        const float f=1-std::pow(1-std::clamp(d,0.f,1.f),4.f);
        const float k=float(current.words[12]>>24)/255;
        const float mid=std::clamp(2*k*std::min(f,1-f),0.f,1.f);
        const float brightness=f*fixture.facing[3]+(1-f)*fixture.glancing[3];
        ShaderVector analytic{};
        for(unsigned i=0;i<3;++i) {
            const auto tint0=f*fixture.facing[i]+(1-f)*fixture.glancing[i];
            const auto midcolor=float(current.words[13]>>((2-i)*8)&255)/255;
            const auto tint=(1-mid)*tint0+mid*midcolor;
            const auto env=fixture.samples[3][i];
            analytic[i]=brightness*(tint*env+(1-tint)*std::pow(env,8.f));
        }
        const auto scalar=halo2_test::scalar_combiners(adapted.words,fixture.samples,fixture.facing,fixture.glancing);
        for(unsigned i=0;i<4;++i)require(std::abs(scalar[i]-analytic[i])<2e-6f,
            "resident environment word interpretation differs from source Fresnel/midrange formula");
        expected.push_back(analytic);fixtures.push_back(fixture);
    }
    halo2_test::Compute<Fixture,1> resident_gpu(argv[2]);const auto resident_actual=resident_gpu.run(fixtures);
    for(unsigned n=0;n<resident_actual.size();++n)for(unsigned c=0;c<4;++c) {
        const auto error=std::abs(resident_actual[n][c]-expected[n][c]);maximum=std::max(maximum,error);
        require(std::isfinite(resident_actual[n][c]) && error<2e-5f,
            "GPU default resident response differs from primary words/source formula");
    }
    nlohmann::json references;std::ifstream(argv[4])>>references;unsigned reference_count{};
    for(const auto& entry:references.at("static_consensus")) {
        const auto* parameters=pstencil::reference_environment_parameters(entry.at("shader").get<std::string>());
        require(parameters,"verified original shader has no exact reference lookup");
        const auto& primary=entry.at("parameters");
        require(parameters->facing==primary.at("facing").get<ShaderVector>() &&
            parameters->glancing==primary.at("glancing").get<ShaderVector>() &&
            parameters->midrange_factor_argb==primary.at("midrange_factor_argb").get<std::uint32_t>() &&
            parameters->midrange_color_argb==primary.at("midrange_color_argb").get<std::uint32_t>(),
            "native reference parameters differ from original resident data");
        auto program=pstencil::adapt_environment_response(pass.forward_program);
        auto before=program;std::array<ShaderVector,16> attributes{};
        pstencil::apply_reference_environment(*parameters,program,attributes);
        for(unsigned n=0;n<60;++n)if(n!=13 && n!=14)
            require(program.words[n]==before.words[n],"reference values changed shader control or camera direction");
        require(attributes[10]==parameters->facing && attributes[11]==parameters->glancing,
            "reference values lost direct diffuse attribute mapping");
        ++reference_count;
    }
    require(!pstencil::reference_environment_parameters("objects\\vehicles\\spectre\\shaders\\spectre"),
        "later equipment received an invented historical material alias");
    std::cout<<reference_count<<" exact static reference material records, "<<source_cases<<" unchanged-original construction fixtures and "<<fixtures.size()
        <<" scalar/GPU fixtures for each of fallback/default resident responses passed; maximum GPU error "<<maximum<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
