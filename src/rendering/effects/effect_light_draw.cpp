#include "halo2/effect_light_draw.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/source_render_policy.hpp"
#include <algorithm>
#include <cstring>

namespace halo2 {
namespace {
bool effect_light_texture_accumulation(const Material &material) {
    return material.forward_raster.depth_write && material.forward_raster.color_write==0x01010101u &&
        source_material_layer(material)==14;
}
void validate_effect_light_inputs(unsigned slot,std::span<const Vertex> vertices,
    const std::array<std::uint32_t,4> &vertex_range,const Material &resident) {
    const auto &r=resident.forward_raster;
    // Source layer 11 implementation 2 for a projected light: an unblended
    // alpha-only write of the gel term, on the same commands and constants.
    const bool alpha_only=r.color_write==0x01000000u;
    // Source layer 10 for a shadowed projected light: a caster drawn into the
    // light's own depth target, with no color write.
    const bool depth_only=r.color_write==0;
    const bool texture_accumulation=effect_light_texture_accumulation(resident);
    const bool writes_depth=depth_only || texture_accumulation;
    if(!resident.forward_pass || resident.forward_vertex!=6 ||
       !resident.vertex_bank || resident.vertex_bank->prt_inputs || resident.vertex_bank->camera_inputs ||
       !r.depth_enable || r.depth_write!=writes_depth || r.depth_function!=(writes_depth?0x203:0x202) ||
       (r.color_write!=0x01010101 && !alpha_only && !depth_only) ||
       vertex_range!=std::array<std::uint32_t,4>{34,6,0,0})
        throw FormatError("Unresolved original sphere diffuse draw binding");
    if(vertices.size()%3)throw FormatError("Incomplete original sphere diffuse triangles");
    for(const auto &v:vertices)if(v.material!=slot)
        throw FormatError("Original sphere diffuse material identity changed");
    // Compiled world-coordinate sphere variant. Skin/palette streams remain
    // separate explicit owners; do not admit those with pretransformed inputs.
    if(resident.forward_pass_index>=resident.passes.size())
        throw FormatError("Unresolved original sphere diffuse vertex stream");
    const auto &program=resident.passes[resident.forward_pass_index].vertex_shader;
    const auto implementation=resident.passes[resident.forward_pass_index].implementation;
    if(texture_accumulation) {
        if(implementation!=0 || resident.forward_blend || resident.accumulation_texture || !vertices.empty())
            throw FormatError("Unresolved original projected light texture accumulation pass");
        return;
    }
    const bool sphere=program.ends_with("\\lightaccum_sphere_diffuse_tex") ||
        program.ends_with("\\lightaccum_sphere_diffuse");
    const bool frustum=program.ends_with("\\lightaccum_frustum") ||
        program.ends_with("\\lightaccum_frustum_tex") ||
        program.ends_with("\\lightaccum_frustum_no_specular") ||
        program.ends_with("\\lightaccum_frustum_no_specular_tex") ||
        program.ends_with("\\lightaccum_frustum_no_specular_gel") ||
        program.ends_with("\\lightaccum_frustum_no_specular_gel_tex");
    if(depth_only) {
        if(!program.ends_with("\\shadow_buffer_generation") || (implementation!=1 && implementation!=2) || resident.forward_blend)
            throw FormatError("Unresolved original projected light shadow generation pass");
        return;
    }
    if(alpha_only) {
        const bool one_tap=program.ends_with("\\shadow_buffer_application_1tap");
        const bool three_tap=program.ends_with("\\shadow_buffer_application_3tap");
        if(!((one_tap && (implementation==2 || implementation==0)) || (three_tap && (implementation==1 || implementation==6))) ||
           resident.forward_blend)
            throw FormatError("Unresolved original projected light alpha pass");
        return;
    }
    if((!sphere && !frustum) || (sphere && (implementation!=1 || resident.forward_blend!=10)) ||
       (frustum && resident.forward_blend!=3 && resident.forward_blend!=8 && resident.forward_blend!=10) ||
       (frustum && (implementation>4 || implementation==1)))
        throw FormatError("Unresolved original sphere diffuse vertex stream");
}
const VertexProgramBank &effect_light_bank(const Material &material) {
    if(!material.vertex_bank)throw FormatError("Missing original sphere constant bank");
    return *material.vertex_bank;
}
}
void validate_effect_light_draw(const EffectLightDraw &draw,const Material &resident) {
    validate_effect_light_inputs(draw.material_slot,draw.vertices,draw.vertex_bank.range,resident);
}
EffectLightDraw source_effect_light_draw(unsigned slot,const Material &material,
    std::span<const Vertex> vertices){
    EffectLightDraw out{slot,vertices,material.forward_program,material.program_constants,
        pack_vertex_program_bank(effect_light_bank(material))};
    validate_effect_light_draw(out,material);return out;
}
MenuWorldSourceConstants source_effect_light_raw_constants(
    const MenuWorldSourceConstants &previous,const Material &material) {
    const auto &bank=effect_light_bank(material);
    validate_effect_light_inputs(0,{},vertex_program_bank_range(bank),material);
    auto result=previous;
    std::copy(material.program_constants[1].begin(),material.program_constants[1].end(),result.begin()+18);
    std::copy(bank.constants.begin(),bank.constants.end(),result.begin()+34);
    return result;
}
EffectLightRawDraw source_effect_light_raw_draw(unsigned slot,const Material &material,
    const MenuWorldSourcePass &pass,MenuWorldSourceBinding source,MenuWorldRasterPacket raster,
    std::array<float,4> clip_range,std::array<float,2> fog_parameters) {
    const auto &bank=effect_light_bank(material);
    validate_effect_light_inputs(slot,{},vertex_program_bank_range(bank),material);
    const bool texture_layer=effect_light_texture_accumulation(material);
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
        const auto& reads=menu_world_vertex_metadata_lookup(source).consumed_input_components;
        for(const auto reg:source.external_attribute_registers()) {
            // Layer 14's stencil_texture copies the entire v10 into oD0,
            // including when its texture combiner supplies colors from pixel
            // externs and leaves v10 in the completed material's initial bank.
            if(texture_layer && pass.choice().layer==14 && reg==10 &&
                compiled.vertex_shader.ends_with("\\stencil_texture") &&
                std::memcmp(&compiled.constants[0][reg],&material.program_constants[0][reg],sizeof(ShaderVector))==0)
                attribute_components[reg]|=15u;
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
    // Layer 11 is admitted only as the validated alpha-only light pass.
    const bool alpha_layer=pass.choice().layer==11 && material.forward_raster.color_write==0x01000000u;
    const bool generate_layer=pass.choice().layer==10 && material.forward_raster.color_write==0;
    if((texture_layer?pass.choice().layer!=14:(pass.choice().layer!=12 && !alpha_layer && !generate_layer)) ||
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
    const auto &constants=source.constants();
    const auto &program_constants=material.program_constants[1];
    const auto &program_bank=bank.constants;
    if(std::memcmp(constants.data()+18,program_constants.data(),sizeof(program_constants)) ||
       std::memcmp(constants.data()+34,program_bank.data(),program_bank.size()*sizeof(ShaderVector)))
        throw FormatError("Raw sphere diffuse source constants differ from completed material/light bank");
    std::array<ShaderVector,6> globals;
    std::copy(program_bank.begin(),program_bank.end(),globals.begin());
    EffectLightRawDraw result{slot,std::move(source),material.forward_program,material.program_constants,globals,
        std::move(raster),clip_range,fog_parameters};
    result.vertex_attribute_components=attribute_components;
    result.texture_accumulation_write=texture_layer;
    return result;
}
}
