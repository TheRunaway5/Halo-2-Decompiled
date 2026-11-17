#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_triangle_result_kernel(View& view) {
    view.normalize_triangle_direction();
    view.project_origin();
    view.publish_transformed_point();
}
} // namespace halo2
