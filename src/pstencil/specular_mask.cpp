#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include <cstdlib>
#include <map>
#include <tuple>
namespace halo2::pstencil {
PixelProgram texture_masked_specular(const PixelProgram& source) {
    const auto& w=source.words;
    if((w[53]&255u)!=7 || w[54]!=0x18460u || w[8]!=0x0c0d0000u ||
       w[9]!=0x80u || w[7] || w[33] || w[41] || w[52])
        throw FormatError("pStencil specular program differs from the supported authored highlight program");
    return source;
}
bool texture_masked_specular_material(const Material& m) {
    return m.forward_pass && m.forward_vertex==6 && m.forward_blend==3 && m.accumulation_texture &&
        m.forward_pass_index<m.passes.size() &&
        m.passes[m.forward_pass_index].vertex_shader.ends_with("\\stencil_light_specular_tangentspace_sphere");
}
bool texture_mask_pass(const Material& m) {
    if(texture_accumulation_material_2003(m))return true;
    return m.forward_pass && m.forward_vertex==3 && m.forward_blend==0 &&
        m.forward_pass_index<m.passes.size() &&
        m.passes[m.forward_pass_index].implementation==0 &&
        m.passes[m.forward_pass_index].vertex_shader.ends_with("\\stencil_texture");
}
std::vector<unsigned> texture_mask_materials(std::span<const Material> materials) {
    using Key=std::pair<std::uint32_t,SourceWorldVertexInputTransform>;
    std::map<Key,unsigned> masks;
    std::map<std::tuple<std::uint32_t,SourceWorldVertexInputTransform,unsigned>,unsigned> combined;
    for(unsigned n=0;n<materials.size();++n) {
        const auto& m=materials[n];if(!texture_mask_pass(m))continue;
        if(m.forward_vertex==6) {
            combined.try_emplace({m.source_shader_datum,m.world_vertex_input_transform,m.selected_shader_lod},n);
            continue;
        }
        const Key key{m.source_shader_datum,m.world_vertex_input_transform};
        auto [at,added]=masks.emplace(key,n);
        if(!added) {
            const auto& prior=materials[at->second];
            const bool converted=texture_accumulation_material_2003(m),was_converted=texture_accumulation_material_2003(prior);
            if((converted && !was_converted) || (converted==was_converted && m.selected_shader_lod<prior.selected_shader_lod))at->second=n;
        }
    }
    std::vector<unsigned> result(materials.size(),~0u);
    static const bool emblem_control=[]{const auto* v=std::getenv("HALO2_REFERENCE_PSTENCIL_EMBLEM_MASK");return v && *v;}();
    for(unsigned n=0;n<materials.size();++n) {
        const auto& m=materials[n];
        // The armor emblem is an alpha-blended overlay coplanar with the
        // armor beneath it, not an opaque base. It keeps its shipped overlay
        // pass and never becomes an opaque texture-target writer.
        if(!emblem_control && m.template_name.ends_with("\\emblem_overlay_simple"))continue;
        const auto at=masks.find({m.source_shader_datum,m.world_vertex_input_transform});
        if(at==masks.end()) {
            const auto exact=combined.find({m.source_shader_datum,m.world_vertex_input_transform,m.selected_shader_lod});
            if(exact!=combined.end())result[n]=exact->second;
        } else if(texture_mask_pass(m) && !texture_accumulation_material_2003(materials[at->second]))
            result[n]=n;
        else result[n]=at->second;
    }
    return result;
}
}
