#include "halo2/biped_body_placement.hpp"
#include "halo2/biped_broadphase_removal.hpp"
#include "halo2/biped_component_rebuild.hpp"
#include "halo2/biped_contact_retirement.hpp"
#include "halo2/biped_death_transition.hpp"
#include "halo2/biped_physics_activation.hpp"
#include "halo2/biped_ragdoll_admission.hpp"
#include "halo2/biped_world_insertion.hpp"
#include "halo2/biped_world_removal.hpp"
#include "halo2/physics_body_disposal.hpp"
#include "halo2/source_point_collision.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
namespace {
void check(bool b, const std::string &s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t word(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  std::vector<std::byte> b;
  check(s.size() % 2 == 0, "literal original byte extent");
  for (unsigned i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(word(s.substr(i, 2))));
  return b;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::byte> data;
  void read(std::uint32_t p, std::span<std::byte> out) const override {
    for (unsigned i = 0; i < out.size(); ++i) {
      auto q = data.find(p + i);
      if (q == data.end()) {
        std::ostringstream s;
        s << "Required admission source backing " << std::hex << p + i;
        throw std::runtime_error(s.str());
      }
      out[i] = q->second;
    }
  }
  void write(std::uint32_t p, std::span<const std::byte> in) override {
    for (unsigned i = 0; i < in.size(); ++i)
      data.at(p + i) = in[i];
  }
};
using Store = halo2::SourcePhysicsContactRecordStore;
struct Plan {
  std::uint32_t entry, eax, ecx, ebx, edx, esi, edi, result;
  std::array<std::uint32_t, 16> args;
  std::uint32_t esp;
  halo2::SourcePointCollisionScratch point;
  halo2::SourceBipedPlacementScratch placement{};
  halo2::SourcePhysicsBodyGroupingScratch grouping{};
  halo2::SourceBipedControllerScratch controller{};
  std::vector<std::pair<std::uint32_t, std::uint32_t>> estimates;
  std::vector<Store> stores;
};
struct Case {
  std::string id;
  std::uint32_t entry, eax, ecx, ebx, esi, edi, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  std::array<std::uint32_t, 16> args;
  std::array<std::uint32_t, 11> scratch;
  halo2::SourceBipedControllerScratch controller{};
  std::vector<std::pair<std::uint32_t, std::uint32_t>> estimates;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<Store> stores;
  std::vector<Plan> plans;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original admission owners fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string k, x;
      in >> k;
      auto n = [&] {
        in >> x;
        return word(x);
      };
      if (k == "C") {
        Case c{};
        in >> c.id;
        c.entry = n();
        c.eax = n();
        c.ecx = n();
        c.ebx = n();
        c.esi = n();
        c.edi = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        for (auto &a : c.args)
          a = n();
        cases.push_back(std::move(c));
      } else {
        check(!cases.empty(), "original admission owner");
        auto &c = cases.back();
        if (k == "S")
          for (auto &a : c.scratch)
            a = n();
        else if (k == "M" || k == "A") {
          const auto p = n();
          in >> x;
          const auto b = bytes(x);
          if (k == "M")
            for (unsigned i = 0; i < b.size(); ++i)
              check(c.memory.data.emplace(p + i, b[i]).second,
                    "unique original input byte");
          else
            check(c.after.emplace(p, b).second, "unique original output span");
        } else if (k == "W" || k == "L") {
          Store s{};
          s.instruction = n();
          s.address = n();
          in >> x;
          s.before = bytes(x);
          in >> x;
          s.written = bytes(x);
          if (k == "W")
            c.stores.push_back(s);
          else {
            check(!c.plans.empty(), "original lower owner");
            c.plans.back().stores.push_back(s);
          }
        } else if (k == "H" || k == "J") {
          auto &s = k == "H" ? c.controller : c.plans.back().controller;
          s.entry_esp = n();
          s.factory.info = n();
          s.factory.quaternion = n();
          s.factory.auxiliary_bounds = n();
          s.factory.radius_bounds = n();
          s.wrapper.extent_maximum = n();
          s.wrapper.extent_minimum = n();
          s.wrapper.transform = n();
          s.wrapper.configuration = n();
        } else if (k == "V") {
          const auto operand = n(), result = n();
          c.estimates.emplace_back(operand, result);
        } else if (k == "G") {
          check(!c.plans.empty(), "Original grouping caller");
          auto &s = c.plans.back().grouping;
          std::array<std::uint32_t *, 15> fields{
              &s.grouping_frame,         &s.selection_frame,
              &s.collection_frame,       &s.region_result,
              &s.permutation_result,     &s.constraints_frame,
              &s.iterator_stride,        &s.factory.info,
              &s.factory.quaternion,     &s.factory.auxiliary_bounds,
              &s.factory.radius_bounds,  &s.wrapper.extent_maximum,
              &s.wrapper.extent_minimum, &s.wrapper.transform,
              &s.wrapper.configuration};
          for (auto p : fields)
            *p = n();
        } else if (k == "Q") {
          check(!c.plans.empty(), "Original platform estimate caller");
          const auto operand = n(), result = n();
          c.plans.back().estimates.emplace_back(operand, result);
        } else if (k == "P") {
          Plan p{};
          p.entry = n();
          p.eax = n();
          p.ecx = n();
          p.ebx = n();
          p.edx = n();
          p.esi = n();
          p.edi = n();
          p.result = n();
          for (auto &a : p.args)
            a = n();
          p.esp = n();
          p.point.record_pointer = n();
          p.point.iterator = n();
          p.point.instance_local_point = n();
          if (in >> x) {
            p.placement.node_slots = word(x);
            p.placement.visited_mask = n();
            p.placement.result_word = n();
            p.placement.constraint_first = n();
            p.placement.constraint_second = n();
            p.placement.child_result = n();
          }
          c.plans.push_back(std::move(p));
        } else
          throw std::runtime_error("unknown original admission record");
      }
    }
    std::size_t allstores = 0, allbytes = 0, allplans = 0,
                native_placements = 0, native_velocity_changes = 0,
                native_maintenance = 0, native_resources = 0,
                native_activation = 0, native_removal = 0, native_disposal = 0,
                native_rebuild = 0, native_world_removal = 0,
                native_world_insertion = 0;
    std::map<std::uint32_t, unsigned> entries;
    for (const auto &c : cases) {
      auto m = c.memory;
      std::vector<Store> observed;
      std::size_t plan_index = 0;
      const auto owned = [&](const Store &s) {
        const auto p = s.address;
        const auto span = [&](std::uint32_t a, unsigned n) {
          return a <= p &&
                 std::uint64_t(p) + s.written.size() <= std::uint64_t(a) + n;
        };
        if (c.entry == 0x183650)
          return span(c.args[0], 0x105);
        if (c.entry == 0xdd9a0)
          return span(c.args[0], 4) || span(c.ebx, 4);
        if (c.entry == 0xbacc0)
          return span(c.esi, 12);
        if (c.entry == 0x1f0e20)
          return span(c.args[0], 4) || span(c.esi, 12);
        return !(0x2000000 <= p && p < 0x2200000);
      };
      const auto observe = [&](const Store &s) {
        if (owned(s))
          observed.push_back(s);
      };
      // Explicit fixture-only original lower owner effects. This provider has
      // no live call site and is never compiled into the game. Arguments are
      // checked before applying the separately observed dependency's retained
      // stores.
      const auto lower = [&](std::uint32_t entry, const auto &validate) {
        check(plan_index < c.plans.size(),
              "unexpected required lower owner " + std::to_string(entry));
        const auto &p = c.plans[plan_index++];
        if (p.entry != entry) {
          std::ostringstream message;
          message << "required original lower owner " << std::hex << entry
                  << " before planned " << p.entry;
          throw std::runtime_error(message.str());
        }
        validate(p);
        for (const auto &s : p.stores) {
          std::vector<std::byte> before(s.before.size());
          m.read(s.address, before);
          check(before == s.before,
                "original lower owner retained before bytes");
          m.write(s.address, s.written);
          observe(s);
        }
        return p.result;
      };
      const auto resource = [&](std::uint32_t graph, std::uint32_t packed) {
        check(plan_index < c.plans.size(), "Original resource caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0x1e13c0 && p.stores.empty() && p.args[0] == packed,
              "original resource packed request with no supplied effects");
        halo2::source_biped_animation_resource_1_1(m, graph, packed);
      };
      halo2::SourceBipedWorldRemovalServices world_removal;
      world_removal.lower.required_owner =
          [&](halo2::SourcePhysicsContactMemory &,
              const halo2::SourcePhysicsContactBatchOwnerCall &call,
              const halo2::SourcePhysicsContactRecordObserver &) {
            return lower(call.entry, [&](const Plan &p) {
              check(call.entry_esp == p.esp &&
                        (!call.ecx || *call.ecx == p.ecx),
                    "Original world removal lower owner registers");
              for (unsigned i = 0; i < call.arguments.size(); ++i)
                check(call.arguments[i] == p.args[i],
                      "Original world removal lower owner arguments");
            });
          };
      const auto remove_component = [&](std::uint32_t component) {
        check(plan_index < c.plans.size(), "Original removal component caller");
        const auto &p = c.plans[plan_index];
        check(p.entry == 0x1d4c50 && p.ecx == component,
              "Original removal component identity");
        if (p.stores.empty()) {
          ++plan_index;
          halo2::source_remove_biped_component_world_1_1(
              m, component, p.esp, world_removal, observe);
        } else
          lower(0x1d4c50, [&](const Plan &) {});
      };
      halo2::SourceBipedWorldInsertionServices world_insertion;
      world_insertion.lower = world_removal.lower;
      const auto insert_component = [&](std::uint32_t component) {
        check(plan_index < c.plans.size(), "Original insertion component caller");
        const auto &p = c.plans[plan_index];
        check(p.entry == 0x1d4f30 && p.ecx == component,
              "Original insertion component identity");
        if (p.stores.empty()) {
          ++plan_index;
          halo2::source_insert_biped_component_world_1_1(
              m, component, p.esp, c.fp, world_insertion, observe);
        } else
          lower(0x1d4f30, [&](const Plan &) {});
      };
      const auto placement = c.scratch[2]
          ? halo2::source_biped_placement_scratch_1_1(c.scratch[2] + 0x108)
          : halo2::SourceBipedPlacementScratch{};
      halo2::SourceBipedPlacementServices placement_services;
      placement_services
          .place_body_abi = [&](const halo2::SourceBipedPlacementRequest &q,
                                 std::uint32_t entry) {
        check(plan_index < c.plans.size(), "Original body placement caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0xe3270, "Original native body placement order");
        check(entry == p.esp, "Original per-recursion placement frame");
        check(p.stores.empty(), "No supplied body placement result stores");
        const std::array<std::uint32_t, 7> a{
            q.object_datum, q.anchor,      q.height_word,    q.slot,
            q.constraint,   q.parent_slot, q.inherited_valid};
        for (unsigned i = 0; i < 7; ++i)
          check(a[i] == (i == 6 ? (p.args[i] & 255u) : p.args[i]),
                "original body placement request " + std::to_string(i));
        halo2::SourceBipedBodyPlacementServices services;
        services.point = [&](std::uint32_t point) {
          return halo2::source_point_collision_1_1(m, 0x80040d, point,
                                                   q.object_datum, 0xffffffffu,
                                                   p.point, c.fp, {}, observe);
        };
        services.set_transform = [&](std::uint32_t component,
                                     std::uint32_t slot, std::uint32_t matrix) {
          halo2::source_biped_body_transform34_1_1(
              m, component, slot, matrix, p.esp - 0x144, c.fp, {}, observe);
        };
        const auto result = halo2::source_place_biped_body_1_1(
            m, q, p.esp, c.fp, services, observe);
        check(result == std::uint8_t(p.result),
              "Original native body placement result");
        return result;
      };
      halo2::SourceBipedRagdollRebuildServices rebuild_services;
      rebuild_services.placement = placement_services;
      rebuild_services.remove_object = [&](std::uint32_t d) {
        check(plan_index < c.plans.size(), "Original removal caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0xb9730 && p.stores.empty() && p.eax == d,
              "Original object removal identity with no supplied effects");
        halo2::source_remove_biped_object_physics_1_1(m, d, remove_component,
                                                      observe);
      };
      rebuild_services.physics.remove = remove_component;
      rebuild_services.physics.dispose = [&](std::uint32_t d) {
        check(plan_index < c.plans.size(), "Original disposal caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0x1d3bb0 && p.stores.empty() && p.args[0] == d,
              "original disposed component with no supplied effects");
        check(halo2::source_dispose_physics_component_bodies_1_1(
                  m, d, {p.esp - 8, p.esp - 4}, {}, observe) == p.result,
              "Original native component disposal result");
      };
      rebuild_services.physics.rebuild = [&](std::uint32_t d) {
        check(plan_index < c.plans.size(), "Original component rebuild caller");
        if (c.plans[plan_index].entry == 0x1d2b10 &&
            c.plans[plan_index].stores.empty()) {
          const auto &p = c.plans[plan_index++];
          check(p.eax == d, "Original native rebuilt component datum");
          halo2::SourceBipedComponentRebuildServices services;
          std::size_t estimate = 0;
          services.reciprocal_sqrt = [&](std::uint32_t operand) {
            check(estimate < p.estimates.size() &&
                      p.estimates[estimate].first == operand,
                  "Original observed platform RSQRTSS operand");
            return p.estimates[estimate++].second;
          };
          halo2::source_rebuild_biped_component_1_1(m, d, p.esp, c.fp, services,
                                                    observe);
          check(estimate == p.estimates.size(),
                "All original platform estimates consumed");
          return;
        }
        lower(0x1d2b10, [&](const Plan &p) {
          check(p.eax == d, "original rebuilt component datum");
        });
      };
      rebuild_services.physics.insert = insert_component;
      rebuild_services.insert_component = insert_component;
      halo2::SourceBipedDeathTransitionServices transition;
      transition.physical.rebuild = rebuild_services.physics;
      halo2::SourceBipedDeathAdmissionServices admission;
      admission.animation_resource = resource;
      admission.update_activation = [&](std::uint32_t d) {
        check(plan_index < c.plans.size(), "Original activation caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0xbaa80 && p.stores.empty() && p.args[0] == d &&
                  (p.edx & 255u) == 0,
              "original activation request with no supplied effects");
        halo2::source_biped_physics_activation_1_1(m, d, false, {}, observe);
      };
      admission.rebuild_ragdoll = [&](std::uint32_t d) {
        return halo2::source_rebuild_biped_ragdoll_1_1(
            m, d, c.scratch[1] + 12, c.fp, rebuild_services, observe);
      };
      admission.maintain_placement = [&](std::uint32_t d) {
        check(plan_index < c.plans.size(), "Original maintenance caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0xe30b0 && p.stores.empty() && p.eax == d,
              "Original maintenance identity with no supplied result stores");
        halo2::source_maintain_biped_ragdoll_1_1(m, d, p.esp, c.fp,
                                                 placement_services, observe);
      };
      admission.impulse = [&](std::uint32_t d, std::int32_t slot,
                              std::uint32_t v) {
        check(plan_index < c.plans.size(), "Original velocity delta caller");
        const auto &p = c.plans[plan_index++];
        check(p.entry == 0x1d4a00 && p.stores.empty(),
              "No supplied velocity result stores");
        check(p.esi == d && p.ecx == std::uint32_t(slot) && p.edx == v,
              "Original root-body velocity delta request");
        halo2::source_add_biped_body_velocity_1_1(m, d, std::uint32_t(slot), v,
                                                  p.esp, c.fp, {}, observe);
      };
      const halo2::SourceBipedDeathAdmissionScratch scratch{
          c.scratch[0], c.scratch[8], c.scratch[9], c.scratch[10]};
      try {
        switch (c.entry) {
        case 0x1d4f30:
          halo2::source_insert_biped_component_world_1_1(
              m, c.ecx, c.scratch[0], c.fp, world_insertion, observe);
          break;
        case 0x1c7320:
          halo2::source_register_biped_world_body_1_1(
              m, c.args[0], std::uint8_t(c.args[1]), c.scratch[0], c.fp,
              world_insertion, observe);
          break;
        case 0x1d5210:
          halo2::source_construct_biped_body_auxiliary_1_1(
              m, c.esi, c.eax, c.args[0], c.scratch[1], c.fp, observe);
          break;
        case 0x314690:
          halo2::source_remove_biped_world_broadphase_1_1(
              m, c.ecx, c.args[0], c.scratch[0], world_removal, observe);
          break;
        case 0x31c6d0:
          halo2::source_retire_physics_island_pair_1_1(m, c.ecx, c.args[0], {},
                                                       observe);
          break;
        case 0x2ef640:
          halo2::source_retire_physics_pair_agent_1_1(m, c.ecx, {}, observe);
          break;
        case 0x31f740:
          halo2::source_destroy_physics_pair_contact_manager_1_1(
              m, c.ecx, c.args[0], {}, observe);
          break;
        case 0x2e8f00:
          halo2::source_remove_physics_broadphase_body_1_1(
              m, c.ecx, c.args[0], c.args[1], c.scratch[0], {}, observe);
          break;
        case 0x2e6520:
          halo2::source_remove_physics_broadphase_axis_1_1(m, c.ecx, c.args[0],
                                                           c.args[1], observe);
          break;
        case 0x2e6340:
          halo2::source_repair_removed_physics_broadphase_indices_1_1(
              m, c.args[0], c.args[1], c.args[2], observe);
          break;
        case 0x1d4c50:
          halo2::source_remove_biped_component_world_1_1(
              m, c.ecx, c.scratch[0], world_removal, observe);
          break;
        case 0x315d50:
          halo2::source_remove_physics_world_body_1_1(m, c.ecx, c.args[0],
                                                      c.args[1], c.scratch[0],
                                                      world_removal, observe);
          break;
        case 0x314810:
          halo2::source_detach_physics_world_body_1_1(
              m, c.ecx, c.args[0], c.scratch[0], world_removal, observe);
          break;
        case 0x313950:
          halo2::source_compact_physics_body_listeners_1_1(m, c.ecx, observe);
          break;
        case 0x31e570:
          halo2::source_destroy_physics_world_island_1_1(m, c.ecx, c.args[0],
                                                         {}, observe);
          break;
        case 0x1da6f0:
        case 0x1d2b10: {
          std::size_t index = 0;
          const halo2::SourcePhysicsReciprocalSqrtEstimate estimate =
              [&](std::uint32_t operand) {
                check(index < c.estimates.size() &&
                          c.estimates[index].first == operand,
                      "Original controller platform RSQRTSS operand");
                return c.estimates[index++].second;
              };
          if (c.entry == 0x1da6f0)
            halo2::source_construct_biped_controller_1_1(
                m,
                {c.args[0], c.args[1], c.args[2], std::uint8_t(c.args[3]),
                 std::uint8_t(c.args[4]), bool(c.args[5] & 255u),
                 bool(c.args[6] & 255u)},
                halo2::source_biped_controller_scratch_1_1(c.controller.entry_esp),
                c.fp, estimate, observe);
          else {
            halo2::SourceBipedComponentRebuildServices services;
            services.reciprocal_sqrt = estimate;
            halo2::source_rebuild_biped_component_1_1(m, c.eax, c.scratch[0],
                                                      c.fp, services, observe);
          }
          check(index == c.estimates.size(),
                "All controller estimates consumed");
          break;
        }
        case 0xdfaf0:
          halo2::source_construct_biped_physical_state_1_1(m, c.args[0], 0,
                                                           c.fp, {}, observe);
          break;
        case 0xe6f60:
          halo2::source_reset_biped_physical_state_1_1(m, c.eax, 0, c.fp, {},
                                                       observe);
          break;
        case 0xe6fb0:
          check(halo2::source_biped_default_physical_state_1_1(m, c.eax) ==
                    std::uint8_t(c.result),
                "Original constructed default physical state");
          break;
        case 0x1f3da0:
          halo2::source_initialize_biped_ground_state_1_1(m, c.eax, observe);
          break;
        case 0xe6050:
          halo2::source_biped_death_transition_1_1(
              m, {c.ecx, c.args[0], c.eax, std::uint8_t(c.args[1])},
              c.scratch[1] + 12, c.fp, transition, observe);
          break;
        case 0xe6dc0:
          halo2::source_transition_biped_dead_1_1(m, c.ebx, c.scratch[1], c.fp,
                                                  transition.physical, observe);
          break;
        case 0x1e8e60:
          halo2::source_biped_physical_state_1_1(
              m, c.edi, std::uint8_t(c.args[0]), c.scratch[1], c.fp,
              transition.physical, observe);
          break;
        case 0x183650:
          halo2::source_biped_rigid_node_slots_1_1(m, c.eax, c.args[0],
                                                   observe);
          break;
        case 0xdd9a0:
          halo2::source_biped_constraint_slots_1_1(m, c.ecx, c.eax, c.args[0],
                                                   c.ebx, observe);
          break;
        case 0xbacc0:
          halo2::source_biped_world_position_1_1(m, c.eax, c.esi, c.fp,
                                                 observe);
          break;
        case 0x13a710:
          check(halo2::source_biped_ragdoll_budget_1_1(m, c.args[0] != 0) ==
                    bool(c.result & 255u),
                "original budget result");
          break;
        case 0x110ee0:
          check(halo2::source_biped_animation_death_gate_1_1(
                    m, c.eax, c.args[0] != 0, c.fp, resource) ==
                    (c.result & 255u),
                "original animation admission result");
          break;
        case 0x1f0e20:
          check(halo2::source_biped_recent_contact_1_1(m, c.ecx, c.args[0],
                                                       c.esi, c.fp, observe) ==
                    bool(c.result & 255u),
                "original recent contact result");
          break;
        case 0xe3190:
          check(halo2::source_place_biped_ragdoll_1_1(
                    m, c.edi, c.args[0], c.args[1], placement,
                    placement_services, observe) == (c.result & 255u),
                "original traversal result");
          break;
        case 0xe2ce0:
          check(halo2::source_rebuild_biped_ragdoll_1_1(
                    m, c.args[0], c.scratch[1] + 12, c.fp, rebuild_services, observe) ==
                    (c.result & 255u),
                "original rebuild result");
          break;
        case 0xe2520:
          check(halo2::source_admit_biped_death_1_1(m, c.args[0], scratch, c.fp,
                                                    admission, observe) ==
                    (c.result & 255u),
                "original admission result");
          break;
        default:
          throw std::runtime_error("unrepresented original admission owner");
        }
        check(plan_index == c.plans.size(),
              "all required lower owners consumed");
        for (unsigned i = 0; i < std::max(observed.size(), c.stores.size());
             ++i)
          if (i >= observed.size() || i >= c.stores.size() ||
              observed[i] != c.stores[i]) {
            std::ostringstream s;
            s << "store " << i << " native count " << observed.size()
              << " original count " << c.stores.size();
            if (i < observed.size())
              s << " native " << std::hex << observed[i].instruction << " at "
                << observed[i].address;
            if (i < c.stores.size())
              s << " original " << std::hex << c.stores[i].instruction << " at "
                << c.stores[i].address;
            throw std::runtime_error(s.str());
          }
        for (const auto &[at, b] : c.after) {
          std::vector<std::byte> actual(b.size());
          m.read(at, actual);
          check(actual == b, "whole original assigned buffer");
          allbytes += b.size();
        }
      } catch (const std::exception &e) {
        throw std::runtime_error("Original admission call " + c.id + ": " +
                                 e.what());
      }
      allstores += observed.size();
      for (const auto &p : c.plans)
        if (p.entry == 0xe3270)
          ++native_placements;
        else if (p.entry == 0x1d4a00)
          ++native_velocity_changes;
        else if (p.entry == 0xe30b0)
          ++native_maintenance;
        else if (p.entry == 0x1e13c0)
          ++native_resources;
        else if (p.entry == 0xbaa80)
          ++native_activation;
        else if (p.entry == 0xb9730)
          ++native_removal;
        else if (p.entry == 0x1d3bb0)
          ++native_disposal;
        else if (p.entry == 0x1d2b10 && p.stores.empty())
          ++native_rebuild;
        else if (p.entry == 0x1d4c50 && p.stores.empty())
          ++native_world_removal;
        else if (p.entry == 0x1d4f30 && p.stores.empty())
          ++native_world_insertion;
        else
          ++allplans;
      ++entries[c.entry];
    }
    check((cases.size() == 4 && entries.size() == 2 && entries[0x1da6f0] == 2 &&
           entries[0x1d2b10] == 2) ||
              (cases.size() == 40 && entries.size() == 12 &&
               entries[0x1d4c50] == 2 && entries[0x315d50] == 2 &&
               entries[0x314810] == 2 && entries[0x313950] == 4 &&
               entries[0x31e570] == 2 && entries[0x2e8f00] == 4 &&
               entries[0x2e6520] == 12 && entries[0x2e6340] == 4 &&
               entries[0x314690] == 2 && entries[0x31c6d0] == 2 &&
               entries[0x2ef640] == 2 && entries[0x31f740] == 2) ||
              (cases.size() == 6 && entries.size() == 3 &&
               entries[0x1d4f30] == 2 && entries[0x1c7320] == 2 &&
               entries[0x1d5210] == 2) ||
              (cases.size() == 203 && entries.size() == 9 &&
               entries[0xe2520] == 12) ||
              (cases.size() == 3 && entries.size() == 3 &&
               entries[0xe6050] == 1 && entries[0xe6dc0] == 1 &&
               entries[0x1e8e60] == 1) ||
              (entries.size() == 6 && entries[0xdfaf0] > 0 &&
               entries[0xdfaf0] == entries[0xe6f60] &&
               entries[0xdfaf0] == entries[0xe6fb0] &&
               entries[0xdfaf0] == entries[0x1f3da0]),
          "complete captured admission owner roster");
    std::cout << cases.size() << " original admission calls matched "
              << allstores << " retained stores and " << allbytes
              << " assigned bytes; " << allplans
              << " explicit fixture-only lower-owner boundaries and "
              << native_placements << " native body placements and "
              << native_velocity_changes << " native velocity changes and "
              << native_maintenance << " native periodic updates and "
              << native_resources << " native animation resource gates and "
              << native_activation << " activation / " << native_removal
              << " object removal / " << native_disposal
              << " controller disposal / " << native_rebuild
              << " component rebuild / " << native_world_removal
              << " world removal / " << native_world_insertion
              << " world insertion owners\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
