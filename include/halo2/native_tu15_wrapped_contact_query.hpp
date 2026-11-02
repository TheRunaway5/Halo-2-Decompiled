#pragma once
#include "halo2/native_tu15_conditional_contact_process.hpp"
#include "halo2/native_tu15_wrapped_contact_process.hpp"
namespace halo2 {
// Actual2F6F40: same affine geometry ownership as2F6CA0, but the real
// downstream owner is child virtual+4 and the fourth argument is a collector.
void native_tu15_query_wrapped_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15ContactQueryCollector,NativeTu15WrappedContactProcessScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15ConditionalContactServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15WrappedContactProcessStats* = nullptr);
struct NativeTu15ReversedWrappedContactQueryScratch {
    NativeTu15ContactQueryCollector collector; //12 retained caller-owned DATA bytes.
    NativeTu15WrappedContactProcessScratch wrapped;
};
// Actual2F73C0. Initialize its source reversed-pair collector, swap the two
// ordinary descriptors for2F6F40, and retain that DATA until the child returns.
// Source FuncInfo at37E878 owns2F0DD0 only on exception, after2F73F1.
void native_tu15_query_reversed_wrapped_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15ContactQueryCollector,NativeTu15ReversedWrappedContactQueryScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15ConditionalContactServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15WrappedContactProcessStats* = nullptr);
} // namespace halo2
