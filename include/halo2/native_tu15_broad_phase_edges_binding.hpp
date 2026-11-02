#pragma once
#include "halo2/native_tu15_broad_phase_edges.hpp"
namespace halo2 {
struct NativeTu15BroadPhaseEdgeBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15BroadPhaseEdgeActivity edges;
};
// Sole semantic sweep and sole existing native array-growth service closure.
// The caller selects authenticated 2EA380; no CPU/executor callback is accepted.
inline void native_tu15_update_bound_broad_phase_edges(
    SourcePhysicsContactMemory&memory,const NativeTu15BroadPhaseEdgeUpdate&input,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&storage,
    const SourcePhysicsContactRecordObserver&observer={},
    NativeTu15BroadPhaseEdgeBindingActivity*activity=nullptr) {
    if(activity)++activity->attempts;
    try {
        native_tu15_update_broad_phase_edges(memory,input,fp,storage,observer,
            activity?&activity->edges:nullptr);
        if(activity)++activity->completed;
    } catch(...) {if(activity)++activity->refusals;throw;}
}
} // namespace halo2
