#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_facet_search_kernel(View& view) {
    view.begin();
    view.clip_seed_facet();
    const bool intersects=view.mark_seed_status();
    if(intersects)view.collect_connected_facets();
    view.finish(intersects);
}
} // namespace halo2
