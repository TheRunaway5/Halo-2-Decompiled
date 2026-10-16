#include "halo2/pstencil/specular_mask.hpp"
namespace halo2::pstencil {
PixelProgram texture_masked_specular(const PixelProgram& source) {
    const auto& w=source.words;
    if((w[53]&255u)!=7 || w[54]!=0x18460u || w[8]!=0x0c0d0000u ||
       w[9]!=0x80u || w[7] || w[33] || w[41] || w[52])
        throw FormatError("pStencil specular lacks the proved unused texture/combiner mask interface");
    return source;
}
bool texture_masked_specular_material(const Material& m) {
    return m.forward_pass && m.forward_vertex==6 && m.forward_blend==3 && m.accumulation_texture &&
        m.forward_pass_index<m.passes.size() &&
        m.passes[m.forward_pass_index].vertex_shader.ends_with("\\stencil_light_specular_tangentspace_sphere");
}
}
