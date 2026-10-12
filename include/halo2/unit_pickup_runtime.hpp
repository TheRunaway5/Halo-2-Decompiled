#pragma once
#include "halo2/physics_contact_runtime.hpp"
namespace halo2 {
// Complete TU1.1 source owners: CEAF0/111490 capability, CEC00 admission,
// 152080 player carry limits, EA660 type20 request, CE510 dispatch and their
// compiled inventory, ammo, selection and action helpers. Input banks, graph
// resolution, action descriptors, world detach/drop, notifications and all
// other reached external owners must come from the caller's real runtime.
// No capability, allocation, physics, animation or transport result defaults.
// The ABI retains all source registers, FP state and stack arguments. These
// banks are not the native Match's typed state and are never manufactured here.
void source_run_unit_pickup_1_1(SourcePhysicsContactMemory &, std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
