#pragma once
#include "halo2/native_tu15_contact_agent_factory.hpp"
namespace halo2 {
struct NativeTu15CollectionContactArguments {
    std::function<std::uint32_t()> first,second,context,user_data;
};
struct NativeTu15CollectionContactScratch {
    // Sixteen child-payload DATA bytes and one filter-result DATA byte.
    std::uint32_t child_payload{},filter_result{};
};
struct NativeTu15CollectionContactServices {
    NativeTu15ArrayGrowthServices allocation;
    NativeTu15ContactAgentDispatchServices dispatch;
    std::function<std::uint32_t(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> count_children;
    // Genuine current virtual filter owner with six ordinary arguments. Its
    // returned byte pointer is consumed by the source; neither that pointer
    // nor its predicate value may be replaced with a supplied outcome.
    std::function<std::uint32_t(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t output,NativeTu15ContactContext,NativeTu15ContactPayload second,
        NativeTu15ContactPayload first,NativeTu15ContactChild collection,
        std::uint32_t index,NativeTu15PhysicsFrameFp&)> filter_child;
};
// Complete actual2F7D20. The positive-count source path has no meaningful
// agent return ABI; the actual factory below retains its allocation identity.
// Array reserve2E31E0 and both real child services retain their own owners.
void native_tu15_initialize_collection_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,const NativeTu15CollectionContactArguments&,
    NativeTu15CollectionContactScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15CollectionContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Complete actual2F81F0, including its deliberate first/second reversal.
// Original FuncInfo46BAE0/46BB70 supplies base, array and factory cleanup.
NativeTu15ContactAgent native_tu15_create_collection_contact_agent(SourcePhysicsContactMemory&,
    const NativeTu15CollectionContactArguments&,NativeTu15CollectionContactScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15CollectionContactServices&,
    const SourcePhysicsContactRecordObserver& = {});
}
