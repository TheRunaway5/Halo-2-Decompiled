#pragma once

namespace halo2 {
template<class View> bool native_tu15_simplex_hull_copyback_kernel(View& view) {
    if(!view.publish_returned_rows())return false;
    view.publish_single_pair();
    view.restore_caller();
    return true;
}
} // namespace halo2
