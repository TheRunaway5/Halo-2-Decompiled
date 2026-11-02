#pragma once
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
struct NativeTu15ContactPointIdStats {
    std::uint64_t attempts{},completed{},refusals{},reused{},appended{},growths{};
};
// Actual325F20. The array contains byte indices; FF marks an available slot.
// A reverse search reuses the last free slot. Otherwise the source array grows
// natively and appends the low input byte. The returned slot is a full DWORD.
// Header/data/count are read again at their source phases, including after
// allocation and after the byte store. Input is ordinary caller-owned DATA.
std::uint32_t native_tu15_allocate_contact_point_id(SourcePhysicsContactMemory&,
    NativeTu15ArrayHeader,std::uint32_t index,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactPointIdStats* = nullptr);
} // namespace halo2
