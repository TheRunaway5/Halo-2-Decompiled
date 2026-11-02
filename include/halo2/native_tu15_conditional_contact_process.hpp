#pragma once
#include "halo2/native_tu15_compound_process.hpp"

namespace halo2 {
struct NativeTu15ContactQueryCollector {std::uint32_t identity{};};
struct NativeTu15ConditionalContactScratch {
    NativeTu15CompoundBodyProxy body; //16 caller-owned DATA bytes.
    NativeTu15ContactQueryCollector collector; //8 caller-owned DATA bytes.
};
struct NativeTu15ConditionalContactServices {
    std::function<std::uint64_t()> timestamp_counter;
    // Actual query virtual+4, thiscall4 / RET16. The collector's source-owned
    // flag byte is written by that query; no query result is supplied here.
    std::function<void(NativeTu15IslandCollisionAgent,NativeTu15PhysicsVirtualOwner,
        NativeTu15CompoundBodyProxy,NativeTu15PhysicsBody,
        NativeTu15PhysicsStepConfiguration,NativeTu15ContactQueryCollector,
        NativeTu15PhysicsFrameFp&)> query;
    std::function<std::uint32_t(std::uint32_t shape,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsFrameFp&)> shape_class;
    // Actual context dispatcher cdecl(first,second,context,manager).
    std::function<NativeTu15IslandCollisionAgent(NativeTu15PhysicsVirtualOwner,
        NativeTu15CompoundBodyProxy,NativeTu15PhysicsBody,
        NativeTu15PhysicsStepConfiguration,std::uint32_t manager,
        NativeTu15PhysicsFrameFp&)> create_agent;
    NativeTu15CompoundProcessServices processing;
    // Actual cached-agent virtual+14, thiscall0 / RET0.
    std::function<void(NativeTu15IslandCollisionAgent,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsFrameFp&)> clear_agent;
};
struct NativeTu15ConditionalContactStats {
    std::uint64_t calls{},completed{},refusals{},query_calls{},class_calls{},
        factory_calls{},process_calls{},clear_calls{},profile_entries{};
};
// Complete source2F0F10 parent, with genuine query/factory/process/lifetime
// dependencies explicit. All16 descriptor bytes and the5 defined collector
// bytes are initialized from current native inputs before the query. The
// collector's remaining3 bytes are never read by this parent.
void native_tu15_process_conditional_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
    NativeTu15IslandContactOutput,NativeTu15ConditionalContactScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15ConditionalContactServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConditionalContactStats* = nullptr);
} // namespace halo2
