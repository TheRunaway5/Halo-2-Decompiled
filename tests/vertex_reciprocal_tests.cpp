#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
using namespace halo2;
namespace {
struct Fixture {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<ShaderVector,192> constants{};
    std::array<unsigned,4> flags{};
};
static_assert(sizeof(Fixture)==5520);
struct Case {Fixture fixture;ShaderVector expected;};
void require(bool value,const char* why){if(!value)throw FormatError(why);}
bool same(const ShaderVector& a,const ShaderVector& b) {
    for(unsigned i=0;i<a.size();++i)
        if(std::isnan(b[i])?!std::isnan(a[i]):std::bit_cast<unsigned>(a[i])!=std::bit_cast<unsigned>(b[i]))return false;
    return true;
}
VertexProgram program(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    require(bool(input),"missing original shadow generation world-class0 program");
    const std::string bytes{std::istreambuf_iterator<char>(input),{}};
    return decode_vertex_program({reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()});
}
ShaderVector execute(const VertexProgram& p,const Fixture& f) {
    VertexProgramState state;state.attributes=f.attributes;state.constants=f.constants;
    execute_vertex_program(p,state);return state.outputs[0];
}
std::vector<Case> cases() {
    std::vector<Case> out;
    VertexProgram rcc;rcc.count=1;rcc.instructions[0]={0,3u<<25u,0,(2u<<28u)|0xf000u|0x805u};
    // Exact powers of two avoid admitting unrelated reciprocal-rounding error.
    for(unsigned bits:{0u,0x80000000u,0x3f800000u,0xbf800000u,0x3a800000u,0xba800000u,
        0x7f800000u,0xff800000u,0x1f800000u,0x9f800000u,0x5f800000u,0xdf800000u,
        0x1f000000u,0x9f000000u,0x60000000u,0xe0000000u,0x7fc00000u,0xffc00000u}) {
        Fixture f;f.instructions=rcc.instructions;f.flags[3]=1;
        const auto value=std::bit_cast<float>(bits);f.attributes[0]={value,0,0,1};
        float reciprocal=1.f/value;
        // Independent retained NV2A translation contract: sign belongs to
        // the reciprocal, including signed zero from an infinite operand.
        reciprocal=std::signbit(reciprocal)?std::clamp(reciprocal,-0x1p64f,-0x1p-64f):
            std::clamp(reciprocal,0x1p-64f,0x1p64f);
        ShaderVector expected;expected.fill(reciprocal);
        require(same(execute(rcc,f),expected),"CPU RCC changed the signed clamped reciprocal");
        out.push_back({f,expected});
    }
    const auto authored=program(std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_shadow_generate_world_class0.vp");
    require(authored.count==14,"original shadow world program changed");
    for(float x:{-1.f,1.f})for(float z:{0.f,.015625f,-.015625f,1.f}) {
        Fixture f;f.instructions=authored.instructions;f.flags[3]=authored.count;
        f.attributes[0]={x,1,z,1};f.attributes[1]={.25f,.5f,0,1};
        f.constants[0]={1,0,0,0};f.constants[1]={0,1,0,0};f.constants[2]=f.constants[3]={0,0,1,0};
        f.constants[10]={320,240,1,1};f.constants[11]={320,240,0,0};f.constants[172]={1,1,0,0};
        out.push_back({f,execute(authored,f)});
    }
    return out;
}
}
int main(int argc,char** argv)try {
    const auto input=cases();
    if(argc==2 && std::string_view(argv[1])=="--gpu") {
        std::vector<Fixture> fixtures;for(const auto& row:input)fixtures.push_back(row.fixture);
        halo2_test::Compute<Fixture,1> gpu(HALO2_VERTEX_RECIPROCAL_SHADER);
        const auto output=gpu.run(fixtures);
        for(unsigned n=0;n<input.size();++n)
            if(!same(output[n],input[n].expected)) {
                std::cerr<<"RCC position mismatch case "<<n<<" expected ";
                for(auto value:input[n].expected)std::cerr<<value<<' ';
                std::cerr<<" actual ";for(auto value:output[n])std::cerr<<value<<' ';std::cerr<<'\n';
                return 1;
            }
        std::cout<<input.size()<<" GPU RCC/actual authored-program cases passed (finite values bit-exact; NaN classification retained)\n";
    } else std::cout<<input.size()<<" CPU signed RCC/actual authored-program cases passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
