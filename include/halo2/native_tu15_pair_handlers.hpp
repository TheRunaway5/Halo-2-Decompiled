#pragma once
#include "halo2/native_tu15_activation_agents.hpp"

namespace halo2 {
// Four caller-owned DATA bytes initially containing the current pair pointer.
// The source reuses that formal DATA as dispatch/body pointers, then as the
// discrete query's byte result. No control-stack identity is required.
struct NativeTu15PairHandlerScratch {std::uint32_t formal_pair_data{};};
struct NativeTu15PairHandlerServices {
    NativeTu15ActivationAgentServices agents;
    std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,
        NativeTu15PhysicsFrameFp&)> activate_island; //316D40
    std::function<void(NativeTu15PhysicsIsland,std::uint32_t pair,
        NativeTu15PhysicsFrameFp&)> defer_pair; //320DA0
    std::function<void(NativeTu15PhysicsWorld,NativeTu15PhysicsBody,
        NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)> merge_bodies; //317200
    std::function<void(NativeTu15ContactAgent,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> destroy_agent;
};
// Actual320EF0/320260; current typed listener and pair DATA, no CPU return
// image. Both sources retain their live cursor/array rereads after services.
void native_tu15_create_pair(SourcePhysicsContactMemory&,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PairHandlerScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15PairHandlerServices&,const SourcePhysicsContactRecordObserver& = {});
// Ordinary source-owned formal DATA, repurposed by3203C9; no CPU frame.
struct NativeTu15PairDeletionScratch {std::uint32_t formal_pair_data{};};
void native_tu15_delete_pair(SourcePhysicsContactMemory&,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PairDeletionScratch,NativeTu15PhysicsFrameFp&,const NativeTu15PairHandlerServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_delete_pair(SourcePhysicsContactMemory&,std::uint32_t handler,
    std::uint32_t pair,NativeTu15PhysicsFrameFp&,const NativeTu15PairHandlerServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
