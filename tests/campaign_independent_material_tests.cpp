#include "halo2/world_material.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <tuple>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    unsigned checked{};
    for(const auto &[path,layer,blend]:std::array{
        std::tuple{"scenarios\\shaders\\solo\\highcharity\\long_volume",15u,3u},
        std::tuple{"scenarios\\shaders\\solo\\highcharity\\black_gradient",2u,6u},
        std::tuple{"scenarios\\shaders\\covenant\\military\\lights\\c_bloom_purplight",7u,0u}}) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto &t){return t.group=="shad" && t.name==path;});
        if(tag==cache.tags().end())throw FormatError("Authored chamber material unavailable");
        const auto authored=cache.load_material(tag->datum);auto material=authored;
        const auto range=authored.layers.at(authored.lods.at(0).layers.first()+layer);
        const auto selected=std::ranges::find_if(authored.passes,[&](const auto &p){return
            p.template_pass>=range.first() && p.template_pass-range.first()<range.count() && p.implementation==0;});
        if(selected==authored.passes.end() || !prepare_independent_object_material(cache,material))
            throw FormatError("Independent chamber material did not reach its authored pass");
        if(material.forward_blend!=blend || material.vertex_program!=selected->vertex_code ||
            material.vertex_declaration!=selected->vertex_declaration || material.forward_program.words!=selected->program.words)
            throw FormatError("Chamber volume/gradient program or blend differs from authored independent implementation");
        ++checked;
    }
    std::cout<<"PASS "<<checked<<" chamber volume/gradient/light independent programs, declarations and blends\n";
    std::cout<<"LIMIT shader binding; full-scene geometry, fog and pixels require GPU observation\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
