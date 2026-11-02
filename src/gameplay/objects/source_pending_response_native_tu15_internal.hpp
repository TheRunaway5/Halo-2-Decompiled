#pragma once
#include "halo2/native_tu15_pending_response.hpp"
#include "halo2/source_pending_response_observation_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::physics_world_runtime_detail {
// Compatibility boundary for the two retained D8330 parents. The semantic
// kernel owns the countdown/result; this adapter retains their original read,
// store, observation and volatile-register/defined-flag contracts. The caller
// continues into the actual eligibility and response callees only on Due.
template<class State,class Site>
NativeTu15PendingResponseStatus pending_response_step_native_tu15(State &s,Site site) {
    const auto at=[&](std::uint32_t offset){return site(0xd74a0u+offset,0xd83f0u+offset);};
    at(0x00);s.gp[2]=r(s.memory,s.gp[4]+0x1cu,4);
    at(0x04);s.gp[1]=s.flags.logic(0u,4);
    at(0x06);s.set(1,r(s.memory,s.gp[2],2),2,0);
    const auto before=std::uint16_t(s.gp[1]);
    const auto step=native_tu15_pending_response_step(before);
    at(0x09);s.gp[0]=s.flags.logic(0u,4);
    at(0x0b);s.set(0,s.reg(1,2,0),2,0);
    at(0x0e);(void)s.flags.right(s.gp[0],3u,2,false);s.gp[0]=step.countdown;
    at(0x12);s.flags.logic(s.gp[0],2);
    at(0x15);
    if(step.status==NativeTu15PendingResponseStatus::Idle) {
        source_observe_pending_response_native_tu15(s.memory,s.gp[2],before,step);
        return step.status;
    }
    at(0x1b);(void)s.flags.logic(s.gp[1]&7u,4);s.gp[1]=step.response;
    at(0x1e);s.gp[0]=step.word&0xfff8u;
    at(0x25);(void)s.flags.logic(s.gp[0]|s.gp[1],4);s.gp[0]=step.word;
    at(0x27);s.flags.logic(s.gp[0]&0xfff8u,4);
    at(0x2c);s.gp[1]=s.gp[2];
    w(s.memory,s.observe,at(0x2e),s.gp[4]+0x13u,1,1u);
    w(s.memory,s.observe,at(0x33),s.gp[1],2,step.word);
    at(0x36);source_observe_pending_response_native_tu15(s.memory,s.gp[2],before,step);
    return step.status;
}
}
