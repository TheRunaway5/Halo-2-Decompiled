#pragma once

namespace halo2 {
template<class View>void native_tu15_solver_hull_facet_plane_kernel(View&view){
    view.capture_triangle();
    view.cross_and_length();
    view.refine_scale();
    view.publish_normal();
    view.publish_plane_distance();
    view.finish();
}
} // namespace halo2
