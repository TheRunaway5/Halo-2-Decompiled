#pragma once
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// Original TU1.1 dynamic motion owners. The inputs are current allocator-owned
// motion, world step and gravity records, not object positions or tag records.
// These advance a body; they do not generate contacts or solve constraints.
void source_apply_physics_angular_impulse_1_1(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t impulse,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRecordObserver & = {});
void source_apply_physics_motion_forces_1_1(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t gravity,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
void source_integrate_physics_motion_1_1(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete3218A0 preparation and321A60 readback for dynamic solver motions.
// Preparation owns forces/damping and the128-byte accumulator. Readback uses
// the actual solver's averaged velocity to advance pose, then restores its
// final instantaneous velocity. Neither function supplies a solver result.
std::uint32_t source_prepare_physics_solver_motion_1_1(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t gravity,
    std::uint32_t accumulator, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_readback_physics_solver_motion_1_1(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t average_scale_word,
    std::uint32_t accumulator, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
// Selected-image motion entry points. TU1.5 operands are bound to their
// authenticated executable addresses; callers supply those actual read-only
// bytes. Ordered observer instruction labels use the canonical TU1.1 pairing.
// This binding covers motion math, not TU1.5 world/allocator construction.
void source_apply_physics_motion_forces(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t gravity,
    std::uint32_t entry_esp, SourcePhysicsContactImage,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRecordObserver & = {});
void source_integrate_physics_motion(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t entry_esp,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_prepare_physics_solver_motion(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t gravity,
    std::uint32_t accumulator, std::uint32_t entry_esp, SourcePhysicsContactImage,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_readback_physics_solver_motion(SourcePhysicsContactMemory &,
    std::uint32_t motion, std::uint32_t step, std::uint32_t average_scale_word,
    std::uint32_t accumulator, std::uint32_t entry_esp, SourcePhysicsContactImage,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
