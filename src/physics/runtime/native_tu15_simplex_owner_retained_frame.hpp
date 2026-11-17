#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_simplex_hull_retained_frame.hpp"
#include "physics/runtime/native_tu15_simplex_owner_frame.hpp"

namespace halo2::native_simplex_owner_retained_detail {
// Private source transport over the same typed reduction and search policy.
// Unknown provider/callback policy and source binding remain separate.
template<class State,class Call,class Before> class Frame:
    public native_simplex_owner_detail::Frame<State,Call,Before> {
    using Base=native_simplex_owner_detail::Frame<State,Call,Before>;
    using Io=native_simplex_hull_retained_detail::Io<State,Call,Before,true>;
    using Continuation=native_simplex_reduction_continuation_detail::Entry;
    State& s_;
    Call& call_;
    Before& before_;
    template<class Work> decltype(auto) with_io(Work work){
        native_raw_x87_detail::Scope fp{s_};Io io{s_,call_,before_,fp};return work(io);
    }
    void continuation(Continuation entry){
        with_io([&](Io& io){native_simplex_reduction_continuation_detail::apply_reduction_continuation(s_,entry,io);});
    }
public:
    Frame(State& s,Call& call,Before& before):Base{s,call,before},s_(s),call_(call),before_(before){}
    auto begin_iteration(){return native_simplex_iteration_detail::run_private_retained_stage(s_,before_);}
    void demote_vertex(bool first){continuation(first?Continuation::first_vertex:Continuation::second_vertex);}
    void segment_normal(){continuation(Continuation::segment_normal);}
    void publish_direction_count(){continuation(Continuation::direction_count);}
    auto face(bool first){
        return with_io([&](Io& io){
            native_simplex_hull_face_call_detail::Schedule<State,Io,0x7cfu> view{s_,io};
            return native_tu15_simplex_hull_face_call_kernel(view,first);
        });
    }
    auto triangle(bool first){
        return with_io([&](Io& io){
            native_simplex_hull_triangle_call_detail::Schedule<State,Io,0x7cfu> view{s_,io};
            return native_tu15_simplex_hull_triangle_call_kernel(view,first);
        });
    }
    void triangle_edges(bool first){
        with_io([&](Io& io){
            native_simplex_hull_triangle_edges_detail::Schedule<State,Io,0x7cfu,0x10u> view{s_,io};
            native_tu15_simplex_hull_triangle_edges_kernel(view,first);
        });
    }
    auto paired(bool first_two){
        return with_io([&](Io& io){
            native_simplex_reduction_paired_detail::Schedule view{s_,io};
            return native_tu15_simplex_reduction_paired_kernel(view,first_two);
        });
    }
    auto segments(){
        return with_io([&](Io& io){
            native_simplex_reduction_segments_detail::Schedule view{s_,io};
            return native_tu15_simplex_hull_segments_kernel(view);
        });
    }
    auto project(){return native_simplex_projection_detail::run_private_retained_stage(s_,before_);}
    void fallback(){
        with_io([&](Io& io){
            native_simplex_fallback_detail::Schedule view{s_,io};native_tu15_simplex_fallback_kernel(view);
        });
    }
    auto support(NativeTu15SimplexSupportEntry entry){
        return native_simplex_support_detail::run_private_retained_stage(s_,entry,call_,before_);
    }
    bool candidate(NativeTu15SimplexCandidateSide side){
        return native_simplex_candidate_detail::run_private_retained_stage(s_,side,before_);
    }
};
template<class State,class Call,class Before> std::uint32_t run_private_simplex_retained(State& s,Call call,Before before){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private retained simplex owner requires supported actual TU1.5 with four free x87 slots");
    Frame<State,Call,Before> frame{s,call,before};return native_tu15_simplex_owner_kernel(frame);
}
} // namespace halo2::native_simplex_owner_retained_detail
#endif
