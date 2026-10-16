#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/texture_program.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace halo2 {
namespace {
void material(const SourceShadowMaterial &authored,SourceShadowPhase phase) {
    const bool receiver=phase==SourceShadowPhase::Apply;
    const auto &m=authored.material;
    if(!m.forward_pass || m.forward_pass_index>=m.passes.size() || m.forward_vertex!=10 ||
       m.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve)
        throw FormatError("raw shadow lacks its authored oD0 pass");
    const auto &p=m.passes[m.forward_pass_index];
    if(p.implementation!=(receiver?4u:0u) ||
       !p.vertex_shader.ends_with(receiver?"\\shadow_buffer_application_lightmap":"\\shadow_buffer_generation"))
        throw FormatError("raw shadow uses an unsupported fixed target10 implementation");
    if(!texture_program_supported(m.forward_program))
        throw FormatError("raw shadow pixel program is unsupported");
    const auto &r=m.forward_raster;
    if(receiver) {
        if(!r.depth_enable || r.depth_function!=0x202 || r.depth_write ||
           r.color_write!=0x10101 || m.forward_blend!=9 ||
           authored.external_targets!=std::array<unsigned,4>{10,0,0,0})
            throw FormatError("raw shadow receiver raster/target differs from implementation4");
        const auto texture=std::find_if(p.textures.begin(),p.textures.end(),
            [](const auto &t){return t.stage==0 && t.external==8;});
        if(texture==p.textures.end())
            throw FormatError("raw shadow receiver lacks authored external8 texture binding");
    } else if(r.depth_enable || m.forward_blend || r.color_write!=0x1010101 ||
              std::any_of(authored.external_targets.begin(),authored.external_targets.end(),
                  [](unsigned target){return target!=0;})) {
        throw FormatError("raw shadow caster raster/targets differ from implementation0");
    }
}
void command(const SourceShadowRawDraw &draw,SourceShadowPhase phase) {
    const auto &proof=draw.command;
    const auto &c=proof.command;
    const auto &u=proof.unit;
    const auto &d=proof.descriptor;
    const auto &pass=draw.pass;
    const auto &source=draw.source;
    const bool receiver=phase==SourceShadowPhase::Apply;
    const auto layer=receiver?11u:10u,category=receiver?1u:2u;
    const auto bank=receiver?0x4ce280u:0x4ce298u;
    const auto implementation=receiver?4u:0u;
    // Actual4D630 changes W-min/c0..3 through12D30 for flag800. A fade
    // byte other than255 takes1C270/402DA0's separate hardware transition.
    // Neither per-command caller bank is owned by this bounded raw route.
    if((u[0]&0x800u) || ((u[6]>>16)&255u)!=255u)
        throw FormatError("raw shadow command requires unowned W-min or fade state");
    // Authenticated TU1.5 callbacks are4CBE0/4D0A0. The older canonical
    //4CBF0/4D0B0 callers are not interchangeable with these source records.
    if(proof.phase!=phase || proof.root_owner==0xffffffffu || proof.bank!=bank ||
       proof.category!=category || proof.layer!=layer || proof.sorted_ordinal>=2560 ||
       (d[0]&65535u)>=(receiver?1024u:512u) ||
       c[0]!=0x4cbe0 || c[1]!=0x4d0a0 || !(c[4]&(1u<<layer)) ||
       (d[0]>>16)!=(c[2]&65535u) || (d[1]&255u)!=implementation ||
       ((d[1]>>8)&255u)!=((u[3]>>29)<=1?1u:2u) ||
       (c[3]>>8&65535u)!=proof.unit_ordinal || d[5]!=u[4] ||
       d[4]!=pass.implementation_address() ||
       pass.choice()!=MenuWorldSourcePassChoice{(c[5]>>8)&255u,layer,c[5]&255u,implementation} ||
       pass.shader()!=c[2] || source.vertex_shader()!=pass.vertex_shader() ||
       source.declaration().class_index!=(d[2]&65535u) ||
       source.owner()!=proof.geometry_owner || !source.external_attribute_registers().empty())
        throw FormatError("raw shadow descriptor/command/pass/owner join changed");
    // SourceShadowUniforms intentionally owns only the six consumed3A540
    // vectors. The full upload's c37 is nonzero in authenticated TU1.5;
    // retaining a previous bank is not evidence for a new program reading
    // c37..39. The admitted fixed generation/application programs read none.
    for(unsigned index:source.consumed_constants())if(index>=37 && index<=39)
        throw FormatError("raw shadow program requires an unowned c37..39 projection tail");
    if((u[3]>>29)!=0 && d[3]!=((u[3]>>9&511u)|((u[1]&65535u)<<11)))
        throw FormatError("raw shadow geometry sort identity changed");
    if(!source.draw().geometry || source.draw().part!=(c[3]&255u) ||
       source.draw().part>=source.draw().geometry->parts().size())
        throw FormatError("raw shadow command lost its original part");
    const auto &geometry=*source.draw().geometry;
    // Shader override admission must carry its actual source override owner;
    // this bounded seam accepts unchanged part shaders only.
    if(geometry.parts()[source.draw().part].shader!=c[2])
        throw FormatError("raw shadow part shader override has no admitted owner");
    if(u[1]!=0xffffffffu) {
        if(geometry.kind()!=MenuWorldSourceSectionKind::render_model ||
           geometry.owner_datum()!=u[1] || geometry.section()!=(u[3]>>9&511u) ||
           source.owner().kind!=MenuWorldSourceOwnerKind::dynamic_object ||
           !source.owner().generation || *source.owner().generation==0xffffffffu ||
           source.owner().ordinal!=(*source.owner().generation&65535u) ||
           source.draw().object_rows.empty())
            throw FormatError("raw shadow model lost its current salted owner/palette");
    } else if(source.owner().kind==MenuWorldSourceOwnerKind::bsp_cluster) {
        if(geometry.kind()!=MenuWorldSourceSectionKind::bsp_cluster ||
           geometry.section()!=(u[3]>>9&511u) || !source.draw().object_rows.empty() ||
           source.declaration().class_index!=0)
            throw FormatError("raw shadow cluster acquired a different section/palette");
    } else if(source.owner().kind!=MenuWorldSourceOwnerKind::bsp_instance ||
              geometry.kind()!=MenuWorldSourceSectionKind::bsp_instance_definition ||
              source.owner().ordinal!=(u[3]>>18&2047u) ||
              source.declaration().class_index!=1 || source.draw().object_rows.size()!=1) {
        throw FormatError("raw shadow instance lost its original instance matrix");
    }
    if(!source.draw().index_range().count || !source.draw().triangle_count())
        throw FormatError("raw shadow draw has no submitted source triangles");
}
}
void validate_source_shadow_raw_material(const SourceShadowMaterial &authored,SourceShadowPhase phase) {
    if(phase!=SourceShadowPhase::Generate && phase!=SourceShadowPhase::Apply)
        throw FormatError("raw shadow material phase is outside the original two-phase path");
    material(authored,phase);
}
MenuWorldSourceConstants source_shadow_raw_constants(const MenuWorldSourceConstants &previous,
    const SourceShadowMaterial &authored,const SourceShadowUniforms &uniforms) {
    if(uniforms.texture_width!=128 || uniforms.texture_height!=128)
        throw FormatError("raw shadow constants differ from fixed target10 dimensions");
    auto result=previous;
    std::copy(authored.material.program_constants[1].begin(),
        authored.material.program_constants[1].end(),result.begin()+18);
    std::copy(uniforms.projection.begin(),uniforms.projection.end(),result.begin()+31);
    std::copy(uniforms.constants.begin(),uniforms.constants.end(),result.begin()+40);
    return result;
}
void validate_source_shadow_raw_draw(const SourceShadowRawDraw &draw,SourceShadowPhase phase,
    const SourceShadowUniforms &uniforms) {
    if(phase!=SourceShadowPhase::Generate && phase!=SourceShadowPhase::Apply)
        throw FormatError("raw shadow draw phase is outside the original two-phase path");
    if(!draw.authored)throw FormatError("raw shadow has no retained authored material");
    const auto &surface_clip=draw.controls.surface_clip;
    if(!surface_clip[2] || !surface_clip[3] ||
       surface_clip[0]>std::uint32_t(std::numeric_limits<std::int32_t>::max()) ||
       surface_clip[1]>std::uint32_t(std::numeric_limits<std::int32_t>::max()))
        throw FormatError("raw shadow lacks a representable actual SURFACE_CLIP rectangle");
    for(float size:draw.surface_size)if(!std::isfinite(size) || size<=0)
        throw FormatError("raw shadow lacks actual positive bound surface dimensions");
    validate_source_shadow_raw_material(*draw.authored,phase);
    command(draw,phase);
    const auto &m=draw.authored->material;
    const auto &p=m.passes[m.forward_pass_index];
    if(draw.pass.shader()!=m.source_shader_datum ||
       draw.pass.choice().shader_lod!=m.selected_shader_lod ||
       p.template_pass!=draw.pass.template_pass() ||
       p.implementation!=draw.pass.choice().implementation ||
       draw.program.words!=m.forward_program.words ||
       draw.program.fragment_offsets!=m.forward_program.fragment_offsets ||
       (draw.controls.cull!=0 && draw.controls.cull!=0x900 && draw.controls.cull!=0x901) ||
       !draw.controls.depth_clamp || draw.raster.source_stencil_mode!=0 || draw.raster.stencil.enabled)
        throw FormatError("raw shadow authored material/raster binding changed");
    auto constants=m.program_constants;
    std::copy_n(uniforms.projection.begin(),3,constants[1].begin()+13);
    if(std::memcmp(constants.data(),draw.constants.data(),sizeof(constants)))
        throw FormatError("raw shadow completed material constants changed");
    const auto completed=source_shadow_raw_constants(draw.source.constants(),*draw.authored,uniforms);
    if(std::memcmp(completed.data()+18,draw.source.constants().data()+18,19*sizeof(ShaderVector)) ||
       std::memcmp(completed.data()+40,draw.source.constants().data()+40,9*sizeof(ShaderVector)))
        throw FormatError("raw shadow source constants differ from this root's material/projection bank");
}
SourceShadowRawDraw source_shadow_raw_draw(unsigned slot,const SourceShadowMaterial &authored,
    MenuWorldSourcePass pass,MenuWorldSourceBinding source,SourceShadowRawCommandTu15 command,
    const SourceShadowUniforms &uniforms,SourceShadowRawRasterInput controls,MenuWorldRasterPacket raster,
    std::array<float,4> clip_range,std::array<float,2> fog_parameters,std::array<float,2> surface_size) {
    auto constants=authored.material.program_constants;
    std::copy_n(uniforms.projection.begin(),3,constants[1].begin()+13);
    SourceShadowRawDraw result{slot,&authored,std::move(pass),std::move(source),std::move(command),
        authored.material.forward_program,constants,controls,std::move(raster),clip_range,fog_parameters,surface_size};
    validate_source_shadow_raw_draw(result,result.command.phase,uniforms);
    return result;
}
} // namespace halo2
