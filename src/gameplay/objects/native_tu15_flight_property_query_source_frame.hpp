#pragma once
#include "halo2/native_tu15_flight_property_query_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_flight_property_query_detail {
using namespace physics_world_runtime_detail;

// Source callbacks can change the live packet at each site. The original
// hardware frame and the retained source transport have different spill
// retirement rules: source w() calls the observer before the modeled pop.
template<bool Literal,bool Fresh,bool DeathCurrent,class State,class Spill> class SourceFrame {
    static_assert(!Fresh||!DeathCurrent);
    State& s;
    Spill& spill_word;
    void step(std::uint32_t pc) {
        if constexpr(Literal)s.instruction_literal(pc);
        else if(s.services.instruction)s.services.instruction(pc);
    }
    void clear_c1() {if constexpr(Fresh)s.fp.status&=~0x200u;}
    std::uint32_t selector(std::uint32_t pc,std::uint32_t original) {
        if constexpr(DeathCurrent)return original;
        else return source_object_function_compiled_symbol_tu15(s.memory,pc,original);
    }
    std::uint32_t finish(std::uint32_t pop_site,std::uint32_t return_site) {
        step(pop_site);s.set(6,s.pop(),4,0);
        step(return_site);s.pop();s.gp[4]+=16u;return s.gp[0];
    }
public:
    SourceFrame(State& state,Spill& spill_policy):s(state),spill_word(spill_policy) {}

    Selection select() {
        step(0xe0900u);s.set(0,r(s.memory,4u+s.gp[4],4),4,0);
        step(0xe0904u);s.set(1,r(s.memory,0x4ecbd0u,4),4,0);
        step(0xe090au);s.set(2,r(s.memory,0x44u+s.gp[1],4),4,0);
        step(0xe090du);s.xmm[2]=_mm_xor_ps(s.xmm[2],s.xmm[2]);
        step(0xe0910u);s.xmm[1]=_mm_set_ss(f(r(s.memory,0x46a0d0u,4)));
        step(0xe0918u);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0xe091du);s.set(0,s.gp[0]+s.gp[0]*2u,4,0);
        step(0xe0920u);s.set(1,r(s.memory,8u+s.gp[2]+s.gp[0]*4u,4),4,0);
        step(0xe0924u);s.set(0,r(s.memory,s.gp[1],4),4,0);
        step(0xe0926u);s.set(2,r(s.memory,0x4f0414u,4),4,0);
        step(0xe092cu);s.set(0,s.flags.logic(s.reg(0,4,0)&0xffffu,4),4,0);
        step(0xe0931u);s.set(0,s.flags.shift(s.reg(0,4,0),4u,4),4,0);
        step(0xe0934u);s.set(2,r(s.memory,8u+s.gp[0]+s.gp[2],4),4,0);
        step(0xe0938u);s.push(0xe0938u,s.reg(6,4,0));
        step(0xe0939u);s.set(6,r(s.memory,0xcu+s.gp[4],4),4,0);
        step(0xe093du);s.set(0,s.flags.logic(0u,1),1,0);
        step(0xe093fu);s.flags.minus(s.reg(6,4,0),selector(0xe093fu,0x6000542u),4);
        step(0xe0945u);s.xmm[0]=s.xmm[2];
        step(0xe0948u);if(s.flags.zf)return Selection::flight;
        step(0xe094au);s.flags.minus(s.reg(6,4,0),0x70000c9u,4);
        step(0xe0950u);if(s.flags.zf)return Selection::extra_flag;
        step(0xe0952u);s.flags.minus(s.reg(6,4,0),selector(0xe0952u,0xc0005a4u),4);
        step(0xe0958u);return s.flags.zf?Selection::speed:Selection::unsupported;
    }
    std::uint32_t unsupported() {return finish(0xe0a05u,0xe0a06u);}
    void flight_value() {
        step(0xe09c1u);s.flags.minus(r(s.memory,0x3dcu+s.gp[1],1),2u,1);
        step(0xe09c8u);if(s.flags.zf){step(0xe09cau);s.xmm[0]=s.xmm[1];}
    }
    bool flag_active() {
        step(0xe09abu);s.set(2,r(s.memory,0x39du+s.gp[1],1),1,0);
        step(0xe09b1u);s.flags.logic(s.reg(2,1,0),1);
        step(0xe09b3u);return !s.flags.cf&&!s.flags.zf;
    }
    void flag_value() {
        step(0xe09b5u);s.xmm[0]=_mm_set_ss(f(r(s.memory,0x46a0d0u,4)));
        step(0xe09bdu);s.set(0,1u,1,0);
        step(0xe09bfu);
        step(0xe09deu);s.xmm[1]=s.xmm[0];
    }
    bool speed_enabled() {
        step(0xe095eu);s.xmm[0]=_mm_set_ss(f(r(s.memory,0x2dcu+s.gp[2],4)));
        step(0xe0966u);s.flags.scalar_compare(s.xmm[0],s.xmm[2],false);
        step(0xe0969u);return !s.flags.cf&&!s.flags.zf;
    }
    void speed_value() {
        step(0xe096fu);s.fp.push(ext(f(r(s.memory,0x90u+s.gp[1],4))));clear_c1();
        step(0xe0975u);s.fp.push(ext(f(r(s.memory,0x8cu+s.gp[1],4))));clear_c1();
        step(0xe097bu);s.fp.push(ext(f(r(s.memory,0x88u+s.gp[1],4))));clear_c1();
        step(0xe0981u);s.fp.push(s.fp.st(0));clear_c1();
        step(0xe0983u);s.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));
        step(0xe0985u);s.fp.push(s.fp.st(2));clear_c1();
        step(0xe0987u);s.fp.st(0)=mul(s.fp.st(0),s.fp.st(3));
        step(0xe0989u);s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
        step(0xe098bu);s.fp.push(s.fp.st(3));clear_c1();
        step(0xe098du);s.fp.st(0)=mul(s.fp.st(0),s.fp.st(4));
        step(0xe098fu);s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
        step(0xe0991u);s.fp.st(0)=root(s.fp.st(0));
        step(0xe0993u);s.fp.st(3)=s.fp.st(0);clear_c1();s.fp.pop();
        step(0xe0995u);s.fp.st(0)=s.fp.st(0);clear_c1();s.fp.pop();
        step(0xe0997u);s.fp.st(0)=s.fp.st(0);clear_c1();s.fp.pop();
        step(0xe0999u);s.fp.st(0)=divide(s.fp.st(0),ext(f(r(s.memory,0x2dcu+s.gp[2],4))));
        step(0xe099fu);
        if constexpr(Fresh||DeathCurrent)w(s.memory,s.observe,0xe099fu,8u+s.gp[4],4,spill_word(s.fp.st(0),s.fp.status));
        else w(s.memory,s.observe,0xe099fu,8u+s.gp[4],4,u(spill(s.fp.st(0))));
        s.fp.pop();
        step(0xe09a3u);s.xmm[0]=_mm_set_ss(f(r(s.memory,8u+s.gp[4],4)));
        step(0xe09a9u);
    }
    void clamp() {
        step(0xe09cdu);s.flags.scalar_compare(s.xmm[2],s.xmm[0],false);
        step(0xe09d0u);s.set(0,1u,1,0);
        step(0xe09d2u);
        if(!s.flags.cf&&!s.flags.zf){step(0xe09d4u);s.xmm[1]=s.xmm[2];step(0xe09d7u);return;}
        step(0xe09d9u);s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
        step(0xe09dcu);
        if(s.flags.cf||s.flags.zf){step(0xe09deu);s.xmm[1]=s.xmm[0];}
    }
    std::uint32_t publish() {
        step(0xe09e1u);s.flags.scalar_compare(s.xmm[0],s.xmm[2],false);
        step(0xe09e4u);s.set(1,r(s.memory,0x10u+s.gp[4],4),4,0);
        step(0xe09e8u);w(s.memory,s.observe,0xe09e8u,s.gp[1],4,u(_mm_cvtss_f32(s.xmm[1])));
        step(0xe09ecu);
        if(s.flags.cf||s.flags.zf){
            step(0xe09fdu);s.set(2,r(s.memory,0x14u+s.gp[4],4),4,0);
            step(0xe0a01u);s.set(1,s.flags.logic(0u,4),4,0);
            step(0xe0a03u);w(s.memory,s.observe,0xe0a03u,s.gp[2],1,s.reg(1,1,0));
            return unsupported();
        }
        step(0xe09eeu);s.set(2,r(s.memory,0x14u+s.gp[4],4),4,0);
        step(0xe09f2u);s.set(1,1u,4,0);
        step(0xe09f7u);w(s.memory,s.observe,0xe09f7u,s.gp[2],1,s.reg(1,1,0));
        return finish(0xe09f9u,0xe09fau);
    }
};

template<bool Literal,bool Fresh,bool DeathCurrent=false,class State,class Spill>
std::uint32_t run_source_frame(State& state,Spill spill_policy) {
    SourceFrame<Literal,Fresh,DeathCurrent,State,Spill> view{state,spill_policy};return query(view);
}
} // namespace halo2::native_flight_property_query_detail
