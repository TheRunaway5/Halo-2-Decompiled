#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_image.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
std::uint32_t word(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  check(!(s.size() & 1u), "Original creation byte extent");
  std::vector<std::byte> b;
  for (unsigned i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(word(s.substr(i, 2))));
  return b;
}
using Store = halo2::SourcePhysicsContactRecordStore;
struct Case {
  std::string name;
  std::uint32_t capacity, name_address, stride, alignment, context, pool, datum;
  std::vector<std::pair<std::uint32_t, std::vector<std::byte>>> initial, after;
  std::vector<Store> stores;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 3, "Supply original creation fixture and selected XBE");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))
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
        Case c;
        in >> c.name;
        c.capacity = n();
        c.name_address = n();
        c.stride = n();
        c.alignment = n();
        c.context = n();
        c.pool = n();
        c.datum = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original creation case");
      auto &c = cases.back();
      if (k == "M" || k == "A") {
        auto p = n();
        in >> t;
        (k == "M" ? c.initial : c.after).emplace_back(p, bytes(t));
      } else if (k == "W") {
        Store s;
        s.instruction = n();
        s.address = n();
        in >> t;
        s.before = bytes(t);
        in >> t;
        s.written = bytes(t);
        c.stores.push_back(std::move(s));
      } else
        check(false, "Original creation record");
    }
    check(cases.size() == 108, "All original creation cases");
    std::size_t total = 0;
    for (auto &c : cases) {
      auto memory = halo2::load_source_runtime_image_1_1(argv[2]);
      for (const auto &[p, b] : c.initial) {
        if (p < 0x810000) {
          const auto old = memory.view(p, b.size());
          if (!std::equal(old.begin(), old.end(), b.begin()))
            memory.write(p, b);
        } else
          memory.mount(p, b, halo2::SourceRuntimeMemory::Access::ReadWrite,
                       "original declared pool allocation backing");
      }
      halo2::SourceRawDatumPoolCreationServices services;
      services.storage.memory = [&](auto p, auto n) {
        return memory.mutable_view(p, n);
      };
      services.storage.read = [&](auto p, auto n) { return memory.view(p, n); };
      std::size_t wi = 0, allocations = 0;
      auto observe = [&](const halo2::SourceRawDatumPoolStore &s) {
        check(wi < c.stores.size(), c.name + " extra original creation store");
        Store actual{s.instruction, s.address, {}, {}};
        for (unsigned i = 0; i < s.bytes; ++i) {
          actual.before.push_back(std::byte(s.before >> (8 * i)));
          actual.written.push_back(std::byte(s.after >> (8 * i)));
        }
        if (actual != c.stores[wi++]) {
          std::ostringstream out;
          out << c.name << " creation store " << wi << " actual " << std::hex
              << s.instruction << ':' << s.address;
          throw std::runtime_error(out.str());
        }
      };
      services.allocate = [&](auto context, auto method,
                              auto size) -> std::uint32_t {
        ++allocations;
        check(context == c.context, c.name + " original allocator context");
        if (method == 0x125e50)
          return halo2::source_allocate_game_arena_1_1(size, services.storage,
                                                       observe);
        // Complete authenticated20DA30 is XOR EAX,EAX; RET4.
        if (method == 0x20da30)
          return 0;
        throw std::runtime_error("Unbound original allocation method");
      };
      const auto pool = halo2::source_raw_datum_create(
          c.capacity, c.name_address, c.stride, c.alignment, c.context,
          services, observe);
      check(pool == c.pool && allocations == 1,
            c.name + " original allocation return");
      std::uint32_t datum = 0xffffffffu;
      if (pool) {
        halo2::source_raw_datum_make_valid(pool, services.storage, observe);
        datum =
            halo2::source_raw_datum_allocate(pool, services.storage, observe);
      }
      check(datum == c.datum && wi == c.stores.size(),
            c.name + " complete original pool creation/first allocation");
      for (const auto &[p, b] : c.after) {
        std::vector<std::byte> actual(b.size());
        memory.read(p, actual);
        check(actual == b,
              c.name + " complete allocation payload/header/CRC bytes");
      }
      total += wi;
    }
    std::cout
        << cases.size()
        << " original pool creation/validity/allocation cases and " << total
        << " retained stores match with authenticated read-only names/tables\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
