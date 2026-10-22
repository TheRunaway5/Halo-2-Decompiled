#include "halo2/death_startup_runtime.hpp"
#include "gameplay/units/death_world_runtime_entries.hpp"
#include <stdexcept>

namespace halo2 {
void source_run_death_startup_bodies_1_1(
    SourcePhysicsContactMemory &, std::uint32_t, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver &);

bool source_death_startup_owns_entry_1_1(std::uint32_t entry) {
  return death_world_runtime_entries::contains(
      death_world_runtime_entries::startup, entry);
}

void source_run_death_startup_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceDeathWorldSchedulerServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  if (!source_death_startup_owns_entry_1_1(entry))
    throw std::runtime_error("Required original death startup entry unavailable");
  auto native = services.runtime;
  native.timestamp_counter = services.timestamp_counter;
  native.required_owner = [&](auto &m, const auto &call, auto &state, const auto &o) {
    if (source_death_startup_owns_entry_1_1(call.target))
      source_run_death_startup_bodies_1_1(m, call.target, state,
          state.floating_point.value_or(fp), native, o);
    else if (source_death_object_phases_owns_entry_1_1(call.target)) {
      auto joined = services;
      joined.runtime = native;
      source_run_death_object_phases_1_1(m, call.target, state,
          state.floating_point.value_or(fp), joined, o);
    } else {
      if (!services.runtime.required_owner)
        throw std::runtime_error("Required original death startup resource owner unavailable");
      services.runtime.required_owner(m, call, state, o);
    }
  };
  native.required_owner(memory, {0, entry, false}, abi, observe);
}
} // namespace halo2
