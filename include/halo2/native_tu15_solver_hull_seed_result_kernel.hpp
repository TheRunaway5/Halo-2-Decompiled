#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_seed_result_kernel(View& view) {
    view.select_direction_components();
    view.publish_unit_direction();
    view.interpolate_seed();
    view.restore_caller();
}
} // namespace halo2
