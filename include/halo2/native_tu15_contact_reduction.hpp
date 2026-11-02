#pragma once
#include "halo2/native_tu15_contact_distance_publication.hpp"
namespace halo2 {
struct NativeTu15ContactReductionStats {
    std::uint64_t attempts{},completed{},refusals{},removed_points{},record_moves{};
};
// Actual30CAA0. The source reads a current five-pointer array, selects four
// separated records (possibly fewer when no positive metric exists), removes
// unselected manager points, then moves the last48B record into each hole.
// Source's two unused payload formals need no native representation.
std::uint32_t native_tu15_reduce_contact_records(SourcePhysicsContactMemory&,
    std::uint32_t pointer_array,std::uint32_t manager,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15ContactDistanceServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactReductionStats* = nullptr);
} // namespace halo2
