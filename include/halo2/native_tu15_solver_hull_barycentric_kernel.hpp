#pragma once

namespace halo2 {
enum class NativeTu15HullDegenerateEdge {
    ab,ac,bc_saved_ratio,bc_duplicate_ratio,zero
};

template<class View> void native_tu15_solver_hull_barycentric_kernel(View& view) {
    if(view.measure_triangle()) {
        view.project_triangle();
        return;
    }
    switch(view.select_degenerate_edge()) {
    case NativeTu15HullDegenerateEdge::ab: view.project_ab();break;
    case NativeTu15HullDegenerateEdge::ac: view.project_ac();break;
    case NativeTu15HullDegenerateEdge::bc_saved_ratio: view.project_bc_saved();break;
    case NativeTu15HullDegenerateEdge::bc_duplicate_ratio: view.project_bc_duplicate();break;
    case NativeTu15HullDegenerateEdge::zero: view.zero_weights();break;
    }
}
} // namespace halo2
