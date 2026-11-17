#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/pstencil/environment.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <openssl/sha.h>
#include <utility>
namespace halo2::pstencil {
namespace {
constexpr std::string_view visor_name="objects\\characters\\masterchief\\shaders\\masterchief_visor";
constexpr std::string_view multiplayer_visor_name=
    "objects\\characters\\masterchief_mp\\shaders\\masterchief_visor_mp";
constexpr std::array<std::uint32_t,60> controls[]{
#include "visor_combined_controls.inc"
};
using Digest=std::array<unsigned char,SHA256_DIGEST_LENGTH>;
constexpr Digest five_mips{
    0x9d,0x4f,0xe6,0x87,0x7f,0x24,0x1b,0xbc,0xc6,0x68,0xce,0x92,0x4a,0xc4,0x6a,0x87,
    0x28,0x18,0xef,0x6e,0x09,0xa2,0x96,0x3e,0x44,0x44,0x8a,0xc2,0x05,0xf3,0xff,0xd2};
constexpr Digest seven_mips{
    0x27,0x04,0x07,0x7e,0x52,0x94,0xb4,0x58,0x01,0x5a,0xe5,0x2f,0xd4,0x20,0xba,0x7c,
    0x9a,0x4e,0xed,0x70,0x49,0x49,0x34,0x69,0xd4,0xa5,0x16,0xc1,0x1a,0x32,0x02,0x64};
}
std::optional<Texture> load_reference_visor_cube(const Cache& cache) {
    const auto* bitmap=cache.query_tag({.group=TagGroup::from_string("bitm"),
        .name="objects\\bitmaps\\reflection_maps\\dark_gray",.require_address=true});
    if(!bitmap)return {};
    Texture cube;
    try {cube=cache.load_bitmap(bitmap->datum,0);}
    catch(const FormatError&) {return {};}
    if(cube.width!=64 || cube.height!=64 || cube.layers!=6 || cube.depth || cube.format!=14)return {};
    const unsigned levels=unsigned(cube.mips.size()/6);
    if((levels!=5 && levels!=7) || cube.mips.size()!=levels*6u)return {};
    std::size_t offset=0;
    for(unsigned face=0;face<6;++face)for(unsigned level=0;level<levels;++level) {
        const auto& mip=cube.mips[face*levels+level];
        const unsigned side=64u>>level,blocks=std::max(side/4,1u);
        if(mip.width!=side || mip.height!=side || mip.depth!=1 ||
           mip.offset!=offset || mip.size!=blocks*blocks*8u)return {};
        offset+=mip.size;
    }
    Digest digest{};
    if(cube.pixels.size()!=offset ||
       !SHA256(reinterpret_cast<const unsigned char*>(std::as_const(cube.pixels).data()),offset,digest.data()) ||
       digest!=(levels==5?five_mips:seven_mips))return {};
    if(levels==7)return cube;

    // The 2003 cache has two zero BC1 blocks at the end of each face.
    // The current cache retained the first five mips byte for byte.
    Texture result{64,64,14,{}, {},6};
    const auto& pixels=std::as_const(cube.pixels);
    for(unsigned face=0;face<6;++face) {
        for(unsigned level=0;level<5;++level) {
            auto mip=cube.mips[face*5+level];
            const auto first=pixels.begin()+mip.offset;
            mip.offset=result.pixels.size();
            result.pixels.insert(result.pixels.end(),first,first+mip.size);
            result.mips.push_back(mip);
        }
        for(unsigned side:{2u,1u}) {
            result.mips.push_back({side,side,result.pixels.size(),8,1});
            result.pixels.resize(result.pixels.size()+8,std::byte{});
        }
    }
    return result;
}
bool supports_combined_visor_environment(const Material& m) {
    if((m.name!=visor_name && m.name!=multiplayer_visor_name) ||
       !m.forward_pass || m.forward_vertex!=6 || m.forward_blend!=0 ||
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
Material separate_visor_environment(const Material& visor,const Material& independent,const Texture* reference_cube) {
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
    if(reference_cube) {
        result.auxiliary=*reference_cube;
        result.animated_textures[3].clear();
        result.texture_frames[3]=0;
    }
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
