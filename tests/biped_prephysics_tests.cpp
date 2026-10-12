#include "halo2/biped_prephysics.hpp"
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
  check(s.size() % 2 == 0, "Original byte extent");
  std::vector<std::byte> v;
  for (unsigned i = 0; i < s.size(); i += 2)
    v.push_back(std::byte(word(s.substr(i, 2))));
  return v;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::byte> data;
  void read(std::uint32_t p, std::span<std::byte> out) const override {
    for (unsigned i = 0; i < out.size(); ++i) {
      const auto q = data.find(p + i);
      if (q == data.end()) {
        std::ostringstream s;
        s << "Required original phase backing " << std::hex << p + i;
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
  std::uint32_t entry, esp, result;
  std::array<std::uint32_t, 6> regs;
  std::array<std::uint32_t, 16> args;
  std::vector<Store> effects;
};
struct Case {
  std::string name;
  std::uint32_t datum, esp, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<Store> stores;
  std::vector<Plan> plans;
  std::vector<std::array<std::uint32_t, 3>> estimates;
  std::vector<std::array<std::uint32_t, 4>> resources;
  std::vector<std::array<std::uint32_t, 2>> world_estimates;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || (argc == 3 && std::string(argv[2]) == "native-point"),
          "Supply original biped pre-physics fixture and optional native-point");
    const bool native_points = argc == 3;
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    bool native_admission = false, native_input = false, native_phase = false;
    bool native_velocity = false, native_world = false;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string k, t;
      in >> k;
      auto n = [&] {
        in >> t;
        return word(t);
      };
      if (k == "N") {
        native_admission = n() != 0;
        if (in >> t)
          native_input = word(t) != 0;
        if (in >> t)
          native_phase = word(t) != 0;
        if (in >> t)
          native_velocity = word(t) != 0;
        if (in >> t)
          native_world = word(t) != 0;
        continue;
      }
      if (k == "C") {
        Case c{};
        in >> c.name;
        c.datum = n();
        c.esp = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original phase case");
      auto &c = cases.back();
      if (k == "M" || k == "A") {
        const auto p = n();
        in >> t;
        const auto raw = bytes(t);
        if (k == "A")
          c.after.emplace(p, raw);
        else
          for (unsigned i = 0; i < raw.size(); ++i)
            check(c.memory.data.emplace(p + i, raw[i]).second,
                  "Unique original input byte");
      } else if (k == "W" || k == "L") {
        Store s{};
        s.instruction = n();
        s.address = n();
        in >> t;
        s.before = bytes(t);
        in >> t;
        s.written = bytes(t);
        if (k == "W")
          c.stores.push_back(s);
        else {
          check(!c.plans.empty(), "Original lower owner");
          c.plans.back().effects.push_back(s);
        }
      } else if (k == "E") {
        in >> t;
        const auto kind = t == "R" ? 0u : 1u;
        const auto x = n(), y = n();
        c.estimates.push_back({kind, x, y});
      } else if (k == "V") {
        const auto operand = n(), result = n();
        c.world_estimates.push_back({operand, result});
      } else if (k == "D") {
        const auto esp = n(), descriptor = n(), name = n(), resident = n();
        c.resources.push_back({esp, descriptor, name, resident});
      } else if (k == "P") {
        Plan p{};
        p.entry = n();
        p.esp = n();
        p.result = n();
        for (auto &v : p.regs)
          v = n();
        for (auto &v : p.args)
          v = n();
        c.plans.push_back(p);
      } else
        check(false, "Original phase fixture record");
    }
    check(cases.size() == 13, "Complete natural pre-physics capture");
    std::size_t stores = 0, calls = 0, total_bytes = 0;
    for (auto &c : cases) {
      std::size_t wi = 0, pi = 0, ei = 0, di = 0, vi = 0;
      auto observe = [&](const Store &s) {
        if (s.address >= 0x2000000 && s.address < 0x2200000)
          return;
        check(wi < c.stores.size(), c.name + " extra retained store");
        const auto &e = c.stores[wi++];
        if (s != e) {
          std::ostringstream out;
          out << c.name << " store " << wi << " actual " << std::hex
              << s.instruction << ':' << s.address << " expected "
              << e.instruction << ':' << e.address;
          throw std::runtime_error(out.str());
        }
      };
      halo2::SourceBipedPrePhysicsServices services;
      if (native_velocity)
        services.object_velocity.emplace();
      if (native_world) {
        auto &world = services.ragdoll_runtime.emplace();
        if (native_points) {
          auto &point = world.point_runtime.emplace();
          point.fs_base = 0; // Complete16A9C0 has no reached thread access here.
          point.runtime.required_owner = [&](auto &, const auto &call, auto &,
                                             const auto &) {
            throw std::runtime_error(c.name + " unbound original point owner " +
                                     std::to_string(call.target));
          };
        }
        world.component.reciprocal_sqrt = [&](std::uint32_t operand) {
          check(vi < c.world_estimates.size() &&
                    c.world_estimates[vi][0] == operand,
                c.name + " original physics CPU estimate input");
          return c.world_estimates[vi++][1];
        };
      }
      check(!native_points || native_world,
            "Native point composition requires the retained ragdoll world");
      services.required_owner = [&](auto &memory,
                                    const halo2::SourceBipedPhaseCall &call,
                                    const auto &observer) {
        check(pi < c.plans.size(), c.name + " extra lower call");
        const auto &p = c.plans[pi++];
        if (call.entry != p.entry || call.entry_esp != p.esp) {
          std::ostringstream out;
          out << c.name << " call " << pi << " actual " << std::hex
              << call.entry << ':' << call.entry_esp << " expected " << p.entry
              << ':' << p.esp;
          throw std::runtime_error(out.str());
        }
        const std::array actual{call.eax, call.ecx, call.edx,
                                call.esi, call.edi, call.ebx};
        for (unsigned i = 0; i < actual.size(); ++i)
          if (actual[i])
            check(*actual[i] == p.regs[i],
                  c.name + " defined input register " + std::to_string(i));
        if (call.dl)
          check(*call.dl == (p.regs[2] & 255u), c.name + " defined DL input");
        if (call.cl)
          check(*call.cl == (p.regs[1] & 255u), c.name + " defined CL input");
        for (unsigned i = 0; i < call.arguments.size(); ++i)
          if ((call.arguments[i] &
               (call.argument_masks.empty() ? 0xffffffffu
                                            : call.argument_masks.at(i))) !=
              (p.args.at(i) &
               (call.argument_masks.empty() ? 0xffffffffu
                                            : call.argument_masks.at(i)))) {
            std::ostringstream out;
            out << c.name << " call " << std::hex << call.entry << " argument "
                << i << " actual " << call.arguments[i] << " expected "
                << p.args.at(i);
            throw std::runtime_error(out.str());
          }
        for (const auto &s : p.effects) {
          std::vector<std::byte> before(s.before.size());
          memory.read(s.address, before);
          // Outgoing ABI scratch has no defined initial contents. Native
          // calls do not reproduce incidental original prolog spill bytes.
          // Retained bank input bytes must still match before each effect.
          if (before != s.before &&
              !(s.address >= 0x2000000 && s.address < 0x2200000)) {
            std::ostringstream out;
            out << c.name << " lower " << std::hex << p.entry << " before "
                << s.instruction << ':' << s.address;
            throw std::runtime_error(out.str());
          }
          memory.write(s.address, s.written);
          if (observer)
            observer(s);
        }
        return p.result;
      };
      auto lower = [&](halo2::SourceBipedPhaseCall request) {
        return services.required_owner(c.memory, request, observe);
      };
      if (native_input || native_phase) {
        halo2::SourceUnitAnimationRuntimeServices physical_input;
        physical_input.estimate = [&](auto kind, auto value) {
          check(ei < c.estimates.size(),
                c.name + " extra physical-input estimate");
          const auto &q = c.estimates[ei++];
          check(q[0] == unsigned(kind) && q[1] == value,
                c.name + " original physical-input CPU estimate");
          return q[2];
        };
        physical_input.resource_delivery = [&](auto &memory, const auto &call,
                                               auto &abi, const auto &) {
          check(di < c.resources.size(), c.name + " extra resource delivery");
          const auto &q = c.resources[di++];
          check(call.target == 0x124f30 && abi.gp[4] == q[0] &&
                    abi.gp[0] == q[1],
                c.name + " original physical-input resource ABI");
          std::array<std::byte, 4> raw;
          memory.read(abi.gp[4] + 4, raw);
          std::uint32_t name = 0;
          for (unsigned i = 0; i < 4; ++i)
            name |= std::to_integer<std::uint32_t>(raw[i]) << (8 * i);
          check(name == q[2],
                c.name + " original physical-input resource name");
          abi.gp[0] = q[3];
          abi.gp[4] += 8;
        };
        physical_input.required_owner = [&](auto &, const auto &call, auto &,
                                            const auto &) {
          throw std::runtime_error(c.name + " unbound physical-input owner " +
                                   std::to_string(call.target));
        };
        if (native_input)
          services.physical_input = physical_input;
        if (native_phase)
          services.phase_runtime = physical_input;
      }
      if (native_admission) {
        const auto entry = c.esp - 0x184u;
        auto &admission = services.death_admission.emplace();
        admission.animation_resource = [&](auto graph, auto packed) {
          halo2::source_biped_animation_resource_1_1(c.memory, graph, packed);
        };
        admission.update_activation = [&, entry](auto d) {
          lower(
              {.entry = 0xbaa80, .entry_esp = entry - 0x60, .arguments = {d}});
        };
        admission.rebuild_ragdoll = [&, entry](auto d) {
          return std::uint8_t(lower(
              {.entry = 0xe2ce0, .entry_esp = entry - 0x60, .arguments = {d}}));
        };
        admission.maintain_placement = [&, entry](auto d) {
          lower({.entry = 0xe30b0, .entry_esp = entry - 0x5c, .eax = d});
        };
        admission.set_velocity = [&, entry](auto d, auto vector) {
          lower({.entry = 0xb86c0,
                 .entry_esp = entry - 0x60,
                 .esi = d,
                 .arguments = {vector}});
        };
        admission.rebuild_controller = [&, entry](auto d) {
          lower({.entry = 0x1c70d0,
                 .entry_esp = entry - 0x64,
                 .arguments = {d, 0}});
        };
        admission.recover_controller = [&, entry](auto d) {
          lower({.entry = 0xe2f00,
                 .entry_esp = entry - 0x60,
                 .eax = d,
                 .arguments = {1}});
        };
        admission.impulse = [&, entry](auto component, auto slot, auto vector) {
          lower({.entry = 0x1d4a00,
                 .entry_esp = entry - 0x60,
                 .ecx = std::uint32_t(slot),
                 .edx = vector,
                 .esi = component});
        };
      }
      const auto result = halo2::source_update_biped_prephysics_1_1(
          c.memory, c.datum, c.esp, c.fp, services, observe);
      check(result == (c.result & 255u), c.name + " original AL result");
      check(wi == c.stores.size() && pi == c.plans.size(),
            c.name + " complete owner sequence");
      check(ei == c.estimates.size() && di == c.resources.size(),
            c.name + " complete physical-input boundaries");
      check(vi == c.world_estimates.size(),
            c.name + " complete physics CPU estimate boundaries");
      for (const auto &[p, e] : c.after) {
        std::vector<std::byte> actual(e.size());
        c.memory.read(p, actual);
        check(actual == e, c.name + " retained final bytes");
        total_bytes += e.size();
      }
      stores += wi;
      calls += pi;
    }
    std::cout << cases.size() << " original pre-physics phases, " << calls
              << " lower owner boundaries, " << stores << " stores and "
              << total_bytes << " retained bytes match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
