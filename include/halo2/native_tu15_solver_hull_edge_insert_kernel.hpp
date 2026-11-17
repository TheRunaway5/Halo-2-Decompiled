#pragma once

namespace halo2 {
template<class View>void native_tu15_solver_hull_edge_insert_kernel(View&view){
    view.append_seed_edge();
    view.reserve_facet_slots();
    view.link_facet_ring();
    if(view.has_owned_dynamic_slots())view.release_dynamic_slots();
    view.finish();
}
} // namespace halo2
