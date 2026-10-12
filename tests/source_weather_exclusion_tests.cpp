#include "halo2/source_weather_exclusion.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <xmmintrin.h>

namespace {
float scalar(std::uint32_t value){return std::bit_cast<float>(value);}
}
int main(int argc,char **argv){try{
    _mm_setcsr(0x1f80);
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream file;
    if(!probe){
        if(argc!=2)throw std::runtime_error("weather exclusion fixture required");
        file.open(argv[1]);if(!file)throw std::runtime_error("weather exclusion fixture missing");
    }
    auto &input=probe?std::cin:file;
    std::string line;std::size_t cases=0,input_words=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        std::array<std::uint32_t,9> fields{};
        for(auto &value:fields)if(!(row>>value))throw std::runtime_error("weather exclusion input truncated");
        const auto n=fields[8];if(n>65536)throw std::runtime_error("weather exclusion plane bank too large");
        std::vector<std::array<float,4>> planes(n);
        for(auto &plane:planes)for(auto &component:plane){
            std::uint32_t value;if(!(row>>value))throw std::runtime_error("weather exclusion plane input truncated");
            component=scalar(value);
        }
        const std::array<float,3> point={scalar(fields[0]),scalar(fields[1]),scalar(fields[2])};
        const halo2::SourceWeatherExclusionVolume volume={
            {scalar(fields[3]),scalar(fields[4]),scalar(fields[5])},scalar(fields[6]),
            std::bit_cast<std::int32_t>(fields[7]),planes};
        std::vector<std::array<std::uint32_t,4>> original_planes;
        for(const auto &plane:planes)original_planes.push_back(std::bit_cast<std::array<std::uint32_t,4>>(plane));
        const bool result=halo2::source_weather_point_in_exclusion(point,volume);
        for(std::size_t i=0;i<point.size();++i){
            if(std::bit_cast<std::uint32_t>(point[i])!=fields[i] ||
                std::bit_cast<std::uint32_t>(volume.center[i])!=fields[i+3])
                throw std::runtime_error("weather exclusion changed point or center input");
        }
        if(std::bit_cast<std::uint32_t>(volume.radius)!=fields[6] ||
            std::bit_cast<std::uint32_t>(volume.plane_count)!=fields[7] ||
            volume.planes.data()!=planes.data() || volume.planes.size()!=fields[8])
            throw std::runtime_error("weather exclusion changed volume input");
        for(std::size_t i=0;i<planes.size();++i){
            if(std::bit_cast<std::array<std::uint32_t,4>>(planes[i])!=original_planes[i])
                throw std::runtime_error("weather exclusion changed plane input");
        }
        if(probe)std::cout<<unsigned(result)<<'\n';
        else{
            std::uint32_t expected;
            if(!(row>>expected) || expected>1 || expected!=unsigned(result))
                throw std::runtime_error("weather exclusion result mismatch case "+std::to_string(cases));
        }
        std::string extra;if(row>>extra)throw std::runtime_error("weather exclusion trailing fixture data");
        ++cases;input_words+=fields.size()+n*4;
    }
    if(!input.eof() || (!probe && (cases!=4421 || input_words!=178629)))
        throw std::runtime_error("weather exclusion frozen corpus incomplete");
    std::array<std::array<float,4>,1> owned{{{1,0,0,0}}};
    unsigned guards=0;
    for(auto [count,storage]:{std::pair<std::int32_t,std::size_t>{1,0},{2,1}}){
        halo2::SourceWeatherExclusionVolume volume{{0,0,0},1,count,std::span(owned).first(storage)};
        try{halo2::source_weather_point_in_exclusion({0,0,0},volume);
            throw std::runtime_error("weather exclusion plane ownership guard failed");
        }catch(const std::invalid_argument&){++guards;}
    }
    if(!probe)std::cout<<cases<<" unchanged source exclusion cases; "<<input_words
        <<" input words preserved; "<<guards<<" ownership guards\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
