#pragma once
#include "halo2/physics_body_slots.hpp"

namespace halo2 {
// Real caller scratch produced at the original stack owners. These packets
// are temporary calculations, never retained shape/body identities or poses.
struct SourcePhysicsShapeWrapperScratch {
  std::uint32_t extent_maximum, extent_minimum, transform, configuration;
};
// Actual1DACF0->1DADF0/184510/1DAD80 caller layout. The input is
// 1DACF0 entry ESP; returned packets remain caller-owned temporaries.
SourcePhysicsShapeWrapperScratch source_physics_shape_wrapper_scratch_1_1(
    std::uint32_t entry_esp);
// Actual184510→184800 size calculation for resident box6, capsule7, convex8
// transform21 and convex wrapper23. Authored shapes and source dispatch
// tables remain required; other positive shape-class owners are explicit.
// Returns the original XMM0 low word, not the incidental EAX scratch address.
std::uint32_t source_physics_convex_extents_1_1(
    SourcePhysicsContactMemory &, std::uint32_t shape,
    const SourcePhysicsShapeWrapperScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Actual1DAD80 and its complete2E2530/2E25A0/2E2260 construction. Uses the
// existing actual small-block allocator; retains underlying shape and body.
std::uint32_t source_construct_physics_shape_wrapper_1_1(
    SourcePhysicsContactMemory &, std::uint32_t shape, std::uint32_t body,
    std::uint32_t scalar_word, const SourcePhysicsShapeWrapperScratch &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Actual1DADF0 direct and transform21 branches. Source game clock518444+2,
// global486564, caller scalar and original SSE/x87 arithmetic own parameters.
// Compound/proxy/other positive shape branches require their owning modules.
std::uint32_t source_prepare_physics_shape_wrapper_1_1(
    SourcePhysicsContactMemory &, std::uint32_t shape, std::uint32_t body,
    std::uint32_t caller_scalar_word, const SourcePhysicsShapeWrapperScratch &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Actual1DACF0 measured mode7 skip and mode4 wrapping/reference commit. A
// zero refcount requires the original deferred-delete/destructor owner and
// stops there after the observed decrement; no successful disposal default.
std::uint32_t source_commit_physics_shape_wrapper_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t body,
    std::uint32_t caller_scalar_word, const SourcePhysicsShapeWrapperScratch &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
