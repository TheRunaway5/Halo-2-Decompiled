#pragma once
// Campaign audio (TU1.5 128650 frame caller, 129F10 listener producer and the
// playback, mix and channel routines they reach). Routines call each other
// at their original addresses through campaign_audio_call, with the ABI state
// the original CALL leaves. Three files implement them:
//   native_tu15_sound_kernels_campaign_playback.cpp  128650 driver and playback state
//   native_tu15_sound_kernels_campaign_mix.cpp       129F10 listener producer, 221AF0 mixer
//   native_tu15_sound_kernels_campaign_channels.cpp  226490 channels, 2B8xxx streams, 12CCC0
#include "audio/native_tu15_sound_kernel.hpp"

namespace halo2::native_tu15_sound {
struct CampaignContext {
    M &m;FP fp;const O &o;const Services &services;
};
using CampaignRoutine=void(*)(CampaignContext &,Abi &);
struct CampaignRoutineEntry {
    std::uint32_t address;CampaignRoutine run;
};
std::span<const CampaignRoutineEntry> campaign_playback_routines();
std::span<const CampaignRoutineEntry> campaign_mix_routines();
std::span<const CampaignRoutineEntry> campaign_channel_routines();

// Runs the routine at an original address: a native campaign routine; else,
// while the group is incomplete, its lifted original; else the live world
// (whose sound kernels and hooks then see the call as the original's).
void campaign_audio_call(CampaignContext &,Abi &,std::uint32_t address);
// CALL from native code: pushes the stack arguments (last first) and the
// original return address, runs the routine, then pops cdecl arguments.
inline void campaign_audio_invoke(CampaignContext &c,Abi &a,std::uint32_t return_address,std::uint32_t address,
    std::span<const std::uint32_t> arguments={},bool callee_pops=false) {
    for(auto n=arguments.size();n-->0;){a.gp[4]-=4u;store(c.m,{},return_address,a.gp[4],arguments[n]);}
    a.gp[4]-=4u;store(c.m,{},return_address,a.gp[4],return_address);
    campaign_audio_call(c,a,address);
    if(!callee_pops)a.gp[4]+=std::uint32_t(arguments.size()*4u);
}
} // namespace halo2::native_tu15_sound
