#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <map>
#include <optional>
namespace halo2 {
struct NativeTu15ConstraintReferenceInput {
    std::uint32_t instance{},datum{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15ConstraintReferenceInputObserver=std::function<void(const SourcePhysicsContactMemory&,const NativeTu15ConstraintReferenceInput&)>;
inline auto&native_tu15_constraint_reference_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15ConstraintReferenceInputObserver>observers;return observers;
}
class NativeTu15ConstraintReferenceInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;std::optional<NativeTu15ConstraintReferenceInputObserver>previous_;
public:
    NativeTu15ConstraintReferenceInputObserverBinding(const SourcePhysicsContactMemory&memory,NativeTu15ConstraintReferenceInputObserver observer):memory_(&memory){
        auto&map=native_tu15_constraint_reference_input_observers();const auto found=map.find(memory_);if(found!=map.end())previous_=found->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15ConstraintReferenceInputObserverBinding(){auto&map=native_tu15_constraint_reference_input_observers();if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);}
    NativeTu15ConstraintReferenceInputObserverBinding(const NativeTu15ConstraintReferenceInputObserverBinding&)=delete;
};
inline void native_tu15_observe_constraint_reference_input(const SourcePhysicsContactMemory&memory,const NativeTu15ConstraintReferenceInput&input){
    const auto&map=native_tu15_constraint_reference_input_observers();const auto found=map.find(&memory);if(found!=map.end())found->second(memory,input);
}
} //namespace halo2
