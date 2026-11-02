#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Current native caller's actual2E5320/304350 thiscall4 DATA, before the unchanged
// remaining-owner call. No GP/SIMD, caller control frame or result is captured.
struct NativeTu15ContactProcessTypedInput {
    std::uint32_t owner{},agent{},first{},second{},configuration{},output{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15ContactProcessTypedInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15ContactProcessTypedInput&)>;
class NativeTu15ContactProcessTypedInputObserverBinding;
namespace native_contact_process_typed_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15ContactProcessTypedInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15ContactProcessTypedInputObserverBinding*>value;return value;
}
}
class NativeTu15ContactProcessTypedInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15ContactProcessTypedInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_contact_process_typed_input(
        const SourcePhysicsContactMemory&,const NativeTu15ContactProcessTypedInput&);
public:
    NativeTu15ContactProcessTypedInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15ContactProcessTypedInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_contact_process_typed_input_detail::mutex());
        native_contact_process_typed_input_detail::bindings().push_back(this);
    }
    ~NativeTu15ContactProcessTypedInputObserverBinding(){
        std::lock_guard lock(native_contact_process_typed_input_detail::mutex());
        auto&bindings=native_contact_process_typed_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15ContactProcessTypedInputObserverBinding(const NativeTu15ContactProcessTypedInputObserverBinding&)=delete;
    NativeTu15ContactProcessTypedInputObserverBinding&operator=(const NativeTu15ContactProcessTypedInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_contact_process_typed_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15ContactProcessTypedInput&input){
    NativeTu15ContactProcessTypedInputObserver observer;
    {
        std::lock_guard lock(native_contact_process_typed_input_detail::mutex());
        const auto&bindings=native_contact_process_typed_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
