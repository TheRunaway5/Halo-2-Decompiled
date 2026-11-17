#pragma once

namespace halo2 {
// Visit the three live facet links. A zero-status neighbor can be clipped
// and visited first; link collection is based on its live status afterward.
template<class View> void native_tu15_solver_hull_facet_visit_kernel(View& view) {
    view.begin();
    do {
        if (view.load_neighbor()) {
            view.clip_neighbor();
            if (view.neighbor_intersects()) {
                view.visit_neighbor();
                if (view.child_stops_walk()) break;
            } else if (view.neighbor_stops_walk()) {
                view.mark_stop();
                break;
            }
        }
        if (view.collect_neighbor()) view.link_neighbor();
    } while (view.advance());
    view.finish();
}
} // namespace halo2
