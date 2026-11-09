#pragma once
#include "halo2/native_tu15_stack_arena.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_stack_arena_detail {
struct Activity {std::uint64_t allocations{},releases{},backing_allocations{},backing_releases{};std::uint32_t last_owner{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_STACK_ARENA");return p&&*p;}();return enabled;}
template<class State,class Call> bool allocate(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().allocations;auto &g=s.gp;const auto arena=g[1];unsigned requested_reads{};
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    s.push(0x2e10a0u,g[3]);s.push(0x2e10a1u,g[5]);s.push(0x2e10a2u,g[6]);
    const auto requested=[&]{
        if(requested_reads++==0u){at(0x2e10a3u);g[6]=physics_step_detail::r(s.memory,g[4]+16u,4);s.flags.minus(g[6],0x1000u,4);s.push(0x2e10adu,g[7]);g[7]=g[1];return g[6];}
        at(0x2e10e5u);return g[3]=physics_step_detail::r(s.memory,g[4]+20u,4);
    };
    auto fp=native_partition_temporary_detail::floating_point(s);NativeTu15ArrayGrowthServices services;
    services.allocate=[&](const auto &request,auto &context){++activity().backing_allocations;activity().last_owner=request.owner;
        at(0x2e10c5u);s.push(0x2e10c5u,0x2e10c8u);call(s,request.owner,0x2e10c5u);context=native_partition_temporary_detail::floating_point(s);return g[0];};
    NativeTu15StackArenaPhases phases;
    phases.observe=[&](NativeTu15StackArenaPhase phase,const NativeTu15StackArenaPhaseData &data){
        switch(phase) {
        case NativeTu15StackArenaPhase::allocation_inputs:g[6]=data.bytes;break;
        case NativeTu15StackArenaPhase::allocation_owner_lookup:g[1]=data.allocator;g[0]=data.table;s.push(0x2e10bfu,0x12u);g[2]=data.bytes+16u;s.push(0x2e10c4u,g[2]);break;
        case NativeTu15StackArenaPhase::allocation_complete:g[2]=arena+8u;g[1]=g[2];break;
        case NativeTu15StackArenaPhase::header_word:if(data.field<3u){g[5]=data.value;if(data.field==0u)g[3]=data.block;}else g[1]=data.value;break;
        case NativeTu15StackArenaPhase::remaining_inputs:g[1]=data.block+16u;g[6]=s.flags.minus(g[6],g[3],4);break;
        case NativeTu15StackArenaPhase::boundary_published:g[7]=s.pop();g[5]=g[1]+g[3];g[6]=s.pop();break;
        default:break;
        }
    };
    native_tu15_allocate_stack_arena_with_phases(s.memory,arena,requested,fp,services,phases,s.observe);
    g[5]=s.pop();g[0]=g[1];g[3]=s.pop();s.pop();g[4]+=4u;at(0x2e1102u);return true;
}
template<class State,class Call> bool release(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().releases;auto &g=s.gp;const auto arena=g[1];auto fp=native_partition_temporary_detail::floating_point(s);
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    NativeTu15ArrayGrowthServices services;
    services.release=[&](const auto &request,auto &context){++activity().backing_releases;activity().last_owner=request.owner;
        at(0x2e114au);s.push(0x2e114au,0x2e114du);call(s,request.owner,0x2e114au);context=native_partition_temporary_detail::floating_point(s);};
    NativeTu15StackArenaPhases phases;
    phases.observe=[&](NativeTu15StackArenaPhase phase,const NativeTu15StackArenaPhaseData &data){
        switch(phase) {
        case NativeTu15StackArenaPhase::release_inputs:g[0]=data.boundary;g[2]=data.available;s.push(0x2e1116u,g[6]);break;
        case NativeTu15StackArenaPhase::release_restore_inputs:g[2]=data.bytes;s.push(0x2e111fu,g[7]);g[0]=s.flags.minus(g[0],16u,4);g[1]=s.flags.plus(g[1],8u,4);g[6]=g[0];break;
        case NativeTu15StackArenaPhase::header_word:if(data.field<3u)g[7]=data.value;else{g[6]=data.value;s.push(0x2e113bu,0x12u);}break;
        case NativeTu15StackArenaPhase::release_owner_lookup:g[1]=data.allocator;g[6]=data.table;s.push(0x2e1148u,g[2]);s.push(0x2e1149u,g[0]);break;
        default:break;
        }
    };
    native_tu15_release_stack_arena_with_phases(s.memory,arena,[&]{at(0x2e1117u);return g[6]=physics_step_detail::r(s.memory,g[4]+12u,4);},fp,services,phases,s.observe);
    g[7]=s.pop();g[6]=s.pop();s.pop();g[4]+=8u;at(0x2e114fu);return true;
}
} // namespace halo2::native_stack_arena_detail
