#include "halo2/biped_ragdoll_runtime.hpp"

namespace halo2 {
namespace {
SourceBipedPlacementServices placement_services(
    SourcePhysicsContactMemory &memory, SourcePhysicsFactoryFloatingPoint fp,
    const SourceBipedRagdollRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  SourceBipedPlacementServices out;
  out.place_body_abi = [&, fp](const auto &request, std::uint32_t entry) {
    auto providers = services.placement
        ? services.placement(request, entry) : SourceBipedBodyPlacementServices{};
    providers.point = [&, request, entry](auto point) {
      if (services.point_runtime)
        return source_world_point_collision_1_1(
            memory, 0x80040d, point, request.object_datum, 0xffffffffu,
            {entry - 0x150, services.point_runtime->fs_base}, fp,
            services.point_runtime->runtime, observe);
      return source_point_collision_1_1(
          memory, 0x80040d, point, request.object_datum, 0xffffffffu,
          source_point_collision_scratch_1_1(entry - 0x150), fp,
          services.point, observe);
    };
    if (services.segment_runtime)
      providers.segment = [&, request, entry](auto anchor, auto direction,
                                               auto packet) {
        // E3270 reserves12Ch, saves four registers, then pushes six
        // arguments and the return address at E362A.
        return source_segment_collision_1_1(
            memory, 0x80040d, anchor, direction, request.object_datum,
            0xffffffffu, packet,
            {entry - 0x158, services.segment_runtime->fs_base}, fp,
            services.segment_runtime->runtime, observe);
      };
    providers.set_transform = [&](auto component, auto slot, auto matrix) {
      source_biped_body_transform34_1_1(memory, component, slot, matrix,
                                         entry - 0x144, fp, services.transform,
                                         observe);
    };
    return source_place_biped_body_1_1(memory, request, entry, fp, providers,
                                        observe);
  };
  return out;
}
} // namespace
std::uint8_t source_rebuild_biped_ragdoll_runtime_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t object,
    std::uint32_t entry, SourcePhysicsFactoryFloatingPoint fp,
    const SourceBipedRagdollRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  SourceBipedRagdollRebuildServices rebuild;
  rebuild.placement = placement_services(memory, fp, services, observe);
  rebuild.remove_object = [&](auto d) {
    source_remove_biped_object_physics_1_1(memory, d, [&](auto component) {
      source_remove_biped_component_world_1_1(memory, component, entry - 0x30,
                                               services.removal, observe);
    }, observe);
  };
  // E2CE0 enters1C70D0 at entry-34h. Its saved registers and each
  // original argument packet determine these complete lower-owner frames.
  rebuild.physics.remove = [&](auto component) {
    source_remove_biped_component_world_1_1(memory, component, entry - 0x44,
                                             services.removal, observe);
  };
  rebuild.physics.dispose = [&](auto component) {
    const auto dispose = entry - 0x48;
    source_dispose_physics_component_bodies_1_1(
        memory, component, {dispose - 8, dispose - 4}, services.disposal,
        observe);
  };
  rebuild.physics.rebuild = [&](auto component) {
    source_rebuild_biped_component_1_1(memory, component, entry - 0x44, fp,
                                        services.component, observe);
  };
  rebuild.physics.insert = [&](auto component) {
    source_insert_biped_component_world_1_1(memory, component, entry - 0x44,
                                             fp, services.insertion, observe);
  };
  rebuild.insert_component = [&](auto component) {
    source_insert_biped_component_world_1_1(memory, component, entry - 0x2c,
                                             fp, services.insertion, observe);
  };
  return source_rebuild_biped_ragdoll_1_1(memory, object, entry, fp, rebuild,
                                          observe);
}
void source_maintain_biped_ragdoll_runtime_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t object,
    std::uint32_t entry, SourcePhysicsFactoryFloatingPoint fp,
    const SourceBipedRagdollRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  source_maintain_biped_ragdoll_1_1(
      memory, object, entry, fp, placement_services(memory, fp, services, observe),
      observe);
}
} // namespace halo2
