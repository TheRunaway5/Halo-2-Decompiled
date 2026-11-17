#pragma once

namespace halo2 {
template<class View>void native_tu15_solver_hull_facet_ring_kernel(View&view){
    if(view.begin()){
        view.save_loop_state();
        while(view.have_next_node()){
            view.acquire_facet_and_successor();
            view.link_facet();
            view.build_plane();
            if(!view.advance())break;
        }
        view.restore_loop_state();
    }
    view.clear_tables_and_finish();
}
} // namespace halo2
