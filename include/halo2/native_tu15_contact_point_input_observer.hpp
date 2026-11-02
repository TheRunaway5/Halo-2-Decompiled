#pragma once
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <functional>
#include <map>
#include <optional>
namespace halo2 {
struct NativeTu15ContactPointRemovalInput {
    std::uint32_t manager{},owner{};
    std::uint16_t point{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15ContactPointRemovalInputObserver=std::function<void(const SourcePhysicsContactMemory&,const NativeTu15ContactPointRemovalInput&)>;
inline auto&native_tu15_contact_point_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15ContactPointRemovalInputObserver>observers;return observers;
}
class NativeTu15ContactPointRemovalInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;std::optional<NativeTu15ContactPointRemovalInputObserver>previous_;
public:
    NativeTu15ContactPointRemovalInputObserverBinding(const SourcePhysicsContactMemory&memory,NativeTu15ContactPointRemovalInputObserver observer):memory_(&memory){
        auto&map=native_tu15_contact_point_input_observers();const auto found=map.find(memory_);if(found!=map.end())previous_=found->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15ContactPointRemovalInputObserverBinding(){auto&map=native_tu15_contact_point_input_observers();if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);}
    NativeTu15ContactPointRemovalInputObserverBinding(const NativeTu15ContactPointRemovalInputObserverBinding&)=delete;
};
inline void native_tu15_observe_contact_point_removal_input(const SourcePhysicsContactMemory&memory,const NativeTu15ContactPointRemovalInput&input){
    const auto&map=native_tu15_contact_point_input_observers();const auto found=map.find(&memory);if(found!=map.end())found->second(memory,input);
}
} // namespace halo2
