#include "halo2/source_runtime_memory.hpp"
#include "halo2/unit_animation_runtime.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool b, const std::string &why) {
  if (!b)
    throw std::runtime_error(why);
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
  std::uint32_t entry, object, manager, esp, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  Memory memory;
  std::vector<Store> stores;
  std::map<std::uint32_t, std::vector<std::byte>> after;
};
} // namespace
int main(int argc, char **argv) {
  std::string running;
  try {
    check(argc == 2, "Supply original animation weight fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string k, s;
      in >> k;
      auto n = [&] {
        in >> s;
        return word(s);
      };
      if (k == "C") {
        Case c{};
        in >> c.name;
        c.entry = n();
        c.object = n();
        c.manager = n();
        c.esp = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original weight case");
      auto &c = cases.back();
      if (k == "M" || k == "A") {
        const auto p = n();
        in >> s;
        auto raw = bytes(s);
        if (k == "M")
          c.memory.mount(p, raw, Memory::Access::ReadWrite,
                         "declared original component backing");
        else
          c.after.emplace(p, std::move(raw));
      } else if (k == "W") {
        Store e{};
        e.instruction = n();
        e.address = n();
        in >> s;
        e.before = bytes(s);
        in >> s;
        e.written = bytes(s);
        c.stores.push_back(std::move(e));
      } else
        check(false, "Original weight fixture record");
    }
    check(cases.size() == 3072, "Complete four-mode weight/clock matrix");
    std::size_t stores = 0;
    for (auto &c : cases) {
      running = c.name;
      std::size_t wi = 0;
      auto observe = [&](const Store &e) {
        if (e.address >= 0x2000000 && e.address < 0x2010000)
          return;
        check(wi < c.stores.size(), c.name + " extra retained store");
        const auto &expected = c.stores[wi++];
        if (e != expected) {
          std::ostringstream out;
          out << "store " << wi << " actual " << std::hex << e.instruction
              << ':' << e.address << " expected " << expected.instruction
              << ':' << expected.address;
          throw std::runtime_error(out.str());
        }
      };
      bool result = false;
      switch (c.entry) {
      case 0x1150d0:
        result = halo2::source_advance_unit_transition_clocks_1_1(
            c.memory, c.object, observe);
        break;
      case 0x10f430:
        result = halo2::source_decay_unit_animation_weights_1_1(
            c.memory, c.object, c.esp, c.fp, observe);
        break;
      case 0x1102b0:
        result = halo2::source_blend_unit_animation_weights_1_1(
            c.memory, c.object, c.esp, c.fp, observe);
        break;
      case 0x1105e0:
        result = halo2::source_unit_animation_completed_1_1(c.memory, c.manager);
        break;
      default:
        check(false, "Unknown original weight owner");
      }
      check(std::uint32_t(result) == c.result, "Original AL result");
      check(wi == c.stores.size(), "Complete original retained store sequence");
      for (const auto &[p, expected] : c.after) {
        std::vector<std::byte> actual(expected.size());
        c.memory.read(p, actual);
        check(actual == expected, "Original retained weight/clock bytes");
      }
      stores += wi;
    }
    std::cout << cases.size() << " original weight/transition-clock cases and "
              << stores << " retained stores match in four FP modes\n";
  } catch (const std::exception &e) {
    std::cerr << running << ": " << e.what() << '\n';
    return 1;
  }
}
