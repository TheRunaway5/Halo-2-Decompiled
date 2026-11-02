#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <map>
#include <optional>
namespace halo2 {
struct NativeTu15ConstraintInfoInput {
    std::uint32_t constraint{},owner{},info{};
    NativeTu15PhysicsFrameFp floating_point;
};
using NativeTu15ConstraintInfoInputObserver=std::function<void(const SourcePhysicsContactMemory&,const NativeTu15ConstraintInfoInput&)>;
inline auto&native_tu15_constraint_info_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15ConstraintInfoInputObserver>observers;return observers;
}
class NativeTu15ConstraintInfoInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;std::optional<NativeTu15ConstraintInfoInputObserver>previous_;
public:
    NativeTu15ConstraintInfoInputObserverBinding(const SourcePhysicsContactMemory&memory,NativeTu15ConstraintInfoInputObserver observer):memory_(&memory){
        auto&map=native_tu15_constraint_info_input_observers();const auto found=map.find(memory_);if(found!=map.end())previous_=found->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15ConstraintInfoInputObserverBinding(){auto&map=native_tu15_constraint_info_input_observers();if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);}
    NativeTu15ConstraintInfoInputObserverBinding(const NativeTu15ConstraintInfoInputObserverBinding&)=delete;
};
inline void native_tu15_observe_constraint_info_input(const SourcePhysicsContactMemory&memory,const NativeTu15ConstraintInfoInput&input){
    const auto&map=native_tu15_constraint_info_input_observers();const auto found=map.find(&memory);if(found!=map.end())found->second(memory,input);
}
} //namespace halo2
