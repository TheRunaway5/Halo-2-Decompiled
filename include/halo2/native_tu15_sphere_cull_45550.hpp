#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
// Whole 458C0(unit, ranges, first, second). The unit's +0 bit0 is set first
// when either list has rows (an unconditional read-modify-write). With
// `ranges`, `first` holds {begin,end} pairs (two rows per step) of the unit
// geometry's +34 leaf-bit table offset by its +8 base; every other row's low
// half is a bit of the unit's +14 mask. `first_count` is the array's count;
// an odd pair count also reads the stored row after it, so `first` must hold
// that row too. The original rereads no list storage its own stores reach.
void native_tu15_sphere_mask_458c0(SourcePhysicsContactMemory&,std::uint32_t unit,bool ranges,
    std::span<const std::uint32_t> first,std::uint32_t first_count,std::span<const std::uint32_t> second);
// Final rows of the two arrays 45550 passes to 2E7C40: `ranges` is the one
// 458C0 reads as pairs (with the row after an odd count), `leaves` the bits.
struct NativeTu15SphereQueryRows {std::vector<std::uint32_t> ranges,leaves;std::uint32_t range_count{};};
struct NativeTu15SphereCullServices {
    // 2E7C40(code, {center, radius}) into 45550's two 512-row inline arrays.
    // The service owns those arrays, including the [48C938]+14 release of a
    // grown backing (leaves first), and returns their final rows.
    std::function<void(std::uint32_t code,const std::array<std::uint32_t,4>& sphere,
        NativeTu15SphereQueryRows&,NativeTu15PhysicsFrameFp&)> query_2e7c40;
};
struct NativeTu15SphereCullCoverage {
    std::uint64_t calls{},no_geometry{},queries{},parts{},already{},unbounded{},bsp_parts{},no_object{},
        same_node{},no_nodes{},transforms{},hits{},misses{};
};
// Whole 45550(ECX unit, center, radius): mark every leaf of the unit's
// geometry that touches the sphere. A geometry with a +30 query code asks
// 2E7C40 and 458C0; otherwise each leaf sphere (+14 rows of 20 bytes) is
// tested, in the original SSE order, in its node's space for model units.
// Requires masked original FP exceptions (std::invalid_argument, no effects).
void native_tu15_sphere_cull_45550(SourcePhysicsContactMemory&,std::uint32_t unit,
    const std::array<std::uint32_t,3>& center,std::uint32_t radius,NativeTu15PhysicsFrameFp&,
    const NativeTu15SphereCullServices&,NativeTu15SphereCullCoverage* = nullptr);
}
