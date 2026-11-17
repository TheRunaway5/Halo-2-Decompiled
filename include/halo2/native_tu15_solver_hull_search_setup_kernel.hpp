#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_search_setup_kernel(View& view) {
    view.make_tetrahedron();
    view.copy_vertices();
    view.copy_payloads();
}
} // namespace halo2
