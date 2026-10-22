#include "halo2/biped_postphysics_runtime.hpp"
#include <stdexcept>

namespace halo2 {
void source_update_biped_physical_contacts_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t state,
    std::uint32_t packet, std::uint32_t output, std::uint32_t entry,
    SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  SourceUnitAnimationRuntimeAbi abi{};
  abi.gp[0] = packet;
  abi.gp[1] = output;
  abi.gp[3] = state;
  abi.gp[4] = entry;
  abi.eflags = 2;
  source_run_biped_postphysics_runtime_1_1(
      memory, 0x1e93f0, abi, fp, services, observe);
  if (abi.gp[4] != entry + 4)
    throw std::runtime_error("Original contact refresh stack ownership mismatch");
}
} // namespace halo2
