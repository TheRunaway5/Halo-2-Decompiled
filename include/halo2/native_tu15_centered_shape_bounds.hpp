#pragma once
#include "halo2/native_tu15_shape_bounds.hpp"
namespace halo2 {
struct NativeTu15CenteredShapeBoundsStats {std::uint64_t centered_box_calls{},radius_calls{};};
// Actual1D64C0: project half extents first, publish both bounds, then reread
// and transform the center. Preserve each four-lane publication and alias.
void native_tu15_project_centered_box_bounds(SourcePhysicsContactMemory&,
  NativeTu15BoundsTransform,std::uint32_t half_extents,std::uint32_t center,
  std::uint32_t margin_bits,NativeTu15BoundsOutput,NativeTu15PhysicsFrameFp&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15CenteredShapeBoundsStats* = nullptr);
// Actual307E30: round margin+shape.radius once to float before projecting the
// source half extents/center at+10/+20. No shape kind is inferred from layout.
void native_tu15_centered_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
  NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
  NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15CenteredShapeBoundsStats* = nullptr);
} // namespace halo2
