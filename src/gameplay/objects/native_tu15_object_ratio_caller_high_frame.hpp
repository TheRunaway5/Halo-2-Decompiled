#pragma once

#include "halo2/native_tu15_object_ratio_caller_high_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_middle_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class HighFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return word(address,width);
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    std::uint32_t record_offset(std::uint32_t index) {
        // With the two authenticated indices, the real 6B C0 34 preserves
        // the CMP residue outside CF/OF at both table reads.
        return s_.flags.product(index,0x34u);
    }
public:
    HighFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}} {}

    bool exact_high_key() const {return s_.flags.zf;}
    bool subtract_high_key(std::uint32_t key) {
        s_.gp[0]=s_.gp[6];return subtract_high_offset(key);
    }
    bool subtract_high_offset(std::uint32_t value) {
        auto& eax=s_.gp[0];eax=s_.flags.minus(eax,value,4);
        return s_.flags.zf;
    }
    bool sample_record(bool second_column) {
        auto& g=s_.gp;
        g[1]=read(second_column?0x104cc5u:0x104c6au,g[3]+0x2d0u);
        g[0]=s_.flags.logic(0u,4);
        s_.flags.minus(g[6],second_column?0x1500056eu:0x14000570u,4);
        s_.set(0,!s_.flags.zf,1,0);
        s_.flags.minus(g[0],g[1],4);
        if(s_.flags.sf==s_.flags.of)return false;
        g[0]=record_offset(g[0]);
        s_.xmm[2]=scalar(second_column?0x104cddu:0x104c82u,g[0]+g[5]+(second_column?0x1b8u:0x1b4u));
        return true;
    }
    void sample_flag(bool upper_middle) {
        const auto reg=upper_middle?0u:2u;
        const auto bits=read(upper_middle?0x104be1u:0x104c4bu,s_.gp[5]+0x16eu,1);
        s_.set(reg,bits,1,0);
        const auto shifted=s_.flags.right(bits,upper_middle?1u:2u,1,false);
        s_.set(reg,shifted,1,0);
        s_.flags.logic(shifted&1u,1);
        if(!s_.flags.zf)s_.xmm[2]=scalar(upper_middle?0x104bf1u:0x104c5du,0x46a0d0u);
    }
    bool sample_clock_digit(bool tens) {
        auto& g=s_.gp;
        g[0]=read(tens?0x104c8du:0x104c1du,g[3]+0x2c0u);
        s_.flags.logic(g[0],4);
        if(s_.flags.zf||s_.flags.sf!=s_.flags.of)return false;
        const auto raw=read(tens?0x104c97u:0x104c2bu,g[5]+0x22cu,2);
        const auto extended=(raw^0x8000u)-0x8000u;
        g[0]=extended;
        if(tens){
            g[1]=extended;
            const auto product=std::int64_t{std::bit_cast<std::int32_t>(g[1])}*0x66666667;
            g[2]=std::uint32_t(std::bit_cast<std::uint64_t>(product)>>32u);
            g[2]=s_.flags.right(g[2],2u,4,true);
            g[0]=s_.flags.right(g[2],31u,4,false);
            g[0]=s_.flags.plus(g[0],g[2],4);
        }
        g[1]=10u;
        // The original CDQ/IDIV writes quotient, remainder and x86 flag residue.
        // Capture the residue before converting the remainder or reading the scale.
        auto dividend=g[0];
        std::uint32_t quotient,remainder;
        unsigned char overflow;
        asm volatile("cltd; idivl %%ecx; seto %[overflow]; movl %%eax,%[quotient]; lahf; movzbl %%ah,%%eax"
            : "+a"(dividend),"=&d"(remainder),[quotient]"=&r"(quotient),[overflow]"=&qm"(overflow)
            : "c"(10u) : "cc");
        g[0]=quotient;g[2]=remainder;
        s_.flags.load((dividend&0xd5u)|(std::uint32_t(overflow)<<11u));
        s_.xmm[2]=_mm_cvtsi32_ss(s_.xmm[2],std::bit_cast<std::int32_t>(g[2]));
        const auto scale=scalar(tens?0x104cbbu:0x104c3eu,0x46a178u);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[2]):"x"(scale));
        return true;
    }
    void finish_unknown() {
        auto& g=s_.gp;
        g[7]=pop(0x1049d0u);g[6]=pop(0x1049d1u);g[5]=pop(0x1049d2u);
        s_.set(0,s_.flags.logic(0u,1),1,0);
        g[3]=pop(0x1049d5u);g[1]=pop(0x1049d6u);pop(0x1049d7u);g[4]+=16u;
    }
};

template<class State,class Before> void run_high_path(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    const auto route=select_initial_route(entry);
    HighFrame<State,Before> high{state,before,fp};
    MiddleFrame<State,Before> middle{state,before,fp};
    auto result=HighResult::finish;
    switch(route){
    case Route::high:result=run_high_keys(high);break;
    case Route::exact:result=high.sample_record(true)?HighResult::clamp:HighResult::finish;break;
    case Route::unknown:result=HighResult::unknown;break;
    case Route::middle:
        switch(const auto mid=select_middle_route(middle);mid){
        case MiddleRoute::first_key:result=high.sample_record(true)?HighResult::clamp:HighResult::finish;break;
        case MiddleRoute::upper:result=run_upper_middle_keys(high);break;
        case MiddleRoute::unknown:result=HighResult::unknown;break;
        default:middle.publish_continuation(static_cast<std::uint32_t>(mid));return;
        }
        break;
    default:entry.publish_continuation(route);return;
    }
    if(result==HighResult::unknown){high.finish_unknown();return;}
    if(result==HighResult::clamp)middle.clamp_result();
    middle.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif
