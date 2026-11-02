#pragma once
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <stdexcept>

namespace halo2 {
struct NativeTu15SmallContactFactoryPolicy {
    std::uint32_t stack_first{},stack_end{};
    bool observers_read_only{};
    const bool*first_trace_active{};
    const bool*second_trace_active{};
    std::function<void(std::uint32_t)> instruction;
};
struct NativeTu15SmallContactFactoryActivity {
    std::uint64_t attempts{},completed{},refusals{};
};
// Exact2F7293 ->2F9990 cdecl4. The source caller consumes only EAX before
// restoring the other registers and replacing flags at2F729C's ADD ESP,16.
// Consume the actual fourth formal at its source phase after real allocation.
// Preserve the first three unread inputs and the caller-owned argument packet.
inline bool native_tu15_handoff_small_contact_factory(SourcePhysicsContactMemory&memory,
    const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&abi,
    NativeTu15SmallContactFactoryPolicy policy,const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15SmallContactFactoryActivity*activity=nullptr){
    if(call.target!=0x2f9990u)return false;
    NativeTu15SmallContactFactoryActivity local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
        const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t n{};
            for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;};
        const auto sp=abi.gp[4];
        if(call.instruction!=0x2f7293u||call.tail||sp<policy.stack_first||
            std::uint64_t(sp)+20u>policy.stack_end||word(sp)!=0x2f7295u)
            throw std::runtime_error("Native small contact factory requires its actual2F7293 caller");
        if(!policy.observers_read_only||(policy.first_trace_active&&*policy.first_trace_active)||
            (policy.second_trace_active&&*policy.second_trace_active))
            throw std::runtime_error("Native small contact factory requires its retained-DATA observer domain");
        if(abi.x87_depth||abi.active_mmx||!abi.floating_point)
            throw std::runtime_error("Native small contact factory requires qualified empty x87/FP input");
        NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
        SourcePhysicsContactRecordObserver retained;
        if(observe)retained=[&](const auto&e){
            if(policy.instruction)policy.instruction(e.instruction);observe(e);};
        NativeTu15ContactAgent agent;
        try{
            if(policy.instruction)policy.instruction(call.target);
            agent=native_tu15_create_contact_agent(memory,NativeTu15ContactAgentFactory::owner_2f9990,
                NativeTu15ContactAgentFactoryArguments{[&]{return word(sp+16u);}},fp,allocation,retained);
        }catch(...){publish();throw;}
        publish();abi.gp[0]=agent.identity;abi.gp[4]=sp+4u;++stats.completed;return true;
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
