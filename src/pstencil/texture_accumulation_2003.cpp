#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include <algorithm>
namespace halo2::pstencil {
namespace {
#include "texture_accumulation_2003_program.inc"
bool normalized(const Texture& t) {
    return !t.depth && t.layers==1 && (t.format==11 || (t.format>=14 && t.format<=16));
}
Texture neutral_detail_texture() {
    return {1,1,11,{std::byte{128},std::byte{128},std::byte{128},std::byte{255}},{}};
}
std::optional<PixelProgram> actor_alpha_coverage(const PixelProgram& source) {
    const auto& w=source.words;
    if(w[53]!=0x11108u || w[54]!=0x18421u || ((w[9]>>8)&255u)!=0x1au ||
       w[0]!=0x1a201120u || w[26]!=0xcbu || w[33]!=0x4a00u ||
       (w[7]!=0x11201812u && w[7]!=0x11201820u))return {};
    // The final T2.a is selected by R0.a (the bump alpha written by stage
    // zero). Its inputs are the original T0.a and stage-7 color alphas.
    // Reject any intermediate write to either of those two registers.
    for(unsigned i=0;i<8;++i) {
        const auto rgb=w[45+i];
        if(((rgb&0x80000u) && ((rgb>>4)&15u)==8u) ||
           ((rgb&0x40000u) && (rgb&15u)==8u) ||
           ((rgb&0x80000u) && ((rgb>>4)&15u)==12u) ||
           ((rgb&0x40000u) && (rgb&15u)==12u))return {};
        if(i==0 || i==7)continue;
        for(unsigned shift:{0u,4u,8u}) {
            const auto dest=(w[26+i]>>shift)&15u;
            if(dest==8u || dest==12u)return {};
        }
    }
    auto result=source;
    auto& out=result.words;
    std::fill(out.begin(),out.begin()+10,0u);
    std::fill(out.begin()+26,out.begin()+43,0u);
    std::fill(out.begin()+45,out.end(),0u);
    out[0]=0x1a200000u;out[26]=0xc0u;
    out[7]=w[7];out[33]=w[33];
    out[8]=0x20u;out[9]=0x1a80u;
    out[53]=0x11108u;out[54]=0x421u;
    return result;
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
        m.detail=neutral_detail_texture();
        m.animated_textures[1].clear();
    }
    m.forward_blend=texture_accumulation_blend_2003;
    // Primary-matched old materials use their exact default_texture samplers.
    // Later assets keep authored addressing/filtering for their own UV layout.
    if(reference_default_texture_samplers(m.name))for(unsigned s:{0u,1u,3u})m.texture_states[s]={};
    return m;
}

std::optional<Material> adapt_actor_texture_accumulation_2003(Material m,bool color_mask,bool neutral_detail) {
    const bool alpha_test=m.forward_raster.alpha_test!=0;
    if(m.vertex_program.empty() || (alpha_test && (color_mask || !normalized(m.bump))) || !normalized(m.diffuse) ||
       ((color_mask || !neutral_detail) && !normalized(m.detail)))return {};
    for(unsigned stage=0;stage<(alpha_test?3u:2u);++stage) {
        if(stage==1 && neutral_detail && !color_mask)continue;
        const auto& state=m.texture_states[stage];
        if(state.color_sign || state.alpha_kill || state.color_key_op)return {};
    }
    auto& pass=m.passes.at(m.forward_pass_index);
    if(color_mask) {
        // The retail layer-14 producer has already isolated the color mask
        // from lighting. Keep its packed constants and per-owner remaps.
        std::array<std::uint32_t,60> controls{};
        controls[8]=0x08090000u;controls[9]=0x1880u;
        controls[34]=0x29200901u;controls[45]=0x900u;
        controls[53]=0x11101u;controls[54]=0x21u;
        auto authored=m.forward_program.words;
        std::fill(authored.begin()+10,authored.begin()+26,0);
        authored[43]=authored[44]=0;
        if(authored!=controls)return {};
    } else {
        // This existing native texture kernel supplies a unit color factor.
        // The selected layer-3 vertex program retains both authored UVs.
        if(alpha_test) {
            const auto coverage=actor_alpha_coverage(m.forward_program);
            if(!coverage)return {};
            m.forward_program=*coverage;
        } else {
            m.forward_program={};
            auto& words=m.forward_program.words;
            words[0]=0x18190000u;words[8]=0x0c010000u;words[9]=0x1c80u;words[26]=0xc0u;
            words[34]=0x08090000u;words[43]=0xffffffffu;words[45]=0x100c0u;
            words[53]=0x11101u;words[54]=0x21u;
            pass.pixel_remaps.clear();pass.pixel_externs.clear();pass.fragments.clear();
        }
        pass.program=m.forward_program;
        if(neutral_detail) {
            m.detail=neutral_detail_texture();
            m.animated_textures[1].clear();
            m.texture_states[1]={};
        }
    }
    std::erase_if(pass.textures,[&](const auto& binding) {
        return binding.stage>=(alpha_test?3u:2u) || (binding.stage==1 && neutral_detail && !color_mask);
    });
    m.auxiliary={};
    if(!alpha_test)m.bump={};
    for(unsigned stage=alpha_test?3u:2u;stage<4;++stage) {
        m.texture_states[stage]={};
        m.animated_textures[stage].clear();
    }
    m.accumulation_texture=false;
    m.baked_lightmap_texture=false;
    m.forward_raster.color_write=0x01010101u;
    m.forward_raster.depth_write=1;
    m.forward_blend=texture_accumulation_blend_2003;
    return m;
}
}
