#include "halo2/physics_body_factory.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
namespace {
void check(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
std::uint32_t number(const std::string &s) {
  return std::stoul(s, nullptr, 16);
}
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size() % 2 == 0, "literal original byte extent");
  std::vector<std::byte> out;
  for (std::size_t i = 0; i < s.size(); i += 2)
    out.push_back(std::byte(number(s.substr(i, 2))));
  return out;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::vector<std::byte>> bank;
  auto at(std::uint32_t p, std::size_t n) const {
    auto i = bank.upper_bound(p);
    check(i != bank.begin(), "real model source storage unavailable");
    --i;
    check(std::uint64_t(p - i->first) + n <= i->second.size(),
          "real model source extent unavailable");
    return i;
  }
  void read(std::uint32_t p, std::span<std::byte> out) const override {
    auto i = at(p, out.size());
    std::copy_n(i->second.begin() + (p - i->first), out.size(), out.begin());
  }
  void write(std::uint32_t p, std::span<const std::byte> in) override {
    auto i = at(p, in.size());
    std::copy(in.begin(), in.end(), bank.at(i->first).begin() + (p - i->first));
  }
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original Spartan shape initializer fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    Memory m;
    decltype(m.bank) expected;
    std::vector<halo2::SourcePhysicsContactRecordStore> stores, observed;
    std::uint32_t datum = 0, clear = 0;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::vector<std::string> v;
      for (std::string x; in >> x;)
        v.push_back(x);
      if (v.empty())
        continue;
      if (v[0] == "C") {
        datum = number(v[1]);
        clear = number(v[2]);
      } else if (v[0] == "M" || v[0] == "A")
        (v[0] == "M" ? m.bank : expected).emplace(number(v[1]), bytes(v[2]));
      else if (v[0] == "W")
        stores.push_back(
            {number(v[1]), number(v[2]), bytes(v[3]), bytes(v[4])});
      else
        throw std::runtime_error("Unknown original model initializer record");
    }
    halo2::source_initialize_physics_model_1_1(
        m, datum, std::uint8_t(clear),
        [&](const auto &s) { observed.push_back(s); });
    if (observed != stores) {
      for (std::size_t i = 0; i < std::min(observed.size(), stores.size()); ++i)
        if (observed[i] != stores[i]) {
          std::cerr << "First store differs " << i << " at " << std::hex
                    << observed[i].instruction << " original "
                    << stores[i].instruction << '\n';
          break;
        }
    }
    check(observed == stores,
          "Original model initializer ordered stores differ");
    check(m.bank == expected, "Original whole resident model differs");
    std::cout << "Original serialized Spartan model and " << stores.size()
              << " retained stores matched\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
