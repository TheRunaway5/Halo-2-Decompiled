#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15BspSegmentQuery {
    std::uint32_t flags{},descriptor{},output{},epsilon_bits{};
    std::uint16_t bitmap_limit{};
    std::uint32_t bitmap{},origin{},displacement{};
};
struct NativeTu15BspSegmentQueryStats {
    std::uint64_t calls{},nodes{},leaves{},surface_queries{},hits{},completed{};
};
// Actual1E6980. Context is initialized caller-owned 40B DATA, not a CPU frame.
bool native_tu15_traverse_bsp_segment(SourcePhysicsContactMemory&,
    std::uint32_t context,std::uint32_t node,std::uint32_t minimum_bits,
    std::uint32_t maximum_bits,NativeTu15PhysicsFrameFp&,unsigned preserved_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15BspSegmentQueryStats* = nullptr);
// Actual1E68E0. The caller owns a separate 40B context lifetime. Padding is
// neither initialized nor consumed; all consumed fields come from query DATA.
bool native_tu15_query_bsp_segment(SourcePhysicsContactMemory&,
    const NativeTu15BspSegmentQuery&,std::uint32_t context,
    NativeTu15PhysicsFrameFp&,unsigned preserved_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15BspSegmentQueryStats* = nullptr);
} // namespace halo2
