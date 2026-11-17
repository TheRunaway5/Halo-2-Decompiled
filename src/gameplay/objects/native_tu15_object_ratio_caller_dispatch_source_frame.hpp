#pragma once
#include "halo2/native_tu15_object_ratio_caller_dispatch_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_ordered.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_object_ratio_caller_detail {
using namespace physics_world_runtime_detail;
template<bool Literal,class State,bool Ordered=false> class SourceDispatch {
    State& s;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address) {return r(s.memory,address,4);}
    bool signed_at_or_below() const {return s.flags.zf||s.flags.sf!=s.flags.of;}
    void compare_key(std::uint32_t pc,std::uint32_t original) {
        step(pc);s.flags.minus(s.reg(6,4,0),source_object_function_compiled_symbol_tu15(s.memory,pc,original),4);
    }
public:
    explicit SourceDispatch(State& state):s(state) {}
    void open_object() {
        auto& g=s.gp;
        step(0x1046c0u);s.push(0x1046c0u,s.reg(1,4,0));
        step(0x1046c1u);s.set(1,read(0x4ecbd0u),4,0);
        step(0x1046c7u);s.set(2,read(g[1]+0x44u),4,0);
        step(0x1046cau);s.xmm[3]=_mm_xor_ps(s.xmm[3],s.xmm[3]);
        step(0x1046cdu);s.xmm[4]=_mm_set_ss(f(read(0x46a0d0u)));
        step(0x1046d5u);s.push(0x1046d5u,s.reg(3,4,0));
        step(0x1046d6u);s.push(0x1046d6u,s.reg(5,4,0));
        step(0x1046d7u);s.push(0x1046d7u,s.reg(6,4,0));
        step(0x1046d8u);s.set(6,read(g[4]+0x18u),4,0);
        step(0x1046dcu);s.push(0x1046dcu,s.reg(7,4,0));
        step(0x1046ddu);s.set(7,read(g[4]+0x18u),4,0);
        step(0x1046e1u);s.set(0,s.reg(7,4,0),4,0);
        step(0x1046e3u);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0x1046e8u);s.set(0,g[0]+g[0]*2u,4,0);
        step(0x1046ebu);s.set(5,read(g[2]+g[0]*4u+8u),4,0);
        step(0x1046efu);s.set(1,read(g[5]),4,0);
        step(0x1046f2u);s.set(0,read(0x4f0414u),4,0);
        step(0x1046f7u);s.set(1,s.flags.logic(s.reg(1,4,0)&0xffffu,4),4,0);
        step(0x1046fdu);shift_definition_index<Ordered>(s);
    }
    void compare_low_key() {compare_key(0x104700u,0x10000573u);}
    void publish_definition() {
        auto& g=s.gp;
        step(0x104706u);s.set(3,read(g[1]+g[0]+8u),4,0);
        step(0x10470au);w(s.memory,s.observe,0x10470au,g[4]+0x10u,4,s.reg(3,4,0));
        step(0x10470eu);s.xmm[2]=s.xmm[3];
    }
    bool at_or_below_low() {step(0x104711u);return signed_at_or_below();}
    bool at_or_below_middle() {
        compare_key(0x104717u,0x1500056eu);step(0x10471du);return signed_at_or_below();
    }
    bool at_or_below_high() {
        compare_key(0x104723u,0x1700056cu);step(0x104729u);return signed_at_or_below();
    }
    bool matches_exact() {compare_key(0x10472fu,0x1700056fu);step(0x104735u);return s.flags.zf;}
    bool matches_extended() {compare_key(0x10473bu,0x1d0007bau);step(0x104741u);return s.flags.zf;}
};

template<bool Literal,bool Ordered=false,class State> Route run_source_dispatch(State& state) {
    SourceDispatch<Literal,State,Ordered> view{state};view.open_object();return select_initial_route(view);
}
} // namespace halo2::native_object_ratio_caller_detail
