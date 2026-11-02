#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Actual zero-formal282C10 boundary: only the real FP/depth context.
struct NativeTu15DirtyIslandObjectsInput {
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15DirtyIslandObjectsInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15DirtyIslandObjectsInput&)>;
class NativeTu15DirtyIslandObjectsInputObserverBinding;
namespace native_dirty_island_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15DirtyIslandObjectsInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15DirtyIslandObjectsInputObserverBinding*> value;return value;
}
}
class NativeTu15DirtyIslandObjectsInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15DirtyIslandObjectsInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_dirty_island_objects_input(
        const SourcePhysicsContactMemory&,const NativeTu15DirtyIslandObjectsInput&);
public:
    NativeTu15DirtyIslandObjectsInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15DirtyIslandObjectsInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_dirty_island_input_detail::mutex());
        native_dirty_island_input_detail::bindings().push_back(this);
    }
    ~NativeTu15DirtyIslandObjectsInputObserverBinding(){
        std::lock_guard lock(native_dirty_island_input_detail::mutex());
        auto&bindings=native_dirty_island_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15DirtyIslandObjectsInputObserverBinding(const NativeTu15DirtyIslandObjectsInputObserverBinding&)=delete;
    NativeTu15DirtyIslandObjectsInputObserverBinding&operator=(const NativeTu15DirtyIslandObjectsInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_dirty_island_objects_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15DirtyIslandObjectsInput&input){
    NativeTu15DirtyIslandObjectsInputObserver observer;
    {
        std::lock_guard lock(native_dirty_island_input_detail::mutex());
        const auto&bindings=native_dirty_island_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
