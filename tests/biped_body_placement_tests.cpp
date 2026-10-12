#include "halo2/biped_body_placement.hpp"
#include "halo2/biped_body_velocity.hpp"
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
std::uint32_t number(const std::string &s) {
  return std::stoul(s, nullptr, 16);
}
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size() % 2 == 0, "Literal original byte extent");
  std::vector<std::byte> b;
  for (unsigned i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(number(s.substr(i, 2))));
  return b;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::byte> data;
  void read(std::uint32_t p, std::span<std::byte> b) const override {
    for (unsigned i = 0; i < b.size(); ++i) {
      auto q = data.find(p + i);
      if (q == data.end()) {
        std::ostringstream s;
        s << "Required original placement backing " << std::hex << p + i;
        throw std::runtime_error(s.str());
      }
      b[i] = q->second;
    }
  }
  void write(std::uint32_t p, std::span<const std::byte> b) override {
    for (unsigned i = 0; i < b.size(); ++i)
      data.at(p + i) = b[i];
  }
};
using Store = halo2::SourcePhysicsContactRecordStore;
struct Case {
  std::string id;
  std::uint32_t entry, esp, eax, ecx, edx, ebx, esi, edi, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  std::array<std::uint32_t, 16> args;
  halo2::SourcePointCollisionScratch point;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<Store> stores;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original body placement fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream in(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(in, line);) {
      std::istringstream row(line);
      std::string k, x;
      row >> k;
      auto n = [&] {
        row >> x;
        return number(x);
      };
      if (k == "C") {
        Case c{};
        row >> c.id;
        c.entry = n();
        c.esp = n();
        c.eax = n();
        c.ecx = n();
        c.edx = n();
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
        check(!cases.empty(), "Original placement owner");
        auto &c = cases.back();
        if (k == "S") {
          c.point.record_pointer = n();
          c.point.iterator = n();
          c.point.instance_local_point = n();
        } else if (k == "M" || k == "A") {
          const auto p = n();
          row >> x;
          auto b = bytes(x);
          if (k == "M")
            for (unsigned i = 0; i < b.size(); ++i)
              check(c.memory.data.emplace(p + i, b[i]).second,
                    "Unique original input");
          else
            check(c.after.emplace(p, b).second, "Unique original output");
        } else if (k == "W") {
          Store s{};
          s.instruction = n();
          s.address = n();
          row >> x;
          s.before = bytes(x);
          row >> x;
          s.written = bytes(x);
          c.stores.push_back(std::move(s));
        } else
          throw std::runtime_error("Unknown original placement row");
      }
    }
    std::size_t total_stores = 0, total_bytes = 0;
    for (const auto &c : cases) {
      try {
        auto m = c.memory;
        std::vector<Store> stores;
        auto owns = [&](const Store &s) {
          auto span = [&](std::uint32_t p, std::uint32_t n) {
            return p <= s.address &&
                   std::uint64_t(s.address) + s.written.size() <=
                       std::uint64_t(p) + n;
          };
          if (c.entry == 0x1d3970)
            return span(c.esi, 12) || span(c.edi, 12);
          if (c.entry == 0x142f20)
            return span(c.args[0], 52);
          if (c.entry == 0x144620)
            return span(c.eax, 36);
          return !(0x2000000 <= s.address && s.address < 0x2200000);
        };
        const auto observe = [&](const Store &s) {
          if (owns(s))
            stores.push_back(s);
        };
        const halo2::SourceBipedBodyTransformServices transform_services{};
        switch (c.entry) {
        case 0xe3270: {
          halo2::SourceBipedBodyPlacementServices services;
          services.point = [&](std::uint32_t p) {
            return halo2::source_point_collision_1_1(m, 0x80040d, p, c.args[0],
                                                     0xffffffffu, c.point, c.fp,
                                                     {}, observe);
          };
          services.set_transform = [&](std::uint32_t component,
                                       std::uint32_t slot,
                                       std::uint32_t matrix) {
            halo2::source_biped_body_transform34_1_1(
                m, component, slot, matrix, c.esp - 0x144, c.fp,
                transform_services, observe);
          };
          const halo2::SourceBipedPlacementRequest q{c.args[0],
                                                     c.args[1],
                                                     c.args[2],
                                                     c.args[3],
                                                     c.args[4],
                                                     c.args[5],
                                                     std::uint8_t(c.args[6])};
          check(halo2::source_place_biped_body_1_1(
                    m, q, c.esp, c.fp, services, observe) == (c.result & 255u),
                "Original placement result");
          break;
        }
        case 0x1d3970:
          halo2::source_biped_constraint_pivots_1_1(
              m, c.eax, c.edi, c.esi, ((c.esp - 4) & ~15u) - 0x90, observe);
          break;
        case 0x1d4610:
          halo2::source_biped_body_transform34_1_1(m, c.args[0], c.ecx, c.eax,
                                                   c.esp, c.fp,
                                                   transform_services, observe);
          break;
        case 0x1d46e0:
          halo2::source_biped_body_transform40_1_1(
              m, c.ebx, c.ecx, c.eax, c.esp, c.fp, transform_services, observe);
          break;
        case 0x1d47c0:
          halo2::source_biped_body_velocity_1_1(
              m, c.args[0], c.ecx, c.eax, c.esp, transform_services, observe);
          break;
        case 0x1d4a00:
          halo2::source_add_biped_body_velocity_1_1(
              m, c.esi, c.ecx, c.edx, c.esp, c.fp, transform_services, observe);
          break;
        case 0x1d4840:
          halo2::source_biped_body_angular_velocity_1_1(
              m, c.args[0], c.eax, c.ecx, c.esp, transform_services, observe);
          break;
        case 0x1d5600:
          check(halo2::source_biped_component_active_1_1(m, c.esi, c.esp, observe)
                    == bool(c.result & 255u), "Original current-island admission");
          break;
        case 0x1c8430:
          halo2::source_biped_object_velocity_1_1(
              m, c.eax, c.ebx, c.args[0], std::uint8_t(c.args[1]), c.esp, {}, observe);
          break;
        case 0x142f20:
          halo2::source_biped_placement_rotation_1_1(m, c.ecx, c.edx, c.args[0],
                                                     c.fp, observe);
          break;
        case 0x144620:
          halo2::source_biped_placement_basis_product_1_1(m, c.edx, c.ecx,
                                                          c.eax, c.fp, observe);
          break;
        default:
          throw std::runtime_error("Unexpected original placement entry");
        }
        check(stores.size() == c.stores.size(),
              "Original ordered store count " + std::to_string(stores.size()) +
                  " vs " + std::to_string(c.stores.size()));
        for (unsigned i = 0; i < stores.size(); ++i) {
          const auto &a = stores[i];
          const auto &b = c.stores[i];
          if (a.instruction != b.instruction || a.address != b.address ||
              a.before != b.before || a.written != b.written) {
            std::ostringstream s;
            s << "Original store " << i << " site " << std::hex << a.instruction
              << " vs " << b.instruction << " address " << a.address << " vs "
              << b.address << " before_equal " << (a.before == b.before)
              << " value_equal " << (a.written == b.written) << " actual ";
            for (auto v : a.written)
              s << unsigned(v) << ',';
            s << " expected ";
            for (auto v : b.written)
              s << unsigned(v) << ',';
            throw std::runtime_error(s.str());
          }
        }
        for (const auto &[p, b] : c.after) {
          std::vector<std::byte> actual(b.size());
          m.read(p, actual);
          check(actual == b, "Original complete output bytes");
          total_bytes += b.size();
        }
        total_stores += stores.size();
      } catch (const std::exception &e) {
        throw std::runtime_error("case " + c.id + " entry " +
                                 std::to_string(c.entry) + ": " + e.what());
      }
    }
    check(!cases.empty(), "Original placement cases exist");
    std::cout << cases.size() << " original body placement cases, "
              << total_stores << " ordered stores, " << total_bytes
              << " assigned bytes match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
