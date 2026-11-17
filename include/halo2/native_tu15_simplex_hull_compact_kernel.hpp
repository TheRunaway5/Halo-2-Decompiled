#pragma once

namespace halo2 {
template<class View> void native_tu15_simplex_hull_compact_kernel(View& view) {
    view.compact_first_shape();
    view.compact_second_shape();
    if(view.begin_transform())do{view.transform_row();}while(view.next_row());
    view.reduce_boundary_if_needed();
}
} // namespace halo2
