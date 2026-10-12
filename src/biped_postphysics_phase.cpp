#include "halo2/biped_postphysics_phase.hpp"
#include "halo2/physics_vehicle_update.hpp"
#include <stdexcept>

namespace halo2 {
void source_run_biped_postphysics_phase_bodies_1_1(
    SourcePhysicsContactMemory &, std::uint32_t, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,
    const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver &);

void source_run_biped_postphysics_phase_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  auto native = services;
  native.required_owner = [&](auto &m, const auto &call, auto &state,
                              const auto &observer) {
    switch (call.target) {
    case 0x1e93f0:
    case 0x1f0e90:
    case 0x1f0d30:
    case 0x1d5600:
    case 0x3121a0:
    case 0x1218c0:
      source_run_biped_postphysics_runtime_1_1(
          m, call.target, state, fp, native, observer);
      return;
    case 0xb8250:
    case 0xbacc0:
    case 0xbbcb0:
    case 0xe5680:
    case 0xe63c0:
    case 0x30bd0:
    case 0x142d00:
    case 0x1441d0:
    case 0x1d42d0:
      source_run_unit_animation_selection_1_1(
          m, call.target, state, fp, native, observer);
      return;
    case 0xbb0c0:
    case 0xb8630:
    case 0xbaa80:
    case 0xbc910:
    case 0x1c8430: {
      SourcePhysicsContactRuntimeServices physical;
      physical.instruction = native.instruction;
      if (native.estimate)
        physical.reciprocal_sqrt = [&](auto x) {
          return native.estimate(
              SourceUnitAnimationEstimate::ReciprocalSquareRoot, x);
        };
      physical.required_owner = [&](auto &bank, const auto &request,
                                     auto &physical_state, const auto &o) {
        SourceUnitAnimationRuntimeAbi full = state;
        static_cast<SourcePhysicsContactRuntimeAbi &>(full) = physical_state;
        native.required_owner(bank, request, full, o);
        physical_state = static_cast<const SourcePhysicsContactRuntimeAbi &>(full);
      };
      source_run_physics_vehicle_update_runtime(
          m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
          physical, observer);
      return;
    }
    default:
      if (!services.required_owner)
        throw std::runtime_error("Required original biped postphysics owner unavailable");
      services.required_owner(m, call, state, observer);
    }
  };
  source_run_biped_postphysics_phase_bodies_1_1(
      memory, entry, abi, fp, native, observe);
}

void source_update_biped_postphysics_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t object,
    std::uint32_t entry, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  std::array<std::byte, 4> input;
  for (unsigned i = 0; i < 4; ++i)
    input[i] = std::byte(object >> (8 * i));
  memory.write(entry + 4, input);
  SourceUnitAnimationRuntimeAbi abi{};
  abi.gp[4] = entry;
  abi.eflags = 2;
  source_run_biped_postphysics_phase_1_1(
      memory, 0xdefc0, abi, fp, services, observe);
  if (abi.gp[4] != entry + 8)
    throw std::runtime_error("Original biped postphysics stack ownership mismatch");
}
} // namespace halo2
