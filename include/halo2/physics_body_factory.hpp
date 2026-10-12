#pragma once
#include "halo2/physics_body_state.hpp"

namespace halo2 {
struct SourcePhysicsFactoryFloatingPoint {
  std::uint16_t x87_control_word;
  std::uint32_t mxcsr;
};
// Scratch storage is owned by the caller's real frame/arena. The addresses
// are never retained as object identities. Distinct source AABB callers have
// distinct scratch packets; missing storage is an error in the memory adapter.
struct SourcePhysicsBodyFactoryScratch {
  std::uint32_t info, quaternion, auxiliary_bounds, radius_bounds;
};
// Actual1DAAC0/318100/317BF0/31AC90/31ABD0/317D10 caller layout.
// Derives temporary addresses from entry ESP, without an execution capture.
SourcePhysicsBodyFactoryScratch source_physics_body_factory_scratch_1_1(
    std::uint32_t entry_esp);
struct SourcePhysicsMotionConstruction {
  std::uint32_t position, quaternion, inertia, center;
  std::uint32_t mass_word, mode;
};
// Required original1DC782 RSQRTSS platform estimate, before source Newton
// refinement. No mathematical inverse-sqrt or successful estimate default.
// Xbox estimate/hardware parity is not established by host x86 instructions.
using SourcePhysicsReciprocalSqrtEstimate =
    std::function<std::uint32_t(std::uint32_t)>;

// Explicit actual host platform instruction provider; it has no captured data
// lookup. Its intermediate estimate is not claimed to equal Xbox hardware.
std::uint32_t source_host_x86_rsqrtss_estimate(std::uint32_t input_word);

// Complete1EE380 resident model initialization and its1EE8F0 serialized
// shape-selector owner. Clear/rebind follows the original byte parameter;
// shapes, children and primitive transforms come from the actual cache.
std::uint32_t
source_resolve_physics_shape_selector_1_1(const SourcePhysicsContactMemory &,
                                          std::uint32_t model_pointer,
                                          std::uint32_t serialized_selector);
void source_initialize_physics_model_1_1(
    SourcePhysicsContactMemory &, std::uint32_t model_datum, std::uint8_t clear,
    const SourcePhysicsContactRecordObserver & = {});
// Compatibility entry for the source's normal initialization parameter0.
void source_initialize_physics_convex_model_1_1(
    SourcePhysicsContactMemory &, std::uint32_t model_datum,
    const SourcePhysicsContactRecordObserver & = {});

void source_physics_quaternion_matrix_1_1(
    SourcePhysicsContactMemory &, std::uint32_t output,
    std::uint32_t quaternion, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
void source_physics_matrix_quaternion_1_1(
    SourcePhysicsContactMemory &, std::uint32_t output, std::uint32_t matrix,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
void source_physics_normalize_quaternion_1_1(
    SourcePhysicsContactMemory &, std::uint32_t quaternion,
    const SourcePhysicsReciprocalSqrtEstimate &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Actual resident type8 convex shape303E30→1D1C00, including its own radius
// and exact source SSE order. Serialized phmo data alone is not a constructed
// resident shape; this checks the source vtable owner before consuming it.
void source_physics_convex_bounds_1_1(
    SourcePhysicsContactMemory &, std::uint32_t shape, std::uint32_t transform,
    std::uint32_t margin_word, std::uint32_t output,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete31AC90 auxiliary initialization and31ABD0 source-shape radius.
// The actual allocator-owned auxiliary and caller AABB scratch are required.
void source_construct_physics_body_auxiliary_1_1(
    SourcePhysicsContactMemory &, std::uint32_t auxiliary, std::uint32_t body,
    std::uint32_t bounds, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Resident virtual shape owners: capsule7, box6, convex8, and transform21.
// These consume the authored shapes and actual transforms, never generated
// hulls or bounding proxies. Unported virtual targets remain explicit errors.
std::uint32_t source_physics_shape_type_1_1(const SourcePhysicsContactMemory &,
                                            std::uint32_t shape);
void source_physics_shape_bounds_1_1(
    SourcePhysicsContactMemory &, std::uint32_t shape, std::uint32_t transform,
    std::uint32_t margin_word, std::uint32_t output,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_motion_1_1(
    SourcePhysicsContactMemory &, std::uint32_t motion, std::uint32_t position,
    std::uint32_t quaternion, std::uint32_t mode,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete actual317EA0 mode4 and mode5 branches; other selector modes remain
// unavailable.318100 mode7 uses its independently measured direct constructor.
std::uint32_t source_allocate_physics_motion_1_1(
    SourcePhysicsContactMemory &, const SourcePhysicsMotionConstruction &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_rigid_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t body, std::uint32_t info,
    const SourcePhysicsBodyFactoryScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Original1DAAC0, crate mode7, PVS mode4 and biped mode5 allocation. Body,
// motion, shape refcount and auxiliary ownership all use real retained memory
// and the verified allocator. Wrapped/compound shapes and other motion modes
// stop at their unported owner after preceding original stores.
std::uint32_t source_create_physics_rigid_body_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    const std::array<std::uint32_t, 14> &arguments,
    const SourcePhysicsBodyFactoryScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
