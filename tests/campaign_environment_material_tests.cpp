#include "halo2/world_material.hpp"
#include "halo2/source_vertex_declaration.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char*why){if(!value)throw FormatError(why);}
}
int main(int argc,char**argv)try{
    require(argc==2,"supply original Cairo Station Xbox cache");
    if(!std::filesystem::exists(argv[1]))return 77;
    const Cache cache(argv[1]);unsigned materials=0,passes=0,lods=0;
    for(const auto&tag:cache.tags())if(tag.group=="shad" &&
        tag.name.starts_with("scenarios\\shaders\\solo\\spacestation\\")){
        const auto authored=cache.load_material(tag.datum);
        for(unsigned lod=0;lod<authored.lods.size();++lod){
            const auto&definition=authored.lods[lod];
            if(!(definition.available_layers&(1u<<8))||definition.layers.count()<=8)continue;
            const auto range=authored.layers.at(definition.layers.first()+8);
            const auto selected=std::find_if(authored.passes.begin(),authored.passes.end(),[&](const auto&p){
                return p.template_pass>=range.first() && p.template_pass-range.first()<range.count() &&
                    p.implementation==0 && p.vertex_shader.ends_with("\\stencil_env");});
            if(selected==authored.passes.end())continue;
            bool prepared=false;
            for(bool per_vertex:{false,true}){
                auto material=authored;material.selected_shader_lod=lod;
                if(!prepare_world_material(cache,material,per_vertex))continue;
                prepared=true;const auto found=std::find_if(material.world_overlays_after.begin(),material.world_overlays_after.end(),[&](const auto&pass){
                    return pass && pass->vertex_program==selected->vertex_code && pass->forward_pass_index==unsigned(selected-authored.passes.begin());});
                require(found!=material.world_overlays_after.end(),"authored campaign stencil_env pass was dropped");
                const auto&pass=**found;
                require(pass.vertex_declaration==selected->vertex_declaration && pass.forward_vertex==4,
                    "campaign environment pass lost its actual source declaration");
                require(pass.forward_program.words==selected->program.words && pass.forward_program.fragment_offsets==selected->program.fragment_offsets,
                    "campaign environment combiner was substituted");
                require(std::ranges::equal(pass.texture_states,selected->texture_states),"campaign environment sampler states changed");
                require(pass.source_shader_datum==tag.datum && pass.selected_shader_lod==lod,
                    "campaign environment shader/LOD owner changed");
                require(source_world_vertex_declaration_supported(*pass.vertex_declaration),"campaign environment declaration lacks the native world adapter");
                require(std::none_of(material.rendering_gaps.begin(),material.rendering_gaps.end(),[](const auto&gap){
                    return gap.find("world environment")!=std::string::npos;}),"campaign environment still reports an unresolved producer");
                ++passes;
            }
            require(prepared,"campaign environment source shader has no admitted base lighting path");++lods;
        }
        if(std::any_of(authored.passes.begin(),authored.passes.end(),[](const auto&p){return p.vertex_shader.ends_with("\\stencil_env");}))++materials;
    }
    require(materials>0 && lods>=materials && passes>=lods,"no authored campaign environment materials exercised");
    std::cout<<"PASS "<<materials<<" authored Cairo materials / "<<lods<<" shader LODs / "<<passes
        <<" original environment pass bindings; matched original pixels remain a separate gate\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
