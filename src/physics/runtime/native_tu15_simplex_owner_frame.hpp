#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_simplex_owner_kernel.hpp"
#include "physics/runtime/native_tu15_simplex_fallback_frame.hpp"
#include "physics/runtime/native_tu15_simplex_finalize_abi.hpp"
#include "physics/runtime/native_tu15_simplex_hull_face_call_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_triangle_call_frame.hpp"
#include "physics/runtime/native_tu15_simplex_hull_triangle_edges_frame.hpp"
#include "physics/runtime/native_tu15_simplex_initialize_abi.hpp"
#include "physics/runtime/native_tu15_simplex_projection_abi.hpp"
#include "physics/runtime/native_tu15_simplex_reduction_continuation_frame.hpp"
#include "physics/runtime/native_tu15_simplex_reduction_paired_frame.hpp"
#include "physics/runtime/native_tu15_simplex_reduction_segments_frame.hpp"
#include "physics/runtime/native_tu15_simplex_support_abi.hpp"

namespace halo2::native_simplex_owner_detail {
template<class State,class Call,class Before> class Frame {
    State& s_;
    Call& call_;
    Before& before_;
    using Continuation=native_simplex_reduction_continuation_detail::Entry;
    void continuation(Continuation entry){
        native_simplex_reduction_continuation_detail::run_private_reduction_continuation(s_,entry,before_);
    }
public:
    Frame(State& s,Call& call,Before& before):s_(s),call_(call),before_(before){}
    void initialize(){native_simplex_initialize_detail::run_private_stage(s_,call_,before_);}
    auto begin_iteration(){return native_simplex_iteration_detail::run_private_stage(s_,before_);}
    auto redispatch(){return native_simplex_reduction_continuation_detail::run_private_reduction_redispatch(s_,before_);}
    void mark_degenerate(){native_simplex_reduction_continuation_detail::mark_private_simplex_degenerate(s_);}
    void direction(NativeTu15SimplexDirectionSide side){native_simplex_direction_detail::run_private_stage(s_,side,before_);}
    void demote_vertex(bool first){continuation(first?Continuation::first_vertex:Continuation::second_vertex);}
    void segment_normal(){continuation(Continuation::segment_normal);}
    void publish_direction_count(){continuation(Continuation::direction_count);}
    auto face(bool first){return native_simplex_hull_face_call_detail::run_private_hull_face_call<0x7cfu>(s_,first,call_,before_);}
    auto triangle(bool first){return native_simplex_hull_triangle_call_detail::run_private_hull_triangle_call<0x7cfu>(s_,first,call_,before_);}
    void triangle_edges(bool first){native_simplex_hull_triangle_edges_detail::run_private_hull_triangle_edges<0x7cfu,0x10u>(s_,first,before_);}
    auto paired(bool first_two){return native_simplex_reduction_paired_detail::run_private_reduction_paired(s_,first_two,call_,before_);}
    auto segments(){return native_simplex_reduction_segments_detail::run_private_reduction_segments(s_,call_,before_);}
    auto project(){return native_simplex_projection_detail::run_private_stage(s_,before_);}
    void fallback(){native_simplex_fallback_detail::run_private_fallback(s_,call_,before_);}
    auto support(NativeTu15SimplexSupportEntry entry){return native_simplex_support_detail::run_private_stage(s_,entry,call_,before_);}
    bool candidate(NativeTu15SimplexCandidateSide side){return native_simplex_candidate_detail::run_private_stage(s_,side,before_);}
    std::uint32_t finalize(){return native_simplex_finalize_detail::run_private_stage(s_,before_);}
};
template<class State,class Call,class Before> std::uint32_t run_private_simplex_owner(State& s,Call call,Before before){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private simplex owner requires supported actual TU1.5 with four free x87 slots");
    Frame<State,Call,Before> frame{s,call,before};return native_tu15_simplex_owner_kernel(frame);
}
} // namespace halo2::native_simplex_owner_detail
#endif
