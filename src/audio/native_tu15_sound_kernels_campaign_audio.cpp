// Native kernels: campaign audio (TU1.5 sound manager). The routines live in
// the campaign_{playback,mix,channels} files; this file resolves their
// original addresses and registers the group's entries once it is complete.
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/source_campaign_audio_tu15.hpp"
#include <cstdlib>
#include <cstring>
#include <map>

namespace halo2::native_tu15_sound {
namespace {
const std::map<std::uint32_t,CampaignRoutine> &routines() {
    static const auto table=[]{
        // HALO2_NATIVE_CAMPAIGN_AUDIO_PARTS (playback,mix,channels) narrows
        // the native parts to bisect an oracle difference; default all.
        const auto *parts=std::getenv("HALO2_NATIVE_CAMPAIGN_AUDIO_PARTS");
        const auto enabled=[&](const char *part){return !parts || std::strstr(parts,part);};
        std::map<std::uint32_t,CampaignRoutine> t;
        const std::pair<const char *,std::span<const CampaignRoutineEntry>> groups[]{
            {"playback",campaign_playback_routines()},{"mix",campaign_mix_routines()},
            {"channels",campaign_channel_routines()}};
        for(const auto &[part,group]:groups)if(enabled(part))
            for(const auto &routine:group)if(!t.emplace(routine.address,routine.run).second)
                throw FormatError("Campaign audio routine registered twice");
        return t;
    }();
    return table;
}
template<std::uint32_t address> void entry(M &m,Abi &a,FP fp,const O &o,const Services &services) {
    CampaignContext c{m,fp,o,services};
    campaign_audio_call(c,a,address);
}
constexpr std::uint32_t entries[]{0x120060u,0x120310u,0x127ad0u,0x127b70u,0x128650u,0x129500u,0x129f10u,0x12ab30u,
    0x12ade0u,0x12bbe0u,0x12bc10u,0x12be90u,0x12bf00u,0x12c5b0u,0x12cbc0u,0x12ccc0u,0x12d6f0u,0x12d730u,0x12d840u,
    0x12db00u,0x12e9a0u,0x150900u,0x2206d0u,0x2207a0u,0x221af0u,0x224c40u,0x225610u,0x225890u,0x225d70u,0x225fb0u,
    0x226490u,0x226c00u,0x2271a0u,0x228310u,0x228480u,0x2b8650u,0x2b8b10u,0x2b8f80u,0x2b92e0u,0x2b9330u};
template<std::size_t... n> constexpr auto all(std::index_sequence<n...>) {
    // The group calls into the rest of the live world: its live check is the
    // dispatcher's snapshot oracle.
    return std::array<Kernel,sizeof...(n)>{Kernel{entries[n],entry<entries[n]>,{},true}...};
}
constexpr auto group_kernels=all(std::make_index_sequence<std::size(entries)>{});
bool complete() {
    static const bool done=[]{
        for(const auto address:entries)if(!routines().contains(address))return false;
        return true;
    }();
    return done;
}
} // namespace

void campaign_audio_call(CampaignContext &c,Abi &a,std::uint32_t address) {
    if(const auto found=routines().find(address);found!=routines().end()){found->second(c,a);return;}
    if(source_campaign_audio_tu15_owns_entry(address) && !complete()) {
        // Transitional: the lifted original of a routine not yet converted.
        source_run_campaign_audio_tu15(c.m,address,a,c.fp,c.services,c.o);return;
    }
    if(!c.services.required_owner)throw FormatError("Campaign audio callee has no live owner");
    c.services.required_owner(c.m,{u32(c.m,a.gp[4])-5u,address,false},a,c.o);
}
std::span<const Kernel> campaign_audio_kernels() {
    return complete()?std::span<const Kernel>(group_kernels):std::span<const Kernel>{};
}
} // namespace halo2::native_tu15_sound

namespace halo2 {
void native_tu15_campaign_audio_run(SourcePhysicsContactMemory &m,std::uint32_t address,SourcePhysicsContactRuntimeAbi &a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices &services) {
    using namespace native_tu15_sound;
    const O none{};
    CampaignContext c{m,fp,none,services};
    campaign_audio_call(c,a,address);
}
} // namespace halo2
