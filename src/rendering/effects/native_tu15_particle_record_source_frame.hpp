#pragma once
#include "halo2/native_tu15_particle_record_kernel.hpp"
#include "rendering/effects/native_tu15_particle_record_flags.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_particle_record_detail {
using namespace physics_world_runtime_detail;

template<bool Literal,class State> class SourceFrame {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    __m128 load(std::uint32_t address) {return _mm_set_ss(f(r(s.memory,address,4)));}
    // REP MOVS reports every completed iteration through the raw service in
    // both source formats, as the retained body does.
    void repeat(std::uint32_t pc,unsigned width) {
        auto& g=s.gp;
        step(pc);
        while(g[1]) {
            const auto value=r(s.memory,g[6],width);w(s.memory,s.observe,pc,g[7],width,value);
            const auto delta=(s.other_flags&1024u)?0u-width:width;
            g[6]+=delta;g[7]+=delta;--g[1];
            if(s.services.instruction)s.services.instruction(pc);
        }
    }
    void leave(std::uint32_t restore,std::uint32_t exit) {
        step(restore);s.set(6,s.pop(),4,0);
        step(exit);s.pop();s.gp[4]+=8u;
    }
public:
    explicit SourceFrame(State& state):s(state) {}
    bool pool_full() {
        step(0x1e4c0u);s.push(0x1e4c0u,s.reg(6,4,0));
        step(0x1e4c1u);s.set(6,r(s.memory,0x4c2aa0u,4),4,0);
        step(0x1e4c7u);s.set(0,s.flags.logic(s.reg(0,1,0)^s.reg(0,1,0),1),1,0);
        step(0x1e4c9u);s.flags.minus(s.reg(6,4,0),0x400u,4);
        step(0x1e4cfu);return s.flags.sf==s.flags.of;
    }
    bool overflow_pending() {
        step(0x1e59cu);s.set(1,r(s.memory,0x4c2ab1u,1),1,0);
        step(0x1e5a2u);s.flags.logic(s.reg(1,1,0)&s.reg(1,1,0),1);
        step(0x1e5a4u);return !s.flags.zf;
    }
    void clear_overflow() {step(0x1e5a6u);w(s.memory,s.observe,0x1e5a6u,0x4c2ab1u,1,0x0u);leave(0x1e5adu,0x1e5aeu);}
    void return_full() {leave(0x1e598u,0x1e599u);}
    bool reserve() {
        auto& g=s.gp;auto& x=s.xmm;
        step(0x1e4d5u);s.set(0,s.reg(6,4,0),4,0);
        step(0x1e4d7u);s.set(0,multiply_slot(s,s.reg(0,4,0)),4,0);
        step(0x1e4dau);s.push(0x1e4dau,s.reg(5,4,0));
        step(0x1e4dbu);s.set(5,r(s.memory,0x4c2aa8u,4),4,0);
        step(0x1e4e1u);w(s.memory,s.observe,0x1e4e1u,g[0]+g[5],4,0x0u);
        step(0x1e4e8u);s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(5,4,0),4),4,0);
        step(0x1e4eau);w(s.memory,s.observe,0x1e4eau,0xcu+g[0],1,0x0u);
        step(0x1e4eeu);x[0]=load(g[2]);
        step(0x1e4f2u);x[1]=load(0x4u+g[2]);
        step(0x1e4f7u);x[2]=load(0x8u+g[2]);
        step(0x1e4fcu);x[0]=subtract(x[0],load(0x491e38u));
        step(0x1e504u);x[1]=subtract(x[1],load(0x491e3cu));
        step(0x1e50cu);x[2]=subtract(x[2],load(0x491e40u));
        step(0x1e514u);s.set(6,s.flags.increment(s.reg(6,4,0),4,false),4,0);
        step(0x1e515u);s.flags.logic(s.reg(1,4,0)&s.reg(1,4,0),4);
        step(0x1e517u);w(s.memory,s.observe,0x1e517u,0x4c2aa0u,4,s.reg(6,4,0));
        step(0x1e51du);return !s.flags.zf;
    }
    void copy_payload() {
        auto& g=s.gp;
        step(0x1e51fu);s.set(6,r(s.memory,0x10u+g[4],4),4,0);
        step(0x1e523u);s.set(5,s.reg(1,4,0),4,0);
        step(0x1e525u);s.push(0x1e525u,s.reg(7,4,0));
        step(0x1e526u);s.set(1,shift_count(s,s.reg(1,4,0)),4,0);
        step(0x1e529u);s.set(7,0x24u+g[0],4,0);
        repeat(0x1e52cu,4);
        step(0x1e52eu);s.set(1,s.reg(5,4,0),4,0);
        step(0x1e530u);s.set(1,s.flags.logic(s.reg(1,4,0)&0x3u,4),4,0);
        repeat(0x1e533u,1);
        step(0x1e535u);s.set(7,s.pop(),4,0);
    }
    void publish() {
        auto& g=s.gp;auto& x=s.xmm;
        step(0x1e536u);s.set(1,r(s.memory,0xcu+g[4],4),4,0);
        step(0x1e53au);w(s.memory,s.observe,0x1e53au,g[0],4,0x3u);
        step(0x1e540u);w(s.memory,s.observe,0x1e540u,0x8u+g[0],4,0x0u);
        step(0x1e547u);w(s.memory,s.observe,0x1e547u,0x20u+g[0],4,s.reg(1,4,0));
        step(0x1e54au);x[3]=load(0x491e4cu);
        step(0x1e552u);x[3]=multiply(x[3],x[2]);
        step(0x1e556u);x[2]=load(0x491e48u);
        step(0x1e55eu);x[2]=multiply(x[2],x[1]);
        step(0x1e562u);x[1]=load(0x491e44u);
        step(0x1e56au);x[1]=multiply(x[1],x[0]);
        step(0x1e56eu);x[0]=_mm_xor_ps(x[0],x[0]);
        step(0x1e571u);x[3]=add(x[3],x[2]);
        step(0x1e575u);x[3]=add(x[3],x[1]);
        step(0x1e579u);x[0]=subtract(x[0],x[3]);
        step(0x1e57du);w(s.memory,s.observe,0x1e57du,0x4u+g[0],4,u(_mm_cvtss_f32(x[0])));
        step(0x1e582u);s.set(1,r(s.memory,g[2],4),4,0);
        step(0x1e584u);s.set(0,s.flags.plus(s.reg(0,4,0),0x64u,4),4,0);
        step(0x1e587u);w(s.memory,s.observe,0x1e587u,g[0],4,s.reg(1,4,0));
        step(0x1e589u);s.set(1,r(s.memory,0x4u+g[2],4),4,0);
        step(0x1e58cu);w(s.memory,s.observe,0x1e58cu,0x4u+g[0],4,s.reg(1,4,0));
        step(0x1e58fu);s.set(2,r(s.memory,0x8u+g[2],4),4,0);
        step(0x1e592u);w(s.memory,s.observe,0x1e592u,0x8u+g[0],4,s.reg(2,4,0));
        step(0x1e595u);s.set(0,0x1u,1,0);
        step(0x1e597u);s.set(5,s.pop(),4,0);
        leave(0x1e598u,0x1e599u);
    }
};

template<bool Literal,class State> void run_source_frame(State& state) {
    SourceFrame<Literal,State> frame{state};append(frame);
}
} // namespace halo2::native_particle_record_detail
