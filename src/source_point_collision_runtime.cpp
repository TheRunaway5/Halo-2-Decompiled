#include "halo2/source_point_collision_runtime.hpp"
#include <stdexcept>

namespace halo2 {
void source_run_point_collision_bodies_1_1(
    SourcePhysicsContactMemory &, std::uint32_t, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver &);

void source_run_point_collision_runtime_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  auto native = services;
  native.required_owner = [&](auto &m, const auto &call, auto &state,
                              const auto &observer) {
    switch (call.target) {
    case 0x14b9d0:
    case 0x16a430:
    case 0x16a490:
    case 0x16cf80:
    case 0xb9830:
    case 0xb98a0:
    case 0x1f3500:
    case 0x1f3590:
    case 0x1f3660:
      source_run_segment_collision_1_1(
          m, call.target, state, fp, native, observer);
      return;
    default:
      if (!services.required_owner)
        throw std::runtime_error("Required original point containment owner unavailable");
      services.required_owner(m, call, state, observer);
    }
  };
  source_run_point_collision_bodies_1_1(
      memory, entry, abi, fp, native, observe);
}

bool source_world_point_collision_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t flags,
    std::uint32_t point, std::uint32_t excluded_a, std::uint32_t excluded_b,
    SourceSegmentCollisionContext context, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  const std::array values{flags, point, excluded_a, excluded_b};
  std::array<std::byte, 16> input;
  for (unsigned i = 0; i < values.size(); ++i)
    for (unsigned k = 0; k < 4; ++k)
      input[i * 4 + k] = std::byte(values[i] >> (8 * k));
  memory.write(context.entry_esp + 4, input);
  SourceUnitAnimationRuntimeAbi abi{};
  abi.gp[4] = context.entry_esp;
  abi.fs_base = context.fs_base;
  abi.eflags = 2;
  source_run_point_collision_runtime_1_1(
      memory, 0x16a9c0, abi, fp, services, observe);
  if (abi.gp[4] != context.entry_esp + 20)
    throw std::runtime_error("Original point containment stack ownership mismatch");
  return (abi.gp[0] & 0xff) != 0;
}
} // namespace halo2
