#include "halo2/biped_component_storage.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include "halo2/source_runtime_image.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v, const std::string &message) {
  if (!v) throw std::runtime_error(message);
}
std::uint32_t word(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  check(!(s.size() & 1u), "Original component storage byte extent");
  std::vector<std::byte> b;
  for (unsigned i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(word(s.substr(i, 2))));
  return b;
}
using Store = halo2::SourcePhysicsContactRecordStore;
struct Case {
  unsigned stages;
  std::uint32_t pool, datum;
  std::array<std::uint32_t, 8> kernel;
  std::vector<std::pair<std::uint32_t, std::vector<std::byte>>> initial, after;
  std::vector<Store> stores;
};
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 3, "Supply original component storage fixture and selected XBE");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))
      return 77;
#if !defined(__linux__)
    return 77;
#endif
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string k, t;
      in >> k;
      const auto n = [&] { in >> t; return word(t); };
      if (k == "C") {
        Case c{};
        c.stages = n(); c.pool = n(); c.datum = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original component storage case");
      auto &c = cases.back();
      if (k == "K") {
        for (auto &v : c.kernel) v = n();
      } else if (k == "M" || k == "A") {
        const auto p = n(); in >> t;
        (k == "M" ? c.initial : c.after).emplace_back(p, bytes(t));
      } else if (k == "W") {
        Store s;
        s.instruction = n(); s.address = n();
        in >> t; s.before = bytes(t);
        in >> t; s.written = bytes(t);
        c.stores.push_back(std::move(s));
      } else check(false, "Original component storage fixture record");
    }
    check(cases.size() == 3, "Complete component storage stages");
    std::size_t total = 0;
    for (auto &c : cases) {
      auto memory = halo2::load_source_runtime_image_1_1(argv[2]);
      for (const auto &[p, b] : c.initial) {
        if (p >= 0x810000) continue; // Host allocator owns all raw bank bytes.
        const auto old = memory.view(p, b.size());
        if (!std::equal(old.begin(), old.end(), b.begin())) memory.write(p, b);
      }
      // Same explicit host-token policy as the original experiment. There
      // is no supplied allocation return or copied component/pool record.
      halo2::SourceRuntimeHostStorage host(memory, 0x1000000);
      halo2::SourceStaticAllocationServices platform;
      unsigned resources = 0;
      platform.allocate = [&](auto request) {
        ++resources;
        const std::array actual{request.base, request.zero_bits, request.bytes,
                                request.allocation_type, request.protection};
        check(std::equal(actual.begin(), actual.end(), c.kernel.begin()),
              "Original actual kernel allocation request");
        const auto result = host.allocate(request);
        check(result.status == c.kernel[5] && result.base == c.kernel[6] &&
                  result.bytes == c.kernel[7],
              "Original host raw allocation adaptation");
        for (const auto &[p, b] : c.initial) {
          if (p < 0x810000) continue;
          const auto raw = memory.view(p, b.size());
          check(std::equal(raw.begin(), raw.end(), b.begin()),
                "Actual host raw bytes before source construction");
        }
        return result;
      };
      halo2::SourceRawDatumPoolCreationServices services;
      services.storage.read = [&](auto p, auto n) { return memory.view(p, n); };
      services.storage.memory = [&](auto p, auto n) { return memory.mutable_view(p, n); };
      services.allocate = [&](auto context, auto method, auto size) {
        check(context == 0x480d68 && method == 0x11e150,
              "Authenticated original static allocator dispatch");
        return halo2::source_allocate_static_memory_1_1(size, platform);
      };
      std::size_t wi = 0;
      const auto observe = [&](const halo2::SourceRawDatumPoolStore &s) {
        check(wi < c.stores.size(), "Extra retained component storage write");
        Store actual{s.instruction, s.address, {}, {}};
        for (unsigned i = 0; i < s.bytes; ++i) {
          actual.before.push_back(std::byte(s.before >> (8 * i)));
          actual.written.push_back(std::byte(s.after >> (8 * i)));
        }
        if (actual != c.stores[wi++]) {
          std::ostringstream out;
          out << "Original component storage write " << wi << " actual "
              << std::hex << s.instruction << ':' << s.address;
          throw std::runtime_error(out.str());
        }
      };
      halo2::source_initialize_biped_component_storage_1_1(services, observe);
      check(resources == 1, "One actual component storage allocation");
      const auto global = memory.view(0x526248, 4);
      std::uint32_t pool = 0;
      for (unsigned i = 0; i < 4; ++i)
        pool |= std::to_integer<std::uint32_t>(global[i]) << (8 * i);
      check(pool == c.pool, "Original owned component pool binding");
      if (c.stages >= 2)
        halo2::source_raw_datum_make_valid(pool, services.storage, observe);
      const auto datum = c.stages >= 3
          ? halo2::source_raw_datum_allocate(pool, services.storage, observe)
          : 0xffffffffu;
      check(datum == c.datum && wi == c.stores.size(),
            "Complete original component pool stage");
      for (const auto &[p, b] : c.after) {
        const auto raw = memory.view(p, b.size());
        check(std::equal(raw.begin(), raw.end(), b.begin()),
              "Complete original pool header/payload/bitmap bytes");
      }
      total += wi;
    }
    std::cout << cases.size() << " original component storage stages and "
              << total << " retained stores match with actual host allocation\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
