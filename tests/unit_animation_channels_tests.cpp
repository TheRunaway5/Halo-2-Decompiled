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
struct Callback {
  std::uint32_t esp;
  std::array<std::uint32_t, 3> arguments;
  std::vector<std::byte> packet;
};
struct Case {
  std::string name;
  std::uint32_t entry, channel, graph, definition, options, from, to, callback,
      context, esp, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  Memory memory;
  std::vector<Store> stores;
  std::vector<Callback> callbacks;
  std::map<std::uint32_t, std::vector<std::byte>> after;
};
} // namespace
int main(int argc, char **argv) {
  std::string running;
  try {
    check(argc == 2, "Supply original animation channel fixture");
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
        c.channel = n();
        c.graph = n();
        c.definition = n();
        c.options = n();
        c.from = n();
        c.to = n();
        c.callback = n();
        c.context = n();
        c.esp = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original channel case");
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
      } else if (k == "P") {
        Callback p{};
        p.esp = n();
        for (auto &v : p.arguments)
          v = n();
        in >> s;
        p.packet = bytes(s);
        c.callbacks.push_back(std::move(p));
      } else
        check(false, "Original channel fixture record");
    }
    check(cases.size() == 2304, "Complete four-mode channel matrix");
    std::size_t stores = 0, calls = 0;
    for (auto &c : cases) {
      running = c.name;
      std::size_t wi = 0, pi = 0;
      auto observe = [&](const Store &e) {
        if (e.address >= 0x2000000 && e.address < 0x2010000)
          return;
        check(wi < c.stores.size(), c.name + " extra retained store");
        const auto &expected = c.stores[wi++];
        if (e != expected) {
          std::ostringstream out;
          out << c.name << " store " << wi << " actual " << std::hex
              << e.instruction << ':' << e.address << " expected "
              << expected.instruction << ':' << expected.address;
          throw std::runtime_error(out.str());
        }
      };
      halo2::SourceRuntimeOwner owner = [&](auto &memory, const auto &q,
                                           const auto &) {
        check(q.entry == c.callback && c.callback,
              c.name + " unexpected source dependency");
        check(pi < c.callbacks.size(), c.name + " extra event observer");
        const auto &expected = c.callbacks[pi++];
        check(q.entry_esp == expected.esp, c.name + " observer entry ESP");
        check(q.arguments.size() == expected.arguments.size(),
              c.name + " observer ABI extent");
        for (unsigned i = 0; i < expected.arguments.size(); ++i)
          check(q.arguments[i] == expected.arguments[i],
                c.name + " event observer argument " + std::to_string(i));
        std::vector<std::byte> packet(expected.packet.size());
        memory.read(q.arguments[2], packet);
        check(packet == expected.packet, c.name + " complete original event packet");
        // An externally owned observer has no animation/effect admission
        // result. Original traversal does not consume its return register.
        return 0u;
      };
      switch (c.entry) {
      case 0x1ca090:
        halo2::source_set_unit_animation_frame_1_1(
            c.memory, c.channel, c.to, c.channel + 0x100, c.callback,
            c.context, c.esp, c.fp, owner, observe);
        break;
      case 0x1c9f50:
        halo2::source_clamp_unit_animation_frame_1_1(
            c.memory, c.channel, c.to, c.esp, c.fp, owner, observe);
        break;
      case 0x1ca2b0:
        halo2::source_advance_unit_animation_channel_1_1(
            c.memory, c.channel, c.channel + 0x100, c.callback, c.context,
            c.esp, c.fp, owner, observe);
        break;
      case 0x1cae40:
        check(halo2::source_traverse_unit_animation_events_1_1(
                  c.memory, c.graph, c.definition, c.options, c.from, c.to,
                  c.callback, c.context, c.esp, c.fp, owner, observe) == c.result,
              c.name + " original timing event mask");
        break;
      default:
        check(false, "Unknown original channel owner");
      }
      check(wi == c.stores.size() && pi == c.callbacks.size(),
            c.name + " complete original store/event sequence");
      for (const auto &[p, expected] : c.after) {
        std::vector<std::byte> actual(expected.size());
        c.memory.read(p, actual);
        check(actual == expected, c.name + " original retained channel bytes");
      }
      stores += wi;
      calls += pi;
    }
    std::cout << cases.size() << " original channel/frame/event cases, "
              << stores << " retained stores and " << calls
              << " complete event packets match in four FP modes\n";
  } catch (const std::exception &e) {
    std::cerr << running << ": " << e.what() << '\n';
    return 1;
  }
}
