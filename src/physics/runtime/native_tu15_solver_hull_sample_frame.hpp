#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_sample_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_solver_hull_sample_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Private hardware view, independent of the generated software x87 owner.
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address){at(pc);return word(address);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){Base::store(pc,address,value,[]{});}
    void product(unsigned reg){
        const auto incoming=s_.flags.bits();
        auto flags=((incoming&0xd5u)<<8u)|((incoming>>11u)&1u);
        std::uint32_t value=s_.gp[reg],overflow;
        // The unchanged i386 packet exposes incoming flag context at IMUL
        // even though several post-IMUL bits are architecturally undefined.
        asm volatile("addb $127,%%al;sahf;imull %3,%2,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"=c"(overflow),"+&r"(value):"i"(native_solver_hull_sample_multiplier):"cc");
        s_.gp[reg]=value;s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    }
    void next(unsigned out,unsigned input){
        s_.gp[out]=s_.flags.minus(s_.gp[out],s_.gp[input],4u);
        s_.gp[out]=s_.flags.logic(s_.gp[out]&native_solver_hull_sample_mask,4u);
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin_range(){
        auto&g=s_.gp;g[1]=read(0x313300u,g[4]+4u);
        const auto upper=read(0x313304u,g[4]+0xcu);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(upper):"memory","st");fp_.publish();
        const auto lower=read(0x313308u,g[4]+8u);
        asm volatile("fsubs %0"::"m"(lower):"memory","st");fp_.publish();
    }
    void begin_seed(){
        auto&g=s_.gp;product(1u);g[2]=read(0x313312u,g[4]+0x10u);
        g[0]=native_solver_hull_sample_offset;next(0u,1u);
        store(0x313322u,g[4]+4u,g[0]);product(0u);
    }
    void scale_sample(unsigned axis){
        const auto integer=read(std::array{0x31332cu,0x313355u,0x313378u}[axis],s_.gp[4]+4u);
        fp_.require_capacity(1u);asm volatile("fildl %0"::"m"(integer):"memory","st");fp_.publish();
        const auto scale=read(std::array{0x313330u,0x313359u,0x31337cu}[axis],0x41d780u);
        asm volatile("fmuls %0"::"m"(scale):"memory","st");fp_.publish();
        if(axis<2u)s_.gp[1u-axis]=native_solver_hull_sample_offset;
    }
    void add_lower(unsigned axis){
        const auto lower=read(std::array{0x31333bu,0x313364u,0x313382u}[axis],s_.gp[4]+8u);
        asm volatile("fadds %0"::"m"(lower):"memory","st");fp_.publish();
        if(axis<2u)next(1u-axis,axis);
    }
    void scale_range(){asm volatile("fmul %%st(1),%%st(0)":::"memory","st");fp_.publish();}
    void advance_seed(unsigned axis){
        const auto reg=1u-axis;store(axis?0x313371u:0x313349u,s_.gp[4]+4u,s_.gp[reg]);
        if(!axis)product(1u);
    }
    void publish_sample(unsigned axis){
        const auto pc=std::array{0x313353u,0x313375u,0x313388u}[axis];
        at(pc);const auto pending=fp_.stage_single(true);
        Base::store(pc,s_.gp[2]+axis*4u,pending.bits,[&]{fp_.commit(pending);},true);
    }
    void finish(){
        asm volatile("fstp %%st(0)":::"memory","st");fp_.publish();
        pop(0x31338du);s_.gp[4]+=16u;
    }
};
template<class State,class Before=Noop>void run_private_owner(State&s,Before before={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private hull sample requires dense masked FP with two free slots");
    native_raw_x87_detail::Scope fp{s};Frame<State,Before>view{s,before,fp};
    native_tu15_solver_hull_sample_kernel(view);
}
} // namespace halo2::native_solver_hull_sample_detail
#endif
