#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_owner_kernel.hpp"
#include "physics/runtime/native_tu15_solver_hull_begin_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_iteration_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_boundary_projection_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_finish_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_result_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_triangle_result_frame.hpp"

namespace halo2::native_solver_hull_owner_detail {
template<class State,class Before,class Call> class Frame {
    State& s_;
    Call& call_;
    Before& before_;
    void publish_result(std::uint32_t pc,std::uint32_t code) {
        native_raw_x87_detail::Scope fp{s_};
        native_solver_hull_seed_route_detail::PhysicalIo<State,Before> io{s_,before_,fp};
        io.store(pc,s_.gp[4]+0x18u,code);
    }
public:
    Frame(State& s,Call& call,Before& before):s_(s),call_(call),before_(before){}
    NativeTu15SolverHullInitialPath begin() {
        return native_solver_hull_begin_detail::run_private_begin(s_,call_,before_);
    }
    NativeTu15SolverHullInitialPath restart() {
        native_raw_x87_detail::Scope fp{s_};
        native_solver_hull_begin_detail::PhysicalIo<State,Before,Call> io{s_,before_,fp,call_};
        native_solver_hull_begin_detail::Schedule<State,decltype(io)> view{s_,io};
        return view.read_initial_route();
    }
    NativeTu15SolverHullSeedExit classify(NativeTu15SolverHullInitialPath route) {
        return native_solver_hull_seed_route_detail::run_private_route(s_,route,before_);
    }
    NativeTu15SolverHullSeedResume transition(NativeTu15SolverHullSeedExit seed) {
        return native_solver_hull_seed_transition_detail::run_private_transition(s_,seed,call_,before_);
    }
    void write_seed_result(){native_solver_hull_seed_result_detail::run_private_seed_result(s_,call_,before_);}
    void write_triangle_result(){native_solver_hull_triangle_result_detail::run_private_triangle_result(s_,call_,before_);}
    void return_near_origin() {
        using namespace native_solver_hull_triangle_result_detail;
        run_private_short_return(s_,Result::near_origin,call_,before_);
    }
    void return_degenerate() {
        using namespace native_solver_hull_triangle_result_detail;
        run_private_short_return(s_,Result::degenerate,call_,before_);
    }
    void setup_search(){native_solver_hull_search_setup_detail::run_private_search_setup(s_,call_,before_);}
    NativeTu15SolverHullSearchResume iterate() {
        return native_solver_hull_search_iteration_detail::run_private_search_iteration(s_,call_,before_);
    }
    NativeTu15SolverHullRefreshResume refresh() {
        return native_solver_hull_search_refresh_detail::run_private_search_refresh(s_,call_,before_);
    }
    void mark_iteration_limit(){publish_result(0x314494u,2u);}
    void mark_inline_limit(){publish_result(0x3141b5u,1u);}
    void project_boundary(){native_solver_hull_boundary_projection_detail::run_private_boundary_projection(s_,call_,before_);}
    void finish(){native_solver_hull_search_finish_detail::run_private_search_finish(s_,call_,before_);}
};

template<class State,class Call,class Before> void run_private_owner(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private 313BB0 solver requires actual TU1.5 with four free masked x87 slots");
    Frame<State,Before,Call> view{s,call,before};native_tu15_solver_hull_owner_kernel(view);
}
} // namespace halo2::native_solver_hull_owner_detail
#endif
