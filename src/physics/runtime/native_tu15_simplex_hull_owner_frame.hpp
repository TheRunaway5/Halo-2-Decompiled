#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_hull_owner_kernel.hpp"
#include "physics/runtime/native_tu15_simplex_hull_compact_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_copyback_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_face_call_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_finish_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_paired_call_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_plane_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_reduction_route_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_seed_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_segments_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_triangle_call_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_triangle_edges_frame.hpp"

namespace halo2::native_simplex_hull_owner_detail {
template<class State,class Call,class Before> class Frame {
    State& s_;
    Call& call_;
    Before& before_;
public:
    Frame(State& s,Call& call,Before& before):s_(s),call_(call),before_(before){}
    void seed(){native_simplex_hull_seed_detail::run_private_hull_seed(s_,call_,before_);}
    bool copyback_returns(){return native_simplex_hull_copyback_detail::run_private_hull_copyback(s_,before_);}
    void compact(){native_simplex_hull_compact_detail::run_private_hull_compact(s_,call_,before_);}
    auto initial_reduction(){return native_simplex_hull_reduction_route_detail::run_private_hull_reduction_route(s_,before_);}
    auto redispatch(){return native_simplex_hull_reduction_route_detail::run_private_hull_redispatch(s_,before_);}
    auto face(bool first){return native_simplex_hull_face_call_detail::run_private_hull_face_call(s_,first,call_,before_);}
    auto triangle(bool first){return native_simplex_hull_triangle_call_detail::run_private_hull_triangle_call(s_,first,call_,before_);}
    void triangle_edges(bool first){native_simplex_hull_triangle_edges_detail::run_private_hull_triangle_edges(s_,first,before_);}
    auto paired(bool first_two){return native_simplex_hull_paired_call_detail::run_private_hull_paired_call(s_,first_two,call_,before_);}
    auto segments(){return native_simplex_hull_segments_detail::run_private_hull_segments(s_,call_,before_);}
    auto plane(NativeTu15SimplexHullPlaneEntry entry){return native_simplex_hull_plane_detail::run_private_hull_plane(s_,entry,before_);}
    void finish(NativeTu15SimplexHullFinish entry){native_simplex_hull_finish_detail::run_private_hull_finish(s_,entry,before_);}
};
template<class State,class Call,class Before> void run_private_hull_owner(State& s,Call call,Before before) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private simplex hull fallback requires supported actual TU1.5 with four free x87 slots");
    Frame<State,Call,Before> frame{s,call,before};native_tu15_simplex_hull_owner_kernel(frame);
}
} // namespace halo2::native_simplex_hull_owner_detail
#endif
