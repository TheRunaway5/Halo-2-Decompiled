#pragma once
#include "halo2/native_tu15_pair_consumer.hpp"
#include <thread>
namespace halo2 {
enum class NativeTu15PairConsumerInputPhase {Entry,OwnedRelease,OwnedAllocation,SlowArenaRelease,SlowArenaAllocation};
// Actual typed DATA only. source_call_site is zero when the caller is not known;
// a service event does not pretend to be the original root's calling convention.
struct NativeTu15PairConsumerInput {
    NativeTu15PairConsumerInputPhase phase=NativeTu15PairConsumerInputPhase::Entry;
    NativeTu15PhysicsIsland island;
    NativeTu15BroadPhasePairs first,second;
    NativeTu15PairConsumerScratch scratch;
    std::uint32_t allocator{},owner{},data{},bytes{},tag{},source_call_site{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
struct NativeTu15PairConsumerInputObserverActivity {std::uint64_t attempts{},completed{},failures{};};
using NativeTu15PairConsumerInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15PairConsumerInput&)>;
class NativeTu15PairConsumerInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15PairConsumerInputObserver observer_;
    NativeTu15PairConsumerInputObserverActivity*activity_;
    std::thread::id executor_;
    friend bool native_tu15_observe_pair_consumer_input(
        const SourcePhysicsContactMemory&,const NativeTu15PairConsumerInput&);
public:
    NativeTu15PairConsumerInputObserverBinding(const SourcePhysicsContactMemory&,
        NativeTu15PairConsumerInputObserver,NativeTu15PairConsumerInputObserverActivity* = nullptr);
    ~NativeTu15PairConsumerInputObserverBinding();
    NativeTu15PairConsumerInputObserverBinding(const NativeTu15PairConsumerInputObserverBinding&)=delete;
    NativeTu15PairConsumerInputObserverBinding&operator=(const NativeTu15PairConsumerInputObserverBinding&)=delete;
};
bool native_tu15_observe_pair_consumer_input(
    const SourcePhysicsContactMemory&,const NativeTu15PairConsumerInput&);
} // namespace halo2
