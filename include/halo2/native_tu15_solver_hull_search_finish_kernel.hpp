#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_search_finish_kernel(View& view) {
    view.interpolate_output();
    view.publish_plane();
    view.release_payload_if_owned();
    view.release_vertices_if_owned();
    view.destroy_tetrahedron();
    view.restore_caller();
}
} // namespace halo2
