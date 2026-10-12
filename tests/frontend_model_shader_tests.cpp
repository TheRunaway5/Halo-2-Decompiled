#include "halo2/frontend_model_shader.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool yes,const std::string& message){if(!yes)throw std::runtime_error(message);}
std::uint32_t word(float v){return std::bit_cast<std::uint32_t>(v);}
}
int main(int argc,char**argv){try {
    require(argc==3,"original fixture and mainmenu map required");
    if(!std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[2]);
    const auto camera=frontend_model_coverage_camera();
    struct Resident {RenderModel model;std::optional<SourceDynamicModelShaders> shaders;SourceTransparentModelParts parts;};
    std::vector<Resident> models;
    const auto prepare_models=[&] {for(auto unit:{0xe23400bfu,0xe2be0120u}) {
        Resident r;
        const auto definition=cache.load_initial_model_definition(unit);
        if(unit==0xe2be0120u) {
            // Actual BR collector in the same-clock reference uses section0.
            // Literal BR region/permutation maps only geometryLOD0 to it.
            const auto selection=source_deterministic_initial_model_selection(definition,0);
            require(source_initial_model_sections(definition,selection,0)==std::vector<unsigned>{0},
                "captured BR geometry section differs from literal model definition");
            r.model=cache.load_model(unit,true,0,selection,0);
        } else r.model=cache.load_model(unit,true);
        const unsigned implementation=unit==0xe2be0120u?3:2;
        for(auto&m:r.model.mesh.materials)if(!m.lods.empty())
            require(implementation==3?prepare_default_object_material(cache,m,0.f):
                prepare_dynamic_material(cache,m,{},true,0.f),"source preview material unresolved: "+m.name);
        const auto mode=definition.render_model;
        r.shaders.emplace(cache,r.model,mode,implementation,0.f);
        std::vector<unsigned> sections;
        for(const auto&v:r.model.vertex_origins)if(std::find(sections.begin(),sections.end(),v.section)==sections.end())sections.push_back(v.section);
        r.parts=cache.load_source_transparent_model_parts(mode,sections);models.push_back(std::move(r));
    }};
    std::ifstream file(argv[1]);require(bool(file),"original fixture unavailable");
    std::string line;unsigned producers=0,collectors=0,choices=0,integrated=0;
    while(std::getline(file,line)) {
        std::istringstream input(line);input>>std::hex;unsigned kind;input>>kind;
        if(kind==0) {
            for(auto v:camera.inverse_view_z){unsigned expected;input>>expected;require(word(v)==expected,"preview collector used model-widget camera instead of enclosing frontend camera");}
            unsigned expected;input>>expected;require(word(camera.viewport_height_metric)==expected,"preview coverage viewport metric differs from original2232A0");++producers;
        } else if(kind==1) {
            std::array<float,5> values;unsigned w;for(auto&v:values){input>>w;v=std::bit_cast<float>(w);}
            unsigned coverage,byte;input>>coverage>>byte;
            SourceModelShaderView view{camera,{},values[4],.85f,true};
            const auto c=source_model_shader_collector({values[0],values[1],values[2]},values[3],view);
            require(word(c.coverage)==coverage && c.coverage_byte==byte,"preview collector coverage changed before source quantization");++collectors;
        } else if(kind==2) {
            if(models.empty())prepare_models();
            unsigned shader,metric,modifier,lod,accepted;input>>shader>>metric;
            SourceModelShaderView view{camera,{},1,.85f,true};unsigned w;
            for(auto&v:view.shader.multipliers){input>>w;v=std::bit_cast<float>(w);}
            input>>modifier>>lod>>accepted;view.view_modifier=modifier;
            const auto definition=cache.load_source_shader_lod_definition(shader);
            const auto direct=source_shader_lod_choice(definition.thresholds,definition.bias,std::bit_cast<float>(metric),view.shader,view.view_modifier);
            require(unsigned(direct.index)==lod && direct.accepted==bool(accepted),"original shader choice changed");++choices;
            bool found=false;
            for(const auto&r:models) {
                auto vertices=r.model.mesh.vertices;
                // Captured whole-owner collector is copied to the BR as well.
                const SourceModelShaderCollector collector{std::bit_cast<float>(0x419234ccu),4};
                r.shaders->select(vertices,collector,view);
                for(const auto&section:r.parts.sections)for(const auto&part:section.parts) {
                    if(part.shader!=shader || word(source_shader_part_metric(4,part.lod_scale>0?part.lod_scale:1.f,.85f))!=metric)continue;
                    bool emitted=false;
                    for(const auto&v:vertices)if(v.source_section==section.section && v.source_part==part.ordinal) {
                        const auto&m=r.model.mesh.materials.at(v.material);
                        require(m.source_shader_datum==shader && m.selected_shader_lod==lod,"resident preview selected wrong source shader bank");emitted=true;
                    }
                    require(emitted==bool(accepted),"resident preview part admission changed");found=true;++integrated;
                }
            }
            if(!found) {
                std::cerr<<"captured shader="<<std::hex<<shader<<" metric="<<metric<<" case="<<choices-1<<'\n';
                for(const auto&r:models)for(const auto&section:r.parts.sections)for(const auto&part:section.parts)
                    if(part.shader==shader)std::cerr<<"resident section="<<section.section<<" part="<<part.ordinal<<" metric="<<word(source_shader_part_metric(4,part.lod_scale>0?part.lod_scale:1.f,.85f))<<'\n';
            }
            require(found,"captured preview shader/scale has no literal resident model part");
        } else throw std::runtime_error("unknown original fixture operation");
        require(bool(input),"truncated original fixture");std::string extra;require(!(input>>extra),"trailing original fixture fields");
    }
    require(producers==8 && collectors==5 && choices==32,"incomplete original preview shader fixture");
    std::cout<<producers<<" enclosing camera producers, "<<collectors<<" actual collector stores and "<<choices<<" captured choices; "<<integrated<<" resident part checks matched\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
