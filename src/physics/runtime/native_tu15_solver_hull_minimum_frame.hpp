#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_minimum_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"

namespace halo2::native_solver_hull_minimum_detail {
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};

// Private actual3131B0 physical view. The saved-CX slot is also the
// source's rounded-candidate scratch and is read live by the final POP.
template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::push;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;
    std::uint32_t read(std::uint32_t pc,std::uint32_t address){at(pc);return word(address);}
    void load_float(std::uint32_t pc,std::uint32_t address){
        const auto bits=read(pc,address);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void drop(){asm volatile("fstp %%st(0)":::"memory","st");fp_.publish();}
    void save_candidate(std::uint32_t pc,std::uint32_t address){
        at(pc);const auto pending=fp_.stage_single(false);
        Base::store(pc,address,pending.bits,[&]{fp_.commit(pending);},true);
    }
    bool promote(){
        std::uint16_t status{};
        asm volatile("fcomp %%st(1);fnstsw %0":"=a"(status)::"memory","st");fp_.publish();
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;
        s_.flags.logic((s_.gp[0]>>8u)&5u,1u);
        return !s_.flags.pf;
    }
    void shift_batch_count(){
        const auto context=s_.flags.bits();
        auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
        auto count=s_.gp[0];unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;shrl $2,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"=c"(overflow),"+&r"(count)::"cc");
        s_.gp[0]=count;s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    }
public:
    Frame(State&s,Before&before,native_raw_x87_detail::Scope<State>&fp)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp){}
    void begin(){
        auto&g=s_.gp;push(0x3131b0u,g[1]);
        g[2]=read(0x3131b1u,g[1]+0x1b0u);g[1]=read(0x3131b7u,g[1]+0x1b4u);
        load_float(0x3131bdu,g[2]+0x10u);
        g[1]=s_.flags.plus(g[1],0xfffffffeu,4u);g[0]=g[1]+1u;s_.flags.minus(g[0],4u,4u);
        push(0x3131c9u,g[6]);g[6]=g[2];
    }
    bool begin_batches(){
        if(s_.flags.sf!=s_.flags.of)return false;
        auto&g=s_.gp;shift_batch_count();push(0x3131d5u,g[7]);g[7]=g[0];
        g[0]=s_.flags.minus(0u,g[0],4u);g[1]+=g[0]*4u;return true;
    }
    void batch_candidate(unsigned slot){
        auto&g=s_.gp;const auto candidate=g[2]+(slot+1u)*0x50u;
        load_float(std::array{0x3131e0u,0x3131f9u,0x313218u,0x313237u}[slot],candidate+0x10u);
        save_candidate(std::array{0x3131e3u,0x3131ffu,0x31321eu,0x31323du}[slot],g[4]+8u);
        if(promote()){
            drop();g[6]=candidate;
            load_float(std::array{0x3131f5u,0x313214u,0x313233u,0x313252u}[slot],g[4]+8u);
        }
    }
    bool next_batch(){
        auto&g=s_.gp;g[2]=s_.flags.plus(g[2],0x140u,4u);g[7]=s_.flags.increment(g[7],4u,true);
        return !s_.flags.zf;
    }
    void end_batches(){s_.gp[7]=pop(0x31325fu);}
    bool begin_tail(){
        auto&g=s_.gp;s_.flags.logic(g[1],4u);
        if(s_.flags.sf!=s_.flags.of)return false;
        g[1]=s_.flags.increment(g[1],4u,false);return true;
    }
    void tail_candidate(){
        auto&g=s_.gp;load_float(0x313265u,g[2]+0x60u);
        g[2]=s_.flags.plus(g[2],0x50u,4u);save_candidate(0x31326bu,g[4]+4u);
        if(promote()){drop();g[6]=g[2];load_float(0x31327cu,g[4]+4u);}
    }
    bool next_tail(){s_.gp[1]=s_.flags.increment(s_.gp[1],4u,true);return !s_.flags.zf;}
    void finish(){
        auto&g=s_.gp;g[0]=g[6];drop();g[6]=pop(0x313287u);g[1]=pop(0x313288u);pop(0x313289u);
    }
};
template<class State,class Before=Noop>void run_private_owner(State&s,Before before={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private hull minimum requires dense masked FP with two free slots");
    native_raw_x87_detail::Scope fp{s};Frame<State,Before>view{s,before,fp};
    native_tu15_solver_hull_minimum_kernel(view);
}
} // namespace halo2::native_solver_hull_minimum_detail
#endif
