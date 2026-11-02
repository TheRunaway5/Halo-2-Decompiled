#pragma once
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_main_heap_release.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeTu15MainHeapReleaseBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15MainHeapReleaseStats kernel;
};
// Sole native-only production service boundary. No executor or unknown child
// callback can enter this interface. The current virtual+C owner is resolved
// by the semantic wrapper; an unsupported lifetime owner throws and is counted.
inline void native_tu15_release_bound_main_heap_allocation(SourcePhysicsContactMemory&memory,
    const NativeTu15ArrayReleaseRequest&request,NativeTu15PhysicsFrameFp&fp,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver&observe = {},
    NativeTu15MainHeapReleaseBindingActivity*activity = nullptr){
    if(activity)++activity->attempts;
    try{
        if(request.owner!=0x232d20u)
            throw std::runtime_error("Native TU1.5 main heap binding requires actual232D20 request owner");
        native_tu15_release_main_heap_allocation(memory,request.receiver,request.data,
            request.bytes,request.memory_class,fp,actual_preserved_x87_depth,{},observe,
            activity?&activity->kernel:nullptr);
        if(activity)++activity->completed;
    }catch(...){if(activity)++activity->refusals;throw;}
}
} // namespace halo2
