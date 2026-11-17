#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_finish_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_finish_frame.hpp"

namespace halo2::native_simplex_hull_finish_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void point_difference() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x30e89bu,1u,g[6]+0xa0u);io_.vector(0x30e8a2u,0u,g[6]+0x20u);
        asm volatile("subps %1,%0":"+x"(x[1]):"x"(x[0]):"memory");
    }
    void publish_pair_difference(){io_.store_vector(0x30e8a9u,s_.gp[6]+0x120u,s_.xmm[1]);}
    void publish_count(){auto& g=s_.gp;g[2]=io_.word(0x30e8b0u,g[7]);io_.store(0x30e8b2u,g[6]+8u,g[2]);}
    void publish_plane_and_restore() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x30e8b5u,0u,0x461960u);io_.vector(0x30e8bcu,1u,g[4]+0x110u);g[0]=io_.word(0x30e8c4u,g[4]+0x1cu);
        g[7]=io_.pop(0x30e8c8u);x[1]=_mm_xor_ps(x[1],x[0]);io_.store_vector(0x30e8ccu,g[6]+0x120u,x[1]);
        g[6]=io_.pop(0x30e8d3u);g[3]=io_.pop(0x30e8d4u);g[4]=g[5];g[5]=io_.pop(0x30e8d7u);io_.pop(0x30e8d8u);g[4]+=16u;
    }
};
template<class State,class Before> void run_private_hull_finish(State& s,NativeTu15SimplexHullFinish entry,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex hull finish requires supported actual TU1.5 frame");
    auto unreachable=[](auto&,std::uint32_t,std::uint32_t){throw std::runtime_error("Childless simplex hull finish dispatched a child");};
    native_raw_x87_detail::Scope fp{s};native_solver_hull_search_finish_detail::PhysicalIo<State,Before,decltype(unreachable)> io{s,before,fp,unreachable};
    Schedule<State,decltype(io)> view{s,io};native_tu15_simplex_hull_finish_kernel(view,entry);
}
} // namespace halo2::native_simplex_hull_finish_detail
#endif
