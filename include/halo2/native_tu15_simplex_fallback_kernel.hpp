#pragma once

namespace halo2 {
template<class View> void native_tu15_simplex_fallback_kernel(View& view) {
    if(view.permits_hull_fallback())view.call_hull_fallback();
    else view.restrict_overlap();
}
} // namespace halo2
