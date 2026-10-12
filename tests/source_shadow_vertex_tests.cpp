#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <map>
#include <sstream>
#include <iostream>
namespace {
using namespace halo2;
constexpr std::array<unsigned,7> outputs{0,3,4,9,10,11,12};
struct Fixture {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,192> constants{};
    std::array<ShaderVector,7> inputs{};
};
static_assert(sizeof(Fixture)==5632);
struct Reference {unsigned event{},vertex{};ShaderVector conversion{};std::array<ShaderVector,7> values;};
void check(bool v,const std::string &message){if(!v)throw std::runtime_error(message);}
float word(std::istream &s){std::string t;check(bool(s>>t)&&t.size()==8,"invalid shadow fixture word");std::size_t n;auto b=std::stoul(t,&n,16);check(n==8&&b<=0xfffffffful,"invalid shadow fixture bits");return std::bit_cast<float>(std::uint32_t(b));}
void compare(std::span<const ShaderVector> result,const std::vector<Reference>&refs,bool gpu) {
    check(result.size()==refs.size()*16,"shadow output count differs");
    std::array<float,7> maximum{};std::array<unsigned,7> differing{},maxulp{};
    for(unsigned sample=0;sample<refs.size();++sample)for(unsigned o=0;o<7;++o) {
        auto actual=result[sample*16+outputs[o]];
        if(o==0) {
            // Original xemu wraps the original NV2A pixel outputs: trunc1/16,
            // actual surface extent/clipRange then multiply by integer W.
            for(unsigned axis=0;axis<2;++axis)actual[axis]=((std::trunc(actual[axis]*16.f)/16.f)*2.f-refs[sample].conversion[axis])/refs[sample].conversion[axis];
            actual[2]/=refs[sample].conversion[2];
            for(unsigned axis=0;axis<3;++axis)actual[axis]*=actual[3];
        }
        for(unsigned axis=0;axis<4;++axis) {
            const auto expected=refs[sample].values[o][axis];check(std::isfinite(actual[axis])&&std::isfinite(expected),"shadow fixture is nonfinite");
            const auto a=std::bit_cast<std::uint32_t>(actual[axis]),b=std::bit_cast<std::uint32_t>(expected);
            const float error=std::abs(actual[axis]-expected);maximum[o]=std::max(maximum[o],error);differing[o]+=a!=b;
            const auto ulp=(a>>31)==(b>>31)?(a>b?a-b:b-a):0xffffffffu;maxulp[o]=std::max(maxulp[o],ulp);
            // CPU DebugVertex reference is exact. Native Vulkan's ordered
            // dot products can differ from original xemu OpDot; measure each
            // consumer separately rather than claiming physical NV2A precision.
            const bool accepted=!gpu?a==b:(a==b || (o==0?ulp<=4u:ulp<=8u));
            check(accepted,std::string(gpu?"GPU":"CPU")+" shadow mismatch event"+std::to_string(refs[sample].event)+" vertex"+std::to_string(refs[sample].vertex)+" o"+std::to_string(outputs[o])+" axis"+std::to_string(axis)+" error"+std::to_string(error)+" ULP"+std::to_string(ulp));
        }
    }
    for(unsigned o=0;o<7;++o)std::cout<<(gpu?"GPU":"CPU")<<" shadow o"<<outputs[o]<<" max "<<maximum[o]<<", differing "<<differing[o]<<", max ULP "<<maxulp[o]<<'\n';
}
}
int main(int argc,char **argv){try {
    check(argc==1 ||(argc==2&&std::string(argv[1])=="--gpu"),"expected optional --gpu");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);std::ifstream file(root/"tests/fixtures/retail_shadow_vertex.tsv");if(!file)return 77;
    std::map<unsigned,Fixture> banks;std::map<unsigned,ShaderVector> conversion;std::map<unsigned,unsigned> count;
    std::vector<Fixture> fixtures;std::vector<Reference> refs;std::string line;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;std::istringstream fields(line);char kind;unsigned event,index;check(bool(fields>>kind>>event>>index),"truncated shadow record");
        if(kind=='B') {
            unsigned ordinal;check(bool(fields>>ordinal),"missing authored shadow class");
            const auto name="retail_shadow_"+std::string(index?"apply":"generate")+"_class"+std::to_string(ordinal)+".vp";
            std::ifstream bin(root/"tests/fixtures"/name,std::ios::binary);check(bool(bin),"original shadow microcode missing");
            const std::string bytes((std::istreambuf_iterator<char>(bin)),{});const auto p=decode_vertex_program({reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()});
            banks[event].instructions=p.instructions;banks[event].flags[3]=p.count;
            for(unsigned i=0;i<3;++i)conversion[event][i]=word(fields);
        } else if(kind=='C') {
            check(index==count[event]&&index<192,"incomplete shadow constant bank");
            for(auto &v:banks[event].constants[index])v=word(fields);++count[event];
        } else if(kind=='V') {
            check(count[event]==192,"source shadow constants incomplete");auto fixture=banks.at(event);Reference reference{event,index,conversion.at(event),{}};
            for(auto &v:fixture.attributes)for(auto &c:v)c=word(fields);
            for(unsigned i=0;i<7;++i)fixture.inputs[i]=fixture.attributes[i];
            for(auto &v:reference.values)for(auto &c:v)c=word(fields);
            fixtures.push_back(fixture);refs.push_back(reference);
        } else throw FormatError("unknown source shadow record");
        std::string extra;check(!(fields>>extra),"trailing shadow fixture data");
    }
    check(banks.size()==11&&fixtures.size()==24,"executed original shadow coverage differs");
    std::vector<ShaderVector> result;
    for(const auto &fixture:fixtures) {
        VertexProgram p;p.instructions=fixture.instructions;p.count=fixture.flags[3];
        VertexProgramState s;s.constants=fixture.constants;s.attributes=fixture.attributes;execute_vertex_program(p,s);
        result.insert(result.end(),s.outputs.begin(),s.outputs.end());
    }
    compare(result,refs,false);
    if(argc==2){halo2_test::Compute<Fixture,16> gpu(HALO2_SHADOW_VERTEX_TEST_SHADER);const auto values=gpu.run(fixtures);compare(values,refs,true);}
    std::cout<<fixtures.size()<<" original shadow vertices across "<<banks.size()<<" draws passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
