#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_search_iteration_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_solver_hull_search_iteration_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void subtract(unsigned to,unsigned from) {
        auto& x=s_.xmm;
        asm volatile("subps %1,%0":"+x"(x[to]):"x"(x[from]):"memory");
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void closest_support_pair() {
        auto& g=s_.gp;
        g[1]=g[4]+0x820u;io_.call(0x313f6au,0x313f6fu,0x3131b0u);
        g[7]=g[0];g[0]=g[4]+0x80u;io_.push(0x313f78u,g[0]);io_.push(0x313f79u,g[7]);
        g[1]=g[6];io_.call(0x313f7cu,0x313f81u,0x313390u);
    }
    bool axes_cross_threshold() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x313f81u,1u,g[4]+0xa0u);io_.vector(0x313f89u,0u,g[4]+0x80u);
        subtract(0u,1u);io_.store_vector(0x313f94u,g[4]+0x20u,x[0]);
        g[0]=io_.word(0x313f99u,g[7]+0x14u);io_.vector(0x313f9cu,1u,g[0]);
        g[0]=io_.word(0x313f9fu,g[7]+0x24u);x[2]=x[0];subtract(2u,1u);
        io_.store_vector(0x313fa8u,g[4]+0xf0u,x[2]);
        io_.vector(0x313fb0u,1u,g[0]);g[0]=io_.word(0x313fb3u,g[7]+0x34u);x[2]=x[0];subtract(2u,1u);
        io_.push(0x313fbcu,g[7]);g[1]=g[4]+0xf4u;io_.store_vector(0x313fc4u,g[4]+0x104u,x[2]);
        io_.vector(0x313fccu,1u,g[0]);io_.push(0x313fcfu,g[1]);subtract(0u,1u);
        g[1]=g[4]+0x78u;io_.store_vector(0x313fd7u,g[4]+0x118u,x[0]);
        io_.call(0x313fdfu,0x313fe4u,0x2e47b0u);
        io_.vector(0x313fe4u,1u,0x46e480u);io_.vector(0x313febu,0u,g[4]+0x70u);
        asm volatile("cmpps $1,%1,%0":"+x"(x[0]):"x"(x[1]):"memory");
        g[0]=static_cast<std::uint32_t>(_mm_movemask_ps(x[0]));s_.flags.logic(g[0]&7u,1u);
        return !s_.flags.zf;
    }
    bool find_adjacent_facet() {
        auto& g=s_.gp;
        g[2]=g[4]+0x20u;io_.push(0x314003u,g[2]);io_.push(0x314004u,g[7]);
        g[0]=g[4]+0x14u;io_.push(0x314009u,g[0]);g[1]=g[4]+0x20u;io_.push(0x31400eu,g[1]);
        g[1]=g[4]+0x830u;io_.store(0x314016u,g[4]+0x24u,g[3]);
        io_.call(0x31401au,0x31401fu,0x314c40u);s_.flags.minus(g[0],g[3],4u);
        return !s_.flags.zf;
    }
    bool has_inline_room() {
        auto& g=s_.gp;
        g[2]=io_.word(0x314027u,g[4]+0x2928u);g[0]=io_.word(0x31402eu,g[4]+0x2924u);
        g[2]=s_.flags.logic(g[2]&0x7fffffffu,4u);s_.flags.minus(g[0],g[2],4u);
        g[0]=(g[0]&0xffffff00u)|(s_.flags.sf!=s_.flags.of);s_.flags.logic(g[0]&0xffu,1u);
        if(s_.flags.zf)return false;
        g[0]=io_.word(0x314048u,g[4]+0x9d8u);g[2]=io_.word(0x31404fu,g[4]+0x9d4u);
        g[1]=io_.word(0x314056u,g[4]+0x82cu);g[0]=s_.flags.logic(g[0]&0x7fffffffu,4u);
        g[0]=s_.flags.minus(g[0],g[2],4u);g[0]=s_.flags.plus(g[0],g[1],4u);
        g[1]=io_.word(0x314066u,g[4]+0x14u);s_.flags.minus(g[0],g[1],4u);
        return s_.flags.sf==s_.flags.of;
    }
    void insert_edge() {
        auto& g=s_.gp;
        g[2]=io_.word(0x314072u,g[4]+0xcu);g[0]=g[4]+0x20u;
        io_.push(0x31407au,g[0]);io_.push(0x31407bu,g[2]);io_.push(0x31407cu,g[1]);
        g[1]=g[4]+0x82cu;io_.call(0x314084u,0x314089u,0x314ed0u);
    }
    bool append_support() {
        auto& g=s_.gp;auto& x=s_.xmm;
        g[2]=io_.word(0x314089u,g[4]+0x4a4u);io_.vector(0x314090u,0u,g[4]+0x80u);
        g[1]=g[2];io_.shift_index(1u);g[1]=s_.flags.plus(g[1],io_.word(0x31409du,g[4]+0x4a0u),4u);
        g[2]=s_.flags.increment(g[2],4u,false);io_.store(0x3140a5u,g[4]+0x4a4u,g[2]);io_.store_vector(0x3140acu,g[1],x[0]);
        g[2]=io_.word(0x3140afu,g[4]+0x124u);io_.vector(0x3140b6u,0u,g[4]+0x90u);
        g[1]=g[2];io_.shift_index(1u);g[1]=s_.flags.plus(g[1],io_.word(0x3140c3u,g[4]+0x120u),4u);
        g[2]=s_.flags.increment(g[2],4u,false);s_.flags.minus(g[0],g[3],4u);
        io_.store(0x3140cdu,g[4]+0x124u,g[2]);io_.store_vector(0x3140d4u,g[1],x[0]);
        return s_.flags.zf;
    }
};

template<class State,class Call,class Before> NativeTu15SolverHullSearchResume run_private_search_iteration(
    State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull search iteration requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};
    native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_solver_hull_search_iteration_kernel(view);
}
} // namespace halo2::native_solver_hull_search_iteration_detail
#endif
