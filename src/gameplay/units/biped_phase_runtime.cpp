#include "halo2/biped_phase_runtime.hpp"
#include "halo2/biped_falling_damage.hpp"
#include "halo2/physics_vehicle_update.hpp"
#include <stdexcept>

namespace halo2 {
std::uint32_t source_call_biped_runtime_1_1(
    SourcePhysicsContactMemory &memory, const SourceRuntimeCall &call,
    SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  if(call.entry==0xe4d30 || call.entry==0xe5700)
    return source_call_biped_falling_damage_1_1(memory,call,fp,services,observe);
  if (call.entry != 0xbc910) {
    switch (call.entry) {
    case 0xddbc0:
    case 0xe1800:
    case 0xe4f40:
    case 0xdf7c0:
    case 0x142e80:
    case 0x143f60:
    case 0x1d1290:
    case 0xc04f0:
    case 0xe8050:
    case 0xe63c0:
      return source_call_unit_animation_pose_1_1(memory, call, fp, services,
                                                 observe);
    default:
      throw std::runtime_error("Unsupported original biped phase call ABI");
    }
  }
  if (!call.eax || !call.arguments.empty() || !call.argument_masks.empty())
    throw std::runtime_error("Missing defined original object marking input");
  SourcePhysicsContactRuntimeAbi abi{};
  abi.gp[0] = *call.eax;
  abi.gp[4] = call.entry_esp;
  abi.eflags = 2;
  SourcePhysicsContactRuntimeServices physical;
  physical.instruction = services.instruction;
  physical.required_owner = [&](auto &m, const auto &request, auto &state,
                                const auto &observer) {
    if (!services.required_owner)
      throw std::runtime_error(
          "Required original object marking lower owner unavailable");
    SourceUnitAnimationRuntimeAbi raw{};
    static_cast<SourcePhysicsContactRuntimeAbi &>(raw) = state;
    services.required_owner(m, request, raw, observer);
    state = static_cast<const SourcePhysicsContactRuntimeAbi &>(raw);
  };
  source_run_physics_vehicle_update_runtime(
      memory, call.entry, abi, SourcePhysicsContactImage::MapPack11, fp,
      physical, observe);
  if (abi.gp[4] != call.entry_esp + 4)
    throw std::runtime_error(
        "Original object marking stack ownership mismatch");
  return abi.gp[0];
}
} // namespace halo2
