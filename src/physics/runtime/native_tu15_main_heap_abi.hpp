#pragma once
#include "halo2/native_tu15_main_heap_release.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_main_heap_detail {
struct Activity {std::uint64_t release_wrappers{},ledger_releases{},release_owners{};std::uint32_t last_owner{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_MAIN_HEAP");return p&&*p;}();return enabled;}
template<class State> bool remove_allocation(State &s) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().ledger_releases;auto &g=s.gp;const auto allocator=g[1];bool active=false,matched=false;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    s.push(0x232c80u,g[3]);NativeTu15MainHeapReleasePhases phases;
    phases.observe=[&](NativeTu15MainHeapReleasePhase phase,const NativeTu15MainHeapReleasePhaseData &data){
        switch(phase) {
        case NativeTu15MainHeapReleasePhase::count_acquired:
            g[3]=data.count;s.push(0x232c84u,g[7]);g[7]=s.flags.logic(0,4);s.flags.logic(g[3],4);break;
        case NativeTu15MainHeapReleasePhase::header_word:
            if(!data.copying){if(data.field==0u)g[2]=data.base;else{g[0]=data.end;s.push(0x232c91u,g[6]);active=true;}}
            else if(data.field==0u)g[0]=data.base;
            else {g[2]=data.end;g[3]=s.flags.minus(g[0],g[6],4);}break;
        case NativeTu15MainHeapReleasePhase::cursor_ready:g[0]=data.cursor;s.push(0x232c9au,g[5]);break;
        case NativeTu15MainHeapReleasePhase::candidate:g[5]=data.value;s.flags.plus(data.value-data.base,data.base,4);s.flags.minus(g[5],g[6],4);break;
        case NativeTu15MainHeapReleasePhase::search_advanced:
            g[5]=data.count;g[7]=s.flags.increment(g[7],4,false);g[0]=s.flags.minus(g[0],8u,4);s.flags.minus(g[7],g[5],4);break;
        case NativeTu15MainHeapReleasePhase::matched:
            matched=true;g[3]=s.flags.increment(g[3],4,true);s.flags.minus(g[7],g[3],4);break;
        case NativeTu15MainHeapReleasePhase::copy_begin:g[6]=data.offset;break;
        case NativeTu15MainHeapReleasePhase::copy_word:
            if(data.field==0u){g[5]=data.value;g[0]=s.flags.minus(g[0],g[6],4);}else g[3]=data.value;break;
        case NativeTu15MainHeapReleasePhase::copy_advanced:
            g[2]=data.count;g[7]=s.flags.increment(g[7],4,false);g[6]=s.flags.plus(g[6],8u,4);g[2]=s.flags.increment(g[2],4,true);s.flags.minus(g[7],g[2],4);break;
        case NativeTu15MainHeapReleasePhase::publication_inputs:
            g[0]=s.flags.increment(data.count,4,true);g[5]=s.pop();break;
        default:break;
        }
    };
    native_tu15_remove_main_heap_allocation_with_phases(s.memory,allocator,[&]{at(0x232c92u);return g[6]=physics_step_detail::r(s.memory,g[4]+16u,4);},phases,s.observe);
    if(active&&!matched)g[5]=s.pop();if(active)g[6]=s.pop();g[7]=s.pop();g[3]=s.pop();s.pop();g[4]+=4u;
    at(active&&!matched?0x232cb7u:0x232cf9u);return true;
}
template<class State,class Call> bool release(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().release_wrappers;auto &g=s.gp;const auto allocator=g[1];auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    NativeTu15MainHeapReleasePhases phases;phases.observe=[&](auto phase,const auto &data){
        if(phase==NativeTu15MainHeapReleasePhase::owner_table){g[0]=data.table;s.push(0x232d26u,g[2]);}
    };
    const auto child=[&](std::uint32_t owner,auto &context){
        ++activity().release_owners;activity().last_owner=owner;at(0x232d27u);s.push(0x232d27u,0x232d2au);
        call(s,owner,0x22e427u);context=native_partition_temporary_detail::floating_point(s);
    };
    NativeTu15MainHeapReleaseServices services;
    services.ledger_entry=[&](auto,auto,auto &context,auto){child(0x232c80u,context);};
    services.release=[&](auto,auto owner,auto,auto &context,auto){child(owner,context);};
    native_tu15_release_main_heap_allocation_with_phases(s.memory,allocator,[&]{at(0x232d20u);return g[2]=physics_step_detail::r(s.memory,g[4]+4u,4);},fp,s.fp.count,services,phases,s.observe);
    s.pop();g[4]+=12u;at(0x232d2au);return true;
}
} // namespace halo2::native_main_heap_detail
