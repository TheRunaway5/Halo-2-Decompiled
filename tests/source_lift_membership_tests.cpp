#include "halo2/cache.hpp"
#include "halo2/source_lift_membership.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
using nlohmann::json;
namespace {
constexpr std::uint32_t base = 0x1000000, entity = base + 0xc000;
constexpr std::uint32_t descriptor = base + 0xe100, array = base + 0xe200;
struct Memory : SourcePhysicsContactMemory {
  std::vector<std::byte> data = std::vector<std::byte>(0x10000);
  std::map<std::uint32_t, std::uint32_t> globals;
  json stores = json::array();
  bool observing{};
  void read(std::uint32_t p, std::span<std::byte> output) const override {
    if (p < base || std::uint64_t(p - base) + output.size() > data.size()) {
      const auto at = globals.find(p);
      if (at == globals.end() || output.size() != 4)
        throw std::runtime_error("Unbacked original lift membership read");
      for (unsigned n = 0; n < 4; ++n)
        output[n] = std::byte((at->second >> (n * 8)) & 255u);
      return;
    }
    std::copy_n(data.begin() + (p - base), output.size(), output.begin());
  }
  void write(std::uint32_t p, std::span<const std::byte> input) override {
    if (p < base || std::uint64_t(p - base) + input.size() > data.size())
      throw std::runtime_error("Unbacked original lift membership write");
    std::uint32_t value{};
    for (unsigned n = 0; n < input.size(); ++n)
      value |= std::to_integer<std::uint32_t>(input[n]) << (n * 8);
    if (observing)
      stores.push_back(json::array({p, input.size(), value}));
    std::copy(input.begin(), input.end(), data.begin() + (p - base));
  }
  void setup(std::uint32_t p, unsigned size, std::uint32_t value) {
    if (p < base) {
      globals[p] = value;
      return;
    }
    for (unsigned n = 0; n < size; ++n)
      data.at(p - base + n) = std::byte((value >> (n * 8)) & 255u);
  }
};
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      return 77;
    std::ifstream file(argv[1]);
    if (!file)
      return 77;
    const auto fixture = json::parse(file);
    if (fixture.at("schema") != 1 || fixture.at("banks").size() != 2)
      throw std::runtime_error("Invalid lift membership fixture");
    unsigned cases{}, operations{}, writes{};
    for (const auto &bank : fixture.at("banks")) {
      const auto image = bank.at("image") == "u5"
                             ? SourcePhysicsContactImage::TitleUpdate15
                             : SourcePhysicsContactImage::MapPack11;
      for (const auto &row : bank.at("cases")) {
        Memory m;
        for (const auto &s : row.at("setup"))
          m.setup(s[0], s[1], s[2]);
        json events = json::array();
        SourceLiftMembershipServices services;
        services.allocate = [&](std::uint32_t size, std::uint32_t category) {
          events.push_back(
              json::array({"allocate", size, category, descriptor}));
          return descriptor;
        };
        services.grow = [&](std::uint32_t address, std::uint32_t stride) {
          events.push_back(json::array({"grow", address, stride}));
          m.setup(address, 4, array);
          m.setup(address + 8, 4, 8);
        };
        services.release = [&](std::uint32_t address, std::uint32_t size,
                               std::uint32_t category) {
          events.push_back(json::array({"release", address, size, category}));
        };
        for (const auto &op : row.at("operations")) {
          m.stores = json::array();
          events = json::array();
          m.observing = true;
          const SourceLiftMembershipInput input{entity, 0xabcd0000,
                                                base + 0x1000, 0, 0};
          if (op.at("operation") == "add")
            source_lift_contact_add(m, input, image, services);
          else
            source_lift_contact_remove(m, input, image, services);
          if (m.stores != op.at("stores") || events != op.at("events")) {
            std::cerr << "case=" << cases << " op=" << op.at("operation")
                      << " config=" << row.at("configuration") << '\n'
                      << "actual=" << m.stores << '\n'
                      << "expected=" << op.at("stores") << '\n'
                      << "events=" << events << '\n';
            throw std::runtime_error("Original lift membership mismatch");
          }
          writes += m.stores.size();
          ++operations;
        }
        ++cases;
      }
    }
    if (cases < 100 || operations < 500 || writes < 1000)
      throw std::runtime_error("Lift membership coverage lost");
    // Required owners must never quietly manufacture a contact allocation,
    // array, or successful cleanup. Exercise the same raw seam as fixtures.
    const SourceLiftMembershipInput input{entity, 0xabcd0000, base + 0x1000, 0,
                                          0};
    const auto &cold = fixture.at("banks")[0].at("cases")[0];
    for (unsigned missing = 0; missing < 3; ++missing) {
      Memory m;
      for (const auto &s : cold.at("setup"))
        m.setup(s[0], s[1], s[2]);
      SourceLiftMembershipServices services;
      services.allocate = [](std::uint32_t, std::uint32_t) {
        return descriptor;
      };
      services.grow = [&](std::uint32_t address, std::uint32_t) {
        m.setup(address, 4, array);
        m.setup(address + 8, 4, 8);
      };
      if (missing == 0)
        services.allocate = {};
      if (missing == 1)
        services.grow = {};
      bool rejected{};
      try {
        source_lift_contact_add(m, input, SourcePhysicsContactImage::MapPack11,
                                services);
        if (missing == 2)
          source_lift_contact_remove(
              m, input, SourcePhysicsContactImage::MapPack11, services);
      } catch (const FormatError &) {
        rejected = true;
      }
      if (!rejected)
        throw std::runtime_error("Missing lift memory owner accepted");
    }
    std::cout << "PASS " << cases << " original lift membership cases, "
              << operations << " operations, " << writes << " ordered writes\n"
              << "LIMIT explicit contact/storage services; live collision, "
                 "forces and world step remain separate\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
