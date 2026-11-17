#pragma once
#include "halo2/native_tu15_object_ratio_caller_middle_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_high_source_frame.hpp"

namespace halo2::native_object_ratio_caller_detail {
template<bool Literal,bool Ordered,class State,class Children>
class SourceMiddleFrame:public SourceRatioFrame<Literal,State> {
    using Base=SourceRatioFrame<Literal,State>;
    using Base::s;using Base::step;using Base::read;using Base::compare_key;
    Children& children;
public:
    SourceMiddleFrame(State& state,Children& calls):Base{state},children(calls) {}

    bool first_key() {step(0x104adfu);return s.flags.zf;}
    void compare_upper_key() {compare_key(0x104ae5u,0x1400056du);}
    bool above_upper() {step(0x104aebu);return !s.flags.zf&&s.flags.sf==s.flags.of;}
    bool equal_upper() {step(0x104af1u);return s.flags.zf;}
    bool matches_record_key() {compare_key(0x104af7u,0x11000575u);step(0x104afdu);return s.flags.zf;}
    bool matches_second_key() {compare_key(0x104affu,0x1200056au);step(0x104b05u);return s.flags.zf;}
    bool matches_mode_key() {compare_key(0x104b07u,0x120006e4u);step(0x104b0du);return s.flags.zf;}

    bool has_record_index() {
        auto& g=s.gp;
        step(0x104b50u);g[1]=read(g[3]+0x2c8u);
        step(0x104b56u);g[0]=s.flags.logic(0u,4);
        compare_key(0x104b58u,0x0f000574u);
        step(0x104b5eu);s.set(0,!s.flags.zf,1,0);
        step(0x104b61u);s.flags.minus(g[0],g[1],4);
        step(0x104b63u);return s.flags.sf!=s.flags.of;
    }
    void call_record_ratio() {
        auto& g=s.gp;
        step(0x104b69u);s.push(0x104b69u,g[0]);
        step(0x104b6au);g[0]=g[7];
        step(0x104b6cu);s.push(0x104b6cu,0x104b71u);children.record_ratio(s);
        step(0x104b71u);s.xmm[2]=s.xmm[0];step(0x104b74u);
    }
    bool sample_signed_ratio() {
        auto& g=s.gp;auto& x=s.xmm;
        step(0x104b79u);g[0]=read(g[3]+0x2c0u);
        step(0x104b7fu);g[1]=s.flags.logic(0u,4);
        compare_key(0x104b81u,0x1200056au);
        step(0x104b87u);s.set(1,!s.flags.zf,1,0);
        step(0x104b8au);s.flags.minus(g[1],g[0],4);
        step(0x104b8cu);if(s.flags.sf==s.flags.of)return false;
        step(0x104b92u);g[6]=read(g[3]+0x2c4u);
        step(0x104b98u);g[0]=g[1];
        step(0x104b9au);g[0]=s.flags.product(g[0],0x5cu);
        step(0x104b9du);g[0]=s.flags.plus(g[0],g[6],4);
        step(0x104b9fu);s.set(0,read(g[0]+0xau,2),2,0);
        step(0x104ba3u);s.flags.logic(s.reg(0,2,0),2);
        step(0x104ba6u);if(s.flags.zf)return false;
        step(0x104bacu);shift_definition_index<Ordered>(s);
        step(0x104bafu);g[1]=(read(g[1]+g[5]+0x22cu,2)^0x8000u)-0x8000u;
        step(0x104bb7u);g[2]=(s.reg(0,2,0)^0x8000u)-0x8000u;
        step(0x104bbau);x[2]=_mm_cvtsi32_ss(x[2],si(g[1]));
        step(0x104bbeu);x[0]=_mm_cvtsi32_ss(x[0],si(g[2]));
        step(0x104bc2u);x[2]=_mm_div_ss(x[2],x[0]);
        step(0x104bc6u);return true;
    }
};
} // namespace halo2::native_object_ratio_caller_detail
