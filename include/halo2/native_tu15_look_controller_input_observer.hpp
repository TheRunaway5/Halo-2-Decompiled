#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Ordinary current Host formals at the actual18A300 call. Onlygp0's named
// scalar index is used; no register/stack/SIMD image or result is transported.
struct NativeTu15LookControllerInput {
    std::uint32_t control_index{},yaw_delta_bits{},pitch_delta_bits{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
    std::uint32_t caller_player{},caller_unit{};
};
using NativeTu15LookControllerInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15LookControllerInput&)>;
class NativeTu15LookControllerInputObserverBinding;
namespace native_look_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15LookControllerInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15LookControllerInputObserverBinding*> value;return value;
}
}
class NativeTu15LookControllerInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15LookControllerInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_look_controller_input(
        const SourcePhysicsContactMemory&,const NativeTu15LookControllerInput&);
public:
    NativeTu15LookControllerInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15LookControllerInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_look_input_detail::mutex());
        native_look_input_detail::bindings().push_back(this);
    }
    ~NativeTu15LookControllerInputObserverBinding(){
        std::lock_guard lock(native_look_input_detail::mutex());
        auto&bindings=native_look_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15LookControllerInputObserverBinding(const NativeTu15LookControllerInputObserverBinding&)=delete;
    NativeTu15LookControllerInputObserverBinding&operator=(const NativeTu15LookControllerInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_look_controller_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15LookControllerInput&input){
    NativeTu15LookControllerInputObserver observer;
    {
        std::lock_guard lock(native_look_input_detail::mutex());
        const auto&bindings=native_look_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
