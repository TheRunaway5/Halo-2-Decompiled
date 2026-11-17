#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_segments_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_segments_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    static void subtract(__m128& a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");}
    bool test(unsigned bit){s_.flags.logic(s_.gp[0]&bit,1u);return !s_.flags.zf;}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void call_closest_segments() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x30e2afu,0u,g[6]+0x20u);io_.vector(0x30e2b3u,1u,g[6]+0x30u);
        g[1]=g[6]+0x130u;io_.push(0x30e2bdu,g[1]);g[3]=g[6]+0x20u;subtract(x[1],x[0]);
        io_.vector(0x30e2c4u,0u,g[6]+0xa0u);g[2]=g[4]+0xc4u;io_.push(0x30e2d2u,g[2]);g[0]=g[6]+0xa0u;
        io_.push(0x30e2d9u,g[0]);g[0]=g[4]+0xbcu;io_.store_vector(0x30e2e1u,g[4]+0xbcu,x[1]);
        io_.vector(0x30e2e9u,1u,g[6]+0xb0u);io_.push(0x30e2f0u,g[0]);subtract(x[1],x[0]);
        io_.push(0x30e2f4u,g[3]);io_.store_vector(0x30e2f5u,g[4]+0xd4u,x[1]);
        io_.call(0x30e2fdu,0x30e302u,0x312620u);g[4]=s_.flags.plus(g[4],0x14u,4u);
    }
    bool has_endpoint_reduction(){s_.flags.logic(s_.gp[0],4u);return !s_.flags.zf;}
    void reduce_first_segment() {
        auto& g=s_.gp;
        if(test(1u)){
            io_.vector(0x30e311u,0u,g[6]+0x30u);io_.store_vector(0x30e315u,g[3],s_.xmm[0]);
        }else if(!test(2u))return;
        io_.store(0x30e31eu,g[6],1u);
    }
    void reduce_second_segment() {
        auto& g=s_.gp;auto& x=s_.xmm;
        if(test(4u)){
            io_.vector(0x30e328u,0u,g[6]+0xb0u);io_.store_vector(0x30e32fu,g[6]+0xa0u,x[0]);
            io_.vector(0x30e336u,0u,g[6]+0xf0u);io_.store_vector(0x30e33du,g[6]+0xe0u,x[0]);
            io_.store(0x30e344u,g[7],1u);
        }else if(test(8u))io_.store(0x30e357u,g[7],1u);
    }
};
template<class State,class Call,class Before> NativeTu15SimplexHullSegmentsResume run_private_hull_segments(
    State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private simplex hull segments requires supported actual TU1.5 with free x87 slots");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_simplex_hull_segments_kernel(view);
}
} // namespace halo2::native_simplex_hull_segments_detail
#endif
