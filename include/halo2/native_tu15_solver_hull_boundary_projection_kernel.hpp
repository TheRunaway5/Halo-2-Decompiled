#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_boundary_projection_kernel(View& view) {
    do {
        view.copy_boundary_vertices();
        view.compute_weights();
        if(view.weights_inside_boundary())break;
    }while(view.advance_facet());
    view.publish_selected_features();
}
} // namespace halo2
