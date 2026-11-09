#pragma once
#include "physics/runtime/native_tu15_array_abi.hpp"
#include "physics/runtime/native_tu15_broadphase_leaves.hpp"

namespace halo2::native_broadphase_add_detail {
struct Activity {std::uint64_t insertions{},upper_moves{},lower_moves{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode() {
    static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_BROADPHASE_ADD");return p&&*p;}();
    return enabled;
}
// Insert the upper endpoint first, then the lower. Equal upper coordinates
// move; equal lower coordinates stop. Read each moved word immediately before
// its publication so aliases retain the original ordered copy semantics.
template<class State,class Reserve> bool insert_edges(State &s,Reserve reserve) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().insertions;auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p,unsigned n=4){at(pc);return physics_step_detail::r(s.memory,p,n);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n=4){at(pc);physics_step_detail::w(s.memory,s.observe,pc,p,n,value);};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4;store(pc,g[4],value);};
    const auto pop=[&](std::uint32_t pc){const auto value=read(pc,g[4]);g[4]+=4;return value;};
    const auto low=[&](unsigned reg,std::uint32_t value){g[reg]=(g[reg]&0xffff0000u)|(value&65535u);};
    const auto signed32=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    push(0x2eafb0u,g[6]);push(0x2eafb1u,g[7]);g[7]=g[1];g[6]=read(0x2eafb4u,g[7]+4u);g[0]=read(0x2eafb7u,g[7]+8u);
    g[6]+=2u;g[0]&=0x7fffffffu;s.flags.minus(g[0],g[6],4);
    if(signed32(g[0])<signed32(g[6])) {
        g[0]+=g[0];s.flags.minus(g[6],g[0],4);if(signed32(g[6])>=signed32(g[0]))g[0]=g[6];
        push(0x2eafceu,4);push(0x2eafd0u,g[0]);push(0x2eafd1u,g[7]);push(0x2eafd2u,0x2eafd7u);reserve(s);g[4]=s.flags.plus(g[4],12u,4);
    }
    g[0]=read(0x2eafdau,g[7]);low(1,read(0x2eafdcu,g[4]+24u,2));g[0]+=g[6]*4u-12u;store(0x2eafe5u,g[7]+4u,g[6]);
    while(std::uint16_t(g[1])<=read(0x2eafe8u,g[0],2)) {
        g[2]=read(0x2eaff0u,g[0]);store(0x2eaff2u,g[0]+8u,g[2]);g[0]-=4u;++activity().upper_moves;
    }
    g[2]=read(0x2eaffdu,g[4]+16u);store(0x2eb001u,g[0]+8u,g[1],2);store(0x2eb005u,g[0]+10u,g[2],2);
    g[6]=read(0x2eb009u,g[7]);g[1]=g[0]-g[6];g[6]=read(0x2eb00fu,g[4]+32u);g[1]=std::uint32_t(signed32(g[1])>>2u)+2u;
    store(0x2eb019u,g[6],g[1],2);low(1,read(0x2eb01cu,g[4]+20u,2));
    while(std::uint16_t(g[1])<read(0x2eb021u,g[0],2)) {
        g[6]=read(0x2eb026u,g[0]);store(0x2eb028u,g[0]+4u,g[6]);g[0]-=4u;++activity().lower_moves;
    }
    store(0x2eb033u,g[0]+6u,g[2],2);g[2]=read(0x2eb037u,g[4]+28u);store(0x2eb03bu,g[0]+4u,g[1],2);
    const auto data=read(0x2eb03fu,g[7]);g[0]=s.flags.minus(g[0],data,4);g[0]=s.flags.right(g[0],2u,4,true);g[0]=s.flags.increment(g[0],4,false);
    g[7]=pop(0x2eb045u);store(0x2eb046u,g[2],g[0],2);g[6]=pop(0x2eb049u);pop(0x2eb04au);g[4]+=24u;at(0x2eb04au);return true;
}
} // namespace halo2::native_broadphase_add_detail
