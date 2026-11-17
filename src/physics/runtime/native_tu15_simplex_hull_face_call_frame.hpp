#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_face_call_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_simplex_hull_face_call_detail {
template<class State,class Io,std::uint32_t PcBias> class Schedule {
    State& s_;
    Io& io_;
    static constexpr std::uint32_t pc(std::uint32_t original){return original+PcBias;}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool select_face(bool first) {
        auto& g=s_.gp;
        io_.push(first?pc(0x30e4d3u):pc(0x30e362u),first?0x3f800000u:0xbf800000u);
        g[0]=g[6]+(first?0x20u:0xa0u);g[3]=g[6]+(first?0xa0u:0x20u);
        io_.call(first?pc(0x30e4e1u):pc(0x30e370u),first?pc(0x30e4e6u):pc(0x30e375u),0x30d5d0u);
        g[4]=s_.flags.plus(g[4],4u,4u);s_.flags.logic(g[0],4u);return s_.flags.sf==s_.flags.of;
    }
    void promote_first_face() {
        auto& g=s_.gp;
        g[2]=io_.word(pc(0x30e4f1u),g[6]);io_.vector(pc(0x30e4f3u),0u,g[6]+0x50u);
        g[2]=s_.flags.increment(g[2],4u,true);g[0]=s_.flags.plus(g[0],2u,4u);io_.shift_index(0u);
        io_.store(pc(0x30e4feu),g[6],g[2]);io_.store_vector(pc(0x30e500u),g[0]+g[6],s_.xmm[0]);
    }
    void promote_second_face() {
        auto& g=s_.gp;
        g[3]=io_.word(pc(0x30e380u),g[7]);io_.vector(pc(0x30e382u),0u,g[6]+0xd0u);g[1]=g[0]+10u;
        g[3]=s_.flags.increment(g[3],4u,true);io_.shift_index(1u);io_.store(pc(0x30e390u),g[7],g[3]);
        g[0]=s_.flags.plus(g[0],14u,4u);io_.store_vector(pc(0x30e395u),g[1]+g[6],s_.xmm[0]);
        io_.vector(pc(0x30e399u),0u,g[6]+0x110u);io_.shift_index(0u);io_.store_vector(pc(0x30e3a3u),g[0]+g[6],s_.xmm[0]);
    }
};
template<std::uint32_t PcBias=0u,class State,class Call,class Before> NativeTu15SimplexHullFaceResume run_private_hull_face_call(
    State& s,bool first,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>5u)
        throw std::runtime_error("Private simplex hull face dispatch requires supported actual TU1.5 with three free x87 slots");
    native_raw_x87_detail::Scope fp{s};native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io),PcBias> view{s,io};return native_tu15_simplex_hull_face_call_kernel(view,first);
}
} // namespace halo2::native_simplex_hull_face_call_detail
#endif
