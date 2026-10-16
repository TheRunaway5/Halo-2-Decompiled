#include "halo2/pstencil/detail.hpp"
#include <array>
#include <algorithm>

namespace halo2::pstencil {
std::optional<unsigned> reference_neutral_detail_stage(const Material& m) {
    constexpr std::array<std::string_view,5> names{
        "objects\\characters\\elite\\shaders\\head",
        "objects\\characters\\elite\\shaders\\helmet",
        "objects\\characters\\elite\\shaders\\arms",
        "objects\\characters\\elite\\shaders\\torso",
        "objects\\characters\\elite\\shaders\\legs"};
    if(std::find(names.begin(),names.end(),m.name)==names.end() ||
       !m.forward_pass || m.forward_vertex!=3 || m.forward_blend!=0 ||
       m.forward_pass_index>=m.passes.size())return {};
    const auto& p=m.passes[m.forward_pass_index];
    if(p.implementation!=0 || !p.vertex_shader.ends_with("\\stencil_texture") ||
       !(p.name.ends_with("\\default_texture_two_change_color_zwrite") ||
         p.name.ends_with("\\default_texture_two_change_color") ||
         p.name.ends_with("\\default_texture_four_change_color_zwrite") ||
         p.name.ends_with("\\default_texture_four_change_color")))return {};
    const auto& w=m.forward_program.words;
    if(w[54]!=0x8021u || !(w[53]&0x10000u))return {};
    std::optional<unsigned> result;
    for(unsigned n=0;n<std::min(w[53]&255u,8u);++n) {
        if(w[34+n]!=0x08090000u || w[n]!=0x18190000u ||
           w[45+n]!=0x100c0u || w[26+n]!=0xc0u)continue;
        if(result)return {};result=n;
    }
    return result;
}
PixelProgram neutral_detail_response(const PixelProgram& source,unsigned stage) {
    if(stage>=8 || source.words[34+stage]!=0x08090000u ||
       source.words[stage]!=0x18190000u || source.words[45+stage]!=0x100c0u ||
       source.words[26+stage]!=0xc0u || !(source.words[53]&0x10000u))
        throw FormatError("pStencil neutral detail requires its isolated source multiply");
    auto out=source;
    out.words[34+stage]=0x08020000u; // RGB: T0 * stage C1, same x2 output.
    out.words[stage]=0x18120000u; // Alpha: T0.a * stage C1.a.
    out.words[18+stage]=0xff808080u;
    return out;
}
}
