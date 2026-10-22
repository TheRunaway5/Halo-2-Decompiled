#include "halo2/death_world_scheduler.hpp"
#include "halo2/physics_vehicle_motion_math.hpp"
#include "halo2/physics_vehicle_update.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/biped_postphysics_runtime.hpp"
#include "halo2/death_world_physics_runtime.hpp"
#include "halo2/physics_vehicle_runtime.hpp"
#include "gameplay/units/death_world_runtime_entries.hpp"
#include <stdexcept>

namespace halo2 {
void source_run_death_world_scheduler_bodies_1_1(
    SourcePhysicsContactMemory &, std::uint32_t, SourceUnitAnimationRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourceUnitAnimationRuntimeServices &,
    const SourcePhysicsContactRecordObserver &);

bool source_death_world_owns_entry_1_1(std::uint32_t entry) {
  namespace e = death_world_runtime_entries;
  return e::contains(e::scheduler, entry) ||
         e::contains(e::death_physics, entry) ||
         e::contains(e::animation, entry) ||
         e::contains(e::postphysics, entry) || e::contains(e::world, entry) ||
         e::contains(e::contact, entry) || e::contains(e::vehicle_update, entry) ||
         e::contains(e::vehicle_motion, entry) || e::contains(e::vehicle, entry) ||
         e::contains(e::math, entry);
}

void source_run_death_world_scheduler_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceDeathWorldSchedulerServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  auto native = services.runtime;
  native.timestamp_counter = services.timestamp_counter;
  native.required_owner = [&](auto &m, const auto &call, auto &state,
                              const auto &observer) {
    namespace entries = death_world_runtime_entries;
    if (entries::contains(entries::scheduler, call.target)) {
      source_run_death_world_scheduler_bodies_1_1(
          m, call.target, state, fp, native, observer);
      return;
    }
    if (entries::contains(entries::death_physics, call.target)) {
      source_run_death_world_physics_runtime_1_1(
          m, call.target, state, fp, native, observer);
      return;
    }
    if (entries::contains(entries::animation, call.target)) {
      source_run_unit_animation_selection_1_1(
          m, call.target, state, fp, native, observer);
      return;
    }
    if (entries::contains(entries::postphysics, call.target)) {
      source_run_biped_postphysics_runtime_1_1(
          m, call.target, state, fp, native, observer);
      return;
    }
    const bool world = entries::contains(entries::world, call.target);
    const bool contact = entries::contains(entries::contact, call.target);
    const bool update = entries::contains(entries::vehicle_update, call.target);
    const bool motion = entries::contains(entries::vehicle_motion, call.target);
    const bool vehicle = entries::contains(entries::vehicle, call.target);
    const bool math = entries::contains(entries::math, call.target);
    if (world || contact || update || motion || vehicle || math) {
      SourcePhysicsContactRuntimeServices physical;
      physical.instruction = native.instruction;
      physical.timestamp_counter = services.timestamp_counter;
      if (native.estimate)
        physical.reciprocal_sqrt = [&](auto x) {
          return native.estimate(
              SourceUnitAnimationEstimate::ReciprocalSquareRoot, x);
        };
      physical.required_owner = [&](auto &bank, const auto &request,
                                     auto &physical_state, const auto &o) {
        SourceUnitAnimationRuntimeAbi full = state;
        static_cast<SourcePhysicsContactRuntimeAbi &>(full) = physical_state;
        try { native.required_owner(bank, request, full, o); }
        catch (...) {
          state.mmx = full.mmx;
          physical_state = static_cast<const SourcePhysicsContactRuntimeAbi &>(full);
          throw;
        }
        state.mmx = full.mmx;
        physical_state = static_cast<const SourcePhysicsContactRuntimeAbi &>(full);
      };
      if (math) {
        auto controls = state.floating_point.value_or(fp);
        source_run_physics_vehicle_motion_math_runtime(
            m, call.target, state, SourcePhysicsContactImage::MapPack11,
            controls, physical, observer);
      } else if (world)
        source_run_physics_world_runtime(
            m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
            physical, observer);
      else if (contact)
        source_run_physics_contact_runtime(
            m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
            physical, observer);
      else if (update)
        source_run_physics_vehicle_update_runtime(
            m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
            physical, observer);
      else if (motion)
        source_run_physics_vehicle_motion_runtime(
            m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
            physical, observer);
      else
        source_run_physics_vehicle_runtime(
            m, call.target, state, SourcePhysicsContactImage::MapPack11, fp,
            physical, observer);
      return;
    }
    if (!services.runtime.required_owner)
      throw std::runtime_error("Required original death world scheduling owner unavailable");
    services.runtime.required_owner(m, call, state, observer);
  };
  if (!source_death_world_owns_entry_1_1(entry))
    throw std::runtime_error("Required original death world entry unavailable");
  native.required_owner(memory, {0, entry, false}, abi, observe);
}

void source_step_death_physics_world_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t entry,
    std::uint32_t fs_base, SourcePhysicsFactoryFloatingPoint fp,
    const SourceDeathWorldSchedulerServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  SourceUnitAnimationRuntimeAbi abi{};
  abi.gp[4] = entry;
  abi.fs_base = fs_base;
  abi.eflags = 2;
  source_run_death_world_scheduler_1_1(
      memory, 0x1c7660, abi, fp, services, observe);
  if (abi.gp[4] != entry + 4)
    throw std::runtime_error("Original world scheduler stack ownership mismatch");
}
} // namespace halo2
