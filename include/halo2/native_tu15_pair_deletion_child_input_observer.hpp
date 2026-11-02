#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <map>
#include <optional>
#include <vector>
namespace halo2 {
struct NativeTu15PairDeletionChildInput {
    std::uint32_t owner{};
    std::optional<std::uint32_t> receiver;
    std::vector<std::uint32_t> arguments;
    unsigned return_pop{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15PairDeletionChildInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15PairDeletionChildInput&)>;
inline auto&native_tu15_pair_deletion_child_input_observers(){
    static thread_local std::map<const SourcePhysicsContactMemory*,NativeTu15PairDeletionChildInputObserver> observers;return observers;
}
class NativeTu15PairDeletionChildInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    std::optional<NativeTu15PairDeletionChildInputObserver>previous_;
public:
    NativeTu15PairDeletionChildInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15PairDeletionChildInputObserver observer):memory_(&memory){
        auto&map=native_tu15_pair_deletion_child_input_observers();const auto found=map.find(memory_);
        if(found!=map.end())previous_=found->second;map[memory_]=std::move(observer);
    }
    ~NativeTu15PairDeletionChildInputObserverBinding(){
        auto&map=native_tu15_pair_deletion_child_input_observers();
        if(previous_)map[memory_]=std::move(*previous_);else map.erase(memory_);
    }
    NativeTu15PairDeletionChildInputObserverBinding(const NativeTu15PairDeletionChildInputObserverBinding&)=delete;
};
inline void native_tu15_observe_pair_deletion_child_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15PairDeletionChildInput&input){
    const auto&map=native_tu15_pair_deletion_child_input_observers();const auto found=map.find(&memory);
    if(found!=map.end())found->second(memory,input);
}
}
