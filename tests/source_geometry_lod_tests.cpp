#include "halo2/source_geometry_lod.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc,char**argv){try{
    if((argc==2 && std::string(argv[1])=="--metric-probe") ||
       (argc==3 && std::string(argv[1])=="--metric")){
        const bool probe=argc==2;std::ifstream fixture;
        if(!probe){fixture.open(argv[2]);if(!fixture)throw std::runtime_error("source metric fixture unavailable");}
        auto&stream=probe?std::cin:fixture;std::string line;unsigned cases=0;
        while(std::getline(stream,line)){
            if(line.empty() || line.front()=='#')continue;
            std::istringstream row(line);row>>std::hex;std::array<std::uint32_t,11> input{};
            for(auto&word:input)if(!(row>>word))throw std::runtime_error("incomplete source metric input");
            const auto real=[&](unsigned i){return std::bit_cast<float>(input[i]);};
            const auto distance=halo2::source_geometry_lod_distance({real(0),real(1),real(2)},
                {real(3),real(4),real(5)},real(6));
            const std::array<std::uint32_t,2> output{std::bit_cast<std::uint32_t>(distance),
                halo2::source_geometry_lod_view_modifier(input[7],input[8],input[9],std::int16_t(input[10]))};
            if(probe){for(auto word:output)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
            else for(auto word:output){std::uint32_t expected;
                if(!(row>>expected) || word!=expected)throw std::runtime_error("source metric differs: "+std::to_string(cases));}
            std::string extra;if(row>>extra)throw std::runtime_error("source metric fixture trailing data");++cases;
        }
        if(!cases)throw std::runtime_error("no source metric cases");
        if(!probe)std::cout<<cases<<" original view metric/modifier cases matched\n";
        return 0;
    }
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream fixture;
    if(!probe){if(argc!=2)throw std::runtime_error("source geometry LOD fixture required");fixture.open(argv[1]);
        if(!fixture)throw std::runtime_error("source geometry LOD fixture unavailable");}
    auto &stream=probe?std::cin:fixture;std::string line;unsigned cases=0;
    while(std::getline(stream,line)){
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);row>>std::hex;
        std::array<std::uint32_t,15> in{};
        for(auto&value:in)if(!(row>>value))throw std::runtime_error("incomplete source LOD input");
        halo2::SourceGeometryLodDefinition definition;definition.model_present=in[0];
        for(unsigned i=0;i<9;++i)definition.model_bank[i]=std::bit_cast<float>(in[6+i]);
        const auto choice=halo2::source_geometry_lod_choice(definition,std::bit_cast<float>(in[1]),std::int8_t(in[2]));
        const auto policy=halo2::source_geometry_lod_policy(in[3]);
        std::array<std::uint32_t,13> out{std::uint32_t(choice.lod),std::bit_cast<std::uint32_t>(choice.transition),
            std::uint32_t(halo2::source_geometry_lod_cap(choice,policy,in[4],bool(in[5]))),
            std::uint32_t(policy.ordinary_maximum),std::bit_cast<std::uint32_t>(policy.coverage_multiplier),
            std::bit_cast<std::uint32_t>(policy.secondary_scale),std::bit_cast<std::uint32_t>(policy.effect_scale)};
        for(unsigned i=0;i<6;++i)out[7+i]=policy.feature_bytes[i];
        if(probe){for(auto word:out)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
        else for(unsigned i=0;i<out.size();++i){std::uint32_t expected;
            if(!(row>>expected) || expected!=out[i])throw std::runtime_error("source LOD differs: "+
                std::to_string(cases)+" field "+std::to_string(i));}
        std::string extra;if(row>>extra)throw std::runtime_error("source LOD fixture trailing data");++cases;
    }
    if(!cases)throw std::runtime_error("no source LOD cases");
    if(!probe)std::cout<<cases<<" original geometry LOD/transition/cap/policy cases matched\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
