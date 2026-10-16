#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_vertex_declaration.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    const auto original=OriginalData::load_xbe(argv[1]);
    if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Cinematic environment requires TU1.5");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);unsigned bindings{};
    for(const auto name:{"panel_mech_ext_window","panel_hall_door_frame"}) {
        const auto path=std::string("scenarios\\shaders\\solo\\spacestation\\")+name;
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto &t){return t.group=="shad" && t.name==path;});
        if(tag==cache.tags().end())throw FormatError("Authored intro panel shader unavailable");
        const auto authored=cache.load_material(tag->datum);
        for(unsigned lod=0;lod<authored.lods.size();++lod) {
            const auto &definition=authored.lods[lod];
            if(!(definition.available_layers&(1u<<8)) || definition.layers.count()<=8)continue;
            const auto range=authored.layers.at(definition.layers.first()+8);
            const auto selected=std::ranges::find_if(authored.passes,[&](const auto &p){return p.template_pass>=range.first() &&
                p.template_pass-range.first()<range.count() && p.implementation==0 && p.vertex_shader.ends_with("\\stencil_env");});
            if(selected==authored.passes.end())throw FormatError("Authored panel environment owner changed");
            auto material=authored;material.selected_shader_lod=lod;
            if(!prepare_dynamic_material(cache,material,{},true,0.f))throw FormatError("Actual cinematic panel environment was rejected");
            const auto pass=std::ranges::find_if(material.world_overlays_after,[&](const auto &p){return p &&
                p->forward_pass_index==unsigned(selected-authored.passes.begin());});
            if(pass==material.world_overlays_after.end() || (*pass)->forward_vertex!=4 ||
                (*pass)->vertex_program!=selected->vertex_code || (*pass)->vertex_declaration!=selected->vertex_declaration ||
                (*pass)->forward_program.words!=selected->program.words ||
                (*pass)->forward_program.fragment_offsets!=selected->program.fragment_offsets ||
                !std::ranges::equal((*pass)->texture_states,selected->texture_states) ||
                !(*pass)->vertex_declaration || !source_world_vertex_declaration_supported(*(*pass)->vertex_declaration))
                throw FormatError("Cinematic panel substituted its authored environment program or input contract");
            ++bindings;
        }
    }
    if(bindings!=4)throw FormatError("Both intro panels and both authored LODs were not checked");
    std::cout<<"PASS four actual TU1.5 cinematic panel environment bindings/programs/declarations/samplers\n";
    std::cout<<"LIMIT binding checks; no rendered panel pixels or whole opening acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
