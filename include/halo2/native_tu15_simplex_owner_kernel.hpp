#pragma once
#include "halo2/native_tu15_simplex_candidate_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_face_call_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_segments_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_triangle_call_kernel.hpp"
#include "halo2/native_tu15_simplex_iteration_kernel.hpp"
#include "halo2/native_tu15_simplex_projection_kernel.hpp"
#include "halo2/native_tu15_simplex_reduction_paired_kernel.hpp"
#include "halo2/native_tu15_simplex_support_kernel.hpp"

namespace halo2 {
template<class View> void native_tu15_simplex_reduce_iteration(View& view) {
    using Kind=NativeTu15SimplexReductionKind;
    using Side=NativeTu15SimplexDirectionSide;
    using Face=NativeTu15SimplexHullFaceResume;
    using Triangle=NativeTu15SimplexHullTriangleResume;
    using Paired=NativeTu15SimplexPairedNext;
    for(auto kind=view.begin_iteration();;kind=view.redispatch()){
        switch(kind){
        case Kind::first_face:case Kind::second_face:
            switch(view.face(kind==Kind::first_face)){
            case Face::plane:view.mark_degenerate();return;
            case Face::first_triangle:kind=Kind::first_triangle;break;
            case Face::second_triangle:kind=Kind::second_triangle;break;
            }
            [[fallthrough]];
        case Kind::first_triangle:case Kind::second_triangle:
            switch(view.triangle(kind==Kind::first_triangle)){
            case Triangle::first_vertex:view.demote_vertex(true);view.direction(Side::first_edge);return;
            case Triangle::second_vertex:view.demote_vertex(false);view.direction(Side::second_edge);return;
            case Triangle::keep_plane:view.publish_direction_count();return;
            case Triangle::first_edges:view.triangle_edges(true);break;
            case Triangle::second_edges:view.triangle_edges(false);break;
            }
            break;
        case Kind::paired_two_three:case Kind::paired_three_two:
            switch(view.paired(kind==Kind::paired_two_three)){
            case Paired::degenerate:view.mark_degenerate();return;
            case Paired::direction_count:view.publish_direction_count();return;
            case Paired::segments:break;
            }
            [[fallthrough]];
        case Kind::segments:
            if(view.segments()==NativeTu15SimplexHullSegmentsResume::retain_both){view.segment_normal();return;}
            break;
        case Kind::first_edge:view.direction(Side::first_edge);return;
        case Kind::second_edge:view.direction(Side::second_edge);return;
        case Kind::points:view.direction(Side::points);return;
        case Kind::degenerate:view.mark_degenerate();return;
        }
    }
}

template<class View> bool native_tu15_simplex_try_support(View& view,NativeTu15SimplexProjectionNext projected) {
    using E=NativeTu15SimplexSupportEntry;
    using N=NativeTu15SimplexSupportNext;
    using S=NativeTu15SimplexCandidateSide;
    auto entry=projected==NativeTu15SimplexProjectionNext::first_support?E::first:E::second;
    for(;;){
        switch(view.support(entry)){
        case N::first_candidate:
            if(view.candidate(entry==E::first?S::first_early:S::first_late))return true;
            if(entry==E::last_first)return false;
            entry=E::second;break;
        case N::second_candidate:
            if(view.candidate(S::second)){view.support(E::transform_second);return true;}
            entry=E::last_first;break;
        case N::second_support:entry=E::second;break;
        case N::last_first:entry=E::last_first;break;
        case N::finalize:return false;
        case N::iteration:return true;
        }
    }
}

template<class View> std::uint32_t native_tu15_simplex_owner_kernel(View& view) {
    view.initialize();
    for(;;){
        native_tu15_simplex_reduce_iteration(view);
        const auto projected=view.project();
        if(projected==NativeTu15SimplexProjectionNext::reduce){view.fallback();break;}
        if(!native_tu15_simplex_try_support(view,projected))break;
    }
    return view.finalize();
}
} // namespace halo2
