#pragma once
#include "halo2/native_tu15_contact_point_ids.hpp"
#include "halo2/native_tu15_contact_producer.hpp"

namespace halo2 {
struct NativeTu15ContactPointStorageScratch {std::uint32_t info{};};
struct NativeTu15ContactPointStorageServices {
    NativeTu15ArrayGrowthServices allocation;
    // Actual island virtual8, RET8(constraint,fully initialized info16).
    std::function<void(std::uint32_t island,NativeTu15ContactVirtualOwner,
        std::uint32_t constraint,std::uint32_t info,NativeTu15PhysicsFrameFp&)> add_info;
};
struct NativeTu15ContactPointStorageStats {
    std::uint64_t attempts{},completed{},refusals{},metadata_updates{},
        point_reservations{},property_reservations{},shifted_property_records{};
    NativeTu15ContactPointIdStats ids;
};
// Actual325F80. Both pointer result cells and info16 are caller-owned DATA.
// All four metadata words are initialized before virtual8. Array capacities,
// pointers and count fields retain their source read/store phases.
std::uint32_t native_tu15_allocate_contact_point_storage(SourcePhysicsContactMemory&,
    std::uint32_t collector,std::uint32_t point_cell,std::uint32_t property_cell,
    NativeTu15ContactPointStorageScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactPointStorageServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactPointStorageStats* = nullptr);
} // namespace halo2
