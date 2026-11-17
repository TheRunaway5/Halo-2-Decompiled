#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SimplexHullSegmentsResume : std::uint32_t { retain_both=0x30e606u,reduce=0x30e5ecu };

template<class View> NativeTu15SimplexHullSegmentsResume native_tu15_simplex_hull_segments_kernel(View& view) {
    view.call_closest_segments();
    if(!view.has_endpoint_reduction())return NativeTu15SimplexHullSegmentsResume::retain_both;
    view.reduce_first_segment();
    view.reduce_second_segment();
    return NativeTu15SimplexHullSegmentsResume::reduce;
}
} // namespace halo2
