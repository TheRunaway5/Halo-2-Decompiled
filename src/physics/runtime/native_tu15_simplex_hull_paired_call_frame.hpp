#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_paired_call_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_setup_frame.hpp"

namespace halo2::native_simplex_hull_paired_call_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void first_two_arguments() {
        auto& g=s_.gp;
        g[2]=io_.word(0x30e25fu,g[6]+8u);io_.push(0x30e262u,1u);io_.push(0x30e264u,g[1]);g[0]=g[4];
        s_.flags.minus(g[2],2u,4u);g[1]=(g[1]&0xffffff00u)|s_.flags.zf;
        io_.push(0x30e26du,g[7]);io_.store_byte(0x30e26eu,g[0],g[1]&0xffu);g[0]=g[6]+0xa0u;
        io_.push(0x30e276u,g[6]);io_.push(0x30e277u,g[0]);g[0]=g[6]+0x20u;
    }
    void second_two_arguments() {
        auto& g=s_.gp;
        g[2]=io_.word(0x30e27du,g[6]+8u);io_.push(0x30e280u,0xffffffffu);io_.push(0x30e282u,g[1]);g[0]=g[4];
        s_.flags.minus(g[2],2u,4u);g[2]=(g[2]&0xffffff00u)|s_.flags.zf;
        io_.push(0x30e28bu,g[6]);io_.store_byte(0x30e28cu,g[0],g[2]&0xffu);g[0]=g[6]+0x20u;
        io_.push(0x30e291u,g[7]);io_.push(0x30e292u,g[0]);g[0]=g[6]+0xa0u;
    }
    void call_paired_reducer(){auto& g=s_.gp;g[1]=g[6];io_.push(0x30e29bu,g[0]);io_.call(0x30e29cu,0x30e2a1u,0x30d830u);}
    NativeTu15SimplexHullPairedResume classify_paired_result() {
        auto& eax=s_.gp[0];eax=s_.flags.increment(eax,4u,true);
        if(s_.flags.zf)return NativeTu15SimplexHullPairedResume::plane;
        eax=s_.flags.increment(eax,4u,true);
        return s_.flags.zf?NativeTu15SimplexHullPairedResume::segments:NativeTu15SimplexHullPairedResume::count_then_plane;
    }
};
template<class State,class Call,class Before> NativeTu15SimplexHullPairedResume run_private_hull_paired_call(
    State& s,bool first_two,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>6u)
        throw std::runtime_error("Private simplex hull paired dispatch requires supported actual TU1.5 with free x87 slots");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_search_setup_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_simplex_hull_paired_call_kernel(view,first_two);
}
} // namespace halo2::native_simplex_hull_paired_call_detail
#endif
