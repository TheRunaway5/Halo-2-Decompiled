#pragma once
#include "physics/runtime/native_tu15_solver_hull_seed_route_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_seed_transition_kernel.hpp"

namespace halo2::native_solver_hull_seed_transition_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void load(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool initialize() {
        auto& g=s_.gp;
        load(0x313db2u,2u,g[5]+8u);io_.push(0x313db5u,g[2]);g[0]=g[4]+35u;
        io_.push(0x313dbau,g[0]);g[1]=g[6];io_.call(0x313dbdu,0x313dc2u,0x313560u);
        s_.flags.minus(io_.byte(0x313dc2u,g[0]),0u,1u);
        if(!s_.flags.zf)return false;
        g[7]=s_.flags.logic(0u,4u);return true;
    }
    void append_line() {
        auto& g=s_.gp;g[1]=g[6];g[7]=1u;io_.call(0x313dd6u,0x313ddbu,0x3136a0u);
    }
    void improve_basis() {
        auto& g=s_.gp;g[1]=g[6];g[7]=2u;io_.call(0x313de7u,0x313decu,0x313790u);
    }
    bool reduce_triangle() {
        auto& g=s_.gp;
        load(0x313df1u,2u,g[6]+0x50u);g[1]=g[4]+0xb0u;
        io_.push(0x313dfbu,g[1]);io_.push(0x313dfcu,g[2]);g[0]=g[4]+0x68u;io_.push(0x313e01u,g[0]);
        g[1]=g[6];g[7]=3u;io_.call(0x313e09u,0x313e0eu,0x313470u);
        io_.load_float(0x313e0eu,g[4]+0xe0u);io_.compare_float(0x313e15u,0x46a180u);
        g[0]=(g[0]&0xffff0000u)|io_.status();s_.flags.logic((g[0]>>8u)&5u,1u);
        return !s_.flags.pf;
    }
    void append_triangle_vertex() {
        auto& g=s_.gp;auto& x=s_.xmm;
        load(0x313e26u,1u,g[6]+0x48u);load(0x313e29u,2u,g[1]+0x80u);load(0x313e2fu,3u,g[6]+0x50u);
        io_.vector(0x313e32u,0u,g[4]+0xd0u);io_.vector(0x313e3au,1u,g[4]+0xb0u);
        g[2]=s_.flags.shift(g[2],4u,4u);g[2]=s_.flags.plus(g[2],g[3],4u);
        asm volatile("subps %1,%0":"+x"(x[1]):"x"(x[0]):"memory");
        io_.store_vector(0x313e4au,g[2],x[1]);
        load(0x313e4du,0u,g[6]+0x48u);load(0x313e50u,1u,g[0]+0x80u);io_.vector(0x313e56u,0u,g[4]+0xb0u);
        io_.shift_index(1u);io_.store_vector(0x313e61u,g[0]+g[1],x[0]);
        load(0x313e65u,0u,g[6]+0x48u);load(0x313e68u,2u,g[0]+0x80u);io_.vector(0x313e6eu,0u,g[4]+0xc0u);
        g[2]=s_.flags.plus(g[2],4u,4u);io_.shift_index(2u);io_.store_vector(0x313e7cu,g[0]+g[2],x[0]);
        load(0x313e80u,0u,g[6]+0x48u);io_.increment(0x313e83u,g[0]+0x80u);
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_solver_hull_seed_route_detail::PhysicalIo<State,Before> {
    using Base=native_solver_hull_seed_route_detail::PhysicalIo<State,Before>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    State& state_;
    native_raw_x87_detail::Scope<State>& fp_;
    Call& call_;
public:
    using Foundation::push;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,fp},state_(s),fp_(fp),call_(call){}
    std::uint32_t byte(std::uint32_t pc,std::uint32_t address){Foundation::at(pc);return Foundation::word(address,1u);}
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target) {
        push(site,returned);fp_.child([&]{call_(state_,target,site);});
    }
    void increment(std::uint32_t pc,std::uint32_t address) {
        Foundation::at(pc);auto flags=state_.flags;const auto value=flags.increment(Foundation::word(address),4u,false);
        Foundation::store(pc,address,value,[&]{state_.flags=flags;},true);
    }
    void shift_index(unsigned reg) {
        const auto context=state_.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);
        auto value=state_.gp[reg];unsigned overflow{};
        asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+&a"(flags),"=c"(overflow),"+&r"(value)::"cc");
        state_.gp[reg]=value;state_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));
    }
};
template<class State,class Call,class Before> NativeTu15SolverHullSeedResume run_private_transition(
    State& s,NativeTu15SolverHullSeedExit entry,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull seed transition requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_solver_hull_seed_transition_kernel(view,entry);
}
} // namespace halo2::native_solver_hull_seed_transition_detail
#endif
