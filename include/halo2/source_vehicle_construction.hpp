#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct SourceVehicleConstructionServices {
  // CompleteBADE0 source point producer, with EAX=full vehicle identity and
  // ECX=output backing. Component/parent/body transform ownership is required.
  std::function<void(std::uint32_t, std::uint32_t)> point;
  // RequiredBAA80(EAX stack unit,DL=true), not a successful body activation
  // default. Caller retains the selected-image implementation/provenance.
  std::function<void(std::uint32_t)> original_baa80;
  // 110BF0, EAX=0, all seven original stack arguments in order; no constant
  // success permitted for live graph/physics/animation state.
  std::function<bool(const std::array<std::uint32_t, 7> &)> animation_request;
  // 1D1940, EDI=body+signed(body+12A). Actual manager reset is required.
  std::function<void(std::uint32_t)> reset_animation;
};
// CompleteEFFF0/F0F10 vehicle reset direct stores and BADE0 delivery.
// Retains untouched bytes, exact widths/order and salted object identity.
void source_vehicle_reset(SourcePhysicsContactMemory &, std::uint32_t vehicle,
                          SourcePhysicsContactImage,
                          const SourceVehicleConstructionServices &,
                          const SourcePhysicsContactRecordObserver & = {});
// CompleteF01C0/F10E0 direct constructor control. Missing phmo returnsfalse
// before any write; every reached external dependency requires its real owner.
// This does not allocate a vehicle, create seats or implement driving physics.
bool source_vehicle_construct(SourcePhysicsContactMemory &,
                              std::uint32_t vehicle, SourcePhysicsContactImage,
                              const SourceVehicleConstructionServices &,
                              const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
