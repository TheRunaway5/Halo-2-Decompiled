#include "halo2/source_segment_collision.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <bit>
#include <stdexcept>
#include <utility>

namespace halo2 {
void source_run_segment_collision_bodies_1_1(
    SourcePhysicsContactMemory &, std::uint32_t, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver &);

void source_run_segment_collision_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  auto native = services;
  native.required_owner = [&](auto &m, const auto &call, auto &state,
                              const auto &observer) {
    switch (call.target) {
    case 0x142d00:
      source_run_unit_animation_selection_1_1(
          m, call.target, state, fp, native, observer);
      return;
    case 0x27f9f0:
    case 0x2dd7f0: {
      SourcePhysicsWorldRuntimeServices world;
      world.instruction = native.instruction;
      if (native.estimate)
        world.reciprocal_sqrt = [&](auto x) {
          return native.estimate(
              SourceUnitAnimationEstimate::ReciprocalSquareRoot, x);
        };
      world.required_owner = [&](auto &bank, const auto &request,
                                 auto &physical_state, const auto &o) {
        SourceUnitAnimationRuntimeAbi full = state;
        static_cast<SourcePhysicsContactRuntimeAbi &>(full) = physical_state;
        native.required_owner(bank, request, full, o);
        physical_state = static_cast<const SourcePhysicsContactRuntimeAbi &>(full);
      };
      source_run_physics_world_runtime(
          m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
          world, observer);
      return;
    }
    default:
      if (!services.required_owner)
        throw std::runtime_error("Required original world segment owner unavailable");
      services.required_owner(m, call, state, observer);
    }
  };
  source_run_segment_collision_bodies_1_1(
      memory, entry, abi, fp, native, observe);
}

bool source_segment_collision_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t flags,
    std::uint32_t start, std::uint32_t delta, std::uint32_t excluded_object,
    std::uint32_t excluded_parent, std::uint32_t result,
    SourceSegmentCollisionContext context, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  std::array<std::byte, 24> input;
  const std::array values{flags, start, delta, excluded_object, excluded_parent, result};
  for (unsigned i = 0; i < values.size(); ++i)
    for (unsigned k = 0; k < 4; ++k)
      input[i * 4 + k] = std::byte(values[i] >> (8 * k));
  const auto entry = context.entry_esp;
  memory.write(entry + 4, input);
  SourceUnitAnimationRuntimeAbi abi{};
  abi.gp[4] = entry;
  abi.fs_base = context.fs_base;
  abi.eflags = 2;
  source_run_segment_collision_1_1(
      memory, 0x16b240, abi, fp, services, observe);
  if (abi.gp[4] != entry + 28)
    throw std::runtime_error("Original world segment stack ownership mismatch");
  return (abi.gp[0] & 0xff) != 0;
}

DeathCameraCollision source_death_camera_collision_1_1(
    SourcePhysicsContactMemory &memory, SourceDeathCameraCollisionScratch scratch,
    SourcePhysicsFactoryFloatingPoint fp, SourceUnitAnimationRuntimeServices services,
    SourcePhysicsContactRecordObserver observe) {
  return [&, scratch, fp, services = std::move(services),
          observe = std::move(observe)](std::uint32_t flags, Vec3 origin,
                                       Vec3 delta) -> std::optional<float> {
    const auto input = [&](std::uint32_t p, Vec3 v) {
      const std::array values{std::bit_cast<std::uint32_t>(v.x),
                              std::bit_cast<std::uint32_t>(v.y),
                              std::bit_cast<std::uint32_t>(v.z)};
      std::array<std::byte, 12> bytes;
      for (unsigned i = 0; i < values.size(); ++i)
        for (unsigned k = 0; k < 4; ++k)
          bytes[i * 4 + k] = std::byte(values[i] >> (k * 8));
      memory.write(p, bytes);
    };
    input(scratch.origin, origin);
    input(scratch.delta, delta);
    if (!source_segment_collision_1_1(
            memory, flags, scratch.origin, scratch.delta, 0xffffffffu,
            0xffffffffu, scratch.result, {scratch.entry_esp, scratch.fs_base},
            fp, services, observe))
      return std::nullopt;
    std::array<std::byte, 4> bytes;
    memory.read(scratch.result + 4, bytes);
    std::uint32_t fraction = 0;
    for (unsigned k = 0; k < 4; ++k)
      fraction |= std::uint32_t(bytes[k]) << (k * 8);
    return std::bit_cast<float>(fraction);
  };
}
} // namespace halo2
