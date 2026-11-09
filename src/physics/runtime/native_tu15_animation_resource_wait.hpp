#pragma once
#include "halo2/native_tu15_animation_resource_wait_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <cstdlib>
namespace halo2::native_animation_resource_wait_detail {
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_ANIMATION_RESOURCE_WAIT");return p&&*p;}();return enabled;}
template<class State,class Call>bool bridge(State&s,Call call){
    if(reference_mode()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if(site>=0x1262b0u&&site<=0x1262edu)return false;
    }
    using P=NativeTu15AnimationWaitPhase;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto low=[&](unsigned reg,std::uint32_t value){s.gp[reg]=(s.gp[reg]&0xffffff00u)|(value&255u);};
    const auto test_ready=[&](const NativeTu15AnimationWait&d,unsigned reg){low(reg,d.ready);s.flags.logic(d.ready,1);};
    const auto progress=[&](P phase,const NativeTu15AnimationWait&d){
        switch(phase){
        case P::save:at(0x1262b0u);s.push(0x1262b0u,s.gp[6]);break;
        case P::job:at(0x1262b1u);break;
        case P::ready:s.gp[6]=d.job;low(0,0);s.flags.logic(0,1);s.flags.minus(d.job,0xffffffffu,4);at(0x1262bbu);break;
        case P::promoted_ready:at(0x1262c9u);break;
        case P::recheck_ready:test_ready(d,0);at(0x1262d0u);break;
        case P::yielded_ready:at(0x1262dcu);break;
        case P::clear:
            if(!d.waited)test_ready(d,1);else{test_ready(d,0);low(0,1);}at(0x1262e5u);break;
        case P::finished:
            if(d.absent){s.gp[6]=d.job;low(0,0);s.flags.logic(0,1);s.flags.minus(d.job,0xffffffffu,4);}
            at(0x1262ecu);s.gp[6]=s.pop();at(0x1262edu);s.pop();break;
        }
    };
    struct Owners{
        State&s;Call&call;decltype(at)&position;decltype(test_ready)&tested;
        std::uint32_t promote(const NativeTu15AnimationWait&d){tested(d,1);position(0x1262c2u);s.push(0x1262c2u,8u);position(0x1262c4u);s.push(0x1262c4u,0x1262c9u);call(s,0x1236a0u,0x1262c4u,false);return s.gp[7];}
        std::uint32_t yield(const NativeTu15AnimationWait&d){tested(d,0);position(0x1262d7u);s.push(0x1262d7u,0x1262dcu);call(s,0x2dbf74u,0x1262d7u,false);return s.gp[7];}
        void store(std::uint32_t address,std::uint32_t value){physics_step_detail::w(s.memory,s.observe,0x1262e5u,address,4,value);}
    }owners{s,call,at,test_ready};
    native_tu15_wait_animation_resource(s.memory,s.gp[7],owners,progress);return true;
}
} // namespace halo2::native_animation_resource_wait_detail
