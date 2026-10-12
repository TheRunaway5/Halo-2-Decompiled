#include "halo2/multiplayer_netgame_postprocess.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <limits>
#if defined(__SSE__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
// Separate scalar instructions retain the original rounding boundaries and
// prevent a compiler from contracting a multiply and accumulation into FMA.
float add(float a, float b) {
#if defined(__SSE__) || defined(_M_X64)
  return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a), _mm_set_ss(b)));
#else
  volatile float result = a + b;
  return result;
#endif
}
float subtract(float a, float b) {
#if defined(__SSE__) || defined(_M_X64)
  return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a), _mm_set_ss(b)));
#else
  volatile float result = a - b;
  return result;
#endif
}
float multiply(float a, float b) {
#if defined(__SSE__) || defined(_M_X64)
  return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a), _mm_set_ss(b)));
#else
  volatile float result = a * b;
  return result;
#endif
}
float value(std::uint32_t bits) { return std::bit_cast<float>(bits); }

class Postprocess {
public:
  Postprocess(MultiplayerNetgamePostprocessMemory memory,
              const MultiplayerNetgamePostprocessConstants& constants,
              MultiplayerNetgamePostprocessImage image,
              MultiplayerNetgamePostprocessTrace* trace)
      : memory_(memory), constants_(constants), image_(image), trace_(trace) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
  }
  MultiplayerNetgamePostprocessResult run() {
    if (read16(memory_.scenario_address + 0x10) != 1) return result_;
    result_.multiplayer = true;
    result_.map_id = read32(read32(read32(memory_.scenario_address + 0x39c) + 0x14));
    if (result_.map_id != 100 &&
        !(image_ == MultiplayerNetgamePostprocessImage::TitleUpdate15 && result_.map_id == 1200))
      return result_;
    result_.recognized_map = true;
    const auto byte_count = read32(memory_.scenario_address + 0x118) << 5;
    if (result_.map_id == 100) {
      protection(byte_count, 4, later() ? 0x11f09d : 0x11df38);
      beaver_creek();
    } else {
      if (!constants_.relic)
        throw FormatError("selected netgame postprocessor lacks original Relic constants");
      protection(byte_count, 4, 0x11f1bf);
      relic();
    }
    protection(byte_count, 2, later() ? 0x11f2d6 : 0x11e039);
    return result_;
  }

