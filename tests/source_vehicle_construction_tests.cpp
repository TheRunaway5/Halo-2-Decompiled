#include "halo2/source_vehicle_construction.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
using Json = nlohmann::json;
namespace {
constexpr std::uint32_t base = 0x1000000, body = base + 0x2000;
std::vector<std::byte> decode(std::string_view s) {
  std::vector<std::byte> out;
  for (std::size_t n = 0; n < s.size(); n += 2)
    out.push_back(
        std::byte(std::stoul(std::string(s.substr(n, 2)), nullptr, 16)));
  return out;
}
std::string encode(std::span<const std::byte> bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (const auto b : bytes) {
    const auto v = std::to_integer<unsigned>(b);
    result.push_back(digits[v >> 4]);
    result.push_back(digits[v & 15]);
  }
  return result;
}
struct Memory : SourcePhysicsContactMemory {
  std::vector<std::byte> bytes = std::vector<std::byte>(0x10000);
  bool later{}, capture{};
  Json stores = Json::array();
  void read(std::uint32_t p, std::span<std::byte> output) const override {
    if (p == (later ? 0x4ecbd0u : 0x4e78d0u) ||
        p == (later ? 0x4f0414u : 0x4eb114u)) {
      const auto value =
          p == (later ? 0x4ecbd0u : 0x4e78d0u) ? base : base + 0x3000;
      if (output.size() != 4)
        throw std::runtime_error("Wrong global read width");
      for (unsigned i = 0; i < 4; ++i)
        output[i] = std::byte((value >> (8 * i)) & 255u);
      return;
    }
    if (p < base || std::uint64_t(p - base) + output.size() > bytes.size())
      throw std::runtime_error("Unbacked vehicle read");
    std::copy_n(bytes.begin() + (p - base), output.size(), output.begin());
  }
  void write(std::uint32_t p, std::span<const std::byte> input) override {
    if (p < base || std::uint64_t(p - base) + input.size() > bytes.size())
      throw std::runtime_error("Unbacked vehicle write");
    std::copy(input.begin(), input.end(), bytes.begin() + (p - base));
    if (capture && p >= body && p + input.size() <= body + 0x600) {
      std::uint64_t value{};
      for (unsigned i = 0; i < input.size(); ++i)
        value |= std::to_integer<std::uint64_t>(input[i]) << (8 * i);
      stores.push_back({p - body, input.size(), value});
    }
  }
  void w(std::uint32_t p, std::uint32_t v, unsigned size = 4) {
    std::array<std::byte, 4> b{};
    for (unsigned i = 0; i < size; ++i)
      b[i] = std::byte((v >> (8 * i)) & 255u);
    write(p, std::span(b).first(size));
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
    Json fixture;
    file >> fixture;
    if (fixture.at("schema") != 1)
      throw std::runtime_error("Wrong vehicle fixture schema");
    unsigned cases{}, writes{};
    for (unsigned bank = 0; bank < 2; ++bank)
      for (unsigned n = 0; n < fixture["inputs"].size(); ++n) {
        const auto &row = fixture["inputs"][n];
        Memory m;
        m.later = bank;
        for (const auto [p, v] :
             std::array<std::pair<std::uint32_t, std::uint32_t>, 6>{
                 {{base + 0x44, base + 0x1000},
                  {base + 0x1008, body},
                  {base + 0x3008, base + 0x4000},
                  {base + 0x3018, base + 0x5000},
                  {base + 0x4038, 0xc1230001},
                  {base + 0x5024, row["phmo"].get<std::uint32_t>()}}})
          m.w(p, v);
        const auto input = decode(row["body_hex"].get<std::string>());
        m.write(body, input);
        m.w(base + 0x42ac, row["flags"], 1);
        m.capture = true;
        Json events = Json::array();
        SourceVehicleConstructionServices service;
        service.point = [&](std::uint32_t unit, std::uint32_t output) {
          if (unit != 0xa1230000)
            throw std::runtime_error("Full vehicle datum lost");
          // Declared unparented/componentNONE point-provider control. The
          // original
          // BADE0/BACC0 path is independently executed by the oracle for these
          // rows.
          for (unsigned i = 0; i < 3; ++i) {
            std::array<std::byte, 4> v{};
            m.read(body + 0x64 + i * 4, v);
            m.write(output + i * 4, v);
          }
        };
        service.original_baa80 = [&](std::uint32_t unit) {
          events.push_back({"baa80", unit});
        };
        service.animation_request =
            [&](const std::array<std::uint32_t, 7> &args) {
              auto e = Json::array({"animation"});
              for (auto a : args)
                e.push_back(a);
              events.push_back(e);
              return row["admission"].get<unsigned>() != 0;
            };
        service.reset_animation = [&](std::uint32_t p) {
          events.push_back({"reset", p - body});
        };
        const auto image = bank ? SourcePhysicsContactImage::TitleUpdate15
                                : SourcePhysicsContactImage::MapPack11;
        const auto result =
            source_vehicle_construct(m, 0xa1230000, image, service);
        Json actual = {{"result", unsigned(result)},
                       {"body_hex",
                        encode(std::span(m.bytes).subspan(body - base, 0x600))},
                       {"events", events},
                       {"stores", m.stores}};
        if (actual != fixture["outputs"][n]) {
          std::cerr << "bank " << bank << " case " << n << "\nactual "
                    << actual.dump() << "\noriginal "
                    << fixture["outputs"][n].dump() << '\n';
          throw std::runtime_error(
              "Original vehicle constructor/reset bytes/order mismatch");
        }
        writes += m.stores.size();
        ++cases;
      }
    if (cases < 300 || !writes)
      throw std::runtime_error("Missing vehicle construction cases");
    // A missing required owner must stop after the original preceding stores,
    // rather than fabricate successful vehicle construction. No-phmo needs
    // none.
    for (const auto &row : fixture["inputs"]) {
      if (row["phmo"] == 0xffffffffu)
        continue;
      Memory m;
      for (const auto [p, v] :
           std::array<std::pair<std::uint32_t, std::uint32_t>, 6>{
               {{base + 0x44, base + 0x1000},
                {base + 0x1008, body},
                {base + 0x3008, base + 0x4000},
                {base + 0x3018, base + 0x5000},
                {base + 0x4038, 0xc1230001},
                {base + 0x5024, 0x99880002}}})
        m.w(p, v);
      const auto input = decode(row["body_hex"].get<std::string>());
      m.write(body, input);
      m.capture = true;
      bool rejected{};
      try {
        source_vehicle_construct(m, 0xa1230000,
                                 SourcePhysicsContactImage::MapPack11, {});
      } catch (const std::runtime_error &) {
        rejected = true;
      }
      if (!rejected || m.stores.size() != 42)
        throw std::runtime_error("Missing vehicle point owner did not preserve "
                                 "source stop boundary");
      break;
    }
    std::cout << "PASS " << cases
              << " paired-image vehicle constructor/reset results; " << writes
              << " ordered writes; untouched bytes retained\n";
    std::cout << "LIMIT cold declared dependencies; vehicle "
                 "allocation/seats/controls/body physics/world integration "
                 "remain separate\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
