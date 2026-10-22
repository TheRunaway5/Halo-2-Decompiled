#include "halo2/biped_component_rebuild.hpp"
#include "halo2/physics_body_disposal.hpp"
#include <stdexcept>

namespace halo2 {
void source_rebuild_biped_component_1_1(
    SourcePhysicsContactMemory &memory, std::uint32_t datum,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint fp,
    const SourceBipedComponentRebuildServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  SourcePhysicsComponentRebuildServices component;
  component.read = [&](auto address, auto output) {
    memory.read(address, output);
  };
  component.write = [&](auto address, auto input) {
    memory.write(address, input);
  };
  const auto grouped = [&](const SourcePhysicsBodyGroupingRequest &request,
                           std::uint32_t entry) {
    const auto scratch = services.grouping.grouping_frame ? services.grouping
        : source_physics_body_grouping_scratch_1_1(entry);
    return source_group_physics_bodies_1_1(memory, request, scratch,
                                           fp, services.reciprocal_sqrt,
                                           observe);
  };
  component.invoke =
      [&](const PhysicsComponentRebuildCall &call) -> std::uint32_t {
    if (call.entry == 0x1da570) {
      SourceBipedPhysicsRebuildServices biped;
      //1D2B10 saves four registers;1DA570 owns18h locals/saved registers.
      //The grouping call adds four arguments and its return address.
      biped.group = [&](auto p) {
        return std::uint8_t(grouped({p, 0, 0, 1}, entry_esp - 0x40));
      };
      biped.dispose = [&](auto p) {
        if (services.dispose) services.dispose(p);
        else {
          const auto entry = entry_esp - 0x34;
          source_dispose_physics_component_bodies_1_1(
              memory, p, {entry - 8, entry - 4}, {}, observe);
        }
      };
      biped.controller = [&](const SourceBipedControllerRequest &request) {
        if (services.controller)
          services.controller(request);
        else
          source_construct_biped_controller_1_1(
              memory, request, services.controller_scratch.entry_esp
                  ? services.controller_scratch
                  : source_biped_controller_scratch_1_1(entry_esp - 0x4c), fp,
              services.reciprocal_sqrt, observe);
      };
      source_rebuild_biped_physics_1_1(memory, call.component, biped, observe);
      return 0; // This caller does not consume1DA570's undefined EAX.
    }
    if (call.entry == 0x1d9330)
      return grouped({call.arguments[0], call.arguments[1], call.arguments[2],
                      call.arguments[3]}, entry_esp - 0x24);
    if (call.entry == 0x1c8430) {
      // Four saved registers, two arguments and CALL precede this entry.
      source_biped_object_velocity_1_1(memory, call.object, call.linear,
                                       call.angular, 0, entry_esp - 28,
                                       services.velocity, observe);
      return 0;
    }
    if (!services.other_owner)
      throw std::runtime_error(
          "Required original non-biped rebuild owner unavailable");
    return services.other_owner(call);
  };
  const ObjectPhysicsConstructionObserver stores = [&](const auto &store) {
    if (!observe)
      return;
    SourcePhysicsContactRecordStore result{
        store.instruction, store.address, {}, {}};
    for (unsigned i = 0; i < store.bytes; ++i) {
      result.before.push_back(std::byte(store.before >> (8 * i)));
      result.written.push_back(std::byte(store.after >> (8 * i)));
    }
    observe(result);
  };
  source_physics_component_rebuild(
      {0x526248, 0x4e78d0, 0x1d2bfc, 0x1d2be4}, datum,
      {0x1d2b6a, 0x1d2b96, 0x1d2b7a, 0x1d2b63, 0x1d2b73}, component, stores);
}
} // namespace halo2
