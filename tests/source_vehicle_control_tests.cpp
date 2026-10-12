#include "halo2/source_vehicle_control.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>

using namespace halo2;
using Json = nlohmann::json;
namespace {
constexpr std::uint32_t base = 0x1000000;
std::vector<std::byte> decode(std::string_view s) {
  std::vector<std::byte> result;
  for (std::size_t n = 0; n < s.size(); n += 2)
    result.push_back(std::byte(std::stoul(std::string(s.substr(n, 2)), nullptr, 16)));
  return result;
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
  std::vector<std::byte> bytes;
  void read(std::uint32_t p, std::span<std::byte> out) const override {
    if (p < base || std::uint64_t(p - base) + out.size() > bytes.size())
      throw std::runtime_error("Unbacked vehicle control read");
    std::copy_n(bytes.begin() + (p - base), out.size(), out.begin());
  }
  void write(std::uint32_t p, std::span<const std::byte> in) override {
    if (p < base || std::uint64_t(p - base) + in.size() > bytes.size())
      throw std::runtime_error("Unbacked vehicle control write");
    std::copy(in.begin(), in.end(), bytes.begin() + (p - base));
  }
};
std::uint16_t control_word() {
  std::uint16_t result{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fnstcw %0" : "=m"(result));
#endif
  return result;
}
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
      throw std::runtime_error("Wrong vehicle control fixture schema");
    unsigned cases{}, stores{};
    for (const bool later : {false, true}) {
      const auto image = later ? SourcePhysicsContactImage::TitleUpdate15
                               : SourcePhysicsContactImage::MapPack11;
      for (const auto &expected : fixture.at("outputs")) {
        const auto &input = fixture.at("inputs").at(expected.at("input_index").get<unsigned>());
        Memory m;
        m.bytes = decode(input.at("initial_hex").get<std::string>());
        Json actual_stores = Json::array();
        SourcePhysicsContactRecordObserver observer = [&](const auto &event) {
          actual_stores.push_back({{"instruction", event.instruction - (later ? 0x4900u : 0u)},
              {"address", event.address}, {"before", encode(event.before)},
              {"written", encode(event.written)}});
        };
        SourcePhysicsFactoryFloatingPoint fp;
        fp.x87_control_word = expected.at("fp").at("x87_control_word");
        fp.mxcsr = expected.at("fp").at("mxcsr");
        const auto saved_mxcsr = _mm_getcsr();
        const auto saved_control = control_word();
        Json result = nullptr;
        const auto p = input.at("value_address").get<std::uint32_t>();
        const auto rates = input.at("rates_address").get<std::uint32_t>();
        const auto direction = input.at("direction").get<std::uint32_t>();
        const auto dt = input.at("dt").get<std::uint32_t>();
        if (expected.at("kernel") == "accelerate") {
          source_vehicle_control_accelerate(m, p, rates, direction, dt, image, fp, observer);
        } else {
          result = unsigned(source_vehicle_control_approach(m, p, rates, direction, dt,
              input.at("target").get<std::uint32_t>(), image, fp, observer));
        }
        if (saved_mxcsr != _mm_getcsr() || saved_control != control_word())
          throw std::runtime_error("Vehicle control changed caller FP state");
        if (encode(m.bytes) != expected.at("final_hex").get<std::string>() ||
            actual_stores != expected.at("stores") || result != expected.at("result")) {
          std::cerr << "image " << later << " case " << cases << " input "
                    << expected.at("input_index") << " kernel " << expected.at("kernel")
                    << " fp " << expected.at("fp") << "\ninput " << input
                    << "\nactual " << encode(m.bytes) << ' ' << actual_stores << ' ' << result
                    << "\noriginal " << expected << '\n';
          throw std::runtime_error("Original vehicle rate control mismatch");
        }
        ++cases;
        stores += actual_stores.size();
      }
    }
    if (cases < 10000 || stores < 10000)
      throw std::runtime_error("Vehicle control coverage missing");
    Memory m;
    m.bytes = decode(fixture.at("inputs").at(0).at("initial_hex").get<std::string>());
    const auto before = m.bytes;
    bool rejected{};
    try {
      source_vehicle_control_accelerate(m, base + 16, base + 32, 0x3f800000, 0,
          static_cast<SourcePhysicsContactImage>(-1), {});
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    if (!rejected || before != m.bytes)
      throw std::runtime_error("Invalid vehicle image accepted");
    std::cout << "original vehicle control: " << cases << " cases, " << stores
              << " ordered stores; full 64-byte banks and FP restoration match\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
