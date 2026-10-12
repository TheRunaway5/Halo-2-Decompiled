#include "halo2/source_lift_contact.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
constexpr std::uint32_t base = 0x1000000;
struct Memory : SourcePhysicsContactMemory {
  std::vector<std::byte> data = std::vector<std::byte>(0x10000);
  std::array<std::pair<std::uint32_t, std::uint32_t>, 3> globals;
  mutable unsigned reads{};
  explicit Memory(bool later) {
    globals = {{{later ? 0x52be5cu : 0x526248u, base},
                {later ? 0x4ecbd0u : 0x4e78d0u, base + 0x2000},
                {later ? 0x4f0414u : 0x4eb114u, base + 0x6000}}};
  }
  void read(std::uint32_t p, std::span<std::byte> out) const override {
    ++reads;
    for (const auto &[address, value] : globals)
      if (p == address && out.size() == 4) {
        for (unsigned i = 0; i < 4; ++i)
          out[i] = std::byte((value >> (8 * i)) & 255u);
        return;
      }
    if (p < base || std::uint64_t(p - base) + out.size() > data.size())
      throw std::runtime_error("Unbacked original lift read");
    std::copy_n(data.begin() + (p - base), out.size(), out.begin());
  }
  void write(std::uint32_t p, std::span<const std::byte> in) override {
    if (p < base || std::uint64_t(p - base) + in.size() > data.size())
      throw std::runtime_error("Unbacked original lift write");
    std::copy(in.begin(), in.end(), data.begin() + (p - base));
  }
  void w(std::uint32_t p, std::uint32_t value, unsigned width = 4) {
    for (unsigned i = 0; i < width; ++i)
      data.at(p - base + i) = std::byte((value >> (8 * i)) & 255u);
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
    std::string schema;
    unsigned count;
    file >> schema >> count;
    if (schema != "H2_LIFT_CONTACT_V1" || count < 40000)
      throw std::runtime_error("Invalid original lift admission fixture");
    std::array<Memory, 2> memories{Memory(false), Memory(true)};
    for (auto &m : memories)
      for (const auto [p, v] :
           std::array<std::pair<std::uint32_t, std::uint32_t>, 17>{
               {{base + 0x44, base + 0x1000},
                {base + 0x2044, base + 0x3000},
                {base + 0x1008, 0x12340000},
                {base + 0x10a8, 0x12350001},
                {base + 0x3008, base + 0x4000},
                {base + 0x3014, base + 0x5000},
                {base + 0x4000, 0x99990000},
                {base + 0x6008, base + 0x7000},
                {base + 0x6018, base + 0x8000},
                {base + 0x6028, base + 0x9000},
                {base + 0x7038, 0x99990001},
                {base + 0x8024, 0x99990002},
                {base + 0x9044, base + 0xa000},
                {base + 0x902c, base + 0xb000},
                {base + 0xa008, 0},
                {base + 0x513c, 0xffffffff},
                {base + 0x510a, 0}}})
        m.w(p, v);
    unsigned allowed{};
    for (unsigned n = 0; n < count; ++n) {
      std::uint32_t flags;
      int minimum, maximum, size, kind, control, player, none, result;
      file >> flags >> minimum >> maximum >> size >> kind >> control >>
          player >> none >> result;
      if (!file)
        throw std::runtime_error("Truncated lift admission fixtures");
      for (unsigned bank = 0; bank < 2; ++bank) {
        auto &m = memories[bank];
        m.w(base + 0xb000, flags);
        m.w(base + 0xb004, std::uint8_t(minimum), 1);
        m.w(base + 0xb005, std::uint8_t(maximum), 1);
        m.w(base + 0x10bc, std::uint8_t(size), 1);
        m.w(base + 0x300f, kind, 1);
        m.w(base + 0x510a, control, 1);
        m.w(base + 0x513c, player ? 0x12345678 : 0xffffffff);
        m.reads = 0;
        const auto image = bank ? SourcePhysicsContactImage::TitleUpdate15
                                : SourcePhysicsContactImage::MapPack11;
        if (source_lift_contact_allowed(m, 0, 0xabcd0000,
                                        none ? 0xffffffff : 0xbcde0001,
                                        image) != bool(result)) {
          std::cerr << "case " << n << " bank " << bank << '\n';
          throw std::runtime_error("Original lift admission mismatch");
        }
        if (none && m.reads)
          throw std::runtime_error("NONE counterpart read source banks");
      }
      allowed += result;
    }
    std::string extra;
    if (file >> extra)
      throw std::runtime_error("Trailing lift fixture data");
    std::cout << "PASS " << count * 2 << " original lift admission results; "
              << allowed << " allowed per image\n";
    std::cout << "LIMIT gate controls only; contact membership/forces/body "
                 "activation/world step remain separate\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
