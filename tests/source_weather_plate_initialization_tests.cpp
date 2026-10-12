#include "halo2/source_weather_initialization.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc,char **argv) {try {
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream file;
    if(!probe) {if(argc!=2)throw std::runtime_error("weather plate fixture required");
        file.open(argv[1]);if(!file)throw std::runtime_error("weather plate fixture unavailable");}
    auto &input=probe?std::cin:file;std::string line;unsigned cases=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;std::uint32_t seed;
        std::array<std::uint32_t,234> words;
        if(!(row>>seed))throw std::runtime_error("weather plate input missing");
        for(auto &v:words)if(!(row>>v))throw std::runtime_error("weather plate bank truncated");
        halo2::source_weather_initialize_plate(std::as_writable_bytes(std::span(words)),seed);
        if(probe) {std::cout<<std::hex<<seed<<' ';for(auto v:words)std::cout<<v<<' ';std::cout<<'\n';}
        else {
            std::uint32_t expected;
            if(!(row>>expected) || expected!=seed)throw std::runtime_error("weather plate seed mismatch");
            for(unsigned i=0;i<words.size();++i)if(!(row>>expected) || expected!=words[i])
                throw std::runtime_error("weather plate mismatch case "+std::to_string(cases)+" word "+std::to_string(i));
        }
        std::string extra;if(row>>extra)throw std::runtime_error("weather plate fixture trailing data");++cases;
    }
    if(!cases)throw std::runtime_error("no weather plate cases");
    if(!probe && cases!=1544)throw std::runtime_error("incomplete retained weather plate corpus");
    std::array<std::byte,936> bank{};std::uint32_t seed=7;
    try {halo2::source_weather_initialize_plate(std::span(bank).first(935),seed);
        throw std::runtime_error("weather plate ownership guard accepted short bank");
    }catch(const std::invalid_argument&) {
        if(bank!=std::array<std::byte,936>{} || seed!=7)throw std::runtime_error("plate ownership guard mutated input");
    }
    if(!probe)std::cout<<cases<<" original plate banks and auxiliary streams; 1 ownership guard\n";
    return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
