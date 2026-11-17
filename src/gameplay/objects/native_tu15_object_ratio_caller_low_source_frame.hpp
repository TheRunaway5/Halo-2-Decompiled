#pragma once
#include "halo2/native_tu15_object_ratio_caller_low_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_high_source_frame.hpp"

namespace halo2::native_object_ratio_caller_detail {
template<bool Literal,class State,class Children> class SourceLowFrame:public SourceRatioFrame<Literal,State> {
    using Base=SourceRatioFrame<Literal,State>;
    using Base::s;using Base::step;using Base::read;using Base::scalar;using Base::compare_key;
    Children& children;

    bool equal(std::uint32_t pc) {step(pc);return s.flags.zf;}
    bool above(std::uint32_t pc) {step(pc);return !s.flags.zf&&s.flags.sf==s.flags.of;}
    bool is_key(std::uint32_t pc,std::uint32_t branch,std::uint32_t key) {
        compare_key(pc,key);return equal(branch);
    }
    bool literal_key(std::uint32_t pc,std::uint32_t branch,std::uint32_t key) {
        step(pc);s.flags.minus(s.gp[6],key,4);return equal(branch);
    }
    bool bit_in_byte(std::uint32_t load,std::uint32_t shift,std::uint32_t test,std::uint32_t branch,
                     unsigned reg,unsigned count) {
        step(load);s.set(reg,read(s.gp[5]+0x16cu,1),1,0);
        step(shift);s.set(reg,s.flags.right(s.reg(reg,1,0),count,1,false),1,0);
        step(test);s.flags.logic(s.reg(reg,1,0)&1u,1);
        return !equal(branch);
    }
    bool parent_mode(std::uint32_t move,std::uint32_t call,std::uint32_t ret,std::uint32_t branch) {
        step(move);s.gp[1]=s.gp[7];
        step(call);s.push(call,ret);children.object_parent_mode(s);
        step(ret);s.flags.logic(s.reg(0,1,0),1);
        return !equal(branch);
    }
public:
    SourceLowFrame(State& state,Children& calls):Base{state},children(calls) {}

    LowRoute select() {
        if(equal(0x10487au))return LowRoute::recent_record;
        compare_key(0x104880u,0x0b0005a9u);
        if(above(0x104886u)){
            compare_key(0x1049a6u,0x0e0006e6u);
            if(above(0x1049acu)){
                if(is_key(0x1049dau,0x1049e0u,0x0f000574u))return LowRoute::record_ratio;
                return is_key(0x1049e6u,0x1049ecu,0x0f0006e5u)?LowRoute::combat_ratio:LowRoute::unknown;
            }
            if(equal(0x1049aeu))return LowRoute::combat_ratio;
            if(is_key(0x1049b0u,0x1049b6u,0x0c000569u))return LowRoute::alternate_extended;
            if(is_key(0x1049bcu,0x1049c2u,0x0d0006e7u))return LowRoute::combat_ratio;
            return is_key(0x1049c4u,0x1049cau,0x0e000572u)?LowRoute::recent_record:LowRoute::unknown;
        }
        if(equal(0x10488cu))return LowRoute::scaled_byte;
        step(0x104892u);s.flags.minus(s.gp[6],0x05000081u,4);
        if(above(0x104898u)){
            if(is_key(0x1048fbu,0x104901u,0x09000568u))return LowRoute::conditional_flag;
            return literal_key(0x104903u,0x104909u,0x0a000066u)?LowRoute::comparison:LowRoute::unknown;
        }
        if(equal(0x10489au))return LowRoute::third_field;
        if(is_key(0x10489cu,0x1048a2u,0x03000577u))return LowRoute::second_field;
        if(is_key(0x1048a4u,0x1048aau,0x04000567u))return LowRoute::first_field;
        return literal_key(0x1048acu,0x1048b2u,0x05000024u)?LowRoute::direct_flag:LowRoute::unknown;
    }
    void direct_flag() {
        if(!parent_mode(0x1048b8u,0x1048bau,0x1048bfu,0x1048c1u))return;
        step(0x1048c7u);s.xmm[2]=scalar(0x46a0d0u);step(0x1048cfu);
    }
    void conditional_flag() {
        if(parent_mode(0x104955u,0x104957u,0x10495cu,0x10495eu)
            &&!bit_in_byte(0x104960u,0x104966u,0x104969u,0x10496bu,0u,5u))return;
        auto& x=s.xmm;
        step(0x104971u);x[0]=scalar(0x46a0d0u);
        step(0x104979u);s.flags.scalar_compare(x[0],scalar(s.gp[5]+0x184u),false);
        step(0x104980u);if(s.flags.cf||s.flags.zf)return;
        step(0x104986u);x[2]=x[0];step(0x104989u);
    }
    void sample_field(std::uint32_t pc,std::uint32_t offset,std::uint32_t jump) {
        step(pc);s.xmm[2]=scalar(s.gp[5]+offset);step(jump);
    }
    void scaled_byte() {
        step(0x10498eu);s.gp[1]=read(s.gp[5]+0x175u,1);
        step(0x104995u);s.xmm[2]=_mm_cvtsi32_ss(s.xmm[2],si(s.gp[1]));
        step(0x104999u);s.xmm[2]=_mm_mul_ss(s.xmm[2],scalar(0x46a0c4u));step(0x1049a1u);
    }
    bool compare_position() {
        auto& x=s.xmm;auto& g=s.gp;
        if(!bit_in_byte(0x10490fu,0x104915u,0x104917u,0x10491au,2u,1u))return false;
        step(0x104920u);x[0]=scalar(g[3]+0x150u);
        step(0x104928u);s.flags.scalar_compare(x[0],x[4],true);
        step(0x10492bu);s.set(0,s.flags.bits()|2u,1,8);
        step(0x10492cu);s.flags.logic(s.reg(0,1,8)&0x44u,1);
        step(0x10492fu);if(!s.flags.pf)return false;
        step(0x104935u);x[2]=scalar(g[5]+0x180u);
        step(0x10493du);x[2]=_mm_sub_ss(x[2],x[0]);
        step(0x104941u);x[0]=x[4];
        step(0x104944u);x[0]=_mm_sub_ss(x[0],scalar(g[3]+0x150u));
        step(0x10494cu);x[2]=_mm_div_ss(x[2],x[0]);
        step(0x104950u);return true;
    }
    bool recent_record() {
        auto& g=s.gp;
        step(0x104a98u);g[1]=read(g[3]+0x2d0u);
        step(0x104a9eu);g[0]=s.flags.logic(0u,4);
        compare_key(0x104aa0u,0x0e000572u);
        step(0x104aa6u);s.set(0,!s.flags.zf,1,0);
        step(0x104aa9u);s.flags.minus(g[0],g[1],4);
        step(0x104aabu);if(s.flags.sf==s.flags.of)return false;
        step(0x104ab1u);g[1]=read(0x51df54u);
        step(0x104ab7u);g[0]=s.flags.product(g[0],0x34u);
        step(0x104abau);g[2]=read(g[1]+8u);
        step(0x104abdu);g[1]=read(g[5]+0x248u);
        step(0x104ac3u);s.xmm[2]=scalar(g[0]+g[5]+0x1b4u);
        step(0x104accu);g[2]=s.flags.minus(g[2],g[1],4);
        step(0x104aceu);s.flags.minus(g[2],1u,4);
        step(0x104ad1u);if(s.flags.zf||s.flags.sf!=s.flags.of)return true;
        step(0x104ad7u);s.xmm[2]=s.xmm[3];step(0x104adau);return false;
    }
};
} // namespace halo2::native_object_ratio_caller_detail
