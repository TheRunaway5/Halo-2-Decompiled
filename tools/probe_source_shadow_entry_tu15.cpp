#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/source_view_projection.hpp"
#include <bit>
#include <iostream>

int main(int argc,char **argv)try {
    using namespace halo2;
    if(argc==2 && std::string_view(argv[1])=="--layers") {
        for(unsigned layer:{10u,11u})std::cout<<source_shadow_raw_entry_stipple_tu15(layer)<<'\n';
        return 0;
    }
    unsigned foreground;std::array<std::uint32_t,48> words;std::uint32_t distance,minimum,scalar;
    while(std::cin>>foreground) {
        for(auto &word:words)std::cin>>word;std::cin>>distance>>minimum>>scalar;
        if(!std::cin || foreground>1)throw FormatError("Invalid native shadow entry probe input");
        const auto result=source_view_shader_constants(std::bit_cast<SourceViewProjectionBank>(words),
            std::bit_cast<float>(distance),bool(foreground));
        for(unsigned row=0;row<4;++row)for(auto value:result[row])std::cout<<std::bit_cast<std::uint32_t>(value)<<' ';
        std::cout<<std::bit_cast<std::uint32_t>(foreground?source_shadow_raw_entry_minimum_w_tu15(
            std::bit_cast<float>(minimum),std::bit_cast<float>(scalar)):std::bit_cast<float>(minimum))<<'\n';
    }
    return std::cin.eof()?0:1;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
