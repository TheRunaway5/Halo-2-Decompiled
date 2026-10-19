#include "../gpu_compute.hpp"
#include "halo2/cache.hpp"
#include "../scalar_combiners.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <nlohmann/json.hpp>
using namespace halo2;
struct Fixture{std::array<std::uint32_t,60> program;std::array<ShaderVector,4> samples;ShaderVector v0,v1;};
static_assert(sizeof(Fixture)==336);
int main(int argc,char** argv)try{
    if(argc!=3)throw FormatError("Supply actual current program fixtures and compute shader");
    nlohmann::json rows;std::ifstream(argv[1])>>rows;std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    std::mt19937 rng(0x200304);std::uniform_real_distribution<float> unit(0,1);
    for(const auto& row:rows)for(unsigned n=0;n<512;++n){
        Fixture f;f.program=row["program"].get<std::array<std::uint32_t,60>>();
        for(auto& s:f.samples)for(float& x:s)x=unit(rng);
        for(auto* v:{&f.v0,&f.v1})for(float& x:*v)x=unit(rng);
        for(unsigned s=0;s<4;++s)if(!((f.program[54]>>(s*5u))&31u))f.samples[s]={};
        auto result=halo2_test::scalar_combiners(f.program,f.samples,f.v0,f.v1);expected.push_back(result);expected.push_back(result);fixtures.push_back(f);
    }
    const auto actual=halo2_test::Compute<Fixture,2>{argv[2]}.run(fixtures);float maximum{};
    for(unsigned i=0;i<actual.size();++i)for(unsigned c=0;c<4;++c){
        const float error=std::abs(actual[i][c]-expected[i][c]);maximum=std::max(maximum,error);
        if(!std::isfinite(actual[i][c])||error>2.e-5f){std::cerr<<"sample "<<i/2<<" phase "<<i%2<<" channel "<<c<<" actual "<<actual[i][c]<<" expected "<<expected[i][c]<<'\n';throw FormatError("GPU current/original combiner composition differs from scalar current output");}
    }
    std::cout<<fixtures.size()<<" GPU current/original texture compositions match independent scalar RGBA; maximum error "<<maximum<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
