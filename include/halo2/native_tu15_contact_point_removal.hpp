#pragma once
#include "halo2/native_tu15_agent_cleanup.hpp"

namespace halo2 {
struct NativeTu15ContactPointRemovalScratch {
    // Ordinary caller-owned DATA: 20-byte event, 16-byte removal information,
    // and a one-byte result. Source event+2..3 are unwritten. Their current
    // caller-owned bytes are preserved; admission must qualify callback readers.
    std::uint32_t event{},removal_info{},result_byte{};
};
struct NativeTu15ContactPointRemovalServices {
    // Actual current virtual+4, RET4(event DATA).
    std::function<void(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t event,NativeTu15PhysicsFrameFp&)> listener_removed;
    // Actual current collector+14 virtual+C, RET8(collector, info16B DATA).
    std::function<void(std::uint32_t receiver,NativeTu15ContactVirtualOwner,
        std::uint32_t collector,std::uint32_t info,NativeTu15PhysicsFrameFp&)> collector_removed;
    // Actual316AA0, RET8(result-byte DATA,collector), remains explicit here.
    std::function<void(std::uint32_t world,std::uint32_t result_byte,
        std::uint32_t collector,NativeTu15PhysicsFrameFp&)> update_world;
};
// Actual3224D0(world) and31FEE0(body), including reverse dispatch and stable
// null compaction. The event argument is ordinary current caller-owned DATA.
void native_tu15_notify_removed_contact_point(SourcePhysicsContactMemory&,
    bool world,std::uint32_t object,std::uint32_t event,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactPointRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual325D90. Ordered seven-word record moves and current map fixups.
void native_tu15_remove_collector_contact_point(SourcePhysicsContactMemory&,
    std::uint32_t collector,std::uint32_t point,std::uint32_t removal_info,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactPointRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual322F70, RET4(point). No source CPU register or control-frame metadata.
void native_tu15_remove_contact_point(SourcePhysicsContactMemory&,
    std::uint32_t manager,std::uint32_t point,NativeTu15ContactPointRemovalScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactPointRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
