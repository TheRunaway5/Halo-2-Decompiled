#pragma once
#include "halo2/native_tu15_contact_completion.hpp"
namespace halo2 {
struct NativeTu15ContactEntityNotificationServices {
    // Actual1D8A20 receives this event DATA as well as its five ordinary
    // parameters. The ABI adapter supplies its genuine implicit event input.
    std::function<void(NativeTu15ContactCompletionPacket,std::uint32_t component,
        std::uint32_t own_datum,std::uint32_t other_datum,std::uint32_t own_slot,
        std::uint32_t other_slot,NativeTu15PhysicsFrameFp&)>process_contacts;
};
struct NativeTu15ContactEntityNotificationStats {
    std::uint64_t calls{},missing_body_pairs{},property_queries{},disabled_components{},dispatch_calls{};
};
// Actual281E10 with closed1D7530/1D75B0 property selection. Follow each actual
// payload parent before entity classification, preserve first/second read order
// and source datum/slot selection; no result or CPU frame enters the kernel.
void native_tu15_notify_contact_entities(const SourcePhysicsContactMemory&,
    NativeTu15ContactCompletionPacket,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactEntityNotificationServices&,
    NativeTu15ContactEntityNotificationStats* = nullptr);
} // namespace halo2
