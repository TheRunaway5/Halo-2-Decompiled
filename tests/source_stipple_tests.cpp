#include "halo2/source_stipple.hpp"
#include "halo2/source_shader_lod.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void origin_controls(const std::array<std::uint32_t,32> &pattern) {
    using namespace halo2;
    // A view rendered into a separate crop keeps the same physical source
    // phase even when its origin is not divisible by the pattern dimensions.
    for(const auto origin:std::array<std::array<std::uint32_t,2>,4>{{{1,3},{683,359},{640,360},{959,539}}})
        for(unsigned y=0;y<65;++y)for(unsigned x=0;x<65;++x)
            require(source_stipple_pixel(pattern,x+origin[0],y+origin[1],{0,31})==
                source_stipple_pixel(pattern,x,y,{origin[0],31u-origin[1]}),
                "stipple cropped target lost actual global pixel phase");
    const auto huge=std::numeric_limits<std::uint32_t>::max();
    require(source_stipple_pixel(pattern,huge,huge,{0,31})==source_stipple_pixel(pattern,31,31,{0,31}),
        "stipple unsigned pixel wrap changed modulo32 phase");
}
void source_mask_controls() {
    // This is the rank-table lowering used by the Vulkan shader. Check it
    // against every literal original upload bank selected by the alpha byte.
    constexpr std::array<unsigned,16> rank{0,12,3,10,8,4,15,6,2,11,1,13,14,7,9,5};
    std::array<bool,17> levels{};
    for(unsigned alpha=0;alpha<256;++alpha) {
        const auto bank=halo2::source_shader_instance_stipple(std::uint8_t(alpha));
        require(bank.level<=16,"source stipple level invalid");levels[bank.level]=true;
        unsigned visible=0;
        for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x) {
            const bool actual=halo2::source_stipple_pixel(bank.rows,x,y,{0,31});
            const unsigned row=(31u-y)&3u,column=(7u-x)&3u;
            require(actual==(rank[row*4+column]<bank.level),
                "Vulkan stipple rank table differs from literal original upload bank");
            visible+=actual;
        }
        require(visible==64u*bank.level,"source stipple mask population changed");
    }
    for(bool present:levels)require(present,"source stipple level lacks alpha coverage");
}
}
int main(int argc,char **argv)try {
    source_mask_controls();
    require(argc==2,"source stipple test requires frozen hardware fixture");
    std::ifstream input(argv[1]);require(bool(input),"source stipple hardware fixture missing");
    std::map<unsigned,std::array<std::uint32_t,32>> patterns;
    std::map<unsigned,unsigned> counts;std::string line;unsigned samples=0;
    while(std::getline(input,line)) {
        if(line.empty())continue;std::istringstream row(line);char kind;unsigned id;row>>kind>>id;
        if(kind=='P') {
            std::array<std::uint32_t,32> pattern;for(auto &word:pattern)require(bool(row>>word),"stipple hardware pattern truncated");
            require(patterns.emplace(id,pattern).second,"stipple hardware pattern repeated");origin_controls(pattern);
        }else if(kind=='S') {
            unsigned x,y,expected;require(bool(row>>x>>y>>expected),"stipple hardware pixel truncated");
            require(expected<2 && patterns.contains(id),"stipple hardware pixel has invalid bank/bit");
            require(halo2::source_stipple_pixel(patterns.at(id),x,y,{0,31})==bool(expected),
                "NV097 stipple pixel differs from Xbox hardware framebuffer");++counts[id];++samples;
        }else throw std::runtime_error("stipple hardware fixture record unknown");
        std::string trailing;require(!(row>>trailing),"stipple hardware fixture has trailing data");
    }
    require(patterns.size()==6 && counts.size()==6 && samples==29400,"stipple hardware coverage incomplete");
    std::cout<<samples<<" frozen Xbox hardware stipple pixels exact; explicit crop phase controls and all 17 source masks pass\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
