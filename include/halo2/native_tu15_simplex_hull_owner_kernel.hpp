#pragma once
#include "halo2/native_tu15_simplex_hull_face_call_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_finish_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_paired_call_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_plane_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_segments_kernel.hpp"
#include "halo2/native_tu15_simplex_hull_triangle_call_kernel.hpp"
#include "halo2/native_tu15_simplex_iteration_kernel.hpp"

namespace halo2 {
// Native control policy for the actual hull fallback. The view owns its
// backed rows and real call transports; the kernel receives no child outcome.
template<class View> void native_tu15_simplex_hull_reductions(View& view) {
    using Kind=NativeTu15SimplexReductionKind;
    using Finish=NativeTu15SimplexHullFinish;
    using Face=NativeTu15SimplexHullFaceResume;
    using Triangle=NativeTu15SimplexHullTriangleResume;
    using Paired=NativeTu15SimplexHullPairedResume;
    using Plane=NativeTu15SimplexHullPlaneEntry;
    using Resume=NativeTu15SimplexHullPlaneResume;
    const auto finish_plane=[&](Plane entry){
        switch(view.plane(entry)){
        case Resume::points:view.finish(Finish::points);break;
        case Resume::pair_difference:view.finish(Finish::pair_difference);break;
        case Resume::count_then_plane:view.finish(Finish::count_then_plane);break;
        }
    };
    for(auto kind=view.initial_reduction();;kind=view.redispatch()){
        switch(kind){
        case Kind::second_face:case Kind::first_face:
            switch(view.face(kind==Kind::first_face)){
            case Face::plane:view.finish(Finish::plane);return;
            case Face::first_triangle:kind=Kind::first_triangle;break;
            case Face::second_triangle:kind=Kind::second_triangle;break;
            }
            [[fallthrough]];
        case Kind::first_triangle:case Kind::second_triangle:
            switch(view.triangle(kind==Kind::first_triangle)){
            case Triangle::keep_plane:view.finish(Finish::count_then_plane);return;
            case Triangle::first_vertex:finish_plane(Plane::first_vertex);return;
            case Triangle::second_vertex:finish_plane(Plane::second_vertex);return;
            case Triangle::first_edges:view.triangle_edges(true);break;
            case Triangle::second_edges:view.triangle_edges(false);break;
            }
            break;
        case Kind::paired_two_three:case Kind::paired_three_two:
            switch(view.paired(kind==Kind::paired_two_three)){
            case Paired::plane:view.finish(Finish::plane);return;
            case Paired::count_then_plane:view.finish(Finish::count_then_plane);return;
            case Paired::segments:break;
            }
            [[fallthrough]];
        case Kind::segments:
            if(view.segments()==NativeTu15SimplexHullSegmentsResume::retain_both){
                finish_plane(Plane::segments);return;
            }
            break;
        case Kind::first_edge:finish_plane(Plane::first_edge);return;
        case Kind::second_edge:finish_plane(Plane::second_edge);return;
        case Kind::points:view.finish(Finish::points);return;
        case Kind::degenerate:view.finish(Finish::plane);return;
        }
    }
}
template<class View> void native_tu15_simplex_hull_owner_kernel(View& view) {
    view.seed();
    if(view.copyback_returns())return;
    view.compact();native_tu15_simplex_hull_reductions(view);
}
} // namespace halo2
