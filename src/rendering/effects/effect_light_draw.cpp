#include "halo2/effect_light_draw.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include <algorithm>
#include <cstring>

namespace halo2 {
void validate_effect_light_draw(const EffectLightDraw &draw,const Material &resident){
    const auto &r=resident.forward_raster;
    if(!resident.forward_pass || resident.forward_vertex!=6 ||
       !resident.vertex_bank || resident.vertex_bank->prt_inputs || resident.vertex_bank->camera_inputs ||
       r.depth_function!=0x202 || !r.depth_enable || r.depth_write || r.color_write!=0x01010101 ||
       draw.vertex_bank.range!=std::array<std::uint32_t,4>{34,6,0,0})
        throw FormatError("Unresolved original sphere diffuse draw binding");
    if(draw.vertices.size()%3)throw FormatError("Incomplete original sphere diffuse triangles");
    for(const auto &v:draw.vertices)if(v.material!=draw.material_slot)
        throw FormatError("Original sphere diffuse material identity changed");
    // Compiled world-coordinate sphere variant. Skin/palette streams remain
    // separate explicit owners; do not admit those with pretransformed inputs.
    if(resident.forward_pass_index>=resident.passes.size())
        throw FormatError("Unresolved original sphere diffuse vertex stream");
    const auto &program=resident.passes[resident.forward_pass_index].vertex_shader;
    const bool sphere=program.ends_with("\\lightaccum_sphere_diffuse_tex") ||
        program.ends_with("\\lightaccum_sphere_diffuse");
    const auto projected=[&](const char* leaf) {
        return program.ends_with(std::string("\\")+leaf) ||
            program.ends_with(std::string("\\")+leaf+"_tex");
    };
    const bool frustum=projected("lightaccum_frustum") ||
        projected("lightaccum_frustum_no_specular") ||
        projected("lightaccum_frustum_no_specular_gel");
    const auto implementation=resident.passes[resident.forward_pass_index].implementation;
    if((!sphere && !frustum) || (sphere && (implementation!=1 || resident.forward_blend!=10)) ||
       (frustum && resident.forward_blend!=3 && resident.forward_blend!=8 && resident.forward_blend!=10) ||
       (frustum && (implementation>4 || implementation==1)))
        throw FormatError("Unresolved original sphere diffuse vertex stream");
}
EffectLightDraw source_effect_light_draw(unsigned slot,const Material &material,
    std::span<const Vertex> vertices){
    if(!material.vertex_bank)throw FormatError("Missing original sphere constant bank");
    EffectLightDraw out{slot,vertices,material.forward_program,material.program_constants,
        pack_vertex_program_bank(*material.vertex_bank)};
    validate_effect_light_draw(out,material);return out;
}
MenuWorldSourceConstants source_effect_light_raw_constants(
    const MenuWorldSourceConstants &previous,const Material &material) {
    (void)source_effect_light_draw(0,material,{});
    auto result=previous;
    std::copy(material.program_constants[1].begin(),material.program_constants[1].end(),result.begin()+18);
    std::copy(material.vertex_bank->constants.begin(),material.vertex_bank->constants.end(),result.begin()+34);
    return result;
}
EffectLightRawDraw source_effect_light_raw_draw(unsigned slot,const Material &material,
    const MenuWorldSourcePass &pass,MenuWorldSourceBinding source,MenuWorldRasterPacket raster,
    std::array<float,4> clip_range,std::array<float,2> fog_parameters) {
    const auto legacy=source_effect_light_draw(slot,material,{});
    std::array<std::uint8_t,16> attribute_components{};
    if(!source.external_attribute_registers().empty()) {
        const auto& compiled=material.passes.at(material.forward_pass_index);
        constexpr std::array<unsigned,21> masks{1,2,4,8,7,1,2,4,8,3,12,3,12,15,11,11,7,7,15,15,15};
        for(const auto&r:compiled.vertex_remaps)if(!(r.destination&16u)) {
            const auto component=(r.destination>>5)&31u;
            if(component>=masks.size())throw FormatError("Raw effect light has an invalid material attribute remap");
            attribute_components[r.destination&15u]|=masks[component];
        }
        // bind_pass resolves the admitted change-color RGB extern from its
        // actual supplied object context; authored alpha remains separate.
        for(const auto&r:compiled.vertex_externs)if(!(r.destination&16u) && r.source==3 &&
            ((r.destination>>5)&31u)==4)
            attribute_components[r.destination&15u]|=7u;
        const auto& reads=menu_world_vertex_metadata_lookup(source.program_bytes(),source.declaration()).consumed_input_components;
        for(const auto reg:source.external_attribute_registers()) {
            // RGB comes from the selected authored remaps. Alpha is already
            // present in the complete owned per-pass bank, including lanes a
            // RGB remap preserves. Carry its actual value; do not infer alpha
            // from RGB, transparency, or the selected light.
            if((reg==10 || reg==11) && (attribute_components[reg]&7u)==7u &&
                std::memcmp(&compiled.constants[0][reg][3],&material.program_constants[0][reg][3],sizeof(float))==0)
                attribute_components[reg]|=8u;
            if((reg!=10 && reg!=11) || (attribute_components[reg]&reads[reg])!=reads[reg])
                throw FormatError("Raw effect light has unbound material attribute components: v"+std::to_string(reg)+
                    " shader="+material.name);
        }
    }
    if(pass.choice().layer!=12 ||
       pass.shader()!=material.source_shader_datum || pass.choice().shader_lod!=material.selected_shader_lod ||
       pass.vertex_shader()!=source.vertex_shader() || source.draw().geometry->owner_datum()==0xffffffffu ||
       source.declaration().class_index>6 ||
       source.draw().geometry->parts().at(source.draw().part).shader!=pass.shader() ||
       material.passes.at(material.forward_pass_index).template_pass!=pass.template_pass() ||
       material.passes.at(material.forward_pass_index).implementation!=pass.choice().implementation ||
       raster.source_stencil_mode!=0 || raster.stencil.enabled)
        throw FormatError("Unresolved raw effect-light pass/owner binding: shader="+material.name+
            " material="+std::to_string(material.source_shader_datum)+" pass="+std::to_string(pass.shader())+
            " part_shader="+std::to_string(source.draw().geometry->parts().at(source.draw().part).shader)+
            " lod="+std::to_string(material.selected_shader_lod)+"/"+std::to_string(pass.choice().shader_lod)+
            " vertex="+std::to_string(pass.vertex_shader())+"/"+std::to_string(source.vertex_shader())+
            " template="+std::to_string(material.passes.at(material.forward_pass_index).template_pass)+"/"+std::to_string(pass.template_pass())+
            " implementation="+std::to_string(material.passes.at(material.forward_pass_index).implementation)+"/"+std::to_string(pass.choice().implementation)+
            " stencil="+std::to_string(raster.source_stencil_mode)+"/"+std::to_string(raster.stencil.enabled));
    const auto completed=source_effect_light_raw_constants(source.constants(),material);
    if(std::memcmp(completed.data()+18,source.constants().data()+18,22*sizeof(ShaderVector)))
        throw FormatError("Raw sphere diffuse source constants differ from completed material/light bank");
    EffectLightRawDraw result{slot,std::move(source),legacy.program,legacy.constants,legacy.vertex_bank,
        std::move(raster),clip_range,fog_parameters};
    result.vertex_attribute_components=attribute_components;
    return result;
}
}
