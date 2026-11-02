#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Actual18A2A0 index/forward formals and completed direction DATA at its
// genuine Host call. No GP/SIMD, after-values or arithmetic outcomes.
struct NativeTu15LookReorientationInput {
    std::uint32_t control_index{},forward{};
    std::array<std::uint32_t,3> forward_words{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
    std::uint32_t caller_player{},caller_unit{};
};
using NativeTu15LookReorientationInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15LookReorientationInput&)>;
class NativeTu15LookReorientationInputObserverBinding;
namespace native_look_reorientation_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15LookReorientationInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15LookReorientationInputObserverBinding*> value;return value;
}
}
class NativeTu15LookReorientationInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15LookReorientationInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_look_reorientation_input(
        const SourcePhysicsContactMemory&,const NativeTu15LookReorientationInput&);
public:
    NativeTu15LookReorientationInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15LookReorientationInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_look_reorientation_input_detail::mutex());
        native_look_reorientation_input_detail::bindings().push_back(this);
    }
    ~NativeTu15LookReorientationInputObserverBinding(){
        std::lock_guard lock(native_look_reorientation_input_detail::mutex());
        auto&bindings=native_look_reorientation_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15LookReorientationInputObserverBinding(const NativeTu15LookReorientationInputObserverBinding&)=delete;
    NativeTu15LookReorientationInputObserverBinding&operator=(const NativeTu15LookReorientationInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_look_reorientation_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15LookReorientationInput&input){
    NativeTu15LookReorientationInputObserver observer;
    {
        std::lock_guard lock(native_look_reorientation_input_detail::mutex());
        const auto&bindings=native_look_reorientation_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
