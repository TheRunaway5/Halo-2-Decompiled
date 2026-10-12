#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_fog_clear.hpp"
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <cfenv>
#include <string_view>

namespace {
int math(const char *path) {
    using namespace halo2;
    std::ifstream input(path);if(!input)throw FormatError("Missing original clear math fixture");
    const auto rounding=std::fegetround(),flags=std::fetestexcept(FE_ALL_EXCEPT);
    unsigned tails=0,packs=0;std::string type,id;
    while(input>>type>>id) {
        auto word=[&](){std::uint32_t value;if(!(input>>std::hex>>value))throw FormatError("Truncated clear math row");return value;};
        auto value=[&](){return std::bit_cast<float>(word());};
        auto color=[&](){return Vec3{value(),value(),value()};};
        if(type=="T") {
            SourceFogClearInput in;in.sky_model_control=std::uint8_t(word());
            in.retained_color=color();in.sky_color_premultiplied=color();in.sky_opacity=value();
            in.primary.color=color();in.primary.maximum_density=value();
            in.secondary.color=color();in.secondary.maximum_density=value();
            in.planar_density=value();in.interpolation=value();
            const std::array<std::uint32_t,3> expected{word(),word(),word()};
            const auto actual=std::bit_cast<std::array<std::uint32_t,3>>(source_fog_clear_color(in));
            if(actual!=expected)throw FormatError("Original terminal fog RGB differs: "+id);
            ++tails;
        } else if(type=="P") {
            const auto control=word();const auto rgb=color();const auto expected=word();
            if(control>65535 || source_clear_color_rgb8(rgb,std::uint16_t(control))!=expected)
                throw FormatError("Original packed clear RGB differs: "+id);
            ++packs;
        } else throw FormatError("Unknown original clear math row");
    }
    if(!tails || !packs)throw FormatError("Missing original clear branch/packing coverage");
    // The bitwise packer must not change the caller's floating environment.
    const auto pack_flags=std::fetestexcept(FE_ALL_EXCEPT);
    for(auto control:{0x023fu,0x063fu,0x0a3fu,0x0e3fu,0x027fu,0x067fu,0x0a7fu,0x0e7fu})
        (void)source_clear_color_rgb8({.5f,-.5f,1.5f},std::uint16_t(control));
    if(std::fegetround()!=rounding || std::fetestexcept(FE_ALL_EXCEPT)!=pack_flags)
        throw FormatError("Clear-color packing changed the host floating environment");
    (void)flags;
    for(auto control:{0x017fu,0x027eu,0x02bfu,0x127fu}) {
        bool rejected=false;
        try {(void)source_clear_color_rgb8({},std::uint16_t(control));}
        catch(const FormatError&){rejected=true;}
        if(!rejected)throw FormatError("Unsupported original clear control was admitted");
    }
    std::cout<<tails<<" original terminal fog RGB and "<<packs<<" original packed RGB cases passed\n";
    return 0;
}
}

int main(int argc,char **argv) {
    using namespace halo2;
    if(argc==3 && std::string_view(argv[1])=="--math")return math(argv[2]);
    const bool sky_density=argc==5 && std::string_view(argv[4])=="--sky-density";
    if(argc!=4 && !sky_density)throw FormatError("Expected base maps, DLC maps and differential inputs");
    std::ifstream input(argv[3]);if(!input)throw FormatError("Missing clear-color differential input");
    std::map<std::string,BspAtmosphere> banks;
    std::string name;float x,y,z;int cluster;
    while(input>>name>>x>>y>>z>>cluster) {
        if(!banks.contains(name)) {
            auto path=std::filesystem::path(argv[1])/name;
            if(!std::filesystem::exists(path))path=std::filesystem::path(argv[2])/name;
            banks.emplace(name,Cache(path).load_bsp_atmosphere(0));
        }
        const auto f=banks.at(name).sample({x,y,z},{1,0,0},cluster);
        const auto clear=f.clear.value_or(Vec3{});
        std::cout<<name<<' '<<cluster<<std::hex<<' '
            <<std::bit_cast<std::uint32_t>(clear.x)<<' '
            <<std::bit_cast<std::uint32_t>(clear.y)<<' '
            <<std::bit_cast<std::uint32_t>(clear.z);
        if(sky_density)std::cout<<' '<<std::bit_cast<std::uint32_t>(f.sky_fog.maximum_density);
        std::cout<<std::dec<<'\n';
    }
    if(!input.eof())throw FormatError("Malformed clear-color input");
}
