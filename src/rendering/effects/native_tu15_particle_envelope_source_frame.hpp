#pragma once
#include "halo2/native_tu15_particle_envelope_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_particle_envelope_detail {
using namespace physics_world_runtime_detail;

template<bool Literal,class State> class SourceFrame {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    __m128 read(std::uint32_t address) {return _mm_set_ss(f(r(s.memory,address,4)));}
    bool taken() const {return s.flags.cf||s.flags.zf;}
    void leave(std::uint32_t pc) {step(pc);s.pop();}
public:
    explicit SourceFrame(State& state):s(state) {}
    bool override_set() {
        auto& g=s.gp;
        step(0x252ec0u);s.set(1,r(s.memory,0x51df50u,4),4,0);
        step(0x252ec6u);s.flags.logic(s.reg(1,4,0)&s.reg(1,4,0),4);
        step(0x252ec8u);s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
        step(0x252ecbu);if(s.flags.zf)return false;
        step(0x252ecdu);s.set(2,r(s.memory,0x5u+g[1],1),1,0);
        step(0x252ed0u);s.flags.logic(s.reg(2,1,0)&s.reg(2,1,0),1);
        step(0x252ed2u);return !s.flags.zf;
    }
    bool after_start() {
        step(0x252ed4u);s.flags.scalar_compare(s.xmm[1],read(0x18u+s.gp[0]),false);
        step(0x252ed8u);return !taken();
    }
    bool before_end() {
        step(0x252edau);s.xmm[2]=read(0x24u+s.gp[0]);
        step(0x252edfu);s.flags.scalar_compare(s.xmm[2],s.xmm[1],false);
        step(0x252ee2u);return !taken();
    }
    bool in_attack() {
        auto& x=s.xmm;
        step(0x252ee4u);x[0]=read(0x1cu+s.gp[0]);
        step(0x252ee9u);x[0]=add(x[0],read(0x18u+s.gp[0]));
        step(0x252eeeu);s.flags.scalar_compare(x[0],x[1],false);
        step(0x252ef1u);return !taken();
    }
    void return_attack() {
        auto& x=s.xmm;
        step(0x252ef3u);x[1]=subtract(x[1],read(0x18u+s.gp[0]));
        step(0x252ef8u);x[1]=multiply(x[1],read(0x20u+s.gp[0]));
        step(0x252efdu);x[0]=x[1];
        leave(0x252f00u);
    }
    bool in_release() {
        auto& x=s.xmm;
        step(0x252f01u);x[0]=read(0x24u+s.gp[0]);
        step(0x252f06u);x[0]=subtract(x[0],read(0x28u+s.gp[0]));
        step(0x252f0bu);s.flags.scalar_compare(x[1],x[0],false);
        step(0x252f0eu);return !taken();
    }
    void return_release() {
        auto& x=s.xmm;
        step(0x252f10u);x[0]=read(0x24u+s.gp[0]);
        step(0x252f15u);x[0]=subtract(x[0],x[1]);
        step(0x252f19u);x[0]=multiply(x[0],read(0x2cu+s.gp[0]));
        leave(0x252f1eu);
    }
    void return_one() {step(0x252f1fu);s.xmm[0]=read(0x46a0d0u);leave(0x252f27u);}
    void return_zero() {leave(0x252f27u);}
};

template<bool Literal,class State> void run_source_frame(State& state) {
    SourceFrame<Literal,State> frame{state};envelope(frame);
}
} // namespace halo2::native_particle_envelope_detail
