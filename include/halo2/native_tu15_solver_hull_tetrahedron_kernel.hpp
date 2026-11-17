#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_tetrahedron_kernel(View& view) {
    view.open_exception_frame();
    view.initialize_inline_storage();
    view.copy_seed_points();
    view.build_facet_planes();
    view.finish();
}
} // namespace halo2
