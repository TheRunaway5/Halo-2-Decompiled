#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_copyback_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_finish_frame.hpp"

namespace halo2::native_simplex_hull_copyback_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void copy_row(std::uint32_t load,std::uint32_t source,std::uint32_t store,std::uint32_t destination){
        io_.vector(load,0u,s_.gp[4]+source);io_.store_vector(store,s_.gp[6]+destination,s_.xmm[0]);
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    bool publish_returned_rows() {
        auto& g=s_.gp;auto& x=s_.xmm;
        io_.vector(0x30e018u,0u,g[4]+0x38u);g[1]=io_.word(0x30e01du,g[4]+0xb8u);
        io_.store_vector(0x30e024u,g[6]+0x20u,x[0]);
        copy_row(0x30e028u,0x48u,0x30e02du,0x30u);copy_row(0x30e031u,0x58u,0x30e036u,0x40u);
        copy_row(0x30e03au,0x78u,0x30e03fu,0xe0u);copy_row(0x30e046u,0x88u,0x30e04eu,0xf0u);
        io_.vector(0x30e055u,0u,g[4]+0x98u);g[4]=s_.flags.plus(g[4],0x18u,4u);s_.flags.minus(g[1],1u,4u);
        io_.store(0x30e063u,g[4]+0x1cu,g[0]);io_.store_vector(0x30e067u,g[6]+0x100u,x[0]);
        return s_.flags.zf;
    }
    void publish_single_pair() {
        auto& g=s_.gp;
        io_.store(0x30e070u,g[6]+0x120u,0x3f800000u);g[1]=s_.flags.logic(0u,4u);
        io_.store(0x30e07cu,g[6]+0x124u,g[1]);io_.store(0x30e082u,g[6]+0x128u,g[1]);
        io_.store(0x30e088u,g[6]+0x12cu,g[1]);io_.store(0x30e08eu,g[7],1u);io_.store(0x30e094u,g[6],1u);
    }
    void restore_caller() {
        auto& g=s_.gp;
        g[7]=io_.pop(0x30e09au);g[6]=io_.pop(0x30e09bu);g[3]=io_.pop(0x30e09cu);
        g[4]=g[5];g[5]=io_.pop(0x30e09fu);io_.pop(0x30e0a0u);g[4]+=16u;
    }
};
template<class State,class Before> bool run_private_hull_copyback(State& s,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private simplex hull copyback requires the supported actual TU1.5 frame");
    auto unreachable=[](auto&,std::uint32_t,std::uint32_t){throw std::runtime_error("Childless simplex hull copyback dispatched a child");};
    native_raw_x87_detail::Scope fp{s};
    native_solver_hull_search_finish_detail::PhysicalIo<State,Before,decltype(unreachable)> io{s,before,fp,unreachable};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_simplex_hull_copyback_kernel(view);
}
} // namespace halo2::native_simplex_hull_copyback_detail
#endif
