#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_search_setup_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"

namespace halo2::native_solver_hull_search_setup_detail {
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
    void vector_to_stack(std::uint32_t read_pc,std::uint32_t offset,
                         std::uint32_t store_pc,std::uint32_t slot) {
        io_.vector(read_pc,0u,s_.gp[0]+offset);
        io_.store_vector(store_pc,s_.gp[4]+slot,s_.xmm[0]);
    }
    void word_to_stack(std::uint32_t pc,std::uint32_t slot,unsigned reg) {
        io_.store(pc,s_.gp[4]+slot,s_.gp[reg]);
    }
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void make_tetrahedron() {
        auto& g=s_.gp;
        g[0]=io_.word(0x313e8eu,g[6]+0x50u);
        g[1]=g[0]+0x30u;io_.push(0x313e94u,g[1]);
        g[2]=g[0]+0x20u;io_.push(0x313e98u,g[2]);
        g[1]=g[0]+0x10u;io_.push(0x313e9cu,g[1]);io_.push(0x313e9du,g[0]);
        g[1]=g[4]+0x830u;io_.call(0x313ea5u,0x313eaau,0x313ab0u);
    }
    void copy_vertices() {
        auto& g=s_.gp;
        g[3]=s_.flags.logic(0u,4u);g[2]=g[4]+0x4b0u;g[1]=0x80000037u;
        word_to_stack(0x313eb8u,0x301cu,3u);word_to_stack(0x313ebfu,0x4a0u,2u);word_to_stack(0x313ec6u,0x4a8u,1u);
        g[0]=io_.word(0x313ecdu,g[6]+0x48u);g[2]=4u;word_to_stack(0x313ed5u,0x4a4u,2u);
        vector_to_stack(0x313edcu,0u,0x313edfu,0x4b0u);
        vector_to_stack(0x313ee7u,0x10u,0x313eebu,0x4c0u);
        vector_to_stack(0x313ef3u,0x20u,0x313ef7u,0x4d0u);
        io_.vector(0x313effu,0u,g[0]+0x30u);g[7]=g[4]+0x130u;
        io_.store_vector(0x313f0au,g[4]+0x4e0u,s_.xmm[0]);
    }
    void copy_payloads() {
        auto& g=s_.gp;
        word_to_stack(0x313f12u,0x120u,7u);word_to_stack(0x313f19u,0x128u,1u);word_to_stack(0x313f20u,0x124u,2u);
        vector_to_stack(0x313f27u,0x40u,0x313f2bu,0x130u);
        vector_to_stack(0x313f33u,0x50u,0x313f37u,0x140u);
        vector_to_stack(0x313f3fu,0x60u,0x313f43u,0x150u);
        io_.vector(0x313f4bu,0u,g[0]+0x70u);
        io_.store_byte(0x313f4fu,g[4]+0x301cu,2u);
        io_.store_vector(0x313f57u,g[4]+0x160u,s_.xmm[0]);word_to_stack(0x313f5fu,0x18u,3u);
    }
};

template<class State,class Before,class Call> class PhysicalIo:
    public native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call> {
    using Base=native_solver_hull_seed_transition_detail::PhysicalIo<State,Before,Call>;
    using Foundation=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    State& state_;
public:
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,Call& call)
        :Base{s,before,fp,call},state_(s){}
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        Foundation::at(pc);Foundation::io([&]{physics_step_detail::w(state_.memory,state_.observe,pc,address,1u,value);});
    }
};
template<class State,class Call,class Before> void run_private_search_setup(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>7u)
        throw std::runtime_error("Private hull search setup requires a supported actual TU1.5 frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,Call> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};native_tu15_solver_hull_search_setup_kernel(view);
}
} // namespace halo2::native_solver_hull_search_setup_detail
#endif
