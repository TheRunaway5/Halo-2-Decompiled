#include "halo2/source_shadow_filter.hpp"

namespace halo2 {
std::array<SourceShadowFilterPass,2> source_shadow_filter_passes() {
    PixelProgram program;
    auto &w=program.words;
    // Original20F30 with XMM0/XMM3 both0. Three ordered half-scale combiners
    // average t0/t1, t2/t3, then both pairs; alpha follows the same stages.
    w[0]=0x18201920;w[1]=0x1a201b20;w[2]=0x1c201d20;
    w[8]=0xc;w[9]=0x1c00;
    w[26]=0x30c00;w[27]=0x30d00;w[28]=0x30c00;
    w[34]=0x08200920;w[35]=0x0a200b20;w[36]=0x0c200d20;
    w[45]=0x30c00;w[46]=0x30d00;w[47]=0x30c00;
    w[53]=0x11003;w[54]=0x8421;
    std::array<SourceShadowFilterPass,2> result;
    for(unsigned step=0;step<2;++step) {
        auto &pass=result[step];pass.source_target=step?11:10;
        pass.destination_target=step?10:11;pass.program=program;
        // 211A0 grows width2 by ((1-.5)+2*.5)=1.5 for the second pass.
        // 34100 uses half that width with the authored two-axis signs.
        const float offset=step?1.5f:1.f;
        for(unsigned stage=0;stage<4;++stage)
            pass.taps[stage]={stage&1?offset:-offset,stage&2?offset:-offset,0,1};
    }
    return result;
}
}
