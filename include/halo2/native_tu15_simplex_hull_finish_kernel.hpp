#pragma once

namespace halo2 {
enum class NativeTu15SimplexHullFinish { points, pair_difference, count_then_plane, plane };

template<class View> void native_tu15_simplex_hull_finish_kernel(View& view,NativeTu15SimplexHullFinish entry) {
    using Finish=NativeTu15SimplexHullFinish;
    if(entry==Finish::points)view.point_difference();
    if(entry==Finish::points||entry==Finish::pair_difference)view.publish_pair_difference();
    if(entry!=Finish::plane)view.publish_count();
    view.publish_plane_and_restore();
}
} // namespace halo2