private:
  MultiplayerNetgamePostprocessMemory memory_;
  const MultiplayerNetgamePostprocessConstants& constants_;
  MultiplayerNetgamePostprocessImage image_;
  MultiplayerNetgamePostprocessTrace* trace_;
  MultiplayerNetgamePostprocessResult result_;

  bool later() const { return image_ == MultiplayerNetgamePostprocessImage::TitleUpdate15; }
  std::size_t offset(std::uint32_t address, std::size_t size) const {
    return TagAddressSpace(Bytes(memory_.bytes), memory_.metadata_magic).offset(address, size);
  }
  std::uint16_t read16(std::uint32_t address) const {
    const auto at = offset(address, 2);
    return std::uint16_t(std::to_integer<unsigned>(memory_.bytes[at]) |
        (std::to_integer<unsigned>(memory_.bytes[at + 1]) << 8));
  }
  std::uint32_t read32(std::uint32_t address) const {
    const auto at = offset(address, 4);
    std::uint32_t bits = 0;
    for (unsigned i = 0; i < 4; ++i)
      bits |= std::to_integer<std::uint32_t>(memory_.bytes[at + i]) << (8 * i);
    return bits;
  }
  void store(std::uint32_t address, std::uint32_t bits, std::uint32_t instruction) {
    const auto at = offset(address, 4);
    if (trace_) trace_->stores.push_back({instruction, at, read32(address), bits});
    for (unsigned i = 0; i < 4; ++i)
      memory_.bytes[at + i] = std::byte((bits >> (i * 8)) & 255);
    ++result_.stores;
  }
  void protection(std::uint32_t byte_count, std::uint32_t protection,
                  std::uint32_t instruction) {
    const auto address = read32(memory_.scenario_address + 0x11c);
    if (trace_) trace_->protection_calls.push_back(
        {instruction, address, byte_count, protection, result_.stores});
  }
  std::uint32_t row(unsigned index) const {
    // Source selects authored indices irrespective of the block count. The
    // count only supplies the wrapped DWORD length to the access wrapper.
    return read32(memory_.scenario_address + 0x11c) + index * 0x20;
  }
  bool nearby(std::uint32_t at, const std::array<std::uint32_t, 3>& target,
              bool z_first, bool first_beaver_row = false) const {
    float distance;
    if (z_first) {
      const auto z = subtract(value(target[2]), value(read32(at + 8)));
      const auto y = subtract(value(target[1]), value(read32(at + 4)));
      const auto x = subtract(value(target[0]), value(read32(at)));
      const auto zz = multiply(z, z), yy = multiply(y, y);
      const auto zy = add(zz, yy), xx = multiply(x, x);
      distance = add(zy, xx);
    } else if (later()) {
      const auto y = subtract(value(target[1]), value(read32(at + 4)));
      const auto x = subtract(value(target[0]), value(read32(at)));
      float z, yy, xx;
      if (first_beaver_row) {
        yy = multiply(y, y);
        xx = multiply(x, x);
        z = subtract(value(target[2]), value(read32(at + 8)));
      } else {
        z = subtract(value(target[2]), value(read32(at + 8)));
        yy = multiply(y, y);
        xx = multiply(x, x);
      }
      const auto yx = add(yy, xx), zz = multiply(z, z);
      distance = add(yx, zz);
    } else {
      const auto x = subtract(value(target[0]), value(read32(at)));
      const auto y = subtract(value(target[1]), value(read32(at + 4)));
      float z{};
      if (!first_beaver_row) z = subtract(value(target[2]), value(read32(at + 8)));
      const auto xx = multiply(x, x), yy = multiply(y, y);
      const auto xy = add(xx, yy);
      if (first_beaver_row) z = subtract(value(target[2]), value(read32(at + 8)));
      const auto zz = multiply(z, z);
      distance = add(xy, zz);
    }
    // COMISS threshold,distance / JBE skips equality and unordered operands.
    return value(constants_.distance_squared_threshold) > distance;
  }
  void beaver_creek() {
    auto at = row(5);
    if (read16(at + 0x10) == 8 && nearby(at, constants_.beaver_creek_targets[0], false, true)) {
      const auto y = read32(at + 4), x = read32(at);
      store(at + 4, y, later() ? 0x11f11a : 0x11dfb5);
      const auto z = read32(at + 8);
      store(at, std::bit_cast<std::uint32_t>(add(value(x), value(constants_.beaver_creek_x_delta))),
            later() ? 0x11f124 : 0x11dfbf);
      store(at + 8, z, later() ? 0x11f128 : 0x11dfc3);
    }
    at = row(6);
    if (read16(at + 0x10) == 8 && nearby(at, constants_.beaver_creek_targets[1], false)) {
      const auto x = read32(at), y = read32(at + 4), z = read32(at + 8);
      store(at, std::bit_cast<std::uint32_t>(subtract(value(x), value(constants_.beaver_creek_x_delta))),
            later() ? 0x11f192 : 0x11e025);
      store(at + 4, y, later() ? 0x11f196 : 0x11e029);
      store(at + 8, z, later() ? 0x11f199 : 0x11e02c);
    }
  }
  void relic() {
    for (unsigned index = 2; index <= 3; ++index) {
      const auto at = row(index);
      if (read16(at + 0x10) != 7 || !nearby(at, constants_.relic->targets[index - 2], true))
        continue;
      store(at, std::bit_cast<std::uint32_t>(add(0.f, value(read32(at)))), 0x11f296);
      store(at + 4, std::bit_cast<std::uint32_t>(add(0.f, value(read32(at + 4)))), 0x11f2ab);
      store(at + 8, std::bit_cast<std::uint32_t>(
          subtract(value(read32(at + 8)), value(constants_.relic->z_delta))), 0x11f2bd);
    }
  }
};
} // namespace

MultiplayerNetgamePostprocessResult apply_multiplayer_netgame_postprocess(
    MultiplayerNetgamePostprocessMemory memory,
    const MultiplayerNetgamePostprocessConstants& constants,
    MultiplayerNetgamePostprocessImage image,
    MultiplayerNetgamePostprocessTrace* trace) {
  return Postprocess(memory, constants, image, trace).run();
}
} // namespace halo2
