#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <map>
#include <optional>
namespace halo2 {
enum class NativeTu15PhantomAddedInputPhase { ListenerEntry,BeforeObjectDeletion };
struct NativeTu15PhantomAddedInput {
    NativeTu15PhantomAddedInputPhase phase{};
    std::uint32_t owner{},listener{},event{},object_datum{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15PhantomAddedInputObserver=std::function<void(const SourcePhysicsContactMemory&,const NativeTu15PhantomAddedInput&)>;
inline auto&native_tu15_phantom_added_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15PhantomAddedInputObserver>observers;return observers;
}
class NativeTu15PhantomAddedInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;std::optional<NativeTu15PhantomAddedInputObserver>previous_;
public:
    NativeTu15PhantomAddedInputObserverBinding(const SourcePhysicsContactMemory&memory,NativeTu15PhantomAddedInputObserver observer):memory_(&memory){
        auto&map=native_tu15_phantom_added_input_observers();const auto old=map.find(memory_);if(old!=map.end())previous_=old->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15PhantomAddedInputObserverBinding(){auto&map=native_tu15_phantom_added_input_observers();if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);}
    NativeTu15PhantomAddedInputObserverBinding(const NativeTu15PhantomAddedInputObserverBinding&)=delete;
};
inline void native_tu15_observe_phantom_added_input(const SourcePhysicsContactMemory&memory,const NativeTu15PhantomAddedInput&input){
    const auto&map=native_tu15_phantom_added_input_observers();const auto found=map.find(&memory);if(found!=map.end())found->second(memory,input);
}
} // namespace halo2
