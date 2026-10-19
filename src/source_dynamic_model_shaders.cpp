#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_cpu_skin_inputs.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>

namespace halo2 {
SourceDynamicModelShaders::SourceDynamicModelShaders(const Cache &cache,RenderModel &model,
    std::uint32_t render_model,unsigned implementation,std::optional<float> camouflage) {
    if(implementation!=1 && implementation!=2 && implementation!=3)
        throw FormatError("model shader bank requires a proved lighting implementation");
    const auto roots=model.mesh.materials;
    std::vector<unsigned> sections;
    for(const auto &origin:model.vertex_origins)
        if(std::find(sections.begin(),sections.end(),origin.section)==sections.end())sections.push_back(origin.section);
    const auto authored=cache.load_source_transparent_model_parts(render_model,sections);
    std::map<std::pair<unsigned,unsigned>,unsigned> input_classes;
    for(const auto &section:authored.sections)for(const auto &part:section.parts) {
        const auto origin=std::find_if(model.vertex_origins.begin(),model.vertex_origins.end(),
            [&](const auto &v){return v.section==section.section && v.part==part.ordinal;});
        if(origin==model.vertex_origins.end())continue;
        const auto source=model.mesh.vertices.at(std::size_t(origin-model.vertex_origins.begin())).material;
        if(source>=roots.size() || part.material!=int(source) || roots[source].source_shader_datum!=part.shader)
            throw FormatError("dynamic model part differs from its authored shader");
        auto [definition,new_definition]=definitions_.try_emplace(part.shader);
        if(new_definition)definition->second=cache.load_source_shader_lod_definition(part.shader);
        // A part's literal compiled class may require a pass-specific input
        // operation after CPU posing. Other classes/programs share the
        // original resident slots, preserving all existing input values.
        auto [input,first_input]=input_classes.try_emplace({source,origin->vertex_class});
        if(first_input) {
        std::vector<Material> resident{roots[source]};
        bool transformed=prepare_cpu_skin_material_inputs(cache,resident.front(),origin->vertex_class);
        const auto raw=cache.load_material(part.shader);
        if(raw.lods.size()!=definition->second.thresholds.size())
            throw FormatError("dynamic model shader differs from its source LOD bank");
        for(unsigned lod=1;lod<raw.lods.size();++lod) {
            auto material=raw;material.selected_shader_lod=lod;
            if(!(implementation==1 ? prepare_world_material(cache,material,true) :
                 implementation==3 ? prepare_default_object_material(cache,material,camouflage) :
                 prepare_dynamic_material(cache,material,{},true,camouflage)))
                throw FormatError("dynamic model shader LOD requires unresolved source inputs: "+raw.name);
            if(roots[source].object_render_colors)
                apply_object_render_colors(material,*roots[source].object_render_colors);
            transformed=prepare_cpu_skin_material_inputs(cache,material,origin->vertex_class)||transformed;
            resident.push_back(std::move(material));
        }
        const unsigned input_class=transformed?origin->vertex_class:0u;
        input->second=input_class;
        auto &slots=slots_[{source,input_class}];
        if(slots.empty()) {
            if(transformed) {
                slots.push_back(unsigned(model.mesh.materials.size()));
                model.mesh.materials.push_back(std::move(resident.front()));
            } else slots.push_back(source);
            for(unsigned lod=1;lod<resident.size();++lod) {
                slots.push_back(unsigned(model.mesh.materials.size()));
                model.mesh.materials.push_back(std::move(resident[lod]));
            }
        }
        }
        auto [binding,inserted]=parts_.emplace(std::pair{section.section,unsigned(part.ordinal)},
            Part{source,part.lod_scale>0?part.lod_scale:1.f,part.shader,input->second});
        if(!inserted && (binding->second.source_material!=source || binding->second.input_class!=input->second))
            throw FormatError("dynamic model duplicate part changed its material");
    }
}
std::map<std::pair<unsigned,unsigned>,unsigned> SourceDynamicModelShaders::select_parts(
    const SourceModelShaderCollector &collector,const SourceModelShaderView &view)const {
    std::map<std::pair<unsigned,unsigned>,unsigned> selected;
    for(const auto &[identity,part]:parts_) {
        const auto choice=source_model_shader_part(collector,part.scale,definitions_.at(part.shader),view).choice;
        selected.emplace(identity,choice.accepted && choice.index>=0?
            slots_.at({part.source_material,part.input_class}).at(unsigned(choice.index)):0xffffffffu);
    }
    return selected;
}
void SourceDynamicModelShaders::select(std::vector<Vertex> &vertices,
    const SourceModelShaderCollector &collector,const SourceModelShaderView &view)const {
    const auto selected=select_parts(collector,view);
    vertices.erase(std::remove_if(vertices.begin(),vertices.end(),[&](auto &vertex) {
        const auto binding=selected.find({vertex.source_section,vertex.source_part});
        if(binding==selected.end())throw FormatError("dynamic model vertex lost its authored shader part");
        if(binding->second==0xffffffffu)return true;
        vertex.material=binding->second;return false;
    }),vertices.end());
}
void SourceDynamicModelShaders::append_selected(std::vector<Vertex> &output,std::span<const Vertex> source,
    const SourceModelShaderCollector &collector,const SourceModelShaderView &view,unsigned material_base)const {
    const auto selected=select_parts(collector,view);
    for(std::size_t first=0;first<source.size();) {
        const auto identity=std::pair{source[first].source_section,source[first].source_part};
        const auto binding=selected.find(identity);
        if(binding==selected.end())throw FormatError("dynamic model vertex lost its authored shader part");
        auto end=first+1;
        while(end<source.size() && std::pair{source[end].source_section,source[end].source_part}==identity)++end;
        if(binding->second!=0xffffffffu) {
            const auto offset=output.size();const auto run=source.subspan(first,end-first);
            output.insert(output.end(),run.begin(),run.end());
            for(std::size_t i=offset;i<output.size();++i)output[i].material=binding->second+material_base;
        }
        first=end;
    }
}

}
