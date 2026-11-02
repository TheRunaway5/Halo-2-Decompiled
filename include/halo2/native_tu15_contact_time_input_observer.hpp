#pragma once
#include "halo2/native_tu15_island_contact_advance.hpp"
#include <thread>
namespace halo2 {
// Read-only native3205B0 input, observed before the first kernel access. These
// are actual typed caller values, with no fabricated guest calling convention.
struct NativeTu15ContactTimeInput {
    NativeTu15PhysicsIsland island;
    NativeTu15PhysicsStepConfiguration configuration;
    NativeTu15IslandAgentEntry entry;
    NativeTu15IslandContactOutput output;
    std::uint32_t time_bits{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
struct NativeTu15ContactTimeInputObserverActivity {
    std::uint64_t attempts{},completed{},failures{};
};
using NativeTu15ContactTimeInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15ContactTimeInput&)>;
// Scoped per-memory/executor diagnostic binding. A nested scope shadows its
// enclosing observer. No ABI image, response or physics admission is stored.
class NativeTu15ContactTimeInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15ContactTimeInputObserver observer_;
    NativeTu15ContactTimeInputObserverActivity*activity_;
    std::thread::id executor_;
    friend bool native_tu15_observe_contact_time_input(
        const SourcePhysicsContactMemory&,const NativeTu15ContactTimeInput&);
public:
    NativeTu15ContactTimeInputObserverBinding(const SourcePhysicsContactMemory&,
        NativeTu15ContactTimeInputObserver,NativeTu15ContactTimeInputObserverActivity* = nullptr);
    ~NativeTu15ContactTimeInputObserverBinding();
    NativeTu15ContactTimeInputObserverBinding(const NativeTu15ContactTimeInputObserverBinding&)=delete;
    NativeTu15ContactTimeInputObserverBinding&operator=(const NativeTu15ContactTimeInputObserverBinding&)=delete;
};
// False means there is no observer. This never selects a physics implementation
// or supplies a result. Observer failure propagates with its diagnostic count.
bool native_tu15_observe_contact_time_input(
    const SourcePhysicsContactMemory&,const NativeTu15ContactTimeInput&);
} // namespace halo2
