#include "halo2/biped_death_runtime.hpp"
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
  check(s.size() % 2 == 0, "Original biped death byte extent");
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
        s << "Required original biped death backing " << std::hex << at + i;
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
  std::uint32_t entry, eax, ecx, result, scratch;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  std::array<std::uint32_t, 16> args;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<halo2::SourcePhysicsContactRecordStore> stores;
};
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(
          "Supply independently executed original biped death owners fixture");
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
        c.fp.x87_control_word = word(x);
        in >> x;
        c.fp.mxcsr = word(x);
        in >> x;
        c.result = word(x);
        in >> x;
        c.scratch = word(x);
        for (auto &a : c.args) {
          in >> x;
          a = word(x);
        }
        cases.push_back(std::move(c));
      } else {
        check(!cases.empty(), "Original biped death call owner");
        auto &c = cases.back();
        std::string at, before, after, site;
        if (kind == "M" || kind == "A") {
          in >> at >> after;
          const auto p = word(at);
          const auto b = bytes(after);
          if (kind == "M")
            for (unsigned i = 0; i < b.size(); ++i)
              check(c.memory.data.emplace(p + i, b[i]).second,
                    "Unique original biped death input");
          else
            check(c.after.emplace(p, b).second,
                  "Unique original biped death output");
        } else if (kind == "W") {
          in >> site >> at >> before >> after;
          c.stores.push_back(
              {word(site), word(at), bytes(before), bytes(after)});
        } else
          throw std::runtime_error(
              "Unknown original biped death fixture record");
      }
    }
    std::size_t total_bytes = 0, total_stores = 0;
    std::map<std::uint32_t, unsigned> entries;
    for (const auto &c : cases) {
      auto m = c.memory;
      std::vector<halo2::SourcePhysicsContactRecordStore> stores;
      const auto observe = [&](const auto &s) {
        if (!(0x2000000 <= s.address && s.address < 0x2200000))
          stores.push_back(s);
      };
      try {
        switch (c.entry) {
        case 0xe5680:
          check(halo2::source_biped_airborne_restriction_1_1(m, c.eax, c.fp) ==
                    ((c.result & 255u) != 0),
                "Original airborne gate");
          break;
        case 0x1f08e0:
          check(halo2::source_biped_contact_ragdoll_request_1_1(
                    m, c.eax, c.fp) == ((c.result & 255u) != 0),
                "Original contact admission gate");
          break;
        case 0xe2430:
          halo2::source_update_biped_death_constraints_1_1(m, c.eax, c.scratch,
                                                           c.fp, observe);
          break;
        case 0x1d6fc0:
          halo2::source_update_biped_ragdoll_constraint_1_1(
              m, c.ecx, c.eax, c.args[0], c.scratch, c.fp, observe);
          break;
        default:
          throw std::runtime_error("Unrepresented original biped death owner");
        }
      } catch (const std::exception &e) {
        throw std::runtime_error("Biped death call " + c.id + ": " + e.what());
      }
      for (unsigned i = 0; i < std::max(stores.size(), c.stores.size()); ++i) {
        const auto equal = i < stores.size() && i < c.stores.size() &&
                           stores[i].instruction == c.stores[i].instruction &&
                           stores[i].address == c.stores[i].address &&
                           stores[i].before == c.stores[i].before &&
                           stores[i].written == c.stores[i].written;
        if (!equal) {
          std::ostringstream s;
          s << "Biped death call " << c.id << " store " << i << " native "
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
        check(actual == b, "Complete original biped death assigned buffer");
        total_bytes += b.size();
      }
      total_stores += stores.size();
      ++entries[c.entry];
    }
    check(cases.size() == 58 && entries.size() == 4,
          "All natural original contact and death constraint owner calls");
    std::cout << cases.size() << " original biped death owner calls; "
              << total_bytes << " assigned bytes; " << total_stores
              << " ordered stores match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
