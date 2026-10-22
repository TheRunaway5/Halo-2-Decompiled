#include "halo2/death_object_phases.hpp"
#include "halo2/unit_object_runtime.hpp"
#include "halo2/biped_postphysics_phase.hpp"
#include "halo2/source_segment_collision.hpp"
#include "halo2/source_point_collision_runtime.hpp"
#include "halo2/source_damage_runtime.hpp"
#include "gameplay/units/death_world_runtime_entries.hpp"
#include <stdexcept>

namespace halo2 {
void source_advance_death_object_clock_1_1(
    SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
  const auto word = [&](std::uint32_t address) {
    std::array<std::byte, 4> bytes;
    memory.read(address, bytes);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
      value |= std::to_integer<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
  };
  // A144845100 / FF4008: MOV EAX,[518444]; INC DWORD PTR[EAX+8].
  const auto address = word(0x518444u) + 8u;
  SourcePhysicsContactRecordStore store{.instruction=0x139868u,
      .address=address,.before=std::vector<std::byte>(4),
      .written=std::vector<std::byte>(4)};
  memory.read(address, store.before);
  const auto value = word(address) + 1u;
  for (unsigned i = 0; i < 4; ++i)
    store.written[i] = std::byte(value >> (i * 8));
  memory.write(address, store.written);
  if (observe) observe(store);
}

void source_run_death_object_phase_bodies_1_1(
    SourcePhysicsContactMemory &, std::uint32_t, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver &);

bool source_death_object_phases_owns_entry_1_1(std::uint32_t entry) {
  namespace e = death_world_runtime_entries;
  return e::contains(e::object_phases, entry) ||
      source_damage_owns_entry_1_1(entry) ||
      source_death_world_owns_entry_1_1(entry) ||
      e::contains(e::construction, entry) || e::contains(e::weapon, entry) ||
      e::contains(e::postphysics_phase, entry) ||
      e::contains(e::segment, entry) || e::contains(e::point, entry);
}

void source_run_death_object_phases_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceDeathWorldSchedulerServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  auto native = services.runtime;
  native.timestamp_counter = services.timestamp_counter;
  native.required_owner = [&](auto &m, const auto &call, auto &state, const auto &o) {
    namespace e = death_world_runtime_entries;
    const auto controls = state.floating_point.value_or(fp);
    if (source_damage_owns_entry_1_1(call.target))
      source_run_damage_1_1(m, call.target, state, controls, native, o);
    else if (e::contains(e::object_phases, call.target))
      source_run_death_object_phase_bodies_1_1(m, call.target, state, controls, native, o);
    else if (source_death_world_owns_entry_1_1(call.target)) {
      auto world = services;
      world.runtime = native;
      source_run_death_world_scheduler_1_1(m, call.target, state, controls, world, o);
    } else if (e::contains(e::construction, call.target) ||
               e::contains(e::weapon, call.target))
      source_run_unit_object_runtime_1_1(m, call.target, state, controls, native, o);
    else if (e::contains(e::postphysics_phase, call.target))
      source_run_biped_postphysics_phase_1_1(m, call.target, state, controls, native, o);
    else if (e::contains(e::segment, call.target))
      source_run_segment_collision_1_1(m, call.target, state, controls, native, o);
    else if (e::contains(e::point, call.target))
      source_run_point_collision_runtime_1_1(m, call.target, state, controls, native, o);
    else {
      if (!services.runtime.required_owner)
        throw std::runtime_error("Required original death object phase owner unavailable");
      services.runtime.required_owner(m, call, state, o);
    }
  };
  if (!source_death_object_phases_owns_entry_1_1(entry))
    throw std::runtime_error("Required original death object phase entry unavailable");
  native.required_owner(memory, {0, entry, false}, abi, observe);
}

void source_step_death_objects_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry_esp,
    std::uint32_t fs_base, SourcePhysicsFactoryFloatingPoint fp,
    const SourceDeathWorldSchedulerServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  for (const auto entry : {0x13a380u, 0xb7e00u, 0x1c7660u, 0xb7f50u, 0xb8040u}) {
    SourceUnitAnimationRuntimeAbi abi{};
    abi.fs_base = fs_base;
    abi.eflags = 2;
    abi.gp[4] = entry_esp;
    source_run_death_object_phases_1_1(memory, entry, abi, fp, services, observe);
    if (abi.gp[4] != entry_esp + 4)
      throw std::runtime_error("Original death object phase stack ownership mismatch");
  }
}
} // namespace halo2
