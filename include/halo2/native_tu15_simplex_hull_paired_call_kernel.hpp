#pragma once

namespace halo2 {
enum class NativeTu15SimplexHullPairedResume { plane, segments, count_then_plane };

template<class View> NativeTu15SimplexHullPairedResume native_tu15_simplex_hull_paired_call_kernel(View& view,bool first_two) {
    if(first_two)view.first_two_arguments();else view.second_two_arguments();
    view.call_paired_reducer();return view.classify_paired_result();
}
} // namespace halo2
