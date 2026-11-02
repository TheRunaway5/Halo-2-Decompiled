#pragma once
#include "halo2/native_tu15_dirty_island_objects.hpp"
#include "halo2/native_tu15_component_state.hpp"
namespace halo2 {
struct NativeTu15DirtyIslandObjectsBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15DirtyIslandObjectStats update;
    NativeTu15ComponentStateStats state;
};
// Sole native-only282C10 builder. Known1CC310/BA8C0 helpers execute native
// algorithms; their selected unqualified lifetime children remain named throws.
// No original executor, CPU register file or fallback callback can enter here.
inline void native_tu15_process_bound_dirty_island_objects(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15DirtyIslandObjectsBindingActivity*activity=nullptr){
    NativeTu15DirtyIslandObjectsBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15DirtyIslandObjectServices services;
    const NativeTu15ComponentStateServices remaining;
    services.set_component_flags=[&](auto datum,auto flags,auto&current,auto current_depth){
        native_tu15_set_component_state_flags(memory,datum,flags,current,current_depth,
            remaining,observe,&out.state);
    };
    services.defer_object=[&](auto datum,auto&current,auto current_depth){
        native_tu15_defer_component_object(memory,datum,current,current_depth,
            remaining,observe,&out.state);
    };
    try{
        native_tu15_process_dirty_island_objects(memory,fp,depth,services,observe,&out.update);
        ++out.completed;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
