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
    // Presence alone never qualifies a candidate. Instruction-level C++
    // transcriptions retain false until semantic conversion, the complete
    // dependency closure and the actual caller contract have original proof.
    bool semantic_original_proven=false;
};
inline constexpr std::array<std::uint32_t,40> campaign_audio_required_entries{
    0x120060u,0x120310u,0x127ad0u,0x127b70u,0x128650u,0x129500u,0x129f10u,0x12ab30u,
    0x12ade0u,0x12bbe0u,0x12bc10u,0x12be90u,0x12bf00u,0x12c5b0u,0x12cbc0u,0x12ccc0u,0x12d6f0u,0x12d730u,0x12d840u,
    0x12db00u,0x12e9a0u,0x150900u,0x2206d0u,0x2207a0u,0x221af0u,0x224c40u,0x225610u,0x225890u,0x225d70u,0x225fb0u,
    0x226490u,0x226c00u,0x2271a0u,0x228310u,0x228480u,0x2b8650u,0x2b8b10u,0x2b8f80u,0x2b92e0u,0x2b9330u};
// Shared by the real registry and its regression. Forty present addresses do
// not pass: each exact owner must be unique, callable and semantic/proven.
constexpr bool campaign_audio_semantic_complete(std::span<const std::uint32_t> required,
    std::span<const CampaignRoutineEntry> candidates) {
    if(required.empty())return false;
    for(std::size_t index=0;index<required.size();++index) {
        for(std::size_t previous=0;previous<index;++previous)
            if(required[previous]==required[index])return false;
        const CampaignRoutineEntry* match=nullptr;
        for(const auto& entry:candidates)if(entry.address==required[index]) {
            if(match)return false;match=&entry;
        }
        if(!match || !match->run || !match->semantic_original_proven)return false;
    }
    return true;
}
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
