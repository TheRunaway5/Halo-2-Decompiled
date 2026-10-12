#include "halo2/source_shield_model_shaders.hpp"
#include "halo2/source_cpu_skin_inputs.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>

namespace halo2 {
bool source_model_shader_override_part(std::uint16_t type,std::uint32_t original,
    std::int32_t selected,std::span<const std::uint32_t> original_layers,
    std::span<const std::uint32_t> override_layers) {
    if(type==0 || type==5 || original==0xffffffffu)return false;
    if(selected<0 || unsigned(selected)>=original_layers.size() ||
       unsigned(selected)>=override_layers.size())
        throw FormatError("source shader override LOD has no proved resident bank");
    const auto mask=original_layers[unsigned(selected)];
    return !(mask&0x8080u) || (mask&0xau) || !(override_layers.front()&0x200000u);
}
SourceShieldModelShaders::SourceShieldModelShaders(const Cache &cache,RenderModel &model,
    std::uint32_t render_model,std::uint32_t shader)
    :SourceShieldModelShaders(cache,static_cast<const RenderModel&>(model),render_model,shader,model.mesh.materials) {}
SourceShieldModelShaders::SourceShieldModelShaders(const Cache &cache,const RenderModel &model,
    std::uint32_t render_model,std::uint32_t shader,std::vector<Material> &output):shader_(shader) {
    auto material=cache.load_material(shader);
    if(!prepare_shield_overlay_material(cache,material))
        throw FormatError("source shield override requires a proved plasma pass");
    // The admitted actual plasma family has exactly one LOD.40F60 also binds
    // shaderLOD0/layer15/implementation0 for the transparent callback.
    if(material.lods.size()!=1 || material.lods.front().available_layers!=0x8000u)
        throw FormatError("source shield override differs from proved layer15 bank");
    definition_=cache.load_source_shader_lod_definition(shader);
    const auto chosen=cache.load_source_transparent_shader(shader);
    layers_=chosen.template_lod_layers;priority_=chosen.shader_priority_flags;
    commands_=chosen.shader_command_flags;
    std::vector<unsigned> sections;
    for(const auto &origin:model.vertex_origins)
        if(std::find(sections.begin(),sections.end(),origin.section)==sections.end())sections.push_back(origin.section);
    const auto authored=cache.load_source_transparent_model_parts(render_model,sections);
    std::map<unsigned,unsigned> slots;
    for(const auto &section:authored.sections)for(const auto &part:section.parts) {
        const auto origin=std::find_if(model.vertex_origins.begin(),model.vertex_origins.end(),
            [&](const auto &v){return v.section==section.section && v.part==part.ordinal;});
        if(origin==model.vertex_origins.end())continue;
        unsigned slot=0;
        if(part.type!=0 && part.type!=5 && part.shader!=0xffffffffu) {
            auto [resident,inserted]=slots.try_emplace(origin->vertex_class,unsigned(output.size()));
            if(inserted) {
                auto adapted=material;prepare_cpu_skin_material_inputs(cache,adapted,origin->vertex_class);
                output.push_back(std::move(adapted));
            }
            slot=resident->second;
        }
        if(!parts_.emplace(std::pair{section.section,unsigned(part.ordinal)},
            Part{part.type,part.shader,part.template_lod_layers,part.lod_scale>0?part.lod_scale:1.f,slot}).second)
            throw FormatError("source shield override has duplicate section/part identity");
    }
}
std::optional<SourceShieldPartBinding> SourceShieldModelShaders::binding(unsigned section,unsigned ordinal,
    const SourceModelShaderCollector &collector,const SourceModelShaderView &view)const {
    const auto &part=parts_.at({section,ordinal});
    if(part.type==0 || part.type==5 || part.original_shader==0xffffffffu)return {};
    const auto choice=source_model_shader_part(collector,part.scale,definition_,view).choice;
    if(!source_model_shader_override_part(part.type,part.original_shader,choice.index,part.original_layers,layers_))return {};
    return SourceShieldPartBinding{part.material,shader_,layers_.at(unsigned(choice.index)),choice.index,priority_,commands_};
}
void SourceShieldModelShaders::select(std::vector<Vertex> &vertices,
    const SourceModelShaderCollector &collector,const SourceModelShaderView &view)const {
    std::map<std::pair<unsigned,unsigned>,std::optional<SourceShieldPartBinding>> selected;
    for(const auto &[identity,part]:parts_)
        selected.emplace(identity,binding(identity.first,identity.second,collector,view));
    vertices.erase(std::remove_if(vertices.begin(),vertices.end(),[&](auto &vertex) {
        const auto found=selected.find({vertex.source_section,vertex.source_part});
        if(found==selected.end())throw FormatError("source shield vertex lost its authored part");
        if(!found->second)return true;
        vertex.material=found->second->material;return false;
    }),vertices.end());
}
}
