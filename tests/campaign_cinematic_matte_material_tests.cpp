#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
using namespace halo2;

int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    const auto original=OriginalData::load_xbe(argv[1]);
    if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Matte check requires TU1.5");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
    unsigned bindings{};
    for(const auto name:{"threshold_space","halo_destroyed_01","halo_destroyed_02",
                         "high_charity_exterior","high_charity_stardust"}) {
        const auto path=std::string("objects\\cinematics\\matte_paintings\\shaders\\")+name;
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto &t){return t.group=="shad" && t.name==path;});
        if(tag==cache.tags().end())throw FormatError("Authored opening matte shader unavailable");
        const auto authored=cache.load_material(tag->datum);
        for(unsigned lod=0;lod<authored.lods.size();++lod) {
            const auto &definition=authored.lods[lod];
            if(definition.available_layers!=(1u<<15) || definition.layers.count()<=15)
                throw FormatError("Opening matte has a separate lighting/pass owner");
            const auto range=authored.layers.at(definition.layers.first()+15);
            const auto selected=std::ranges::find_if(authored.passes,[&](const auto &p){return
                p.template_pass>=range.first() && p.template_pass-range.first()<range.count() && p.implementation==0;});
            if(selected==authored.passes.end())throw FormatError("Original matte transparent pass unavailable");
            auto dynamic=authored,ordinary=authored;dynamic.selected_shader_lod=ordinary.selected_shader_lod=lod;
            if(!prepare_dynamic_material(cache,dynamic,{},true,0.f) ||
               !prepare_default_object_material(cache,ordinary,0.f))
                throw FormatError("Authored matte transparent pass was rejected");
            const auto blend=std::string_view(name)=="high_charity_stardust"?1u:5u;
            // The cloud pass supplies two stage records; the native material
            // retains four NV2A slots. Compare every supplied record, rather
            // than treating the additional unused slots as authored samplers.
            for(const auto *material:{&dynamic,&ordinary})
                if(material->forward_vertex!=1 || material->forward_blend!=blend ||
                   material->vertex_program!=selected->vertex_code || material->vertex_declaration!=selected->vertex_declaration ||
                   material->forward_program.words!=selected->program.words ||
                   material->forward_program.fragment_offsets!=selected->program.fragment_offsets ||
                   selected->texture_states.size()>material->texture_states.size() ||
                   !std::equal(selected->texture_states.begin(),selected->texture_states.end(),material->texture_states.begin()))
                    throw FormatError(std::string(name)+" matte binding differs: vertex="+std::to_string(material->forward_vertex)+
                        " blend="+std::to_string(material->forward_blend)+" program="+
                        std::to_string(material->forward_program.words==selected->program.words)+" declaration="+
                        std::to_string(material->vertex_declaration==selected->vertex_declaration)+" samplers="+
                        std::to_string(std::equal(selected->texture_states.begin(),selected->texture_states.end(),material->texture_states.begin())));
            ++bindings;
        }
    }
    if(bindings!=5)throw FormatError("Opening matte shader LOD census changed");
    std::cout<<"PASS five original matte bindings, independent lighting classifications, exact programs/declarations/samplers and RGBA blends\n";
    std::cout<<"LIMIT binding checks; rendered matte pixels and whole opening remain separate checks\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
