#pragma once

namespace halo2 {
// Follow one adjacent facet at a time, until the three-edge scan rejects
// every candidate or the selected neighbor already bears the visit mark.
template<class View> void native_tu15_solver_hull_facet_select_kernel(View& view) {
    view.begin();
    while(view.continue_walk()) {
        view.begin_edges();
        do {
            if(view.current_edge_is_distinct()&&view.edge_projection_eligible()&&view.prefer_neighbor()) {
                view.adopt_neighbor();
                break;
            }
            view.advance_edge();
        } while(view.edge_remaining());
        if(!view.edge_remaining())break;
    }
    view.finish();
}
} // namespace halo2
