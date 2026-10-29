// Native kernels: campaign audio (TU1.5 sound manager). The routines live in
// the campaign_{playback,mix,channels} files; this file resolves their
// original addresses and registers the group's entries once it is complete.
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "audio/native_campaign_audio_ducking_adapter.hpp"
#include "audio/native_campaign_audio_fades_adapter.hpp"
#include "audio/native_campaign_audio_retirement_adapter.hpp"
#include "audio/native_campaign_audio_stop_callback_adapter.hpp"
#include "halo2/source_campaign_audio_tu15.hpp"
#include <cstdlib>
#include <cstring>
#include <map>

namespace halo2::native_tu15_sound {
namespace {
const std::map<std::uint32_t,CampaignRoutineEntry> &routines() {
    static const auto table=[]{
        // HALO2_NATIVE_CAMPAIGN_AUDIO_PARTS (playback,mix,channels,ducking,fades,retirement,stop_callback) narrows
        // the native parts to bisect an oracle difference; default all.
        const auto *parts=std::getenv("HALO2_NATIVE_CAMPAIGN_AUDIO_PARTS");
        const auto enabled=[&](const char *part){return !parts || std::strstr(parts,part);};
        std::map<std::uint32_t,CampaignRoutineEntry> t;
        const std::pair<const char *,std::span<const CampaignRoutineEntry>> groups[]{
            {"playback",campaign_playback_routines()},{"mix",campaign_mix_routines()},
            {"channels",campaign_channel_routines()},{"ducking",campaign_ducking_routines()},{"fades",campaign_fade_routines()},{"retirement",campaign_retirement_routines()},{"stop_callback",campaign_stop_callback_routines()}};
        for(const auto &[part,group]:groups)if(enabled(part))
            for(const auto &routine:group)if(!t.emplace(routine.address,routine).second)
                throw FormatError("Campaign audio routine registered twice");
        return t;
    }();
    return table;
}
template<std::uint32_t address> void entry(M &m,Abi &a,FP fp,const O &o,const Services &services) {
    CampaignContext c{m,fp,o,services};
    campaign_audio_call(c,a,address);
}
constexpr auto& entries=campaign_audio_required_entries;
template<std::size_t... n> constexpr auto all(std::index_sequence<n...>) {
    // The group calls into the rest of the live world: its live check is the
    // dispatcher's snapshot oracle.
    return std::array<Kernel,sizeof...(n)>{Kernel{entries[n],entry<entries[n]>,{},true}...};
}
constexpr auto group_kernels=all(std::make_index_sequence<std::size(entries)>{});
bool complete() {
    static const bool done=[]{
        std::vector<CampaignRoutineEntry> candidates;
        for(const auto& [address,entry]:routines())candidates.push_back(entry);
        return campaign_audio_semantic_complete(entries,candidates);
    }();
    return done;
}
} // namespace

void campaign_audio_call(CampaignContext &c,Abi &a,std::uint32_t address) {
    if(const auto found=routines().find(address);found!=routines().end()){found->second.run(c,a);return;}
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
