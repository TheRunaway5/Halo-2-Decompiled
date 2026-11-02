#pragma once
#include "halo2/native_tu15_phantom_overlap_add.hpp"

namespace halo2 {
struct NativeTu15ObjectPhantomDeleteProtectionStats {
    std::uint64_t queries{},other_modes{},mode_four{},protected_objects{};
};
// Exact B1870 AL predicate. Its only input is the object datum DATA identity;
// upper EAX bits and saved GP words are not semantic bool output.
bool native_tu15_object_phantom_delete_protected(const SourcePhysicsContactMemory&,
    std::uint32_t object_datum,NativeTu15ObjectPhantomDeleteProtectionStats* = nullptr);
struct NativeTu15PhantomAddedListenerServices {
    std::function<void(std::uint32_t object_datum,NativeTu15PhysicsFrameFp&)> delete_object;
};
struct NativeTu15PhantomAddedListenerStats {
    std::uint64_t calls{},ignored_wrappers{},null_bodies{},property_searches{},
        property_matches{},null_datums{},marked_components{},protected_objects{},
        delete_requests{},unbound_delete_requests{};
    NativeTu15ObjectPhantomDeleteProtectionStats protection;
};
// Actual282700 added-listener RET4(event). Incoming ECX receiver is never read.
// BA570 remains a typed required child when selected; no CPU fallback exists.
void native_tu15_notify_gameplay_phantom_added(SourcePhysicsContactMemory&,
    NativeTu15PhantomOverlapAddedEvent,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhantomAddedListenerServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15PhantomAddedListenerStats* = nullptr);
} // namespace halo2
