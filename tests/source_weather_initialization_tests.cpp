#include "halo2/source_weather_initialization.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

int main(int argc,char **argv) {try {
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream file;
    if(!probe) {if(argc!=2)throw std::runtime_error("weather initialization fixture required");
        file.open(argv[1]);if(!file)throw std::runtime_error("weather initialization fixture unavailable");}
    auto &input=probe?std::cin:file;std::string line;unsigned cases=0;std::uint64_t compared_words=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        std::uint32_t seed,count,n;
        if(!(row>>seed>>count>>n) || n>65536)throw std::runtime_error("invalid weather initialization input");
        std::vector<std::uint32_t> words(35+n*13);
        for(auto &v:words)if(!(row>>v))throw std::runtime_error("truncated weather initialization input");
        auto bytes=std::as_writable_bytes(std::span(words));
        const bool result=halo2::source_weather_initialize_particles(bytes.first(140),
            std::bit_cast<std::int32_t>(count),bytes.subspan(140,n*20),
            bytes.subspan(140+n*20,n*32),seed);
        if(probe) {std::cout<<std::hex<<seed<<' '<<unsigned(result)<<' ';
            for(auto v:words)std::cout<<v<<' ';std::cout<<'\n';}
        else {
            std::uint32_t expected;
            if(!(row>>expected) || expected!=seed)throw std::runtime_error("weather seed mismatch "+std::to_string(cases));
            if(!(row>>expected) || expected!=unsigned(result))throw std::runtime_error("weather return mismatch");
            for(unsigned i=0;i<words.size();++i)if(!(row>>expected) || expected!=words[i])
                throw std::runtime_error("weather initialization mismatch case "+std::to_string(cases)+" word "+std::to_string(i));
        }
        std::string extra;if(row>>extra)throw std::runtime_error("weather fixture trailing data");
        ++cases;compared_words+=2+words.size();
    }
    if(!cases)throw std::runtime_error("no weather initialization cases");
    if(!probe && (cases!=1548 || compared_words!=201719))
        throw std::runtime_error("incomplete retained weather initialization corpus");
    // Rejected ownership inputs must leave every bank and the RNG untouched.
    std::array<std::byte,140> bank{};bank[0x64]=std::byte{1};
    std::array<std::byte,20> render{};std::array<std::byte,32> state{};
    unsigned guards=0;
    for(unsigned i=0;i<3;++i) {
        auto b=bank;auto r=render;auto s=state;std::uint32_t seed=7;
        try {halo2::source_weather_initialize_particles(
            std::span(b).first(i==0?139:140),1,
            std::span(r).first(i==1?19:20),std::span(s).first(i==2?31:32),seed);
            throw std::runtime_error("weather ownership guard accepted invalid input");
        }catch(const std::invalid_argument&) {
            if(b!=bank || r!=render || s!=state || seed!=7)
                throw std::runtime_error("weather ownership guard mutated input");
            ++guards;
        }
    }
    if(!probe)std::cout<<cases<<" unchanged original weather initialization cases; "<<guards<<" ownership guards\n";
    return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
