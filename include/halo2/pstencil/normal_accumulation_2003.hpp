#pragma once
#include "halo2/render_program.hpp"

namespace halo2::pstencil {
// Original game964B0 builtin1, retained in accumulation-construction-v1.
// The final combiner copies normalization-cube RGB and the bump image alpha.
inline PixelProgram normal_accumulation_2003() {
    PixelProgram result{};
    result.words[8]=0x0000000b;result.words[9]=0x00001800;
    result.words[53]=1;result.words[54]=0x00074621;result.words[55]=0x00000111;
    return result;
}
// Original resident anisotropic_bump and normal-space utility sampler states.
inline ShaderTextureState normal_bump_sampler_2003(bool anisotropic=true) {
    ShaderTextureState result{};if(anisotropic){result.min_filter=3;result.anisotropy=4;}return result;
}
inline ShaderTextureState normalization_cube_sampler_2003() {
    ShaderTextureState result{};result.address_u=result.address_v=result.address_w=3;return result;
}
inline ShaderTextureState normal_target_sampler_2003() {
    auto result=normalization_cube_sampler_2003();
    result.mag_filter=result.min_filter=1;result.mip_filter=0;return result;
}
}
