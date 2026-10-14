#include "halo2/source_particle_sprite_context.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply DISC_MAPS DLC_MAPS ORIGINAL_CONTEXT_FIXTURE");
    std::ifstream fixture(argv[3]);
    if(!fixture)throw FormatError("Original sprite-context fixture absent");
    std::unique_ptr<Cache> cache;std::string current,line;std::set<std::string> maps;
    unsigned cases{},rectangular{},atlases{};
    while(std::getline(fixture,line)) {
        const auto row=nlohmann::json::parse(line);const auto name=row.at("map").get<std::string>();
        if(name!=current) {
            auto path=std::filesystem::path(argv[1])/(name+".map");
            if(!std::filesystem::exists(path))path=std::filesystem::path(argv[2])/(name+".map");
            if(!std::filesystem::exists(path))return 77;
            cache=std::make_unique<Cache>(path,argv[1]);current=name;maps.insert(name);
        }
        const auto datum=row.at("datum").get<std::uint32_t>();
        const auto sprite=cache->load_particle_sprite(datum);
        const auto context=source_particle_sprite_context(*cache,datum,sprite);
        if(bool(context)!=row.at("draw").get<bool>())throw FormatError("Original sprite-context admission differs");
        if(context) {
            if(context->selected_sequence!=row.at("sequence").get<std::int32_t>() ||
               std::bit_cast<std::uint32_t>(context->width)!=row.at("width_bits").get<std::uint32_t>() ||
               context->has_bitmap!=row.at("has_bitmap").get<bool>())
                throw FormatError("Original sprite-context tuple differs");
            rectangular+=std::bit_cast<std::uint32_t>(context->width)!=0x3f800000u;
            atlases+=context->selected_sequence>=0;
        }
        ++cases;
    }
    if(maps.size()!=21 || cases!=3655 || rectangular!=282 || atlases!=3138)
        throw FormatError("Incomplete original Xbox sprite-context coverage");
    std::cout<<"PASS 3655 original sprite-context prefixes across21 Xbox maps; 282 bitmap aspect ratios, 3138 atlas contexts\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
