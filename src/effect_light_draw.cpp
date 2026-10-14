#include "halo2/effect_light_draw.hpp"
#include <algorithm>
#include <cstring>

namespace halo2 {
void validate_effect_light_draw(const EffectLightDraw &draw,const Material &resident){
    const auto &r=resident.forward_raster;
    if(!resident.forward_pass || resident.forward_vertex!=6 || resident.forward_blend!=10 ||
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
    if(!program.ends_with("\\lightaccum_sphere_diffuse_tex") &&
       !program.ends_with("\\lightaccum_sphere_diffuse"))
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
    if(pass.choice().layer!=12 || pass.choice().implementation!=1 ||
       pass.shader()!=material.source_shader_datum || pass.choice().shader_lod!=material.selected_shader_lod ||
       pass.vertex_shader()!=source.vertex_shader() || source.draw().geometry->owner_datum()==0xffffffffu ||
       !source.external_attribute_registers().empty() || source.declaration().class_index>6 ||
       source.draw().geometry->parts().at(source.draw().part).shader!=pass.shader() ||
       material.passes.at(material.forward_pass_index).template_pass!=pass.template_pass() ||
       material.passes.at(material.forward_pass_index).implementation!=pass.choice().implementation ||
       raster.source_stencil_mode!=0 || raster.stencil.enabled)
        throw FormatError("Unresolved original raw sphere diffuse pass/owner binding");
    const auto completed=source_effect_light_raw_constants(source.constants(),material);
    if(std::memcmp(completed.data()+18,source.constants().data()+18,22*sizeof(ShaderVector)))
        throw FormatError("Raw sphere diffuse source constants differ from completed material/light bank");
    return {slot,std::move(source),legacy.program,legacy.constants,legacy.vertex_bank,
        std::move(raster),clip_range,fog_parameters};
}
}
