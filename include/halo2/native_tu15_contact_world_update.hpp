#pragma once
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"

namespace halo2 {
struct NativeTu15ContactWorldUpdateArguments {
    // Current ordinary formal DATA, consumed at the source phases after growth
    // and after current callbacks. They never supply a source register outcome.
    std::function<std::uint32_t()> result_byte,collector;
};
struct NativeTu15ContactWorldUpdateServices {
    NativeTu15ArrayGrowthServices allocation;
    // Current world listener virtual+C, RET4(collector).
    std::function<void(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t collector,NativeTu15PhysicsFrameFp&)> listener;
    // Current island virtual+4, RET4(collector). The selected island is cached
    // before notifying world listeners, just as in316AA0.
    std::function<void(std::uint32_t island,NativeTu15ContactVirtualOwner,
        std::uint32_t collector,NativeTu15PhysicsFrameFp&)> remove_from_island;
};
struct NativeTu15ContactWorldUpdateStats {
    std::uint64_t attempts{},completed{},queued{},immediate{},listeners{},islands{},growths{};
};
// Actual3223D0, reverse world+60 listener dispatch and stable null compaction.
void native_tu15_notify_contact_world_update(SourcePhysicsContactMemory&,
    std::uint32_t world,std::uint32_t collector,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactWorldUpdateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactWorldUpdateStats* = nullptr);
// Actual316AA0. Deferred queue uses the existing qualified native array grower.
void native_tu15_update_contact_world(SourcePhysicsContactMemory&,
    std::uint32_t world,const NativeTu15ContactWorldUpdateArguments&,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactWorldUpdateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactWorldUpdateStats* = nullptr);
void native_tu15_update_contact_world(SourcePhysicsContactMemory&,
    std::uint32_t world,std::uint32_t result_byte,std::uint32_t collector,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactWorldUpdateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactWorldUpdateStats* = nullptr);
} // namespace halo2
