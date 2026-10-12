#include "halo2/physics_body_grouping.hpp"
#include "halo2/physics_ragdoll_constraints.hpp"
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
  check(s.size() % 2 == 0, "Original ragdoll byte extent");
  for (unsigned i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(word(s.substr(i, 2))));
  return b;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::byte> data;
  void read(std::uint32_t at, std::span<std::byte> out) const override {
    for (unsigned i = 0; i < out.size(); ++i) {
      auto q = data.find(at + i);
      if (q == data.end()) {
        std::ostringstream s;
        s << "Required original ragdoll backing " << std::hex << at + i;
        throw std::runtime_error(s.str());
      }
      out[i] = q->second;
    }
  }
  void write(std::uint32_t at, std::span<const std::byte> in) override {
    for (unsigned i = 0; i < in.size(); ++i)
      data.at(at + i) = in[i];
  }
};
struct Case {
  std::string id;
  std::uint32_t entry, eax, ecx, edx, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  std::array<std::uint32_t, 16> args;
  halo2::SourcePhysicsBodyGroupingScratch scratch{};
  std::vector<std::array<std::uint32_t, 14>> factories;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<halo2::SourcePhysicsContactRecordStore> stores;
  std::vector<std::pair<std::uint32_t, std::uint32_t>> estimates;
};
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(
          "Supply independently executed original ragdoll owners fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string kind, x;
      in >> kind;
      if (kind == "C") {
        Case c{};
        in >> c.id >> x;
        c.entry = word(x);
        in >> x;
        c.eax = word(x);
        in >> x;
        c.ecx = word(x);
        in >> x;
        c.edx = word(x);
        in >> x;
        c.fp.x87_control_word = word(x);
        in >> x;
        c.fp.mxcsr = word(x);
        for (auto &a : c.args) {
          in >> x;
          a = word(x);
        }
        cases.push_back(std::move(c));
      } else {
        check(!cases.empty(), "Original ragdoll call owner");
        auto &c = cases.back();
        std::string at, before, after, site;
        if (kind == "M" || kind == "A") {
          in >> at >> after;
          const auto p = word(at);
          const auto b = bytes(after);
          if (kind == "M")
            for (unsigned i = 0; i < b.size(); ++i)
              check(c.memory.data.emplace(p + i, b[i]).second,
                    "Unique original ragdoll input");
          else
            check(c.after.emplace(p, b).second,
                  "Unique original ragdoll output");
        } else if (kind == "D") {
          in >> before >> after;
          c.scratch.constraints_frame = word(before);
          c.scratch.iterator_stride = word(after);
        } else if (kind == "G") {
          std::array<std::uint32_t *, 15> fields{
              &c.scratch.grouping_frame,
              &c.scratch.selection_frame,
              &c.scratch.collection_frame,
              &c.scratch.region_result,
              &c.scratch.permutation_result,
              &c.scratch.constraints_frame,
              &c.scratch.iterator_stride,
              &c.scratch.factory.info,
              &c.scratch.factory.quaternion,
              &c.scratch.factory.auxiliary_bounds,
              &c.scratch.factory.radius_bounds,
              &c.scratch.wrapper.extent_maximum,
              &c.scratch.wrapper.extent_minimum,
              &c.scratch.wrapper.transform,
              &c.scratch.wrapper.configuration};
          for (auto f : fields) {
            in >> x;
            *f = word(x);
          }
        } else if (kind == "T") {
          in >> x;
          c.result = word(x);
        } else if (kind == "B") {
          std::array<std::uint32_t, 14> args{};
          for (auto &a : args) {
            in >> x;
            a = word(x);
          }
          c.factories.push_back(args);
        } else if (kind == "W") {
          in >> site >> at >> before >> after;
          c.stores.push_back(
              {word(site), word(at), bytes(before), bytes(after)});
        } else if (kind == "Q") {
          in >> before >> after;
          c.estimates.emplace_back(word(before), word(after));
        } else
          throw std::runtime_error("Unknown original ragdoll fixture record");
      }
    }
    std::size_t total_bytes = 0, total_stores = 0;
    std::map<std::uint32_t, unsigned> entries;
    for (const auto &c : cases) {
      auto m = c.memory;
      std::vector<halo2::SourcePhysicsContactRecordStore> stores;
      std::size_t qi = 0;
      std::vector<std::array<std::uint32_t, 14>> factories;
      const auto observe = [&](const auto &s) {
        if ((c.entry != 0x1db1c0 && c.entry != 0x1d9330) ||
            !(0x2000000 <= s.address && s.address < 0x2200000))
          stores.push_back(s);
      };
      const auto estimate = [&](auto input) {
        check(qi < c.estimates.size(), "Actual original RSQRTSS required");
        const auto q = c.estimates[qi++];
        check(input == q.first, "Original ragdoll RSQRTSS input");
        return q.second;
      };
      using K = halo2::SourceRagdollConstraintKind;
      try {
        switch (c.entry) {
        case 0x1d9f50:
          halo2::source_physics_body_property_mask_1_1(m, c.args[0], c.args[1],
                                                       c.args[2], observe);
          break;
        case 0x1da020:
          halo2::source_physics_collect_body_groups_1_1(
              m,
              {c.ecx, c.args[0], c.args[1], c.args[2], c.args[3], c.args[6],
               c.args[7], c.args[8], c.args[10], c.args[11]},
              observe);
          break;
        case 0x318e80:
        case 0x3198c0:
          halo2::source_initialize_ragdoll_constraint_packet_1_1(
              m, c.ecx, c.entry == 0x318e80 ? K::ragdoll : K::limited_hinge,
              observe);
          break;
        case 0x1d20f0:
        case 0x1d1ce0:
          halo2::source_set_ragdoll_constraint_axes_1_1(
              m,
              {c.eax,
               c.ecx,
               c.args[0],
               {c.args[1], c.args[2], c.args[3], c.args[4], c.args[5],
                c.args[6]}},
              c.entry == 0x1d20f0 ? K::ragdoll : K::limited_hinge, c.fp,
              estimate, observe);
          break;
        case 0x3192b0:
        case 0x319af0:
          check(halo2::source_construct_ragdoll_constraint_1_1(
                    m, c.ecx, c.args[0],
                    c.entry == 0x3192b0 ? K::ragdoll : K::limited_hinge, c.fp,
                    {}, observe) == c.ecx,
                "Original constructor identity");
          break;
        case 0x1d3b40:
          halo2::source_commit_ragdoll_constraint_1_1(
              m, c.edx, c.args[0], c.args[1], c.args[2], observe);
          break;
        case 0x1db8e0:
          halo2::source_ragdoll_constraint_frames_1_1(
              m,
              {c.args[0], c.args[1], c.args[2], c.args[6], c.eax, c.args[3],
               c.args[4], c.args[5]},
              c.fp, {}, observe);
          break;
        case 0x1d9330:
          check(halo2::source_group_physics_bodies_1_1(
                    m, {c.args[0], c.args[1], c.args[2], c.args[3]}, c.scratch,
                    c.fp, estimate, observe,
                    [&](const auto &a) { factories.push_back(a); }) == c.result,
                "Original full Spartan body request");
          break;
        case 0x1db1c0:
          halo2::source_physics_body_constraints_1_1(
              m, c.args[0], c.args[1], c.scratch, c.fp, estimate, observe);
          break;
        default:
          throw std::runtime_error("Unrepresented original ragdoll owner");
        }
      } catch (const std::exception &e) {
        throw std::runtime_error("Ragdoll call " + c.id + ": " + e.what());
      }
      check(factories == c.factories,
            "Original complete factory arguments/order");
      check(qi == c.estimates.size(),
            "Complete original ragdoll RSQRTSS order");
      for (unsigned i = 0; i < std::max(stores.size(), c.stores.size()); ++i) {
        const auto equal = i < stores.size() && i < c.stores.size() &&
                           stores[i].instruction == c.stores[i].instruction &&
                           stores[i].address == c.stores[i].address &&
                           stores[i].before == c.stores[i].before &&
                           stores[i].written == c.stores[i].written;
        if (!equal) {
          std::ostringstream s;
          s << "Ragdoll call " << c.id << " store " << i << " native "
            << stores.size() << " original " << c.stores.size();
          if (i < stores.size())
            s << " native site " << std::hex << stores[i].instruction << " at "
              << stores[i].address;
          if (i < c.stores.size())
            s << " original site " << std::hex << c.stores[i].instruction
              << " at " << c.stores[i].address;
          throw std::runtime_error(s.str());
        }
      }
      for (const auto &[at, b] : c.after) {
        std::vector<std::byte> actual(b.size());
        m.read(at, actual);
        check(actual == b, "Complete original ragdoll assigned buffer");
        total_bytes += b.size();
      }
      total_stores += stores.size();
      ++entries[c.entry];
    }
    check(cases.size() == 115 && entries.size() == 12,
          "All original Spartan grouping and nine joint owner calls");
    std::cout << cases.size() << " original ragdoll owner calls; "
              << total_bytes << " assigned bytes; " << total_stores
              << " ordered stores match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
