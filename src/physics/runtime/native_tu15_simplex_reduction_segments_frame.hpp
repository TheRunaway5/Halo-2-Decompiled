#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_segments_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_reduction_segments_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    static void subtract(__m128& a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");}
    bool test(unsigned bit){s_.flags.logic(s_.gp[0]&bit,1u);return !s_.flags.zf;}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void call_closest_segments() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x30ea8eu,0u,g[6]+0x20u);io_.vector(0x30ea92u,1u,g[6]+0x30u);
        g[2]=g[6]+0x130u;io_.push(0x30ea9cu,g[2]);g[1]=g[4]+0x54u;io_.push(0x30eaa1u,g[1]);
        g[0]=g[6]+0xa0u;g[3]=g[6]+0x20u;subtract(x[1],x[0]);
        io_.vector(0x30eaaeu,0u,g[0]);io_.push(0x30eab1u,g[0]);g[2]=g[4]+0x6cu;
        io_.store_vector(0x30eab6u,g[4]+0x6cu,x[1]);
        io_.vector(0x30eabbu,1u,g[6]+0xb0u);io_.push(0x30eac2u,g[2]);subtract(x[1],x[0]);
        io_.push(0x30eac6u,g[3]);io_.store_vector(0x30eac7u,g[4]+0x64u,x[1]);
        io_.call(0x30eaccu,0x30ead1u,0x312620u);g[4]=s_.flags.plus(g[4],0x14u,4u);
    }
    bool has_endpoint_reduction(){s_.flags.logic(s_.gp[0],4u);return !s_.flags.zf;}
    void reduce_first_segment(){
        auto& g=s_.gp;
        if(test(1u)){
            io_.vector(0x30eae0u,0u,g[6]+0x30u);io_.store_vector(0x30eae4u,g[3],s_.xmm[0]);
        }else if(!test(2u))return;
        io_.store(0x30eaedu,g[6],1u);
    }
    void reduce_second_segment(){
        auto& g=s_.gp;auto& x=s_.xmm;
        if(test(4u)){
            io_.vector(0x30eaf7u,0u,g[6]+0xb0u);io_.store_vector(0x30eafeu,g[6]+0xa0u,x[0]);
            io_.vector(0x30eb05u,0u,g[6]+0xf0u);io_.store_vector(0x30eb0cu,g[6]+0xe0u,x[0]);
            io_.store(0x30eb13u,g[7],1u);
        }else if(test(8u))io_.store(0x30eb26u,g[7],1u);
    }
};
template<class State,class Call,class Before> NativeTu15SimplexHullSegmentsResume run_private_reduction_segments(
    State& s,Call call,Before before){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private simplex segment reduction requires supported actual TU1.5 with free x87 slots");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_simplex_hull_segments_kernel(view);
}
} // namespace halo2::native_simplex_reduction_segments_detail
#endif
