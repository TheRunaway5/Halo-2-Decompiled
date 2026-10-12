#include "halo2/source_weather_plate_simulation.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc,char **argv) {try {
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;
    if(!probe) {
        if(argc!=2)throw std::runtime_error("weather plate step fixture required");
        file.open(argv[1]);if(!file)throw std::runtime_error("weather plate step fixture unavailable");
    }
    auto &input=probe?std::cin:file;std::string line;unsigned cases=0;std::size_t checked=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        std::uint32_t seed,elapsed,intensity;
        if(!(row>>seed>>elapsed>>intensity))throw std::runtime_error("weather plate step input missing");
        std::array<std::uint32_t,338> words;
        for(auto &v:words)if(!(row>>v))throw std::runtime_error("weather plate step bank truncated");
        const auto bytes=std::as_writable_bytes(std::span(words));
        const auto result=halo2::source_weather_step_plate(bytes.first(936),
            {bytes.subspan(936,244),bytes.subspan(1180,172),
             std::bit_cast<float>(elapsed),std::bit_cast<float>(intensity)},seed);
        if(probe) {
            std::cout<<std::hex<<seed<<' '<<unsigned(result)<<' ';
            for(auto v:words)std::cout<<v<<' ';
            std::cout<<'\n';
        } else {
            std::uint32_t expected;
            if(!(row>>expected) || expected!=seed)throw std::runtime_error("weather plate step RNG differs case "+std::to_string(cases));
            if(!(row>>expected) || expected!=unsigned(result))throw std::runtime_error("weather plate step return differs");
            for(unsigned i=0;i<words.size();++i)if(!(row>>expected) || expected!=words[i])
                throw std::runtime_error("weather plate step differs case "+std::to_string(cases)+" word "+std::to_string(i));
            checked+=340;
        }
        std::string extra;if(row>>extra)throw std::runtime_error("weather plate step fixture trailing data");++cases;
    }
    if(!cases)throw std::runtime_error("no weather plate step cases");
    if(!probe && (cases!=1810 || checked!=615400))
        throw std::runtime_error("weather plate step frozen corpus incomplete");
    std::array<std::byte,936> plate{};std::array<std::byte,244> view{};std::array<std::byte,172> weather{};
    unsigned guards=0;
    for(unsigned kind=0;kind<11;++kind) {
        auto p=plate;auto v=view;auto w=weather;std::uint32_t seed=7;
        if(kind==3){v[0]=std::byte{0xff};v[1]=std::byte{0xff};}
        if(kind==4)v[0]=std::byte{4};
        if(kind>=5) {
            const std::uint32_t angle[]={0x7fc12345u,0x7fa12345u,0x7f800000u,0xff800000u,0x5f800000u,0xdf800000u};
            for(unsigned byte=0;byte<4;++byte)v[0x40+byte]=std::byte((angle[kind-5]>>(8*byte))&255u);
        }
        const auto before_p=p;const auto before_v=v;const auto before_w=w;
        bool rejected=false;
        try {halo2::source_weather_step_plate(std::span(p).first(kind==0?935:936),
            {std::span(v).first(kind==1?243:244),std::span(w).first(kind==2?171:172),0,0},seed);
        }catch(const std::invalid_argument&){rejected=true;}
        if(!rejected || p!=before_p || v!=before_v || w!=before_w || seed!=7)
            throw std::runtime_error("weather plate step ownership guard failed");
        ++guards;
    }
    if(!probe)std::cout<<cases<<" original weather plate step cases; "<<checked<<" words exact; "<<guards<<" ownership guards\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
