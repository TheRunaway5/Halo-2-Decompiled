#pragma once
#include "halo2/native_tu15_island_partition_temporary.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_partition_temporary_detail {
struct Activity {
    std::uint64_t constructors{},destructors{},arena_allocations{},arena_releases{},owned_releases{};
    std::uint32_t last_residual_owner{};
};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode() {
    static const bool enabled=[]{const auto *v=std::getenv("HALO2_REFERENCE_PARTITION_TEMPORARIES");return v&&*v;}();
    return enabled;
}
template<class State> NativeTu15PhysicsFrameFp floating_point(const State &s) {
    std::uint16_t control;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0":"=m"(control));
#else
    throw std::runtime_error("Native temporary ABI requires the original x87 control owner");
#endif
    return {control,s.fp.word(),_mm_getcsr()};
}
template<class State> bool admitted(const State &s) {
    if(reference_mode() || native_tu15_leaves::reference_mode() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction) {
        if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return false;
        const auto *cursor=s.instruction_cursor;
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        if(s.observe&&!cursor->observation_only_stores)return false;
    }
    return true;
}
// The DATA owner is shared with island repartitioning. This adapter supplies
// only the original caller ABI at its named input/allocation/publication
// boundaries. Actual slow arena and owned-allocation virtual owners remain
// explicit residual services until their complete ABI adapters qualify.
template<class State,class Call> bool construct(State &s,Call call) {
    using namespace physics_step_detail;
    if(!admitted(s))return false;
    ++activity().constructors;auto &gp=s.gp;
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value){w(s.memory,s.observe,pc,p,4,value);};
    s.push(0x2eba20u,0xffffffffu);s.push(0x2eba22u,0x37e0a8u);
    gp[0]=r(s.memory,s.fs_base,4);s.push(0x2eba2du,gp[0]);store(0x2eba2eu,s.fs_base,gp[4]);
    s.push(0x2eba35u,gp[1]);s.push(0x2eba36u,gp[3]);s.push(0x2eba37u,gp[6]);
    gp[6]=gp[1];gp[0]=s.flags.logic(0,4);store(0x2eba3cu,gp[4]+8u,gp[6]);
    const NativeTu15ArrayHeader header{gp[6]};
    NativeTu15PhysicsFrameFp fp=floating_point(s);
    NativeTu15IslandLifetimeServices services;
    services.allocate_arena=[&](auto,auto owner,auto,auto&context) {
        ++activity().arena_allocations;activity().last_residual_owner=owner.identity;
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x2eba6eu;
        s.push(0x2eba6eu,0x2eba71u);call(s,owner.identity,0x2e7deeu);
        context=floating_point(s);
        return gp[0];
    };
    NativeTu15PartitionTemporaryPhases phases;
    phases.observe=[&](NativeTu15PartitionTemporaryPhase phase,const NativeTu15PartitionTemporaryPhaseData &data) {
        switch(phase) {
        case NativeTu15PartitionTemporaryPhase::allocation_inputs:
            gp[1]=data.arena;gp[2]=data.available;store(0x2eba59u,gp[4]+20u,0);
            gp[0]=data.bytes;s.flags.minus(gp[0],gp[2],4);break;
        case NativeTu15PartitionTemporaryPhase::slow_allocation_table:
            gp[2]=data.table;s.push(0x2eba6du,gp[0]);break;
        case NativeTu15PartitionTemporaryPhase::fast_allocation_begin:
            s.push(0x2eba73u,gp[5]);s.push(0x2eba74u,gp[7]);break;
        case NativeTu15PartitionTemporaryPhase::fast_allocation_inputs:
            gp[7]=data.cursor;gp[5]=gp[7]+gp[0];gp[2]=s.flags.minus(gp[2],gp[0],4);
            gp[0]=gp[7];gp[7]=s.pop();break;
        case NativeTu15PartitionTemporaryPhase::fast_allocation_complete:
            gp[5]=s.pop();break;
        case NativeTu15PartitionTemporaryPhase::allocation_complete:
            gp[0]=data.block;gp[1]=gp[3];break;
        case NativeTu15PartitionTemporaryPhase::data_published:
            gp[1]=s.flags.logic(gp[1]|0x80000000u,4);break;
        case NativeTu15PartitionTemporaryPhase::capacity_published:
            gp[1]=r(s.memory,gp[4]+12u,4);break;
        default:break;
        }
    };
    native_tu15_construct_island_partition_temporary_with_phases(s.memory,header,[&]{
        gp[3]=r(s.memory,gp[4]+28u,4);return gp[3];
    },NativeTu15IslandPartitionTemporaryKind::indices,fp,services,phases,s.observe);
    gp[0]=gp[6];gp[6]=s.pop();gp[3]=s.pop();store(0x2ebaa2u,s.fs_base,gp[1]);
    gp[4]=s.flags.plus(gp[4],16u,4);s.pop();gp[4]+=4u;
    if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x2ebaacu;
    return true;
}
template<class State,class Call> bool destroy(State &s,Call call) {
    using namespace physics_step_detail;
    if(!admitted(s))return false;
    ++activity().destructors;auto &gp=s.gp;
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value){w(s.memory,s.observe,pc,p,4,value);};
    s.push(0x2eb850u,0xffffffffu);s.push(0x2eb852u,0x37e0a8u);
    gp[0]=r(s.memory,s.fs_base,4);s.push(0x2eb85du,gp[0]);store(0x2eb85eu,s.fs_base,gp[4]);
    s.push(0x2eb865u,gp[1]);s.push(0x2eb866u,gp[6]);gp[6]=gp[1];s.push(0x2eb869u,gp[7]);
    store(0x2eb86au,gp[4]+8u,gp[6]);const NativeTu15ArrayHeader header{gp[6]};
    NativeTu15PhysicsFrameFp fp=floating_point(s);NativeTu15IslandLifetimeServices services;
    services.release_arena=[&](auto,auto owner,auto,auto,auto&context) {
        ++activity().arena_releases;activity().last_residual_owner=owner.identity;
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x2eb89du;
        s.push(0x2eb89du,0x2eb8a0u);call(s,owner.identity,0x2e80ddu);
        context=floating_point(s);
    };
    services.allocation.release=[&](const auto &request,auto&context) {
        ++activity().owned_releases;activity().last_residual_owner=request.owner;
        if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x2eb8d7u;
        s.push(0x2eb8d7u,0x2eb8dau);call(s,request.owner,0x2e8117u);
        context=floating_point(s);
    };
    NativeTu15PartitionTemporaryPhases phases;
    phases.observe=[&](NativeTu15PartitionTemporaryPhase phase,const NativeTu15PartitionTemporaryPhaseData &data) {
        switch(phase) {
        case NativeTu15PartitionTemporaryPhase::release_inputs:
            gp[0]=data.bytes;gp[2]=data.block;gp[1]=data.arena;
            s.flags.logic(gp[0],4);store(0x2eb884u,gp[4]+20u,0);break;
        case NativeTu15PartitionTemporaryPhase::release_boundary:
            gp[7]=data.boundary;s.flags.minus(gp[2],gp[7],4);
            if(!s.flags.zf)s.flags.logic(gp[7],4);break;
        case NativeTu15PartitionTemporaryPhase::slow_release_table:
            gp[7]=data.table;s.push(0x2eb89bu,gp[0]);s.push(0x2eb89cu,gp[2]);break;
        case NativeTu15PartitionTemporaryPhase::fast_release_inputs:
            gp[7]=s.flags.minus(data.cursor,gp[0],4);gp[2]=s.flags.plus(data.available,gp[0],4);break;
        case NativeTu15PartitionTemporaryPhase::release_capacity:
            gp[0]=data.flags;s.flags.logic(gp[0],4);store(0x2eb8b7u,gp[4]+20u,0xffffffffu);break;
        case NativeTu15PartitionTemporaryPhase::owned_release_allocator:
            gp[1]=data.allocator;gp[2]=data.table;gp[0]=s.flags.logic(gp[0]&0x7fffffffu,4);
            s.push(0x2eb8ceu,0x12u);gp[0]=s.flags.shift(gp[0],2u,4);s.push(0x2eb8d3u,gp[0]);break;
        case NativeTu15PartitionTemporaryPhase::owned_release_data:
            gp[0]=data.data;s.push(0x2eb8d6u,gp[0]);break;
        default:break;
        }
    };
    native_tu15_destroy_island_partition_temporary_with_phases(s.memory,header,
        NativeTu15IslandPartitionTemporaryKind::indices,fp,services,phases,s.observe);
    gp[1]=r(s.memory,gp[4]+12u,4);gp[7]=s.pop();gp[6]=s.pop();store(0x2eb8e0u,s.fs_base,gp[1]);
    gp[4]=s.flags.plus(gp[4],16u,4);s.pop();
    if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=0x2eb8eau;
    return true;
}
} // namespace halo2::native_partition_temporary_detail
