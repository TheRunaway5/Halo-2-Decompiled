#pragma once
#include "halo2/native_tu15_island_agent_phase.hpp"
namespace halo2 {
// Source2F7A50 owns this16-byte body descriptor for the complete child-call
// lifetime. The caller supplies storage, not descriptor contents or results.
struct NativeTu15CompoundBodyProxy {std::uint32_t identity{};};
struct NativeTu15CompoundChildProcessCall {
    NativeTu15IslandCollisionAgent receiver;
    NativeTu15PhysicsVirtualOwner owner;
    NativeTu15CompoundBodyProxy first;
    NativeTu15PhysicsBody second;
    NativeTu15PhysicsStepConfiguration configuration;
    NativeTu15IslandContactOutput output;
};
struct NativeTu15CompoundProcessServices {
    // The actual child's vtable+10, thiscall4 / RET16. It receives the current
    // native-built descriptor and performs genuine processing. No contact,
    // count, success value or guest volatile postimage is a supplied result.
    std::function<void(const NativeTu15CompoundChildProcessCall&,
        NativeTu15PhysicsFrameFp&)>process_child;
};
struct NativeTu15CompoundProcessStats {
    std::uint64_t calls{},child_calls{},child_returns{};
};
// Complete actual2F7A50 integer parent. Arguments are ordinary immutable values;
// the explicit proxy is source data storage, not a guest control/register frame.
// Count/list are cached as source; the shape-child array is reread each loop.
void native_tu15_process_compound_agent(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15IslandContactOutput,NativeTu15CompoundBodyProxy,
    NativeTu15PhysicsFrameFp&,const NativeTu15CompoundProcessServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15CompoundProcessStats* = nullptr);
} // namespace halo2
