#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/pstencil/environment.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
namespace halo2::pstencil {
namespace {
constexpr std::string_view visor_name="objects\\characters\\masterchief\\shaders\\masterchief_visor";
constexpr std::array<std::uint32_t,60> controls[]{
#include "visor_combined_controls.inc"
};
}
bool supports_combined_visor_environment(const Material& m) {
    if(m.name!=visor_name || !m.forward_pass || m.forward_vertex!=6 || m.forward_blend!=0 ||
       m.forward_pass_index>=m.passes.size())return false;
    const auto& p=m.passes[m.forward_pass_index];
    if(p.implementation!=2)return false;
    const bool plain=p.name.ends_with("\\lightmap_env") && p.vertex_shader.ends_with("\\lightmap_dynamic_env");
    const bool textured=p.name.ends_with("\\lightmap_tex_env") && p.vertex_shader.ends_with("\\lightmap_dynamic_tex_env");
    if(!plain && !textured)return false;
    const auto& expected=controls[textured?1:0];
    for(unsigned n=0;n<expected.size();++n) {
        if((n>=10 && n<=25) || n==43 || n==44)continue;
        if(m.forward_program.words[n]!=expected[n])return false;
    }
    return true;
}
const Material* combined_visor_environment(const Material& m) {
    if(supports_combined_visor_environment(m))return &m;
    if(m.environment_pass)if(auto* p=combined_visor_environment(*m.environment_pass))return p;
    for(const auto& child:m.world_overlays_before)if(auto* p=combined_visor_environment(*child))return p;
    for(const auto& child:m.world_overlays_after)if(auto* p=combined_visor_environment(*child))return p;
    return nullptr;
}
void suppress_combined_visor_environment(PixelProgram& program) {
    // The current consumer's isolated facing/glancing strengths. Tint remains
    // untouched because its current combined role also affects diffuse.
    program.words[17]&=0x00ffffffu;program.words[25]&=0x00ffffffu;
}
Material separate_visor_environment(const Material& visor,const Material& independent) {
    const auto* current=combined_visor_environment(visor);
    const auto* reference=reference_environment_parameters(visor.name);
    if(!current || !reference || !supports_environment_response(independent) ||
       visor.world_vertex_input_transform!=independent.world_vertex_input_transform)
        throw FormatError("Separate visor environment lacks its actual supported interfaces");
    if(current->bump.layers!=1 || !current->bump.width || current->detail.layers!=6 ||
       current->auxiliary.layers!=6 || !current->animation.overlays.empty())
        throw FormatError("Separate visor environment requires its static actual bump and cube bindings");
    // The existing independent VP and the two current visor VPs both apply
    // C18 to decompressed V1, then C20 to the bump coordinates. Keep those
    // complete affine rows, rather than assigning a texture scale preset.
    auto result=independent;
    result.name=visor.name;result.source_shader_datum=visor.source_shader_datum;
    result.selected_shader_lod=visor.selected_shader_lod;
    result.world_vertex_input_transform=visor.world_vertex_input_transform;
    result.program_constants[1][0]=current->program_constants[1][0];
    result.program_constants[1][2]=current->program_constants[1][2];
    result.forward_transforms[0]=current->program_constants[1][0];
    result.forward_transforms[2]=current->program_constants[1][2];
    result.program_constants[0][10]=reference->facing;
    result.program_constants[0][11]=reference->glancing;
    result.forward_program.words[12]=(result.forward_program.words[12]&0x00ffffffu)|reference->midrange_factor_argb;
    result.forward_program.words[13]=(result.forward_program.words[13]&0xff000000u)|reference->midrange_color_argb;
    result.diffuse=current->bump;result.auxiliary=current->detail;
    result.bump=current->auxiliary;
    result.animated_textures[0]=current->animated_textures[2];
    result.animated_textures[1].clear();
    result.animated_textures[2]=current->animated_textures[3];
    result.animated_textures[3]=current->animated_textures[1];
    result.texture_frames={current->texture_frames[2],0,current->texture_frames[3],current->texture_frames[1]};
    result.texture_states[0]=current->texture_states[2];
    result.texture_states[3]=current->texture_states[1];
    result.texture_states[2]=current->texture_states[3];
    result.forward_blend=3; // Own masked additive companion; authored program remains intact.
    result.accumulation_texture=false;
    result.environment_pass.reset();result.planar_fog_pass.reset();
    result.world_overlays_before.clear();result.world_overlays_after.clear();
    result.world_bloom_passes.clear();result.world_planar_passes={};
    result.animation={};
    const auto pass=result.passes.at(result.forward_pass_index);
    result.passes={pass};result.forward_pass_index=0;
    // This is an own companion, not another authored current-template pass.
    // The renderer retains the carrier's actual layer8 policy separately.
    result.lods.clear();result.layers.clear();result.pass_implementations.clear();
    return result;
}
void trace_visor_environment(std::uint64_t frame,unsigned lod,unsigned vertices) {
    const auto* path=std::getenv("HALO2_PSTENCIL_VISOR_TRACE");
    if(!path || !*path)return;
    static std::ofstream output(path);
    if(!output)throw FormatError("Cannot retain actual pStencil visor submissions");
    output<<frame<<'\t'<<lod<<'\t'<<vertices<<'\n';
}
}
