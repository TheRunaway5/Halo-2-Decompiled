#include "halo2/pstencil/material.hpp"
#include "halo2/render_program.hpp"
#include <algorithm>

namespace halo2::pstencil {
namespace {
std::optional<Material> prepare(const Cache& cache,Material authored,unsigned index,
    const EffectLightUniforms& uniforms) {
    auto& pass=authored.passes.at(index);
    authored.shader_switches[0]=authored.shader_switches[1]=std::int32_t(uniforms.color_shift);
    // This pass uses the shader's actual specular selector (switch4), not a
    // guessed reflectivity constant or a replacement Phong exponent.
    pass.program=assemble_pixel_program(pass.definition,pass.fragments,pass.permutations,
        pass.combiners,authored.shader_switches);
    remap_pixel_constants(pass.program,authored.pixel_constants,pass.pixel_remaps,
        pass.fragments,authored.shader_switches);
    std::array<float,256> scalars{};scalars[21]=authored.specular_brightness;
    scalars[22]=authored.ambient_brightness;std::vector<ShaderRemap> brightness;
    for(const auto& r:pass.pixel_externs)if(r.source==21 || r.source==22)brightness.push_back(r);
    remap_pixel_externs(pass.program,scalars,{},brightness,pass.fragments,authored.shader_switches);
    return bind_light_pass(cache,authored,index,uniforms);
}
}
void MaterialBank::preload(const Material& authored,std::uint32_t,const EffectLightDefinition&) {
    const auto key=std::pair{authored.source_shader_datum,authored.selected_shader_lod};
    if(passes_.contains(key))return;
    auto& out=passes_[key];if(authored.lods.empty())return;
    const auto& lod=authored.lods.at(authored.selected_shader_lod);
    if(!(lod.available_layers&(1u<<13)))return;
    const auto range=authored.layers.at(lod.layers.first()+13);
    EffectLightUniforms uniforms{};
    for(unsigned i=0;i<authored.passes.size();++i) {
        const auto& pass=authored.passes[i];
        if(pass.template_pass<range.first() || pass.template_pass-range.first()>=range.count() || pass.implementation!=0)continue;
        if(!pass.vertex_shader.ends_with("\\stencil_light_specular_tangentspace_sphere"))
            throw FormatError("pStencil specular pass requires its authored sphere program");
        auto material=prepare(cache_,authored,i,uniforms);
        if(!material)throw FormatError("pStencil specular material has unresolved authored bindings: "+authored.name);
        if(material->forward_blend!=8)throw FormatError("pStencil specular pass changed its destination-alpha blend");
        const auto resident=std::find_if(mesh_.materials.begin(),mesh_.materials.end(),[&](const auto& prior) {
            return prior.source_shader_datum==authored.source_shader_datum &&
                prior.selected_shader_lod==authored.selected_shader_lod &&
                prior.forward_pass && prior.forward_pass_index==i &&
                prior.forward_program.words==material->forward_program.words &&
                prior.vertex_program==material->vertex_program;
        });
        const auto slot=unsigned(resident-mesh_.materials.begin());
        if(resident==mesh_.materials.end())mesh_.materials.push_back(std::move(*material));
        out.push_back({slot,i,authored});
    }
}
std::shared_ptr<const std::vector<EffectLightDraw>> MaterialBank::complete(
    std::uint32_t shader,unsigned lod,std::uint32_t,const EffectLightUniforms& uniforms) const {
    const auto at=passes_.find({shader,lod});if(at==passes_.end() || at->second.empty())return {};
    auto out=std::make_shared<std::vector<EffectLightDraw>>();
    for(const auto& pass:at->second) {
        auto material=prepare(cache_,pass.authored,pass.index,uniforms);
        if(!material || !material->vertex_bank)throw FormatError("pStencil specular bank failed to complete");
        out->push_back({pass.slot,{},material->forward_program,material->program_constants,
            pack_vertex_program_bank(*material->vertex_bank)});
    }
    return out;
}
}
