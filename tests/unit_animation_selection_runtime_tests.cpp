#include "halo2/source_runtime_memory.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include "halo2/biped_postphysics_runtime.hpp"
#include "halo2/biped_postphysics_phase.hpp"
#include "halo2/source_segment_collision.hpp"
#include "halo2/source_point_collision_runtime.hpp"
#include "halo2/death_world_scheduler.hpp"
#include "halo2/death_object_phases.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
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
  std::vector<std::byte> b;
  for (unsigned i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(word(s.substr(i, 2))));
  return b;
}
using Memory = halo2::SourceRuntimeMemory;
using Store = halo2::SourcePhysicsContactRecordStore;
struct Case {
  std::string name;
  std::uint32_t entry, result, esp_delta;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  halo2::SourceUnitAnimationRuntimeAbi abi{};
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> initial;
  std::vector<Store> stores;
  std::vector<std::array<std::uint32_t, 3>> estimates;
  std::vector<std::array<std::uint32_t, 4>> resources;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<std::uint32_t> arguments;
};
} // namespace
int main(int argc, char **argv) {
  std::string running;
  try {
    check(argc >= 2 && argc <= 4,
          "Supply original animation runtime fixture and optional case count");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string kind, t;
      in >> kind;
      auto n = [&] {
        in >> t;
        return word(t);
      };
      if (kind == "C") {
        Case c{};
        in >> c.name;
        c.entry = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        in >> c.esp_delta;
        for (auto &v : c.abi.gp)
          v = n();
        c.abi.eflags = 2;
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original runtime case");
      auto &c = cases.back();
      if (kind == "M" || kind == "A") {
        const auto p = n();
        in >> t;
        auto b = bytes(t);
        if (kind == "M") {
          c.initial.emplace(p, b);
          c.memory.mount(p, b, Memory::Access::ReadWrite,
                         "original complete selection backing");
        } else
          c.after.emplace(p, std::move(b));
      } else if (kind == "W") {
        Store e{};
        e.instruction = n();
        e.address = n();
        in >> t;
        e.before = bytes(t);
        in >> t;
        e.written = bytes(t);
        c.stores.push_back(std::move(e));
      } else if (kind == "E") {
        in >> t;
        const auto kind = t == "R" ? 0u : 1u;
        const auto x = n(), y = n();
        c.estimates.push_back({kind, x, y});
      } else if (kind == "P") {
        std::array<std::uint32_t, 4> q;
        for (auto &v : q)
          v = n();
        c.resources.push_back(q);
      } else if (kind == "Q") {
        while (in >> t)
          c.arguments.push_back(word(t));
      } else if (kind == "F") {
        c.abi.fs_base = n();
      } else if (kind == "X") {
        unsigned lane;
        in >> lane;
        for (auto &v : c.abi.xmm.at(lane))
          v = n();
      } else
        check(false, "Original runtime fixture record");
    }
    check(cases.size() == (argc >= 3 ? std::stoul(argv[2]) : 22),
          "Complete natural animation captures");
    const std::string selected_mode = argc == 4 ? argv[3] : "raw";
    const bool cycle = selected_mode == "object-cycle-adapted";
    const bool continuous = selected_mode.starts_with("object-continuous") || cycle;
    const auto private_stack = [](std::uint32_t p) {
      return p >= 0x2000000u && p < 0x2200000u;
    };
    Memory persistent;
    if (continuous) {
      std::map<std::uint32_t, std::byte> initial, current;
      const auto read_word = [&](std::uint32_t p) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
          value |= std::to_integer<std::uint32_t>(current.at(p + i)) << (i * 8);
        return value;
      };
      for (unsigned ci = 0; ci < cases.size(); ++ci) {
        const auto &c = cases[ci];
        check(c.entry == std::array{0x13a380u, 0xb7e00u, 0x1c7660u,
                                   0xb7f50u, 0xb8040u}[ci % 5],
              "Original continuous phase order");
        if (ci && ci % 5 == 0) {
          const auto at = read_word(0x518444u) + 8u;
          const auto next = read_word(at) + 1u;
          for (unsigned i = 0; i < 4; ++i)
            current.at(at + i) = std::byte(next >> (i * 8));
        }
        for (const auto &[p, bytes] : c.initial)
          for (unsigned i = 0; i < bytes.size(); ++i) {
            const auto at = p + i;
            initial.try_emplace(at, bytes[i]);
            if (private_stack(at)) continue;
            const auto [it, fresh] = current.try_emplace(at, bytes[i]);
            check(fresh || it->second == bytes[i],
                  c.name + " original persistent input continuity");
          }
        for (const auto &w : c.stores) {
          if (private_stack(w.address)) continue;
          for (unsigned i = 0; i < w.before.size(); ++i)
            check(current.at(w.address + i) == w.before[i],
                  c.name + " original persistent store continuity");
          for (unsigned i = 0; i < w.written.size(); ++i)
            current.at(w.address + i) = w.written[i];
        }
      }
      for (auto i = initial.begin(); i != initial.end();) {
        const auto p = i->first;
        std::vector<std::byte> bytes;
        do { bytes.push_back(i->second); ++i; }
        while (i != initial.end() && i->first == p + bytes.size());
        persistent.mount(p, bytes, Memory::Access::ReadWrite,
                         "original initial continuous death bank");
      }
    }
    std::size_t total = 0, estimate_count = 0, resource_count = 0, phase_index = 0;
    if (cycle) {
      for (auto &c : cases)
        std::erase_if(c.stores, [&](const auto &w) { return private_stack(w.address); });
      for (unsigned first = 0; first < cases.size(); first += 5) {
        if (first) halo2::source_advance_death_object_clock_1_1(persistent);
        Case *active = nullptr;
        std::size_t wi = 0, ei = 0, ri = 0;
        const auto finish = [&] {
          if (!active) return;
          check(wi == active->stores.size() && ei == active->estimates.size() &&
                    ri == active->resources.size(), "Complete native cycle phase");
          for (const auto &[p, wanted] : active->after) {
            if (private_stack(p)) continue;
            std::vector<std::byte> actual(wanted.size());
            persistent.read(p, actual);
            check(actual == wanted, active->name + " native cycle final bytes");
          }
          total += wi; estimate_count += ei; resource_count += ri;
        };
        halo2::SourceDeathWorldSchedulerServices native;
        unsigned begun = 0;
        native.runtime.instruction = [&](std::uint32_t pc) {
          if (pc != 0x13a380 && pc != 0xb7e00 && pc != 0x1c7660 &&
              pc != 0xb7f50 && pc != 0xb8040) return;
          finish();
          check(begun < 5 && cases[first + begun].entry == pc,
                "Original semantic cycle phase order");
          active = &cases[first + begun++]; running = active->name;
          wi = ei = ri = 0;
          for (const auto &[p, wanted] : active->initial) {
            if (private_stack(p)) persistent.write(p, wanted);
            else {
              std::vector<std::byte> actual(wanted.size());
              persistent.read(p, actual);
              check(actual == wanted, running + " semantic cycle retained inputs");
            }
          }
        };
        native.runtime.estimate = [&](auto kind, auto input) {
          check(active && ei < active->estimates.size(), "Native cycle CPU estimate");
          const auto &q = active->estimates[ei++];
          check(q[0] == unsigned(kind) && q[1] == input, "Original cycle CPU input");
          return q[2];
        };
        native.runtime.resource_delivery = [&](auto &memory, const auto &call,
                                               auto &abi, const auto &) {
          check(active && ri < active->resources.size(), "Native cycle cache delivery");
          const auto &q = active->resources[ri++];
          check(call.target == 0x124f30 && abi.gp[4] == q[0] && abi.gp[0] == q[1],
                "Original cycle cache ABI");
          std::array<std::byte, 4> raw;
          memory.read(abi.gp[4] + 4, raw);
          std::uint32_t name = 0;
          for (unsigned i = 0; i < 4; ++i)
            name |= std::to_integer<std::uint32_t>(raw[i]) << (i * 8);
          check(name == q[2], "Original cycle cache name");
          abi.gp[0] = q[3]; abi.gp[4] += 8;
        };
        native.runtime.required_owner = [&](auto &, const auto &call, auto &, const auto &) {
          std::ostringstream out; out << "Unbound cycle owner " << std::hex << call.target;
          throw std::runtime_error(out.str());
        };
        std::uint32_t fs_base = 0;
        for (unsigned n = 0; n < 5; ++n) {
          const auto candidate = cases[first + n].abi.fs_base;
          check(!candidate || !fs_base || fs_base == candidate, "Original cycle thread bank");
          if (candidate) fs_base = candidate;
        }
        halo2::source_step_death_objects_1_1(persistent, cases[first].abi.gp[4],
            fs_base, cases[first].fp, native, [&](const Store &w) {
              if (private_stack(w.address)) return;
              check(active && wi < active->stores.size() && active->stores[wi++] == w,
                    running + " original semantic cycle store");
            });
        finish(); check(begun == 5, "Complete five-phase native cycle");
      }
      std::cout << cases.size()/5 << " continuous native semantic cycles, " << total
                << " stores, " << estimate_count << " CPU estimates and "
                << resource_count << " cache boundaries match\n";
      return 0;
    }
    for (auto &c : cases) {
      running = c.name;
      std::size_t wi = 0, ei = 0, ri = 0;
      const auto &mode = selected_mode;
      auto &memory = continuous ? persistent : c.memory;
      if (continuous) {
        if (phase_index && phase_index % 5 == 0)
          halo2::source_advance_death_object_clock_1_1(memory);
        for (const auto &[p, bytes] : c.initial) {
          if (private_stack(p)) {
            check(private_stack(p + bytes.size() - 1), "Private ABI span extent");
            memory.write(p, bytes);
          } else {
            std::vector<std::byte> actual(bytes.size());
            memory.read(p, actual);
            check(actual == bytes, running + " native persistent phase inputs");
          }
        }
      }
      ++phase_index;
      check(mode == "raw" || mode == "adapted" || mode == "postphysics" ||
                mode == "postphysics-adapted" || mode == "postphysics-phase" ||
                mode == "postphysics-phase-adapted" || mode == "segment" ||
                mode == "segment-adapted" || mode == "segment-camera-adapted" ||
                mode == "point" || mode == "point-adapted" ||
                mode == "world" || mode == "world-adapted" ||
                mode == "object-phases" || mode == "object-continuous" ||
                mode == "object-continuous-adapted",
            "Runtime comparison mode");
      const bool segment = mode.starts_with("segment");
      const bool point = mode.starts_with("point");
      const bool world = mode.starts_with("world");
      const bool object_phases = mode == "object-phases" || continuous;
      const bool postphysics = mode.starts_with("postphysics");
      const bool postphysics_phase = mode.starts_with("postphysics-phase");
      const bool adapter = mode == "adapted" || mode.ends_with("-adapted");
      // Unicorn does not report the host SSE precision sticky bit written by
      // STMXCSR in1C7660. World comparisons retain strict gameplay stores;
      // their private caller/FP scratch is outside this component assertion.
      // The production owner still executes actual STMXCSR/LDMXCSR unchanged.
      const bool retained_only = adapter || world || object_phases;
      const auto stack = [](std::uint32_t p) {
        return p >= 0x2000000u && p < 0x2200000u;
      };
      if (retained_only)
        std::erase_if(c.stores,
                      [&](const auto &s) { return stack(s.address); });
      const auto esp = c.abi.gp[4];
      auto observe = [&](const Store &e) {
        if (retained_only && stack(e.address))
          return;
        check(wi < c.stores.size(), running + " extra original store");
        const auto &wanted = c.stores[wi++];
        if (e != wanted) {
          std::ostringstream s;
          s << running << " store " << wi << " actual " << std::hex
            << e.instruction << ':' << e.address << " expected "
            << wanted.instruction << ':' << wanted.address;
          const auto raw = [&](const auto &bytes) {
            for (auto b : bytes)
              s << std::setfill('0') << std::setw(2)
                << unsigned(std::to_integer<unsigned char>(b));
          };
          s << " before="; raw(e.before);
          s << " written="; raw(e.written);
          s << " expected-before="; raw(wanted.before);
          s << " expected-written="; raw(wanted.written);
          throw std::runtime_error(s.str());
        }
      };
      halo2::SourceUnitAnimationRuntimeServices services;
      services.estimate = [&](auto kind, std::uint32_t input) {
        check(ei < c.estimates.size(), running + " unexpected CPU estimate");
        const auto &q = c.estimates[ei++];
        check(q[0] == (kind == halo2::SourceUnitAnimationEstimate::Reciprocal
                           ? 0u
                           : 1u) &&
                  q[1] == input,
              running + " original CPU estimate input");
        return q[2];
      };
      if (!c.resources.empty())
        services.resource_delivery = [&](auto &memory, const auto &call,
                                         auto &abi, const auto &) {
          check(ri < c.resources.size(),
                running + " extra cache byte delivery");
          const auto &q = c.resources[ri++];
          std::array<std::byte, 4> b;
          memory.read(abi.gp[4] + 4, b);
          std::uint32_t name = 0;
          for (unsigned i = 0; i < 4; ++i)
            name |= std::uint32_t(b[i]) << (8 * i);
          check(call.target == 0x124f30u && abi.gp[4] == q[0] &&
                    abi.gp[0] == q[1] && name == q[2],
                running + " original cache byte delivery ABI");
          abi.gp[0] = q[3];
          abi.gp[4] += 8;
        };
      services.required_owner = [&](auto &, const auto &call, auto &,
                                    const auto &) {
        std::ostringstream message;
        message << running << " unexpected original lower dependency 0x"
                << std::hex << call.target << " at 0x" << call.instruction;
        throw std::runtime_error(message.str());
      };
      if (object_phases) {
        halo2::SourceDeathWorldSchedulerServices scheduler;
        scheduler.runtime = services;
        if (adapter) {
          const auto fs_base = c.abi.fs_base;
          c.abi = {};
          c.abi.gp[4] = esp;
          c.abi.fs_base = fs_base;
          c.abi.eflags = 2;
        }
        halo2::source_run_death_object_phases_1_1(
            memory, c.entry, c.abi, c.fp, scheduler, observe);
      } else if (world) {
        halo2::SourceDeathWorldSchedulerServices scheduler;
        scheduler.runtime = services;
        if (adapter) {
          check(c.entry == 0x1c7660,
                "Defined world scheduler entry");
          halo2::source_step_death_physics_world_1_1(
              memory, esp, c.abi.fs_base, c.fp, scheduler, observe);
          c.abi.gp[4] = esp + 4;
        } else
          halo2::source_run_death_world_scheduler_1_1(
              memory, c.entry, c.abi, c.fp, scheduler, observe);
      } else if (point) {
        if (adapter) {
          check(c.entry == 0x16a9c0 && c.arguments.size() >= 4,
                "Defined world point containment entry");
          const bool contained = halo2::source_world_point_collision_1_1(
              memory, c.arguments[0], c.arguments[1], c.arguments[2],
              c.arguments[3], {esp, c.abi.fs_base}, c.fp, services, observe);
          check(contained == ((c.result & 0xff) != 0),
                "Original point containment result");
          c.abi.gp[4] = esp + 20;
        } else
          halo2::source_run_point_collision_runtime_1_1(
              memory, c.entry, c.abi, c.fp, services, observe);
      } else if (segment) {
        if (adapter) {
          check(c.entry == 0x16b240 && c.arguments.size() >= 6,
                "Defined world segment query entry");
          bool hit;
          if (mode == "segment-camera-adapted") {
            check(c.arguments[3] == 0xffffffffu && c.arguments[4] == 0xffffffffu,
                  "Original camera exclusions");
            const auto vector = [&](std::uint32_t p) {
              std::array<std::byte, 12> bytes;
              memory.read(p, bytes);
              std::array<float, 3> values;
              for (unsigned i = 0; i < 3; ++i) {
                std::uint32_t bits = 0;
                for (unsigned k = 0; k < 4; ++k)
                  bits |= std::uint32_t(bytes[i * 4 + k]) << (k * 8);
                values[i] = std::bit_cast<float>(bits);
              }
              return halo2::Vec3{values[0], values[1], values[2]};
            };
            const auto cast = halo2::source_death_camera_collision_1_1(
                memory, {c.arguments[1], c.arguments[2], c.arguments[5], esp,
                            c.abi.fs_base},
                c.fp, services, observe);
            const auto result = cast(c.arguments[0], vector(c.arguments[1]),
                                     vector(c.arguments[2]));
            hit = result.has_value();
            if (result) {
              std::array<std::byte, 4> raw;
              memory.read(c.arguments[5] + 4, raw);
              std::uint32_t bits = 0;
              for (unsigned k = 0; k < 4; ++k)
                bits |= std::uint32_t(raw[k]) << (k * 8);
              check(std::bit_cast<std::uint32_t>(*result) == bits,
                    "Original camera segment fraction bits");
            }
          } else
            hit = halo2::source_segment_collision_1_1(
                memory, c.arguments[0], c.arguments[1], c.arguments[2],
                c.arguments[3], c.arguments[4], c.arguments[5], {esp,c.abi.fs_base},
                c.fp, services, observe);
          check(hit == ((c.result & 0xff) != 0), "Original segment hit result");
          c.abi.gp[4] = esp + 28;
        } else
          halo2::source_run_segment_collision_1_1(
              memory, c.entry, c.abi, c.fp, services, observe);
      } else if (postphysics) {
        if (postphysics_phase && adapter) {
          check(c.entry == 0xdefc0 && !c.arguments.empty(),
                "Defined biped postphysics phase entry");
          halo2::source_update_biped_postphysics_1_1(
              memory, c.arguments[0], esp, c.fp, services, observe);
          c.abi.gp[4] = esp + 8;
        } else if (postphysics_phase)
          halo2::source_run_biped_postphysics_phase_1_1(
              memory, c.entry, c.abi, c.fp, services, observe);
        else if (adapter) {
          check(c.entry == 0x1e93f0, "Defined contact-dispatch entry");
          halo2::source_update_biped_physical_contacts_1_1(
              memory, c.abi.gp[3], c.abi.gp[0], c.abi.gp[1], esp,
              c.fp, services, observe);
          c.abi.gp[4] = esp + 4;
        } else
          halo2::source_run_biped_postphysics_runtime_1_1(
              memory, c.entry, c.abi, c.fp, services, observe);
      } else if (adapter) {
        unsigned count = 0;
        switch (c.entry) {
        case 0xc04f0:
          count = 3;
          break;
        case 0xe8050:
          count = 2;
          break;
        case 0xe1800:
          count = 2;
          break;
        case 0x1d1290:
          count = 1;
          break;
        case 0xe4f40:
        case 0xdf7c0:
        case 0x142e80:
        case 0x143f60:
          count = 0;
          break;
        case 0xddbc0:
          count = 2;
          break;
        case 0xbea50:
          count = 3;
          break;
        case 0xbe860:
          count = 6;
          break;
        case 0x10d1e0:
          count = 3;
          break;
        case 0x1ca950:
          count = 4;
          break;
        case 0x280030:
          count = 8;
          break;
        case 0x1d0f80:
          count = 7;
          break;
        default:
          throw std::runtime_error("Missing original pose adapter test entry");
        }
        check(c.arguments.size() >= count, running + " captured API arguments");
        halo2::SourceRuntimeCall q{
            .entry = c.entry,
            .entry_esp = esp,
            .arguments = {c.arguments.begin(), c.arguments.begin() + count}};
        switch (c.entry) {
        case 0xbea50:
        case 0x142e80:
        case 0x143f60:
          q.eax = c.abi.gp[0];
          q.ecx = c.abi.gp[1];
          q.edx = c.abi.gp[2];
          break;
        case 0x10d1e0:
        case 0x1ca950:
          q.ebx = c.abi.gp[3];
          break;
        case 0x1d0f80:
          q.esi = c.abi.gp[6];
          break;
        case 0x280030:
        case 0xe4f40:
        case 0xdf7c0:
          q.eax = c.abi.gp[0];
          break;
        case 0x1d1290:
        case 0xe8050:
          q.eax = c.abi.gp[0];
          q.esi = c.abi.gp[6];
          break;
        case 0xbe860:
        case 0xc04f0:
        case 0xddbc0:
        case 0xe1800:
          break;
        default:
          throw std::runtime_error("Missing defined pose input register test");
        }
        c.abi.gp[0] = halo2::source_call_unit_animation_pose_1_1(
            memory, q, c.fp, services, observe);
        c.abi.gp[4] = esp + 4 + 4 * count;
      } else
        halo2::source_run_unit_animation_selection_1_1(memory, c.entry, c.abi,
                                                       c.fp, services, observe);
      // Raw ABI comparisons retain all captured registers. Defined-input
      // adapters expose only actual source return owners: E1800's AL and
      // 143F60's output pointer. Other reached caller results are ignored.
      const auto mask = adapter ? (c.entry == 0xe1800    ? 255u
                                   : c.entry == 0x143f60 ? 0xffffffffu
                                                         : 0u)
                        : c.entry == 0x1cfde0 ? 255u
                                              : 0xffffffffu;
      check((c.abi.gp[0] & mask) == (c.result & mask),
            running + " original defined return");
      check(c.abi.gp[4] == esp + c.esp_delta,
            running + " original stack return");
      check(wi == c.stores.size(),
            running + " complete original store sequence");
      check(ei == c.estimates.size() && ri == c.resources.size(),
            running + " complete original CPU and resource boundaries");
      for (const auto &[p, b] : c.after) {
        if (retained_only && stack(p))
          continue;
        std::vector<std::byte> a(b.size());
        memory.read(p, a);
        check(a == b, running + " original final bytes");
      }
      total += wi;
      estimate_count += ei;
      resource_count += ri;
    }
    std::cout << cases.size()
              << " complete original runtime calls and " << total
              << " stores match; " << estimate_count
              << " original CPU estimates and " << resource_count
              << " explicit cache byte delivery boundaries\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << running << ' ' << e.what() << '\n';
    return 1;
  }
}
