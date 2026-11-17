#pragma once

namespace halo2 {
template<class View> void native_tu15_solver_hull_seed_facets_kernel(View& view) {
    if(view.begin_facets()) do {
        if(view.begin_links()) do {view.link_edge();} while(view.more_links());
    } while(view.next_facet());
    view.build_first_plane();
    if(view.requires_flip())view.flip_winding();
    view.begin_rebuild();
    do {view.rebuild_plane();} while(view.advance_rebuild());
    view.finish();
}
} // namespace halo2
