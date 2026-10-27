#pragma once
#include "halo2/native_tu15_shape_bounds.hpp"
namespace halo2 {
struct NativeTu15CompoundBoundsScratch {std::uint32_t bounds{};};
struct NativeTu15CompoundBoundsStats {std::uint64_t endpoint_calls{},compound_calls{},child_calls{};};
// Actual2E8120: transform the two source endpoints with cached matrix columns,
// then publish upper, lower, and the reread expanded upper in source order.
void native_tu15_endpoint_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
    NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
    NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CompoundBoundsStats* = nullptr);
// Actual306170: first child is unconditional, even for nonpositive count.
// Reread the list and count at each later phase. Scratch is32 aligned retained
// bytes across each genuine child call, never a virtual CPU control frame.
void native_tu15_compound_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
    NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
    NativeTu15CompoundBoundsScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ShapeBoundsServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15CompoundBoundsStats* = nullptr);
} // namespace halo2
