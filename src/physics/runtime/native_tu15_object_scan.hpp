#pragma once
#include "halo2/native_tu15_bsp_object_association.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

namespace halo2::native_object_scan_detail {
inline std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t at,unsigned count=4) {
    std::array<std::byte,4>bytes{};memory.read(at,std::span(bytes).first(count));std::uint32_t value{};
    for(unsigned n=0;n<count;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
inline bool reference_mode() {
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_SCAN");return p&&*p;}();return enabled;
}
template<class State>bool bridge(State&state) {
    if(reference_mode()||state.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{state.selected_observation_pcs;})if(!state.selected_observation_pcs)return false;
    if(state.services.instruction) {
        if(!state.literal_service_sites||!state.instruction_cursor||!state.instruction_cursor->position)return false;
        const auto*cursor=state.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(state.observe&&!cursor->observation_only_stores)return false;
        for(const auto site:cursor->service_sites)if(site>=0xbcee0u&&site<=0xbcf65u)return false;
    }
    // The global, argument, rows and cursor reads straddle the four real PUSH
    // stores. Capturing them together after a host-side prologue loses aliases.
    auto&gp=state.gp;const auto stack=gp[4],pool=word(state.memory,0x4ecbd0u);
    state.push(0xbcee6u,gp[3]);state.push(0xbcee7u,gp[5]);
    const NativeTu15ObjectScanState scan{word(state.memory,stack+4u)};
    state.push(0xbceecu,gp[6]);const auto rows=word(state.memory,pool+0x44u);
    state.push(0xbcef0u,gp[7]);const auto cursor=word(state.memory,scan.identity+6u,2);
    const auto limit=word(state.memory,pool+0x38u);
    const auto result=native_tu15_scan_objects(state.memory,{scan,rows,cursor,limit},state.observe,
        NativeTu15ObjectScanStores::source_abi);
    gp[0]=result.object;gp[1]=result.index;
    std::uint32_t final_pc{};
    switch(result.exit) {
    case NativeTu15ObjectScanExit::initial_bound:
        gp[2]=pool;state.flags.minus(result.index,limit,4);final_pc=0xbcf65u;break;
    case NativeTu15ObjectScanExit::exhausted:
        gp[2]=result.datum;state.flags.logic(0,4);final_pc=0xbcf47u;break;
    case NativeTu15ObjectScanExit::matched:
        gp[1]=(result.index&0xffffff00u)|result.kind;gp[2]=result.datum;
        state.flags.logic(result.selected_bit,4);final_pc=0xbcf58u;break;
    }
    gp[7]=state.pop();gp[6]=state.pop();gp[5]=state.pop();gp[3]=state.pop();
    if(state.instruction_cursor&&state.instruction_cursor->position)*state.instruction_cursor->position=final_pc;
    state.pop();gp[4]+=4u;return true;
}
} // namespace halo2::native_object_scan_detail
