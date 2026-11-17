#pragma once
#include "halo2/native_tu15_object_ratio_caller_low_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_high_frame.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_maximum_frame.hpp"
#include "gameplay/weapons/native_tu15_object_parent_mode.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class LowFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;
    Before& before_;
    native_raw_x87_detail::Scope<State>& fp_;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {at(pc);return word(address,width);}
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    bool bit_in_byte(std::uint32_t pc,std::uint32_t address,unsigned reg,unsigned shift) {
        const auto bits=read(pc,address,1);
        s_.set(reg,bits,1,0);
        const auto shifted=s_.flags.right(bits,shift,1,false);
        s_.set(reg,shifted,1,0);
        s_.flags.logic(shifted&1u,1);
        return !s_.flags.zf;
    }
    bool parent_mode(std::uint32_t pc,std::uint32_t next) {
        s_.gp[1]=s_.gp[7];
        push(pc,next);
        native_object_parent_mode_detail::run_scoped(s_,before_,fp_,native_object_parent_mode_detail::Observation::actual);
        s_.flags.logic(s_.reg(0,1,0),1);
        return !s_.flags.zf;
    }
    void lahf_test(std::uint32_t mask) {
        const auto ah=(s_.flags.bits()&0xd5u)|2u;
        s_.set(0,ah,1,8);s_.flags.logic(ah&mask,1);
    }
public:
    LowFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},before_(before),fp_(fp) {}

    bool equal() const {return s_.flags.zf;}
    bool above() const {return !s_.flags.zf&&s_.flags.sf==s_.flags.of;}
    void compare(std::uint32_t key) {s_.flags.minus(s_.gp[6],key,4);}
    bool is_key(std::uint32_t key) {compare(key);return equal();}
    void direct_flag() {
        if(parent_mode(0x1048bau,0x1048bfu))s_.xmm[2]=scalar(0x1048c7u,0x46a0d0u);
    }
    void conditional_flag() {
        if(parent_mode(0x104957u,0x10495cu)&&!bit_in_byte(0x104960u,s_.gp[5]+0x16cu,0,5u))return;
        s_.xmm[0]=scalar(0x104971u,0x46a0d0u);
        const auto other=scalar(0x104979u,s_.gp[5]+0x184u);
        s_.flags.scalar_compare(s_.xmm[0],other,false);
        if(!s_.flags.cf&&!s_.flags.zf)s_.xmm[2]=s_.xmm[0];
    }
    void sample_field(std::uint32_t pc,std::uint32_t offset) {s_.xmm[2]=scalar(pc,s_.gp[5]+offset);}
    void scaled_byte() {
        s_.gp[1]=read(0x10498eu,s_.gp[5]+0x175u,1);
        s_.xmm[2]=_mm_cvtsi32_ss(s_.xmm[2],std::int32_t(s_.gp[1]));
        const auto scale=scalar(0x104999u,0x46a0c4u);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[2]):"x"(scale));
    }
    bool compare_position() {
        auto& g=s_.gp;auto& x=s_.xmm;
        if(!bit_in_byte(0x10490fu,g[5]+0x16cu,2,1u))return false;
        x[0]=scalar(0x104920u,g[3]+0x150u);
        s_.flags.scalar_compare(x[0],x[4],true);lahf_test(0x44u);
        if(!s_.flags.pf)return false;
        x[2]=scalar(0x104935u,g[5]+0x180u);
        asm volatile("subss %1,%0":"+x"(x[2]):"x"(x[0]));
        x[0]=x[4];
        const auto threshold=scalar(0x104944u,g[3]+0x150u);
        asm volatile("subss %1,%0":"+x"(x[0]):"x"(threshold));
        asm volatile("divss %1,%0":"+x"(x[2]):"x"(x[0]));
        return true;
    }
    bool recent_record() {
        auto& g=s_.gp;
        g[1]=read(0x104a98u,g[3]+0x2d0u);
        g[0]=s_.flags.logic(0u,4);compare(0x0e000572u);s_.set(0,!equal(),1,0);
        s_.flags.minus(g[0],g[1],4);
        if(s_.flags.sf==s_.flags.of)return false;
        g[1]=read(0x104ab1u,0x51df54u);
        g[0]=s_.flags.product(g[0],0x34u);
        g[2]=read(0x104abau,g[1]+8u);
        g[1]=read(0x104abdu,g[5]+0x248u);
        s_.xmm[2]=scalar(0x104ac3u,g[0]+g[5]+0x1b4u);
        g[2]=s_.flags.minus(g[2],g[1],4);s_.flags.minus(g[2],1u,4);
        if(s_.flags.zf||s_.flags.sf!=s_.flags.of)return true;
        s_.xmm[2]=s_.xmm[3];return false;
    }
    bool sample_signed_ratio() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[0]=read(0x104b79u,g[3]+0x2c0u);
        g[1]=s_.flags.logic(0u,4);compare(0x1200056au);s_.set(1,!equal(),1,0);
        s_.flags.minus(g[1],g[0],4);
        if(s_.flags.sf==s_.flags.of)return false;
        g[6]=read(0x104b92u,g[3]+0x2c4u);
        g[0]=s_.flags.plus(s_.flags.product(g[1],0x5cu),g[6],4);
        s_.set(0,read(0x104b9fu,g[0]+0xau,2),2,0);
        const auto denominator=s_.reg(0,2,0);
        s_.flags.logic(denominator,2);
        if(s_.flags.zf)return false;
        auto index=g[1];std::uint32_t result;
        asm volatile("shll $4,%%ecx; movl %%ecx,%%edx; seto %%cl; lahf; movb %%ah,%%ch"
            :"+c"(index),"=d"(result)::"eax","cc");
        g[1]=result;s_.flags.load(((index>>8u)&0xd5u)|((index&1u)<<11u));
        g[1]=(read(0x104bafu,g[1]+g[5]+0x22cu,2)^0x8000u)-0x8000u;
        g[2]=(denominator^0x8000u)-0x8000u;
        x[2]=_mm_cvtsi32_ss(x[2],std::bit_cast<std::int32_t>(g[1]));
        x[0]=_mm_cvtsi32_ss(x[0],std::bit_cast<std::int32_t>(g[2]));
        asm volatile("divss %1,%0":"+x"(x[2]):"x"(x[0]));
        return true;
    }
    void publish_continuation(LowRoute route) {at(static_cast<std::uint32_t>(route));}
};

