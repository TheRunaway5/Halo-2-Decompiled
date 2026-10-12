#include "halo2/source_weather_assets.hpp"
#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
struct Fixture {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,192> constants{};
};
static_assert(sizeof(Fixture)==5520 && offsetof(Fixture,constants)==2448);
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
float scalar(std::istream &stream){std::uint32_t value;require(bool(stream>>std::hex>>value),"truncated weather vertex fixture");return std::bit_cast<float>(value);}
}
int main(int argc,char**argv){try{
    require(argc==1||(argc==2&&std::string(argv[1])=="--gpu"),"optional --gpu expected");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    const auto map=root/"assets/disc/maps/lockout.map";if(!std::filesystem::exists(map))return 77;
    Cache cache(map);const auto definition=cache.load_source_weather_definition(0xe77d0608u);
    const std::array programs{decode_vertex_program(definition.vertex_programs[1]),decode_vertex_program(definition.vertex_programs[0])};
    require(programs[0].count==9&&programs[1].count==5,"authored weather built-in program changed");
    std::ifstream file(root/"tests/fixtures/retail_weather_vertex_analytic.tsv");require(bool(file),"independent weather fixture missing");
    std::vector<Fixture> fixtures;std::vector<std::array<ShaderVector,16>> expected;
    std::vector<ShaderVector> cpu;
    for(std::string line;std::getline(file,line);){if(line.empty()||line[0]=='#')continue;
        std::istringstream fields(line);unsigned kind;require(bool(fields>>std::hex>>kind)&&kind<2,"invalid weather vertex class");
        Fixture f;f.instructions=programs[kind].instructions;f.flags[3]=programs[kind].count;
        for(auto &a:f.attributes)for(auto &v:a)v=scalar(fields);
        for(unsigned i=0;i<24;++i)for(auto &v:f.constants[i])v=scalar(fields);
        auto &result=expected.emplace_back();for(auto&o:result)for(auto&v:o)v=scalar(fields);
        std::string extra;require(!(fields>>extra),"trailing weather vertex fixture words");
        VertexProgramState state;state.attributes=f.attributes;state.constants=f.constants;
        execute_vertex_program(programs[kind],state);
        cpu.insert(cpu.end(),state.outputs.begin(),state.outputs.end());fixtures.push_back(f);
    }
    require(fixtures.size()==448,"weather resolution/view/sample fixture coverage changed");
    const auto compare=[&](const std::vector<ShaderVector>&actual,const char *producer){
        require(actual.size()==fixtures.size()*16,"weather GPU output count differs");
        std::array<ShaderVector,16> maxima{};unsigned different=0;std::uint32_t maximum_ulp=0;
        std::string failure;
        for(unsigned i=0;i<fixtures.size();++i)for(unsigned o=0;o<16;++o)for(unsigned c=0;c<4;++c){
            const auto a=actual[i*16+o][c],b=expected[i][o][c];require(std::isfinite(a),"weather vertex result nonfinite");
            maxima[o][c]=std::max(maxima[o][c],std::abs(a-b));const auto aw=std::bit_cast<std::uint32_t>(a),bw=std::bit_cast<std::uint32_t>(b);
            if(aw!=bw){++different;require((aw>>31)==(bw>>31),"weather output sign differs");
                const auto ulp=aw>bw?aw-bw:bw-aw;maximum_ulp=std::max(maximum_ulp,ulp);
                // Only the particle reciprocal's consumers may differ.
                // These are measured bounds for this synthetic fixture on
                // Vulkan, not evidence of NV2A division precision. Colors,
                // plate UVs, projected W and unused output channels are exact.
                const bool division_consumer=(o==0&&c<3&&ulp<=4)||(o==6&&c==0&&ulp<=2);
                if(!(std::string(producer)=="GPU"&&division_consumer) && failure.empty())
                    failure="weather builtin output exceeds division bound at case "+std::to_string(i)+
                        " output "+std::to_string(o)+" component "+std::to_string(c)+
                        " actual "+std::to_string(a)+" expected "+std::to_string(b)+" ULP "+std::to_string(ulp);}
        }
        std::cout<<producer<<" weather built-ins: "<<different<<" differing words, max "<<maximum_ulp<<" ULP\n";
        for(const auto o:{0,3,6,9,10,11,12}){
            std::cout<<"  o"<<o<<" max absolute";for(float v:maxima[o])std::cout<<' '<<v;
            std::cout<<'\n';}
        if(!failure.empty())throw std::runtime_error(failure);
    };
    compare(cpu,"CPU");
    if(argc==2){halo2_test::Compute<Fixture,16> compute(HALO2_WEATHER_VERTEX_TEST_SHADER);
        compare(compute.run(fixtures),"GPU");VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(compute.physical,&properties);std::cout<<properties.deviceName<<'\n';}
    std::cout<<fixtures.size()<<" analytic source-program cases; no original weather GPU rasterization claim\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
