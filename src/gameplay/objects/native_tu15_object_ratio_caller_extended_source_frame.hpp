#pragma once
#include "halo2/native_tu15_object_ratio_caller_extended_kernel.hpp"
#include "halo2/native_tu15_object_ratio_caller_maximum_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_high_source_frame.hpp"

namespace halo2::native_object_ratio_caller_detail {
template<bool Literal,bool Ordered,class State,class Children>
class SourceExtendedFrame:public SourceRatioFrame<Literal,State> {
    using Base=SourceRatioFrame<Literal,State>;
    using Base::s;using Base::step;using Base::read;using Base::scalar;using Base::below_or_equal;
    Children& children;

    void sample(std::uint32_t compare,std::uint32_t branch,std::uint32_t reload,std::uint32_t address) {
        step(compare);s.flags.scalar_compare(s.xmm[2],scalar(address),false);
        step(branch);
        if(below_or_equal()){step(reload);s.xmm[2]=scalar(address);}
    }
    void keep_maximum(std::uint32_t compare,std::uint32_t branch,std::uint32_t move) {
        step(compare);s.flags.scalar_compare(s.xmm[2],s.xmm[0],false);
        step(branch);
        if(below_or_equal()){step(move);s.xmm[2]=s.xmm[0];}
    }
public:
    SourceExtendedFrame(State& state,Children& calls):Base{state},children(calls) {}

