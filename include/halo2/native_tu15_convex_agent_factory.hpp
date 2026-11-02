#pragma once
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "halo2/native_tu15_contact_cache.hpp"
namespace halo2 {
struct NativeTu15ConvexAgentArguments {
    std::function<std::uint32_t()> first_payload,second_payload,user_data;
};
struct NativeTu15ConvexAgentScratch {
    // Fully written64B relative matrix, plus native cache-vector DATA.
    std::uint32_t relative_transform{};
    NativeTu15ContactCacheScratch cache;
};
struct NativeTu15ConvexAgentServices {
    NativeTu15ArrayGrowthServices allocation;
    NativeTu15ContactCacheServices cache;
    std::function<std::uint32_t(NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,NativeTu15PhysicsFrameFp&)> shape_class;
};
// Actual2F8300; caller owns DATA scratch and ordinary payload inputs only.
NativeTu15ContactAgent native_tu15_initialize_convex_contact_agent(
    SourcePhysicsContactMemory&,NativeTu15ContactAgent,const NativeTu15ConvexAgentArguments&,
    NativeTu15ConvexAgentScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConvexAgentServices&,const SourcePhysicsContactRecordObserver& = {});
// Actual2E4EB0; reads lateuser/second/first inputs after real256B allocation
// and size publication. Does not initialize any source-unwritten heap byte.
NativeTu15ContactAgent native_tu15_create_convex_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15ConvexAgentArguments&,
    NativeTu15ConvexAgentScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConvexAgentServices&,const SourcePhysicsContactRecordObserver& = {});
// Actual2F83B0; the base GSK factory allocates240B and retains2F8300's
// vtable41CD08. It has no derived vtable or out-of-object +F0 store.
NativeTu15ContactAgent native_tu15_create_gsk_contact_agent(
    SourcePhysicsContactMemory&,const NativeTu15ConvexAgentArguments&,
    NativeTu15ConvexAgentScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConvexAgentServices&,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
