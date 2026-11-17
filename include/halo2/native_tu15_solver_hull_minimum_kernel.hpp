#pragma once
namespace halo2 {
// Select the minimum facet in four-candidate batches and a scalar tail.
// The view owns the ordered x87 comparison and the distinct batch/tail
// address and publication rules, including the live rounded winner reload.
template<class View>void native_tu15_solver_hull_minimum_kernel(View&view){
    view.begin();
    if(view.begin_batches()){
        do{
            for(unsigned slot=0;slot<4u;++slot)view.batch_candidate(slot);
        }while(view.next_batch());
        view.end_batches();
    }
    if(view.begin_tail()){
        do{view.tail_candidate();}while(view.next_tail());
    }
    view.finish();
}
} // namespace halo2
