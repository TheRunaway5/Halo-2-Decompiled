#include "halo2/biped_prephysics.hpp"
#include "halo2/source_runtime_memory.hpp"
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
  check(s.size() % 2 == 0, "Original movement helper extent");
  std::vector<std::byte> v;
  for (unsigned i = 0; i < s.size(); i += 2)
    v.push_back(std::byte(word(s.substr(i, 2))));
  return v;
}
using Memory = halo2::SourceRuntimeMemory;
using Store = halo2::SourcePhysicsContactRecordStore;
struct Case {
  std::string name;
  std::uint32_t owner, datum, output, mode, byte, esp, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  Memory memory;
  std::vector<Store> stores;
  std::map<std::uint32_t, std::vector<std::byte>> after;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original movement helper fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string k, t;
      in >> k;
      auto n = [&] {
        in >> t;
        return word(t);
      };
      if (k == "C") {
        Case c{};
        in >> c.name;
        c.owner = n();
        c.datum = n();
        c.output = n();
        c.mode = n();
        c.byte = n();
        c.esp = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original movement helper case");
      auto &c = cases.back();
      if (k == "M" || k == "A") {
        auto p = n();
        in >> t;
        auto raw = bytes(t);
        if (k == "A")
          c.after.emplace(p, raw);
        else
          c.memory.mount(p, raw, Memory::Access::ReadWrite,
                         "original helper backing");
      } else if (k == "W") {
        Store s{};
        s.instruction = n();
        s.address = n();
        in >> t;
        s.before = bytes(t);
        in >> t;
        s.written = bytes(t);
        c.stores.push_back(std::move(s));
      } else
        check(false, "Original movement helper record");
    }
    check(cases.size() == 1324, "All declared movement helper experiments");
    std::size_t stores = 0;
    for (auto &c : cases) {
      std::size_t wi = 0;
      auto observe = [&](const Store &s) {
        if (s.address >= 0x2000000 && s.address < 0x2010000)
          return;
        check(wi < c.stores.size(), c.name + " extra retained store");
        const auto &e = c.stores[wi++];
        if (s != e) {
          std::ostringstream out;
          out << c.name << " store " << wi << " actual " << std::hex
              << s.instruction << ':' << s.address;
          if (s.written.size() == 4 && e.written.size() == 4) {
            std::uint32_t av = 0, ev = 0;
            for (unsigned i = 0; i < 4; ++i) {
              av |= std::to_integer<unsigned>(s.written[i]) << (8 * i);
              ev |= std::to_integer<unsigned>(e.written[i]) << (8 * i);
            }
            out << " value " << av << " expected " << ev;
          }
          throw std::runtime_error(out.str());
        }
      };
      if (c.owner == 0xdf5e0)
        check(halo2::source_biped_physical_activation_1_1(c.memory, c.datum,
                                                          c.fp) == c.result,
              c.name + " original AL");
      else if (c.owner == 0xdee00)
        check(halo2::source_biped_physical_fraction_1_1(
                  c.memory, c.datum, c.esp, c.fp, observe) == c.result,
              c.name + " original XMM0");
      else if (c.owner == 0xe44c0)
        halo2::source_biped_movement_action_1_1(
            c.memory, c.datum, c.output, c.output + 4, c.output + 8, c.mode,
            c.byte, c.esp, {}, observe);
      else
        check(false, "Original helper identity");
      check(wi == c.stores.size(), c.name + " missing retained stores");
      stores += wi;
      for (auto &[p, raw] : c.after) {
        std::vector<std::byte> actual(raw.size());
        c.memory.read(p, actual);
        check(actual == raw, c.name + " retained output");
      }
    }
    std::cout << cases.size() << " original movement helper cases, " << stores
              << " retained stores, four FP rounding modes\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
