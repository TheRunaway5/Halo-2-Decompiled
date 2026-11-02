#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Nine source-owned scalar DATA cells at+C..+30. Every consumed value has a
// source arithmetic writer; this is not CPU control or register storage.
struct NativeTu15ClosestSegmentsScratch {std::uint32_t identity{};};
// Actual312620: closest points of two bounded segments. The output contains
// point on the first segment, first-minus-second displacement, squared length.
// Returns the source endpoint clamp mask. Uses native SIMD/x87 arithmetic.
std::uint32_t native_tu15_closest_segment_pair(SourcePhysicsContactMemory&,
    std::uint32_t first_point,std::uint32_t first_direction,
    std::uint32_t second_point,std::uint32_t second_direction,std::uint32_t output,
    NativeTu15ClosestSegmentsScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
