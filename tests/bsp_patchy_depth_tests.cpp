#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("TU1.5 depth fixture required");
        std::ifstream input(argv[1]);
        if(!input){std::cout<<"TU1.5 depth fixture not generated\n";return 77;}
        std::array<std::uint32_t,6> words{};
        unsigned count=0,enabled=0,disabled=0,fallback=0;
        while(input>>std::hex>>words[0]>>words[1]>>words[2]>>words[3]>>words[4]>>words[5]) {
            halo2::PatchyFog fog;
            fog.distance={std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1])};
            fog.density={0,.5f};fog.color={.6f,.35f,.2f};fog.noise.resize(1);
            const halo2::BspPatchyDefinition definition{0,fog};
            halo2::PatchyFogState state;std::uint32_t random=0x27aa0;
            const auto frame=halo2::bsp_patchy_fog_frame(definition,fog,state,random,{},0,1,.75f,0,
                std::bit_cast<float>(words[3]),std::bit_cast<float>(words[2]));
            if(frame.quantized_composite!=bool(words[4]) ||
                std::bit_cast<std::uint32_t>(frame.composite_minimum_depth)!=words[5])
                throw std::runtime_error("original quantized fog depth differs at fixture "+std::to_string(count));
            ++count;
            if(!words[4])++fallback;else if(words[5])++enabled;else ++disabled;
        }
        if(!input.eof() || count!=265 || !enabled || !disabled || !fallback)
            throw std::runtime_error("incomplete TU1.5 fog depth coverage");
        std::cout<<count<<" TU1.5 fog range/depth states exact: "<<enabled<<" enabled, "
            <<disabled<<" disabled, "<<fallback<<" fallback\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
