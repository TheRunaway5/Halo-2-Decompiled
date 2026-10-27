#pragma once
#include "halo2/native_tu15_geometry.hpp"
namespace halo2 {
struct NativeTu15BoundsShape {std::uint32_t identity{};};
struct NativeTu15BoundsTransform {std::uint32_t identity{};};
struct NativeTu15BoundsOutput {std::uint32_t identity{};};
struct NativeTu15BoundsScratch {std::uint32_t transform{};};
struct NativeTu15ShapeBoundsServices {
  // Actual child vtable+18, RET12. No bound or admission result is supplied.
  std::function<void(NativeTu15BoundsShape,std::uint32_t actual_owner,
    NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
    NativeTu15PhysicsFrameFp&)> child_bounds;
};
struct NativeTu15ShapeBoundsStats {
  std::uint64_t box_calls{},convex_calls{},transformed_calls{},forward_calls{},motion_calls{};
};
// Actual306690. Source vectors/constants are read lazily, including the two
// translation rereads after bound publication. All four lanes are significant.
void native_tu15_project_box_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsTransform,
  std::uint32_t extent_vector,std::uint32_t margin_bits,NativeTu15BoundsOutput,
  NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ShapeBoundsStats* = nullptr);
// Actual306760: the source radius addition rounds once to float before the
// native box projection. Live incoming x87 values remain owned by the caller.
void native_tu15_convex_box_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
  NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
  NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ShapeBoundsStats* = nullptr);
// Actual2E8870. The caller owns64 aligned scratch bytes across the real child
// call. Output is read after composition; margin after child/vtable inputs.
void native_tu15_transformed_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
  NativeTu15BoundsTransform,const std::function<std::uint32_t()>&margin_input,
  const std::function<NativeTu15BoundsOutput()>&output_input,NativeTu15BoundsScratch,
  NativeTu15PhysicsFrameFp&,const NativeTu15ShapeBoundsServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ShapeBoundsStats* = nullptr);
// Actual2E9600, the genuine forwarding owner.
void native_tu15_forward_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
  NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
  NativeTu15PhysicsFrameFp&,const NativeTu15ShapeBoundsServices&,
  NativeTu15ShapeBoundsStats* = nullptr);
// Actual2E5E80, including the post-child source motion sweep and exact MIN/MAX
// operand order. The body/velocity fields are reread after genuine child work.
void native_tu15_motion_shape_bounds(SourcePhysicsContactMemory&,NativeTu15BoundsShape,
  NativeTu15BoundsTransform,std::uint32_t margin_bits,NativeTu15BoundsOutput,
  NativeTu15PhysicsFrameFp&,const NativeTu15ShapeBoundsServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ShapeBoundsStats* = nullptr);
} // namespace halo2
