#pragma once
#include "halo2/native_tu15_simplex_iteration_kernel.hpp"

namespace halo2 {
template<class View> NativeTu15SimplexReductionKind native_tu15_simplex_hull_reduction_route_kernel(View& view) {
    return view.publish_packed_counts()?view.resolve_reduction():NativeTu15SimplexReductionKind::degenerate;
}
} // namespace halo2
