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
  check(s.size() % 2 == 0, "Original byte extent");
  std::vector<std::byte> v;
  for (unsigned i = 0; i < s.size(); i += 2)
    v.push_back(std::byte(word(s.substr(i, 2))));
  return v;
}
using Memory = halo2::SourceRuntimeMemory;
using Store = halo2::SourcePhysicsContactRecordStore;
struct Plan {
  std::uint32_t entry, esp, result;
  std::array<std::uint32_t, 6> regs;
  std::array<std::uint32_t, 16> args;
  std::vector<Store> effects;
};
struct Case {
  std::string name;
  std::uint32_t state, physical, output, esp, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<Store> stores;
  std::vector<Plan> plans;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original biped physical step fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    bool native_admission = false;
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
        continue;
      }
      if (k == "C") {
        Case c{};
        in >> c.name;
        c.state = n();
        c.physical = n();
        c.output = n();
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
          c.memory.mount(p, raw, Memory::Access::ReadWrite,
                         "original caller backing");
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
    check(cases.size() == 13, "Complete natural physical step capture");
    std::size_t stores = 0, calls = 0, total_bytes = 0;
    for (auto &c : cases) {
      std::size_t wi = 0, pi = 0;
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
        for (unsigned i = 0; i < call.arguments.size(); ++i)
          if (call.arguments[i] != p.args.at(i)) {
            std::ostringstream out;
            out << c.name << " call " << std::hex << call.entry << " argument "
                << i << " actual " << call.arguments[i] << " expected "
                << p.args.at(i);
            throw std::runtime_error(out.str());
          }
        for (const auto &s : p.effects) {
          std::vector<std::byte> before(s.before.size());
          memory.read(s.address, before);
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
      halo2::source_step_biped_physical_1_1(c.memory, c.state, c.physical,
                                            c.output, c.esp, c.fp, services,
                                            observe);
      check(wi == c.stores.size() && pi == c.plans.size(),
            c.name + " complete owner sequence");
      for (const auto &[p, e] : c.after) {
        std::vector<std::byte> actual(e.size());
        c.memory.read(p, actual);
        check(actual == e, c.name + " retained final bytes");
        total_bytes += e.size();
      }
      stores += wi;
      calls += pi;
    }
    std::cout << cases.size() << " original physical step phases, " << calls
              << " lower owner boundaries, " << stores << " stores and "
              << total_bytes << " retained bytes match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
