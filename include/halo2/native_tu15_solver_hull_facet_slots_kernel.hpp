#pragma once

namespace halo2 {
// Exhaust the live reuse list first. A request can then add fresh 80-byte
// facet slots. The views own every count read/write and both signed tests.
template<class View>void native_tu15_solver_hull_facet_slots_kernel(View&view){
    if(view.begin()){
        view.begin_reuse();
        while(view.have_reusable_slot()){
            view.reuse_slot();
            if(view.reuse_request_done())break;
        }
        if(view.finish_reuse())do{view.append_slot();}while(!view.append_request_done());
    }
    view.finish();
}
} // namespace halo2
