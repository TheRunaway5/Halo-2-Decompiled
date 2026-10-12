#pragma once
#include "halo2/biped_body_placement.hpp"
#include "halo2/biped_physics_activation.hpp"

namespace halo2 {
// Complete1D4840 and31FEA0 angular-velocity delivery. Component bit1 and
// static-body admission are tested before touching the caller's vector.
void source_biped_body_angular_velocity_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    std::uint32_t velocity3, std::uint32_t entry_esp,
    const SourceBipedBodyTransformServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D5600/3121A0: an actual active island on any committed body.
bool source_biped_component_active_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t entry_esp, const SourcePhysicsContactRecordObserver & = {});
struct SourceBipedVelocityServices {
  SourceBipedBodyTransformServices body;
  SourceBipedPhysicsActivationServices activation;
  std::function<std::uint32_t(std::uint32_t motion, std::uint32_t method)>
      motion_type;
};
// Complete1C8430 dispatcher on current object/component/header banks. Ordinary
// objects visit every dynamic body; type1 uses its actual signed root slot.
// A non-forced request requires live island admission, and forced activation
// occurs only after the original velocity delivery and registered-body test.
void source_biped_object_velocity_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t linear3,
    std::uint32_t angular3, std::uint8_t force, std::uint32_t entry_esp,
    const SourceBipedVelocityServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