    bool allow_extended() {
        auto& g=s.gp;
        step(0x104747u);g[2]=read(0x4eabc4u);
        step(0x10474du);s.set(1,read(g[2]+0x82u,1),1,0);
        step(0x104753u);s.flags.logic(s.reg(1,1,0),1);
        step(0x104755u);return s.flags.zf;
    }
    void reload_definition() {
        auto& g=s.gp;
        step(0x10475bu);g[1]=read(g[5]);
        step(0x10475eu);g[1]=s.flags.logic(g[1]&0xffffu,4);
        step(0x104764u);shift_definition_index<Ordered>(s);
        step(0x104767u);g[3]=read(g[1]+g[0]+8u);
        step(0x10476bu);w(s.memory,s.observe,0x10476bu,g[4]+0x10u,4,g[3]);
    }
    ExtendedStage locate_initial_scan() {
        auto& g=s.gp;
        step(0x10476fu);g[2]=read(g[3]+0x2d0u);
        step(0x104775u);g[6]=s.flags.logic(0u,4);
        step(0x104777u);s.flags.minus(g[2],4u,4);
        step(0x10477au);
        if(s.flags.sf==s.flags.of)return ExtendedStage::vector_maximum;
        return has_remaining()?ExtendedStage::scalar_maximum:ExtendedStage::scan_records;
    }
    void begin_groups() {
        auto& g=s.gp;
        step(0x10477cu);g[1]=g[2]-4u;
        step(0x10477fu);g[1]=s.flags.right(g[1],2u,4,false);
        step(0x104782u);g[1]=s.flags.increment(g[1],4,false);
        step(0x104783u);g[0]=g[5]+0x1f0u;
        step(0x104789u);g[6]=g[1]*4u;
    }
    void sample_group() {
        const auto address=s.gp[0];
        sample(0x104790u,0x104794u,0x104796u,address-0x34u);
        sample(0x10479bu,0x10479eu,0x1047a0u,address);
        sample(0x1047a4u,0x1047a8u,0x1047aau,address+0x34u);
        sample(0x1047afu,0x1047b3u,0x1047b5u,address+0x68u);
    }
    bool advance_group() {
        auto& g=s.gp;
        step(0x1047bau);g[0]=s.flags.plus(g[0],0xd0u,4);
        step(0x1047bfu);g[1]=s.flags.increment(g[1],4,true);
        step(0x1047c0u);return !s.flags.zf;
    }
    bool has_remaining() {
        step(0x1047c2u);s.flags.minus(s.gp[6],s.gp[2],4);
        step(0x1047c4u);return s.flags.sf!=s.flags.of;
    }
    void begin_remainder() {
        auto& g=s.gp;
        step(0x1047c6u);g[0]=g[6];
        step(0x1047c8u);g[0]=s.flags.product(g[0],0x34u);
        step(0x1047cbu);g[0]+=g[5]+0x1bcu;
        step(0x1047d2u);g[2]=s.flags.minus(g[2],g[6],4);
    }
    void sample_remainder() {sample(0x1047d4u,0x1047d7u,0x1047d9u,s.gp[0]);}
    bool advance_remainder() {
        auto& g=s.gp;
        step(0x1047ddu);g[0]=s.flags.plus(g[0],0x34u,4);
        step(0x1047e0u);g[2]=s.flags.increment(g[2],4,true);
        step(0x1047e1u);return !s.flags.zf;
    }
    bool begin_record_scan() {
        auto& g=s.gp;
        step(0x1047e3u);g[3]=read(g[3]+0x2c8u);
        step(0x1047e9u);g[7]=s.flags.logic(0u,4);
        step(0x1047ebu);s.flags.logic(g[3],4);
        step(0x1047edu);w(s.memory,s.observe,0x1047edu,g[4]+0x1cu,4,g[3]);
        step(0x1047f1u);if(s.flags.zf||s.flags.sf!=s.flags.of)return false;
        step(0x1047f3u);g[1]=read(g[4]+0x10u);
        step(0x1047f7u);g[6]=read(g[1]+0x2ccu);
        step(0x1047fdu);g[6]=s.flags.plus(g[6],0x28u,4);
        step(0x104800u);g[3]=g[5]+0x20cu;
        return true;
    }
    bool has_positive_weight() {
        step(0x104806u);s.xmm[0]=scalar(s.gp[6]-0xcu);
        step(0x10480bu);s.flags.scalar_compare(s.xmm[0],s.xmm[3],false);
        step(0x10480eu);return !below_or_equal();
    }
    void sample_record() {
        auto& g=s.gp;auto& x=s.xmm;
        step(0x104810u);g[0]=read(g[4]+0x18u);
        step(0x104814u);s.push(0x104814u,g[7]);
        step(0x104815u);s.push(0x104815u,0x10481au);children.record_ratio(s);
        step(0x10481au);x[0]=_mm_mul_ss(x[0],scalar(g[6]));
        keep_maximum(0x10481eu,0x104821u,0x104823u);
    }
    bool has_secondary_term() {
        step(0x104826u);s.flags.minus(read(s.gp[3],1),2u,1);
        step(0x104829u);return s.flags.zf;
    }
    void add_secondary_term() {
        auto& g=s.gp;auto& x=s.xmm;
        step(0x10482bu);x[0]=x[4];
        step(0x10482eu);x[0]=_mm_sub_ss(x[0],scalar(g[6]));
        step(0x104832u);x[0]=_mm_mul_ss(x[0],scalar(g[5]+0x188u));
        step(0x10483au);x[0]=_mm_add_ss(x[0],scalar(g[6]));
        keep_maximum(0x10483eu,0x104841u,0x104843u);
    }
    bool advance_record() {
        auto& g=s.gp;
        step(0x104846u);g[0]=read(g[4]+0x1cu);
        step(0x10484au);g[7]=s.flags.increment(g[7],4,false);
        step(0x10484bu);g[6]=s.flags.plus(g[6],0x40u,4);
        step(0x10484eu);g[3]=s.flags.plus(g[3],0xcu,4);
        step(0x104851u);s.flags.minus(g[7],g[0],4);
        step(0x104853u);return s.flags.sf!=s.flags.of;
    }
    void include_object_term() {
        auto& g=s.gp;auto& x=s.xmm;
        step(0x104855u);g[2]=read(g[4]+0x10u);
        step(0x104859u);x[0]=scalar(g[5]+0x180u);
        step(0x104861u);x[0]=_mm_mul_ss(x[0],scalar(g[2]+0x164u));
        step(0x104869u);s.flags.scalar_compare(x[2],x[0],false);
        step(0x10486cu);
        if(below_or_equal()){step(0x104872u);x[2]=x[0];step(0x104875u);}
    }
};
} // namespace halo2::native_object_ratio_caller_detail
