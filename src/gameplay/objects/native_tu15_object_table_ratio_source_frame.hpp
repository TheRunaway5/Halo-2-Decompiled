#pragma once
#include "halo2/native_tu15_object_table_ratio_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_object_table_ratio_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,class State> class SourceFrame {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address,unsigned width=4) {return r(s.memory,address,width);}
    static std::uint32_t signed_word(std::uint32_t value) {return (value^0x8000u)-0x8000u;}
public:
    explicit SourceFrame(State& state):s(state) {}
    void open_object_row() {
        auto& g=s.gp;
        step(0x24b430u);s.set(1,read(0x4ecbd0u),4,0);
        step(0x24b436u);s.set(2,read(g[1]+0x44u),4,0);
        step(0x24b439u);s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
        step(0x24b43cu);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x24b441u);s.set(0,g[0]+g[0]*2u,4,0);
        step(0x24b444u);s.set(0,read(g[2]+g[0]*4u+8u),4,0);
        step(0x24b448u);s.set(2,read(0x4f3218u),4,0);
        step(0x24b44eu);s.set(1,signed_word(read(g[0]+0x17eu,2)),4,0);
        step(0x24b455u);s.set(0,read(g[2]+0x244u,2),2,0);
        step(0x24b45cu);s.flags.logic(s.reg(0,2,0),2);
        step(0x24b45fu);
        if(s.flags.zf){
            step(0x24b461u);s.set(0,1u,4,0);
            step(0x24b466u);
        }else{step(0x24b468u);s.set(0,signed_word(s.reg(0,2,0)),4,0);}
    }
    bool has_index() {
        step(0x24b46bu);s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
        step(0x24b46eu);s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(0,4,0)));
        step(0x24b472u);return !s.flags.zf;
    }
    bool has_constant_flag() {
        auto& g=s.gp;
        step(0x24b474u);s.set(0,read(0x52d710u),4,0);
        step(0x24b479u);s.flags.logic(read(g[0]+g[1]+0x170u,1)&8u,1);
        step(0x24b481u);return !s.flags.zf;
    }
    void load_constant() {step(0x24b483u);s.xmm[0]=_mm_set_ss(f(read(0x46a0d0u)));}
    void scale_positive_count() {
        auto& g=s.gp;
        step(0x24b48cu);s.set(1,read(g[0]+g[1]*2u+0x1a0u,2),2,0);
        step(0x24b494u);s.flags.logic(s.reg(1,2,0),2);
        step(0x24b497u);
        if(!s.flags.zf&&s.flags.sf==s.flags.of){
            step(0x24b499u);s.set(0,signed_word(s.reg(1,2,0)),4,0);
            step(0x24b49cu);
        }else{step(0x24b49eu);s.set(0,s.flags.logic(0u,4),4,0);}
        step(0x24b4a0u);s.set(1,read(0x51df54u),4,0);
        step(0x24b4a6u);s.xmm[0]=_mm_set_ss(f(read(g[1]+4u)));
        step(0x24b4abu);s.xmm[0]=_mm_div_ss(s.xmm[0],s.xmm[1]);
        step(0x24b4afu);s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(0,4,0)));
        step(0x24b4b3u);s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[1]);
    }
    __m128 finish_constant() {step(0x24b48bu);s.pop();return s.xmm[0];}
    __m128 finish() {step(0x24b4b7u);s.pop();return s.xmm[0];}

    template<class Child> void run_mode_owner(Child child) {
        step(0x1655f0u);s.set(0,read(0x4f3218u),4,0);
        step(0x1655f5u);s.flags.minus(read(s.gp[0]+0x180u),9u,4);
        step(0x1655fcu);
        if(s.flags.zf){
            step(0x1655feu);s.set(0,read(s.gp[4]+4u),4,0);
            step(0x165602u);s.push(0x165602u,0x165607u);child(s);
            step(0x165607u);
        }else{
            step(0x16560au);s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
            step(0x16560du);
        }
        s.pop();s.gp[4]+=4u;
    }
};

template<bool Literal,class State> void run_source_frame(State& state) {
    SourceFrame<Literal,State> view{state};query(view);
}
template<bool Literal,class State,class Child> void run_mode_owner(State& state,Child child) {
    SourceFrame<Literal,State> view{state};view.run_mode_owner(child);
}
} // namespace halo2::native_object_table_ratio_detail
