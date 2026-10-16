#include "halo2/source_live_shadow_main_draws_tu15.hpp"
#include <cstring>
namespace halo2 {
SourceShadowMainDrawHandoffTu15 source_live_shadow_main_draws_tu15(
    const SourceRuntimeMemory& memory,const SourceDrawUnitState& state,const SourceBspAdmission& bsp,
    std::span<const std::array<std::uint32_t,4>> associations,
    std::uint32_t mask_token,std::uint32_t instance_token,std::span<const Transform> transforms,
    const SourceShadowMainInputWriterTu15& write) {
    constexpr std::uint32_t context=0x556e18u,arena=context+0x25cu;
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    if(!write || !mask_token || !instance_token || state.draw_count!=state.main_prefix ||
       state.draw_count>850 || state.override_count>192 || state.section_highwater>32 ||
       state.arena.used>=512 || associations.size()>512 || word(0x4cd3a0u)!=context ||
       word(context+0x2a78u) || word(context+0x2a7cu))
        throw FormatError("TU1.5 ordinary main draw handoff lacks its bounded collected inputs");
    const auto native_bsp=word(0x4ecc18u);
    if(!native_bsp || word(native_bsp+0x140u)!=transforms.size())
        throw FormatError("TU1.5 main draw handoff changed its authored BSP instance bank");
    const auto native_instances=word(native_bsp+0x144u);
    auto units=state.units;
    SourceShadowMainDrawHandoffTu15 result{state.draw_count,state.arena.used,unsigned(associations.size()),
        state.override_count,state.section_highwater};
    for(unsigned n=0;n<state.draw_count;++n) {
        auto& u=units[n].words;
        if(state.override_slots[n]!=255 && state.override_slots[n]>=state.override_count)
            throw FormatError("TU1.5 main draw consumes an absent override slot");
        if((u[0]&0x40u) && u[5]) {
            if(u[5]<mask_token || u[5]>=std::uint64_t(mask_token)+sizeof(state.arena.words) ||
               ((u[5]-mask_token)&3u))
                throw FormatError("TU1.5 main draw mask does not belong to the collected range arena");
            u[5]=arena+(u[5]-mask_token);++result.mask_relocations;
        }
        if(u[1]==0xffffffffu && (u[0]&0x1000u)) {
            const auto instance=(u[3]>>18)&2047u;
            if(instance>=transforms.size() || u[7]!=instance_token+instance*88u)
                throw FormatError("TU1.5 main draw instance does not belong to the collected transform bank");
            const auto native=native_instances+instance*88u;
            if(std::memcmp(memory.view(native,sizeof(Transform)).data(),&transforms[instance],sizeof(Transform)))
                throw FormatError("TU1.5 main draw instance transform differs from actual native BSP bytes");
            u[7]=native;++result.instance_relocations;
        }
    }
    const auto put=[&](std::uint32_t address,std::span<const std::byte> bytes){if(!bytes.empty())write(address,bytes);};
    const auto scalar=[&](std::uint32_t address,const auto& value){put(address,std::as_bytes(std::span(&value,1)));};
    // Literal2CBD0 banks; retain inactive unit/coverage/override storage.
    put(0x4c6958u,std::as_bytes(std::span(units).first(state.draw_count)));
    scalar(0x4cd398u,state.draw_count);scalar(0x4cd39cu,state.main_prefix);
    put(0x4cd3a4u,std::as_bytes(std::span(state.coverage).first(state.draw_count)));
    put(0x4cd6f6u,std::as_bytes(std::span(state.override_slots).first(state.draw_count)));
    put(0x4cda48u,std::as_bytes(std::span(state.overrides).first(state.override_count)));
    scalar(0x4cdd48u,state.override_count);put(0x4cdd4cu,std::as_bytes(std::span(state.sections)));
    scalar(0x4ce1ccu,state.section_highwater);
    //1348E0/1332B0 query-owned range and association banks, not2CB10 state.
    put(context+0x1cu,std::as_bytes(std::span(bsp.cluster_entries)));
    put(context+0x21cu,std::as_bytes(std::span(bsp.marked_clusters)));
    put(arena,std::as_bytes(std::span(state.arena.words)));scalar(context+0xa5cu,state.arena.used);
    scalar(context+0xa5eu,std::uint16_t(associations.size()));put(context+0xa60u,std::as_bytes(associations));
    return result;
}
}
