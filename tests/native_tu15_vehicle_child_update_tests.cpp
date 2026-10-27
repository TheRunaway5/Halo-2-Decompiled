// Native B4C40 (vehicle rider update) and D2B60/D2BD0 (rider iterator)
// against the untouched English TU1.5 instructions. The fixture
// (tools/oracle_vehicle_child_update_tu15.py) executed the original bytes
// over declared synthetic object trees; each case seeds the same words here,
// runs the native kernel, and requires exactly the original's written
// addresses and final bytes (outside its stack). Deterministic; no assets.
#include "halo2/native_tu15_object_motion.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
// Sparse little-endian memory: unseeded bytes read as zero (Unicorn's
// zero-filled mapping); every written address is recorded.
class Memory final : public SourcePhysicsContactMemory {
public:
  std::map<std::uint32_t, std::byte> bytes;
  std::set<std::uint32_t> written;
  void read(std::uint32_t address, std::span<std::byte> output) const override {
    for (std::size_t i = 0; i < output.size(); ++i) {
      const auto found = bytes.find(std::uint32_t(address + i));
      output[i] = found == bytes.end() ? std::byte{} : found->second;
    }
  }
  void write(std::uint32_t address, std::span<const std::byte> input) override {
    for (std::size_t i = 0; i < input.size(); ++i) {
      bytes[std::uint32_t(address + i)] = input[i];
      written.insert(std::uint32_t(address + i));
    }
  }
  std::uint32_t word(std::uint32_t address) const {
    std::array<std::byte, 4> b{};
    read(address, b);
    return std::to_integer<std::uint32_t>(b[0]) | std::to_integer<std::uint32_t>(b[1]) << 8 |
           std::to_integer<std::uint32_t>(b[2]) << 16 | std::to_integer<std::uint32_t>(b[3]) << 24;
  }
};
std::vector<std::byte> unhex(const std::string &text) {
  check(text.size() % 2 == 0, "Odd fixture hex");
  std::vector<std::byte> out(text.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i] = std::byte(std::stoul(text.substr(i * 2, 2), nullptr, 16));
  return out;
}
Vec3 vector(const Memory &m, std::uint32_t at) {
  return {std::bit_cast<float>(m.word(at)), std::bit_cast<float>(m.word(at + 4)), std::bit_cast<float>(m.word(at + 8))};
}
} // namespace

int main(int argc, char **argv) try {
  if (argc != 2)
    throw std::runtime_error("Supply tests/fixtures/tu15_vehicle_child_update.original.json");
  std::ifstream input(argv[1]);
  if (!input)
    return 77;
  const auto fixture = nlohmann::json::parse(input);
  check(fixture.at("xbe_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Fixture is not from English TU1.5");
  check(fixture.at("executed_ranges").at("b4c40") == 78 && fixture.at("executed_ranges").at("d2b60") == 40 &&
            fixture.at("executed_ranges").at("d2bd0") == 152,
        "Fixture no longer executes every B4C40/D2B60/D2BD0 site");
  const SourcePhysicsFactoryFloatingPoint fp{std::uint16_t(fixture.at("x87_control_word").get<unsigned>()),
                                             fixture.at("mxcsr").get<std::uint32_t>()};
  unsigned cases = 0, compared = 0, riders = 0, vehicles = 0, gated = 0;
  for (const auto &row : fixture.at("cases")) {
    Memory memory;
    const auto words = unhex(row.at("words").get<std::string>());
    for (std::size_t i = 0; i + 8 <= words.size(); i += 8) {
      std::uint32_t at = 0;
      for (unsigned k = 0; k < 4; ++k)
        at |= std::to_integer<std::uint32_t>(words[i + k]) << (8 * k);
      memory.write(at, std::span(words).subspan(i + 4, 4));
    }
    memory.written.clear();
    const auto vehicle = row.at("vehicle").get<std::uint32_t>();
    const auto position_at = row.at("position").get<std::uint32_t>(), linear_at = row.at("linear").get<std::uint32_t>();
    const Vec3 position = vector(memory, position_at), linear = vector(memory, linear_at);
    const auto listed = native_tu15_vehicle_riders(memory, vehicle);
    check(memory.written.empty(), "The native rider iterator wrote memory");
    native_tu15_vehicle_child_update(memory, vehicle, position_at ? &position : nullptr,
                                     linear_at ? &linear : nullptr, fp);
    std::map<std::uint32_t, std::byte> expected;
    for (const auto &run : row.at("written")) {
      const auto start = run.at(0).get<std::uint32_t>();
      const auto data = unhex(run.at(1).get<std::string>());
      for (std::size_t i = 0; i < data.size(); ++i)
        expected[std::uint32_t(start + i)] = data[i];
    }
    std::set<std::uint32_t> wanted;
    for (const auto &[at, value] : expected)
      wanted.insert(at);
    if (memory.written != wanted) {
      std::ostringstream why;
      why << "case " << cases << ": native wrote " << memory.written.size() << " bytes, original "
          << wanted.size();
      throw std::runtime_error(why.str());
    }
    for (const auto &[at, value] : expected) {
      const auto actual = memory.bytes.at(at);
      if (actual != value) {
        std::ostringstream why;
        why << "case " << cases << " byte " << std::hex << at << ": native " << std::to_integer<unsigned>(actual)
            << " original " << std::to_integer<unsigned>(value);
        throw std::runtime_error(why.str());
      }
      ++compared;
    }
    // The original flags +134 of exactly the riders the native iterator
    // lists, and writes nothing unless the header type is a vehicle.
    const auto headers = memory.word(memory.word(0x4ecbd0u) + 0x44);
    const bool is_vehicle = (memory.word(headers + (vehicle & 0xffffu) * 12u) >> 24) == 1;
    std::set<std::uint32_t> flagged;
    for (const auto rider : listed)
      flagged.insert(memory.word(headers + (rider & 0xffffu) * 12u + 8u) + 0x134u);
    for (const auto at : flagged)
      check(!is_vehicle || wanted.contains(at), "The original did not flag a listed rider");
    check(is_vehicle || wanted.empty(), "The original wrote for a non-vehicle");
    gated += !is_vehicle;
    vehicles += is_vehicle;
    riders += unsigned(listed.size());
    ++cases;
  }
  std::cout << "native B4C40 matches original TU1.5 execution: cases=" << cases << " compared_bytes=" << compared
            << " listed_riders=" << riders << " vehicle_cases=" << vehicles << " non_vehicle_cases=" << gated << '\n';
  check(cases >= 200 && compared > 5000, "Fixture compared too little");
  return 0;
} catch (const std::exception &error) {
  std::cerr << "native B4C40 fixture failed: " << error.what() << '\n';
  return 1;
}
