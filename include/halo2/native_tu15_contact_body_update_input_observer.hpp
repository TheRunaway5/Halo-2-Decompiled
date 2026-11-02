#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <map>
#include <optional>
namespace halo2 {
struct NativeTu15ContactBodyUpdateInput {
    std::uint32_t manager{},row{},skip_impulse{};
    bool reversed{};
    std::uint32_t component_datum{},body_slot{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15ContactBodyUpdateInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15ContactBodyUpdateInput&)>;
inline auto&native_tu15_contact_body_update_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15ContactBodyUpdateInputObserver>observers;return observers;
}
class NativeTu15ContactBodyUpdateInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    std::optional<NativeTu15ContactBodyUpdateInputObserver>previous_;
public:
    NativeTu15ContactBodyUpdateInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15ContactBodyUpdateInputObserver observer):memory_(&memory){
        auto&map=native_tu15_contact_body_update_input_observers();const auto found=map.find(memory_);
        if(found!=map.end())previous_=found->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15ContactBodyUpdateInputObserverBinding(){
        auto&map=native_tu15_contact_body_update_input_observers();
        if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);
    }
    NativeTu15ContactBodyUpdateInputObserverBinding(const NativeTu15ContactBodyUpdateInputObserverBinding&)=delete;
};
inline void native_tu15_observe_contact_body_update_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15ContactBodyUpdateInput&input){
    const auto&map=native_tu15_contact_body_update_input_observers();const auto found=map.find(&memory);
    if(found!=map.end())found->second(memory,input);
}
}
