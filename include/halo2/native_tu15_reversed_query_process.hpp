#pragma once
#include "halo2/native_tu15_compound_process.hpp"

namespace halo2 {
struct NativeTu15ReversedQueryProcessStats {
    std::uint64_t calls{}, child_calls{}, child_returns{}, flipped_records{},
        completed{}, refusals{};
};
// Actual2F1A50 / RET16. Snapshot the current output count, reverse the first
// two payloads into genuine2F0F10, then flip every newly appended48-byte
// record's vector at+10. The mutable holder/header/backing/count are reread
// at their source phases. No scratch, output count or child result is supplied.
// process_child is the explicitly typed2F0F10 dependency; the wrapper alone
// does not establish that dependency's native closure.
void native_tu15_process_reversed_query_agent(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent, NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second, NativeTu15PhysicsStepConfiguration,
    NativeTu15IslandContactOutput, NativeTu15PhysicsFrameFp&,
    const NativeTu15CompoundProcessServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ReversedQueryProcessStats* = nullptr);
} // namespace halo2
