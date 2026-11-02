#pragma once
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <functional>
#include <map>
#include <optional>
namespace halo2 {
struct NativeTu15ContactManagerCleanupInput {
    std::uint32_t manager{},disposing{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15ContactManagerCleanupInputObserver=std::function<void(const SourcePhysicsContactMemory&,const NativeTu15ContactManagerCleanupInput&)>;
inline auto&native_tu15_contact_manager_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15ContactManagerCleanupInputObserver>observers;return observers;
}
class NativeTu15ContactManagerCleanupInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;std::optional<NativeTu15ContactManagerCleanupInputObserver>previous_;
public:
    NativeTu15ContactManagerCleanupInputObserverBinding(const SourcePhysicsContactMemory&memory,NativeTu15ContactManagerCleanupInputObserver observer):memory_(&memory){
        auto&map=native_tu15_contact_manager_input_observers();const auto found=map.find(memory_);if(found!=map.end())previous_=found->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15ContactManagerCleanupInputObserverBinding(){auto&map=native_tu15_contact_manager_input_observers();if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);}
    NativeTu15ContactManagerCleanupInputObserverBinding(const NativeTu15ContactManagerCleanupInputObserverBinding&)=delete;
};
inline void native_tu15_observe_contact_manager_cleanup_input(const SourcePhysicsContactMemory&memory,const NativeTu15ContactManagerCleanupInput&input){
    const auto&map=native_tu15_contact_manager_input_observers();const auto found=map.find(&memory);if(found!=map.end())found->second(memory,input);
}
} //namespace halo2
