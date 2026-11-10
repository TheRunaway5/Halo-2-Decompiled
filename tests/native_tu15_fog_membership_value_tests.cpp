#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
namespace halo2 {
#include "fog_membership_value_reference.inc"
}
namespace {
struct Result {bool value{};std::string error;bool operator==(const Result&)const=default;};
template<class Callback>Result result(Callback&&callback){try{return{callback(),{}};}catch(const halo2::FormatError&error){return{false,error.what()};}}
}
int main()try{
    using namespace halo2;std::mt19937 random(0x171070);unsigned comparisons{},errors{};
    constexpr std::array codes{-2147483647-1,-129,-128,-127,-3,-2,-1,0,1,3,127,128,255,2147483647};
    for(unsigned variant=0;variant<256;++variant){
        BspAtmosphere atmosphere;atmosphere.clusters.resize(variant%5);atmosphere.fog_planes.resize(std::array{0u,1u,4u,128u}[variant%4]);atmosphere.fog_palette.resize(std::array{0u,1u,4u,128u,300u}[variant%5]);
        for(auto&cluster:atmosphere.clusters)cluster.planar_designator=codes[(variant+random())%codes.size()];
        for(auto&plane:atmosphere.fog_planes)plane.palette=std::array{-2,-1,0,1,3,127,128,299,32767}[random()%9];
        for(auto&palette:atmosphere.fog_palette)palette.datum=std::array{0u,1u,0x80000000u,0xffffffffu}[random()%4];
        atmosphere.instance_membership={{0,1},{3},{},{0xffffffffu}};
        for(int cluster: {-7,-5,-4,-3,-2,-1,0,1,3,4})for(auto datum:{0u,1u,0x80000000u,0xffffffffu}){
            const auto expected=result([&]{return reference_planar(atmosphere,cluster,datum);});const auto actual=result([&]{return atmosphere.contains_planar(cluster,datum);});
            if(actual!=expected)throw std::runtime_error("Public planar membership or FormatError changed");
            ++comparisons;errors+=!actual.error.empty();
            for(unsigned policy=0;policy<8;++policy){
                SourceBspFogAdmissionInput input;input.scenario_loaded=policy&1u;input.bsp_loaded=policy&2u;input.flags_4e64c8=(policy&4u)?0x10u:0u;input.datum=datum;
                const auto expected_source=result([&]{return reference_source(atmosphere,cluster,input);});const auto actual_source=result([&]{return source_bsp_cluster_contains_fog(atmosphere,cluster,input);});
                if(actual_source!=expected_source)throw std::runtime_error("Actual BSP fog caller policy changed");
                ++comparisons;errors+=!actual_source.error.empty();
            }
        }
    }
    std::cout<<"{\"passed\":true,\"public_api_comparisons\":"<<comparisons<<",\"exact_format_errors\":"<<errors<<"}\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
