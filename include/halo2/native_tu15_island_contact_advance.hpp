#pragma once
#include "halo2/native_tu15_island_agent_phase.hpp"
namespace halo2 {
// Actual configuration+10 is passed unchanged to motion vtable+74. Its
// unconverted owner retains the interpretation of this authenticated word.
struct NativeTu15MotionAdvanceConfigurationWord {std::uint32_t value{};};
struct NativeTu15IslandContactAdvanceServices {
    std::function<void(NativeTu15PhysicsMotion,NativeTu15PhysicsVirtualOwner,
        std::uint32_t time_bits,NativeTu15MotionAdvanceConfigurationWord,
        NativeTu15PhysicsFrameFp&)>advance_motion;
    std::function<void(NativeTu15IslandCollisionAgent,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
        NativeTu15PhysicsStepConfiguration,NativeTu15IslandContactOutput,
        NativeTu15PhysicsFrameFp&)>process_agent;
};
struct NativeTu15IslandContactAdvanceStats {
    std::uint64_t calls{},motion_calls{},process_calls{},single_motion_iterations{},
        paired_motion_iterations{},negative_single_restores{},negative_paired_restores{};
};
// Actual TU1.5 3205B0: align the two actual cached motion times, classify
// genuine current contacts, and bisect motion time using source x87 rounding.
// No GP/decoder or supplied collision/motion result is part of this kernel.
// The named genuine virtual owners remain explicit until independently ported.
void native_tu15_advance_island_contact_time(SourcePhysicsContactMemory&,
    NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,
    NativeTu15IslandAgentEntry,NativeTu15IslandContactOutput,std::uint32_t time_bits,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15IslandContactAdvanceServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandContactAdvanceStats* = nullptr);
} // namespace halo2
