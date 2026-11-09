#pragma once
#include "platform/native_tu15_async_order.hpp"

namespace halo2::native_async_promote_detail {
// Complete1236A0: promote a still-owned task by unlinking its current queue
// position and passing its live priority/frame to the real insertion child.
// Mutex waits/releases remain actual required-owner calls; their native
// platform ABI is an explicit boundary rather than a supplied game outcome.
template<class State,class Insert>bool promote(State& state,Insert&& insert) {
    if(!native_async_order_detail::admitted(state))return false;
    using namespace physics_step_detail;
    auto&g=state.gp;
    const auto at=[&](std::uint32_t pc){if(state.instruction_cursor&&state.instruction_cursor->position)*state.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t address){at(pc);return r(state.memory,address,4);};
    const auto write=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value){at(pc);w(state.memory,state.observe,pc,address,4,value);};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){at(pc);state.push(pc,value);};
    const auto pop=[&](std::uint32_t pc){at(pc);return state.pop();};
    g[0]=read(0x1236a0u,0x4ecc2cu);push(0x1236a5u,g[3]);push(0x1236a6u,0xffffffffu);push(0x1236a8u,g[0]);
    g[3]&=0xffffff00u;state.flags.logic(0,1);push(0x1236abu,0x1236b0u);state.required(0x1236abu,0x2e0478u,false);
    g[0]=((g[6]&255u)<<6u)+0x4ecc38u;
    const auto current_handle=read(0x1236bfu,g[0]+4u);state.flags.minus(current_handle,g[6],4);
    if(current_handle==g[6]) {
        g[1]=read(0x1236c4u,0x4ef1bcu);
        if(g[1]==g[0]) {
            state.flags.minus(g[1],g[0],4);g[1]=read(0x1236ceu,g[0]+0x3cu);write(0x1236d1u,0x4ef1bcu,g[1]);
        } else {
            auto next=read(0x1236d9u,g[1]+0x3cu);
            while(next!=g[0]) {g[1]=read(0x1236e0u,g[1]+0x3cu);next=read(0x1236e3u,g[1]+0x3cu);}
            state.flags.minus(next,g[0],4);g[2]=read(0x1236e8u,g[0]+0x3cu);write(0x1236ebu,g[1]+0x3cu,g[2]);
        }
        g[1]=read(0x1236eeu,g[4]+8u);push(0x1236f2u,g[0]);write(0x1236f3u,g[0],g[1]);
        push(0x1236f5u,0x1236fau);insert(state);g[3]=(g[3]&0xffffff00u)|1u;
    }
    g[2]=read(0x1236fcu,0x4ecc2cu);push(0x123702u,g[2]);push(0x123703u,0x123708u);
    state.required(0x123703u,0x2e0342u,false);g[0]=(g[0]&0xffffff00u)|(g[3]&255u);
    g[3]=pop(0x12370au);pop(0x12370bu);g[4]+=4u;return true;
}
}
