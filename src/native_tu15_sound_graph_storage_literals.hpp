#pragma once
#include "halo2/native_tu15_sound_ring_effect.hpp"
namespace halo2::tu15_sound_graph_storage {
// Named scalar parameters from literal installed phase, not a DSP state bank.
inline void literal_reverb_parameters(tu15_sound_ring_effect::State& state,unsigned index){
    if(index==0){
        state.delays={0x2a0u,0x310u,0x2e0u,0x350u,0x390u,0x6b0u,0xe90u,0x7b0u,0x46a0u,0x5dc0u,0x5320u,0x5a50u,0x5320u,0x3160u,0x260u,0x230u,0x250u,0x290u,0x16bf0u};
        state.reflection_gains={0x7fffffu,0x800001u,0x800001u,0x7fffffu,0x7fffffu,0x7fffffu,0x800001u,0x800001u};
        state.target_gains={0x24eu,0x21au,0x22fu,0x26du,0x65eau,0x65eau};
        state.diffusion_gains={0xc00000u,0xc00000u,0xc00000u,0x400000u,0xc00000u,0xc00000u,0xc00000u};
        state.wet_gains={0xcccccdu,0x333333u,0xcccccdu,0xcccccdu};
        state.cluster_taps={0x0u,0xe8u,0x218u,0x3a0u,0x0u,0xe8u,0x218u,0x3a0u,0x15c0u,0x1a50u,0x1ff0u,0x26b0u};
        state.flags=2;state.cross_gain=0u;state.mono_gain=1677721u;state.output_mode=4u;
        state.filters[0]={0,1104729u,7283878u};
        state.filters[1]={0,1949948u,2090718u};
        state.filters[2]={0,1949948u,2090718u};
        state.filters[3]={0,1949948u,2090718u};
        state.filters[4]={0,573926u,5976009u};
        state.filters[5]={0,682352u,5569848u};
        state.filters[6]={0,818498u,5106251u};
        state.filters[7]={0,959520u,4607183u};
        state.filters[8]={0,4975145u,37948u};
        state.filters[9]={0,4975145u,37948u};
    }
    else {
        state.delays={0x2a0u,0x310u,0x2e0u,0x350u,0x390u,0x6b0u,0xe90u,0x7b0u,0x46a0u,0x5dc0u,0x5320u,0x5a50u,0x5320u,0x3160u,0x260u,0x230u,0x250u,0x290u,0x16bf0u};
        state.reflection_gains={0x7fffffu,0x800001u,0x800001u,0x7fffffu,0x7fffffu,0x7fffffu,0x800001u,0x800001u};
        state.target_gains={0xb89u,0xa86u,0xaeeu,0xc25u,0x5095u,0x5095u};
        state.diffusion_gains={0xc00000u,0xc00000u,0xc00000u,0x400000u,0xc00000u,0xc00000u,0xc00000u};
        state.wet_gains={0xcccccdu,0x333333u,0xcccccdu,0xcccccdu};
        state.cluster_taps={0x0u,0xe8u,0x218u,0x3a0u,0x0u,0xe8u,0x218u,0x3a0u,0x15c0u,0x1a50u,0x1ff0u,0x26b0u};
        state.flags=2;state.cross_gain=0u;state.mono_gain=1677721u;state.output_mode=4u;
        state.filters[0]={0,2447250u,5941357u};
        state.filters[1]={0,1625998u,2195909u};
        state.filters[2]={0,1625998u,2195909u};
        state.filters[3]={0,1625998u,2195909u};
        state.filters[4]={0,462025u,6061582u};
        state.filters[5]={0,551832u,5664184u};
        state.filters[6]={0,660933u,5212532u};
        state.filters[7]={0,787540u,4713837u};
        state.filters[8]={0,4353618u,44858u};
        state.filters[9]={0,4353618u,44858u};
    }
}
}
