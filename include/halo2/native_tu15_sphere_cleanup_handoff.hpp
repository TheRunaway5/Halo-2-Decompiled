#pragma once
#include "halo2/native_tu15_small_contact_factory_handoff.hpp"
#include "halo2/native_tu15_agent_cleanup_children.hpp"
#include "halo2/native_tu15_contact_point_binding.hpp"

namespace halo2 {
struct NativeTu15SphereCleanupHandoffActivity {
    std::uint64_t attempts{},completed{},refusals{},scalar_deletions{},empty_point_callbacks{};
    NativeTu15ContactPointBindingActivity points;
};
// Actual2F6C88 ->2F9960, thiscall0/RET0. The wrapper consumes no returned
// GP/SIMD value before dispatching its own scalar destructor. Point removal
// and deleting destructors use the same native owners as World pair cleanup.
inline bool native_tu15_handoff_sphere_cleanup(SourcePhysicsContactMemory&memory,
    const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&abi,
    NativeTu15SmallContactFactoryPolicy policy,std::uint32_t data_first,
    const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15SphereCleanupHandoffActivity*activity=nullptr){
    if(call.target!=0x2f9960u)return false;
    NativeTu15SphereCleanupHandoffActivity local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
        const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t n{};
            for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;};
        const auto sp=abi.gp[4];
        if(call.instruction!=0x2f6c88u||call.tail||sp<policy.stack_first||
            std::uint64_t(sp)+4u>policy.stack_end||word(sp)!=0x2f6c8bu)
            throw std::runtime_error("Native sphere cleanup requires its actual2F6C88 caller");
        if(!policy.observers_read_only||(policy.first_trace_active&&*policy.first_trace_active)||
            (policy.second_trace_active&&*policy.second_trace_active))
            throw std::runtime_error("Native sphere cleanup requires its retained-DATA observer domain");
        if(abi.x87_depth||abi.active_mmx||!abi.floating_point)
            throw std::runtime_error("Native sphere cleanup requires qualified empty x87/FP input");
        if((data_first&15u)||data_first<policy.stack_first||std::uint64_t(data_first)+0x100u>sp)
            throw std::runtime_error("Native sphere cleanup requires disjoint caller-owned DATA");
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        SourcePhysicsContactRecordObserver retained;
        if(observe)retained=[&](const auto&e){if(policy.instruction)policy.instruction(e.instruction);observe(e);};
        NativeTu15AgentCleanupChildrenServices services;services.allocation=allocation;
        services.remove_point=[&](auto manager,auto owner,auto point,auto&current){
            if(owner.identity==0x2e4d60u){++stats.empty_point_callbacks;return;}
            if(owner.identity!=0x322f70u)
                throw std::runtime_error("Native sphere cleanup requires its point-removal owner="+std::to_string(owner.identity));
            native_tu15_remove_bound_contact_point(memory,manager,std::uint32_t(point),
                {data_first,data_first+0x20u,data_first+0x40u,data_first+0x60u},current,0,allocation,retained,&stats.points);
        };
        services.agents.dispose=[&](auto object,auto owner,auto disposing,auto&current){
            if(owner.identity==0x2e4d60u)return; // Authentic RET4, no DATA/FP effect.
            if(owner.identity!=0x2fc540u&&owner.identity!=0x2f8090u&&owner.identity!=0x3051a0u)
                throw std::runtime_error("Native sphere cleanup requires its deleting destructor="+std::to_string(owner.identity));
            const auto kind=owner.identity==0x2fc540u?NativeTu15AgentDestructor::owner_2fc540:
                owner.identity==0x2f8090u?NativeTu15AgentDestructor::owner_2f8090:NativeTu15AgentDestructor::owner_3051a0;
            native_tu15_destroy_contact_agent(memory,kind,{object},disposing,current,services,retained);++stats.scalar_deletions;
        };
        try{
            if(policy.instruction)policy.instruction(call.target);
            native_tu15_cleanup_sphere_contact_agent(memory,{abi.gp[1]},fp,services,retained);
        }catch(...){publish();throw;}
        publish();abi.gp[4]=sp+4u;++stats.completed;return true;
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
