#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SimplexHullTriangleResume : std::uint32_t {
    second_edges=0x30e3d3u,first_edges=0x30e530u,second_vertex=0x30e68bu,first_vertex=0x30e7a7u,
    keep_plane=0x30e8b0u
};

template<class View> NativeTu15SimplexHullTriangleResume native_tu15_simplex_hull_triangle_call_kernel(
    View& view,bool first) {
    using Resume=NativeTu15SimplexHullTriangleResume;
    if(view.reduce_triangle(first))return Resume::keep_plane;
    if(view.select_triangle_feature(first))return first?Resume::first_vertex:Resume::second_vertex;
    return first?Resume::first_edges:Resume::second_edges;
}
} // namespace halo2
