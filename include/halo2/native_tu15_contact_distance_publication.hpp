#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
#include "halo2/native_tu15_array_reserve.hpp"

namespace halo2 {
struct NativeTu15ContactDistanceServices {
    NativeTu15ArrayGrowthServices allocation;
    // Actual current contact-manager virtual8, RET4(uint16 point).
    std::function<void(std::uint32_t manager, NativeTu15ContactVirtualOwner,
        std::uint16_t point, NativeTu15PhysicsFrameFp&)> remove_point;
};
struct NativeTu15ContactDistanceStats {
    std::uint64_t attempts{}, completed{}, refusals{}, reservations{},
        tested_records{}, published_records{}, removed_points{};
};
// Actual30CF40, with the eight cdecl DATA inputs. Output is an indirection to
// the current array header. Capacity/count/data and point metadata are reread
// at their source phases, including after callbacks. The scalar temporaries
// are ordinary native values; no original private/control frame is an input.
void native_tu15_publish_contact_distances(SourcePhysicsContactMemory&,
    NativeTu15ContactPayload first, NativeTu15ContactPayload second,
    NativeTu15ContactContext, std::uint32_t cutoff_bits,
    std::uint32_t records, std::uint32_t count_field, NativeTu15ContactOutput,
    std::uint32_t manager, NativeTu15PhysicsFrameFp&, unsigned incoming_x87_depth,
    const NativeTu15ContactDistanceServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactDistanceStats* = nullptr);
} // namespace halo2
