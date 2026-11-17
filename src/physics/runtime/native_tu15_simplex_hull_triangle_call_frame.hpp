#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_triangle_call_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_triangle_call_detail {
template<class State,class Io,std::uint32_t PcBias> class Schedule {
    State& s_;
    Io& io_;
    static constexpr std::uint32_t pc(std::uint32_t original){return original+PcBias;}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool reduce_triangle(bool first) {
        auto& g=s_.gp;
        io_.push(first?pc(0x30e504u):pc(0x30e3a7u),first?0xffffffffu:1u);
        g[0]=g[6]+(first?0x20u:0xa0u);io_.push(first?pc(0x30e509u):pc(0x30e3afu),g[0]);
        g[3]=g[6]+(first?0xa0u:0x20u);io_.push(first?pc(0x30e510u):pc(0x30e3b3u),g[3]);
        g[1]=g[6];io_.call(first?pc(0x30e513u):pc(0x30e3b6u),first?pc(0x30e518u):pc(0x30e3bbu),0x30d3f0u);
        s_.flags.minus(g[0],7u,4u);return s_.flags.zf;
    }
    bool select_triangle_feature(bool first) {
        auto& result=s_.gp[0];
        const auto value=io_.byte(first?pc(0x30e521u):pc(0x30e3c4u),0x41d758u+result);
        result=(value^0x80u)-0x80u;
        s_.flags.logic(result,4u);return s_.flags.sf!=s_.flags.of;
    }
};
template<std::uint32_t PcBias=0u,class State,class Call,class Before> NativeTu15SimplexHullTriangleResume run_private_hull_triangle_call(
    State& s,bool first,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private simplex hull triangle dispatch requires supported actual TU1.5 with a free x87 slot");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io),PcBias> view{s,io};return native_tu15_simplex_hull_triangle_call_kernel(view,first);
}
} // namespace halo2::native_simplex_hull_triangle_call_detail
#endif
