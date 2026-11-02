#pragma once
#include "halo2/native_tu15_contact_entity_notification.hpp"
namespace halo2 {
struct NativeTu15ContactEntityProcessorServices {
    // Actual1D77D0 consumes manager/row DATA, a reversed-side byte, the owning
    // datum and slot. Its first stack parameter is the source constant zero.
    std::function<void(std::uint32_t manager,std::uint32_t row,bool reversed,
        std::uint32_t own_datum,std::uint32_t own_slot,NativeTu15PhysicsFrameFp&)>update_contact;
};
struct NativeTu15ContactEntityProcessorStats {
    std::uint64_t calls{},contacts{},candidate_rows{},matched_rows{},updates{};
};
// Actual1D8A20 contact/row matching. Counts, backing and event ownership are
// reread at their source boundaries after callbacks. No CPU frame is retained.
void native_tu15_process_entity_contacts(const SourcePhysicsContactMemory&,
    NativeTu15ContactCompletionPacket,std::uint32_t component,
    std::uint32_t own_datum,std::uint32_t other_datum,std::uint32_t own_slot,
    std::uint32_t other_slot,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactEntityProcessorServices&,
    NativeTu15ContactEntityProcessorStats* = nullptr);
} // namespace halo2
