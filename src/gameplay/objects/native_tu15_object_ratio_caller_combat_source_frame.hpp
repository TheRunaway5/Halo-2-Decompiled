#pragma once
#include "halo2/native_tu15_object_ratio_caller_combat_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_high_source_frame.hpp"

namespace halo2::native_object_ratio_caller_detail {
template<bool Literal,class State,class Children> class SourceCombatFrame:public SourceRatioFrame<Literal,State> {
    using Base=SourceRatioFrame<Literal,State>;
    using Base::s;using Base::step;using Base::read;using Base::scalar;using Base::compare_key;
    Children& children;

    bool equal(std::uint32_t pc) {step(pc);return s.flags.zf;}
    bool below_or_equal(std::uint32_t pc) {step(pc);return s.flags.cf||s.flags.zf;}
    bool test_parity(std::uint32_t lahf,std::uint32_t test,std::uint32_t branch) {
        step(lahf);s.set(0,s.flags.bits()|2u,1,8);
        step(test);s.flags.logic(s.reg(0,1,8)&0x44u,1);
        step(branch);return s.flags.pf;
    }
public:
    SourceCombatFrame(State& state,Children& calls):Base{state},children(calls) {}

    bool allow_ratio(bool middle) {
        auto& g=s.gp;
        step(middle?0x104b13u:0x1049eeu);g[0]=read(0x4f3218u);
        step(middle?0x104b18u:0x1049f3u);s.flags.minus(read(g[0]+8u),2u,4);
        step(middle?0x104b1cu:0x1049f7u);s.xmm[2]=s.xmm[3];
        if(!equal(middle?0x104b1fu:0x1049fau))return false;
        step(middle?0x104b25u:0x104a00u);s.flags.minus(read(g[5]+0x17eu,2),0xffffu,2);
        return !equal(middle?0x104b2du:0x104a08u);
    }
    bool has_definition_flag(bool middle) {
        step(middle?0x104b33u:0x104a0eu);s.gp[1]=s.gp[7];
        const auto pc=middle?0x104b35u:0x104a10u,ret=middle?0x104b3au:0x104a15u;
        step(pc);s.push(pc,ret);children.combat(s,0x165590u,pc);
        step(ret);s.flags.logic(s.reg(0,1,0),1);
        return !equal(middle?0x104b3cu:0x104a17u);
    }
    void call_mode_ratio(bool middle) {
        const auto argument=middle?0x104b42u:0x104a1du;
        const auto pc=middle?0x104b43u:0x104a1eu,ret=middle?0x104b48u:0x104a23u;
        step(argument);s.push(argument,s.gp[7]);
        step(pc);s.push(pc,ret);children.combat(s,0x1655f0u,pc);
    }
    void copy_ratio() {step(0x104b48u);s.xmm[2]=s.xmm[0];step(0x104b4bu);}
    void pick_low_ratio() {
        auto& x=s.xmm;
        compare_key(0x104a23u,0x0f0006e5u);
        if(equal(0x104a29u)){
            step(0x104a2bu);s.flags.scalar_compare(x[0],x[3],true);
            if(!test_parity(0x104a2eu,0x104a2fu,0x104a32u)){
                step(0x104a38u);x[2]=scalar(0x46a0d0u);step(0x104a40u);
            }
            return;
        }
        compare_key(0x104a45u,0x0e0006e6u);
        if(equal(0x104a4bu)){
            step(0x104a4du);s.flags.scalar_compare(x[0],x[3],false);
            if(below_or_equal(0x104a50u))return;
            step(0x104a56u);x[1]=scalar(0x46a0d0u);
            step(0x104a5eu);s.flags.scalar_compare(x[1],x[0],false);
            if(!below_or_equal(0x104a61u)){step(0x104a67u);x[2]=x[1];step(0x104a6au);}
            return;
        }
        compare_key(0x104a6fu,0x0d0006e7u);
        if(!equal(0x104a75u))return;
        step(0x104a7bu);x[1]=scalar(0x46a0d0u);
        step(0x104a83u);s.flags.scalar_compare(x[0],x[1],true);
        if(!test_parity(0x104a86u,0x104a87u,0x104a8au)){step(0x104a90u);x[2]=x[1];step(0x104a93u);}
    }
};
} // namespace halo2::native_object_ratio_caller_detail
