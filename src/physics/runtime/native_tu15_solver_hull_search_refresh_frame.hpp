#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_search_refresh_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_setup_frame.hpp"

namespace halo2::native_solver_hull_search_refresh_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void read(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.gp[reg]=io_.word(pc,address);}
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    NativeTu15SolverHullRefreshStep advance_attempt() {
        auto& g=s_.gp;
        read(0x3140ddu,2u,g[6]+0x4cu);g[2]=s_.flags.increment(g[2],4u,false);g[0]=g[2];
        s_.flags.minus(g[0],1u,4u);io_.store(0x3140e6u,g[6]+0x4cu,g[2]);
        if(s_.flags.zf)return NativeTu15SolverHullRefreshStep::first_restart;
        s_.flags.minus(g[0],20u,4u);
        return s_.flags.sf==s_.flags.of?NativeTu15SolverHullRefreshStep::exhausted:NativeTu15SolverHullRefreshStep::perturb;
    }
    void reset_to_one_seed() {
        auto& g=s_.gp;read(0x3140ebu,0u,g[6]+0x48u);io_.store(0x3140eeu,g[0]+0x80u,g[2]);
    }
    void reseed_translation() {
        auto& g=s_.gp;auto& x=s_.xmm;
        read(0x3140ffu,1u,g[6]+0x48u);g[2]=g[4]+0x40u;
        io_.push(0x314106u,g[2]);io_.push(0x314107u,0x38d1b717u);
        io_.store(0x31410cu,g[1]+0x80u,g[3]);read(0x314112u,0u,g[6]+0x4cu);
        io_.push(0x314115u,0xb8d1b717u);io_.push(0x31411au,g[0]);g[1]=g[6];
        io_.call(0x31411du,0x314122u,0x313300u);
        io_.vector(0x314122u,1u,g[6]+0x30u);io_.vector(0x314126u,0u,g[4]+0x40u);
        asm volatile("addps %1,%0":"+x"(x[0]):"x"(x[1]):"memory");
        io_.store_vector(0x31412eu,g[4]+0x40u,x[0]);io_.store_vector(0x314133u,g[6]+0x30u,x[0]);
    }
    void release_payload_if_owned() {
        auto& g=s_.gp;
        read(0x314137u,0u,g[4]+0x128u);s_.flags.logic(g[0],4u);
        io_.store_byte(0x314140u,g[4]+0x301cu,1u);
        if(s_.flags.sf)return;
        read(0x31414au,1u,0x48c938u);read(0x314150u,2u,g[1]);g[0]=s_.flags.logic(g[0]&0x7fffffffu,4u);
        io_.shift_index(0u);io_.push(0x31415au,0x12u);io_.push(0x31415cu,g[0]);
        read(0x31415du,0u,g[4]+0x128u);io_.push(0x314164u,g[0]);
        io_.call_allocator(0x314165u,0x314168u,g[2]+0x14u);
    }
    void release_vertices_if_owned() {
        auto& g=s_.gp;
        read(0x314168u,0u,g[4]+0x4a8u);s_.flags.logic(g[0],4u);
        io_.store_byte(0x314171u,g[4]+0x301cu,0u);
        if(s_.flags.sf)return;
        read(0x31417bu,1u,0x48c938u);read(0x314181u,2u,g[1]);g[0]=s_.flags.logic(g[0]&0x7fffffffu,4u);
        io_.push(0x314188u,0x12u);io_.shift_index(0u);io_.push(0x31418du,g[0]);
        read(0x31418eu,0u,g[4]+0x4a8u);io_.push(0x314195u,g[0]);
        io_.call_allocator(0x314196u,0x314199u,g[2]+0x14u);
    }
    void destroy_tetrahedron() {
        auto& g=s_.gp;g[1]=g[4]+0x820u;io_.store(0x3141a0u,g[4]+0x301cu,0xffffffffu);
        io_.call(0x3141abu,0x3141b0u,0x3139f0u);
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_solver_hull_search_setup_detail::PhysicalIo<State,Before,Call> {
    using Base=native_solver_hull_search_setup_detail::PhysicalIo<State,Before,Call>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    State& s_;
    native_raw_x87_detail::Scope<State>& fp_;
    Call& call_;
public:
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,fp,call},s_(s),fp_(fp),call_(call){}
    void call_allocator(std::uint32_t site,std::uint32_t returned,std::uint32_t slot) {
        Foundation::at(site);const auto target=Foundation::word(slot);
        const auto address=s_.gp[4]-4u;
        Foundation::store(site,address,returned,[&]{s_.gp[4]=address;},true);
        fp_.child([&]{call_(s_,target,site);});
    }
};
template<class State,class Call,class Before> NativeTu15SolverHullRefreshResume run_private_search_refresh(
    State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull search refresh requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_solver_hull_search_refresh_kernel(view);
}
} // namespace halo2::native_solver_hull_search_refresh_detail
#endif
