#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SimplexHullPlaneEntry : std::uint32_t {
    segments=0x30e606u,second_vertex=0x30e68bu,second_edge=0x30e6bfu,first_vertex=0x30e7a7u,first_edge=0x30e7c7u
};
enum class NativeTu15SimplexHullPlaneResume : std::uint32_t {
    points=0x30e89bu,pair_difference=0x30e8a9u,count_then_plane=0x30e8b0u
};

template<class View> NativeTu15SimplexHullPlaneResume native_tu15_simplex_hull_plane_kernel(
    View& view,NativeTu15SimplexHullPlaneEntry entry) {
    using Entry=NativeTu15SimplexHullPlaneEntry;
    using Resume=NativeTu15SimplexHullPlaneResume;
    if(entry==Entry::segments){
        if(view.segments_plane())return Resume::count_then_plane;
        view.reverse_segments_plane();return Resume::pair_difference;
    }
    const bool first=entry==Entry::first_vertex||entry==Entry::first_edge;
    if(entry==Entry::first_vertex||entry==Entry::second_vertex)view.demote_vertex(first);
    if(view.crosses_edge(first)){
        view.edge_plane(first);return Resume::count_then_plane;
    }
    view.collapse_edge(first);return Resume::points;
}
} // namespace halo2
