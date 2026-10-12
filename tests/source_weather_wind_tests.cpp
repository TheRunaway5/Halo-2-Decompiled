#include "halo2/source_weather_wind.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using Words=std::array<std::uint32_t,39>;
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream fixture;
    if(!probe){if(argc!=2)throw std::runtime_error("source wind fixture required");fixture.open(argv[1]);
        if(!fixture)throw std::runtime_error("source wind fixture unavailable");}
    auto &stream=probe?std::cin:fixture;std::string line;unsigned cases=0;
    while(std::getline(stream,line)){
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);row>>std::hex;std::uint32_t auxiliary{},elapsed{};Words words{};
        if(!(row>>auxiliary>>elapsed))throw std::runtime_error("incomplete source wind input");
        for(auto &word:words)if(!(row>>word))throw std::runtime_error("incomplete source wind bank");
        auto state=std::bit_cast<halo2::SourceWeatherWind>(words);
        halo2::source_weather_wind_step(state,std::bit_cast<float>(elapsed),auxiliary);
        words=std::bit_cast<Words>(state);
        if(probe){std::cout<<std::hex<<auxiliary<<' ';for(auto word:words)std::cout<<word<<' ';std::cout<<'\n';}
        else {std::uint32_t expected{};if(!(row>>expected) || expected!=auxiliary)
                throw std::runtime_error("source wind auxiliary stream differs: "+std::to_string(cases));
            for(unsigned field=0;field<words.size();++field)
                if(!(row>>expected) || expected!=words[field])throw std::runtime_error("source wind bank differs: "+
                    std::to_string(cases)+" field "+std::to_string(field));}
        std::string extra;if(row>>extra)throw std::runtime_error("source wind fixture trailing data");++cases;
    }
    if(!cases)throw std::runtime_error("no source wind cases");
    if(!probe)std::cout<<cases<<" original wind banks and auxiliary sequences matched\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
