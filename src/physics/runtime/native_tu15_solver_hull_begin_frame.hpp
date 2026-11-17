#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_begin_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_solver_hull_begin_detail {
using InitialPath=NativeTu15SolverHullInitialPath;
template<class State,class Io> class Schedule {
    State& s_;
    Io& io_;
public:
    Schedule(State& s,Io& io):s_(s),io_(io){}
    void open_exception_frame() {
        auto& g=s_.gp;
        io_.push(0x313bb0u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4u);
        io_.push(0x313bb6u,0xffffffffu);io_.push(0x313bb8u,0x37f231u);
        g[0]=io_.word(0x313bbdu,s_.fs_base);io_.push(0x313bc3u,g[0]);
        io_.store(0x313bc4u,s_.fs_base,g[4]);io_.push(0x313bcbu,g[1]);
    }
    void reserve_solver_stack() {
        s_.gp[0]=0x3004u;io_.probe_stack();
    }
    void capture_returned_context() {
        auto& g=s_.gp;
        io_.push(0x313bd6u,g[3]);io_.push(0x313bd7u,g[6]);io_.push(0x313bd8u,g[7]);
        g[6]=g[1];
    }
    InitialPath read_initial_route() {
        auto& g=s_.gp;
        g[0]=io_.word(0x313be0u,g[6]+0x48u);g[0]=io_.word(0x313be3u,g[0]+0x80u);
        g[7]=s_.flags.logic(0xffffffffu,4u);s_.flags.minus(g[0],4u,4u);
        if(!s_.flags.cf&&!s_.flags.zf)return InitialPath::build_tetrahedron;
        const auto target=io_.word(0x313bf5u,0x3146f8u+g[0]*4u);
        switch(static_cast<InitialPath>(target)) {
        case InitialPath::count_zero:case InitialPath::count_one:case InitialPath::count_two:
        case InitialPath::count_three:case InitialPath::count_four:return static_cast<InitialPath>(target);
        default:throw std::runtime_error("Required original hull initialization route unavailable");
        }
    }
};

template<class State,class Before,class StackCall> class PhysicalIo:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;
    native_raw_x87_detail::Scope<State>& fp_;
    StackCall& call_;
public:
    using Base::push;
    PhysicalIo(State& s,Before& before,native_raw_x87_detail::Scope<State>& fp,StackCall& call)
        :Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp),call_(call){}
    std::uint32_t word(std::uint32_t pc,std::uint32_t address){Base::at(pc);return Base::word(address);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t bits){Base::store(pc,address,bits,[]{});}
    void probe_stack(){
        push(0x313bd1u,0x313bd6u);
        fp_.child([&]{call_(s_,0x32a660u,0x313bd1u);});
    }
};
template<class State,class StackCall,class Before> InitialPath run_private_begin(State& s,StackCall call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))
        throw std::runtime_error("Private hull solver begin requires the actual TU1.5 backed FP frame");
    native_raw_x87_detail::Scope fp{s};PhysicalIo<State,Before,StackCall> io{s,before,fp,call};
    Schedule<State,decltype(io)> view{s,io};return native_tu15_solver_hull_begin_kernel(view);
}
} // namespace halo2::native_solver_hull_begin_detail
#endif
