#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include <algorithm>
namespace halo2::pstencil {
namespace {
#include "texture_accumulation_2003_program.inc"
bool normalized(const Texture& t) {
    return !t.depth && t.layers==1 && (t.format==11 || (t.format>=14 && t.format<=16));
}
}
PixelProgram texture_accumulation_2003(){return {texture_program_2003,{}};}
bool supports_texture_accumulation_2003(const Material& m) {
    if(m.vertex_program.empty() || !m.forward_pass || m.forward_vertex!=3 || m.forward_blend || m.forward_raster.alpha_test ||
       m.forward_pass_index>=m.passes.size() || !m.passes[m.forward_pass_index].vertex_shader.ends_with("\\stencil_texture") ||
       !normalized(m.diffuse) || !normalized(m.detail))return false;
    const auto& w=m.forward_program.words;const auto stages=w[53]&255u;
    if(w[53]!=(0x11100u|stages) || (stages!=1 && stages!=4 && stages!=7))return false;
    if(w[54]!=(stages==1?0x21u:0x8021u) || (stages!=1 && !normalized(m.auxiliary)))return false;
    for(unsigned s:{0u,1u,3u})if(m.texture_states[s].color_sign || m.texture_states[s].alpha_kill || m.texture_states[s].color_key_op)return false;
    if(w[8]!=(stages==1?0x0c010000u:0x0c0f0000u) || w[9]!=(stages==1?0x00001c80u:0x04051c80u))return false;
    constexpr std::array<std::uint32_t,8> rgb1{0x08090000},out1{0x100c0};
    constexpr std::array<std::uint32_t,8> rgb4{0x010b020b,0x2c200c01,0x2d200d01,0x08090000},out4{0x30cd,0x400,0x500,0x100c0};
    constexpr std::array<std::uint32_t,8> rgb7{0x010b020b,0x2c200c01,0x2d200d01,0x3c201c01,0x3b201b01,0x04050c0d,0x08090000},out7{0x30cd,0x400,0x500,0xc00,0xd00,0x45,0x100c0};
    const auto& rgb=stages==1?rgb1:stages==4?rgb4:rgb7;
    const auto& out=stages==1?out1:stages==4?out4:out7;
    for(unsigned s=0;s<8;++s) {
        if(w[34+s]!=rgb[s] || w[45+s]!=out[s])return false;
        const unsigned alpha=s+1==stages?0x18190000u:stages==7 && s==0?0x0b200000u:0u;
        const unsigned alpha_out=(s+1==stages || (stages==7 && s==0))?0xc0u:0u;
        if(w[s]!=alpha || w[26+s]!=alpha_out)return false;
    }
    return true;
}
Material adapt_texture_accumulation_2003(Material m,std::optional<unsigned> neutral_detail_stage) {
    if(!supports_texture_accumulation_2003(m))throw FormatError("Unsupported current texture accumulation subgraph");
    if(neutral_detail_stage) {
        const unsigned stage=*neutral_detail_stage;const auto& w=m.forward_program.words;
        if(stage>=8 || w[34+stage]!=0x08090000u || w[stage]!=0x18190000u ||
           w[45+stage]!=0x100c0u || w[26+stage]!=0xc0u)
            throw FormatError("Texture accumulation lost its completed neutral-detail multiply");
        // Preserve the existing primary-matched Elite effective sample while
        // the original constructor owns the final multiply and target alpha.
        m.detail=Texture{1,1,11,std::vector<std::byte>{std::byte{128},std::byte{128},std::byte{128},std::byte{255}}, {}};
        m.animated_textures[1].clear();
    }
    m.forward_blend=texture_accumulation_blend_2003;
    // Primary-matched old materials use their exact default_texture samplers.
    // Later assets keep authored addressing/filtering for their own UV layout.
    if(reference_light_material(m.name))for(unsigned s:{0u,1u,3u})m.texture_states[s]={};
    return m;
}
}