template<class State,class Before> void run_low_path(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    MiddleFrame<State,Before> exit{state,before,fp};
    LowFrame<State,Before> view{state,before,fp};
    const auto route=select_initial_route(entry);
    bool clamp=false;
    if(route==Route::middle){
        const auto mid=select_middle_route(exit);
        if(mid!=MiddleRoute::second_key){exit.publish_continuation(static_cast<std::uint32_t>(mid));return;}
        clamp=view.sample_signed_ratio();
    }else if(route==Route::low){
        const auto low=select_low_route(view);
        switch(low){
        case LowRoute::direct_flag:view.direct_flag();break;
        case LowRoute::conditional_flag:view.conditional_flag();break;
        case LowRoute::first_field:view.sample_field(0x1048d4u,0x180u);clamp=true;break;
        case LowRoute::second_field:view.sample_field(0x1048e1u,0x184u);clamp=true;break;
        case LowRoute::third_field:view.sample_field(0x1048eeu,0x18cu);clamp=true;break;
        case LowRoute::comparison:clamp=view.compare_position();break;
        case LowRoute::scaled_byte:view.scaled_byte();clamp=true;break;
        case LowRoute::recent_record:clamp=view.recent_record();break;
        case LowRoute::record_ratio:run_record_branch(exit);return;
        case LowRoute::alternate_extended:{
            ExtendedFrame<State,Before> extended{state,before,fp};
            const auto next=extended.locate_initial_scan();
            if(next!=ExtendedStage::scan_records){
                MaximumFrame<State,Before> maximum{state,before,fp};scan_initial_maximum(maximum,next);
            }
            scan_extended_records(extended);clamp=true;break;
        }
        case LowRoute::unknown:{
            HighFrame<State,Before> high{state,before,fp};high.finish_unknown();return;
        }
        case LowRoute::combat_ratio:view.publish_continuation(low);return;
        }
    }else{entry.publish_continuation(route);return;}
    if(clamp)exit.clamp_result();
    exit.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif
