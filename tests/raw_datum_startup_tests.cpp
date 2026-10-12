#include "halo2/raw_datum_pool.hpp"
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
  check(s.size() % 2 == 0, "Original raw pool startup extent");
  std::vector<std::byte> v;
  for (unsigned i = 0; i < s.size(); i += 2)
    v.push_back(std::byte(word(s.substr(i, 2))));
  return v;
}
using Memory = halo2::SourceRuntimeMemory;
using Store = halo2::SourcePhysicsContactRecordStore;
struct Case {
  std::string name;
  std::uint32_t owner, datum, name_address, capacity, stride, alignment,
      context, bitmap, elements;
  Memory memory;
  std::vector<Store> stores;
  std::map<std::uint32_t, std::vector<std::byte>> after;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original raw pool startup fixture");
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
        c.name_address = n();
        c.capacity = n();
        c.stride = n();
        c.alignment = n();
        c.context = n();
        c.bitmap = n();
        c.elements = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original raw pool startup case");
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
        check(false, "Original raw pool startup record");
    }
    check(cases.size() == 300, "All original pool startup cases");
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
      halo2::SourceRawDatumPoolServices services;
      services.memory = [&](std::uint32_t p, std::size_t n) {
        return c.memory.mutable_view(p, n);
      };
      auto observe_pool = [&](const halo2::SourceRawDatumPoolStore &e) {
        Store event{e.instruction, e.address, {}, {}};
        for (unsigned i = 0; i < e.bytes; ++i) {
          event.before.push_back(std::byte(e.before >> (8 * i)));
          event.written.push_back(std::byte(e.after >> (8 * i)));
        }
        observe(event);
      };
      switch (c.owner) {
      case 0x16d0d0:
        halo2::source_raw_datum_header(c.datum, c.name_address, c.capacity,
                                       c.stride, c.alignment, c.context,
                                       c.bitmap, services, observe_pool);
        break;
      case 0x16d070:
        halo2::source_raw_datum_initialize_inplace(
            c.datum, c.name_address, c.capacity, c.stride, c.alignment,
            c.context, services, observe_pool);
        break;
      case 0x16d130:
        halo2::source_raw_datum_bind_existing(c.datum, c.elements, c.capacity,
                                              services, observe_pool);
        break;
      case 0x16d210:
        halo2::source_raw_datum_make_valid(c.datum, services, observe_pool);
        break;
      case 0x16d220:
        halo2::source_raw_datum_reset(c.datum, services, observe_pool);
        break;
      default:
        throw std::runtime_error("Original pool startup owner");
      }
      check(wi == c.stores.size(), c.name + " missing retained stores");
      stores += wi;
      for (auto &[p, raw] : c.after) {
        std::vector<std::byte> actual(raw.size());
        c.memory.read(p, actual);
        check(actual == raw, c.name + " retained output");
      }
    }
    std::cout
        << cases.size() << " original raw pool startup cases, " << stores
        << " retained stores, payload, bitmap and name-derived salts match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
