#pragma once
#include "halo2/native_tu15_shape_bounds.hpp"
namespace halo2 {
struct NativeTu15SphereBoundsStats {std::uint64_t calls{};};
// Actual2E8D90: one x87-rounded margin+radius, then ordered native SIMD
// center-minus/plus stores. The upper store rereads the current center after
// the lower store; all four lanes and aliased/NaN behavior are preserved.
void native_tu15_sphere_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
    NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SphereBoundsStats* = nullptr);
} // namespace halo2
