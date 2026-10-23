#include "halo2/motion_sensor.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <xmmintrin.h>
#pragma STDC FENV_ACCESS ON
namespace halo2 {
Texture motion_sensor_reference_composite(const MotionSensorRenderPlan &,
                                          const MotionSensorArtwork &, Vec3,
                                          unsigned);
}
using namespace halo2;
using Fn = Texture (*)(const MotionSensorRenderPlan &,
                       const MotionSensorArtwork &, Vec3, unsigned);
constexpr float interval = 1.f / 30.f;
const std::array<Fn, 2> functions{
    motion_sensor_reference_composite,
    static_cast<Fn>(source_motion_sensor_composite)};
const std::array<const char *, 2> names{"reference", "actual"};
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
Texture texture(unsigned width, unsigned height, unsigned seed, bool mips) {
  Texture t;
  t.width = width;
  t.height = height;
  t.format = 11;
  auto w = width, h = height;
  std::size_t offset = 0;
  std::vector<std::byte> bytes;
  unsigned level = 0;
  do {
    const auto size = std::size_t(w) * h * 4;
    bytes.resize(offset + size);
    for (unsigned y = 0; y < h; ++y)
      for (unsigned x = 0; x < w; ++x)
        for (unsigned c = 0; c < 4; ++c)
          bytes[offset + (y * w + x) * 4 + c] = std::byte(
              c == 3
                  ? ((x + y + seed + level) % 7 == 0
                         ? 0
                         : (x * 17 + y * 11 + seed * 31 + level * 53) % 256)
                  : (x * (c + 3) + y * (c + 7) + seed * 19 + level * 47) % 256);
    if (mips)
      t.mips.push_back({w, h, offset, size, 1});
    offset += size + 13;
    w = std::max(1u, w / 2);
    h = std::max(1u, h / 2);
    ++level;
  } while (mips && (w != 1 || h != 1 || t.mips.back().width != 1 ||
                    t.mips.back().height != 1));
  t.pixels = TexturePixels(std::move(bytes));
  return t;
}
MotionSensorArtwork artwork(unsigned n) {
  const std::array<unsigned, 4> w{32, 64, 48, 96}, h{32, 64, 31, 48};
  return {texture(w[n], h[n], 1, n != 0), texture(h[n], w[n], 2, n != 0),
          texture(w[n], h[n], 3, n != 0), texture(h[n], w[n], 4, n != 0)};
}
MotionSensorRenderPlan plan(unsigned n, unsigned tick) {
  MotionSensorRenderPlan p;
  p.visible = n != 0;
  p.pulse = source_motion_sensor_pulse(tick, interval);
  if (n == 1 || n == 0)
    return p;
  if (n == 2)
    p.blips = {{{32, 32}, .5f, 2, {1, .5f, 0}, false}};
  if (n == 3)
    for (unsigned i = 0; i < 6; ++i)
      p.blips.push_back({{float(int(i) * 16 - 8), float(i % 2 ? 64 : 0)},
                         .7f,
                         4,
                         {.3f, 1, .3f},
                         bool(i % 2)});
  if (n == 4)
    p.blips = {{{-100, 20}, 1, 1, {1, 0, 0}, false},
               {{164, 32}, 1, 1, {0, 1, 0}, true},
               {{32, 32}, 1, 0x1p-22f, {0, 0, 1}, false},
               {{32, 32}, 1, -1, {1, 1, 0}, true}};
  if (n == 5)
    for (unsigned i = 0; i < 8; ++i)
      p.blips.push_back(
          {{float(29 + i), float(32 - i / 2)},
           float(i + 1) / 8,
           1 + float(i) / 2,
           {float(i % 3) / 2, float((i + 1) % 3) / 2, float((i + 2) % 3) / 2},
           bool(i % 2)});
  if (n == 6)
    p.blips = {{{.0625f, 31.9375f}, .01f, .74999994f, {.5f, .5f, 1}, true},
               {{63.9375f, 32.0625f}, 1, 1.0000001f, {1, 1, .3f}, false}};
  if (n == 7 || n == 8) {
    MotionSensorHistory hist{};
    for (unsigned i = 0; i < 10; ++i) {
      auto &s = hist.players[0][i];
      s.count = n == 8 ? 16u : 2u;
      for (unsigned j = 0; j < s.count; ++j)
        s.blips[j] = {std::uint8_t((i * 7 + j * 3) % 65),
                      std::uint8_t((i * 11 + j * 5) % 65),
                      std::uint8_t(1 + (i + j) % 8), std::uint8_t((i + j) % 4)};
    }
    p = source_motion_sensor_render_plan(hist, 0, 0, {0, 0}, {0, 0}, tick,
                                         interval);
  }
  return p;
}
struct Env {
  int flags, round, error;
  unsigned mxcsr;
  unsigned short cw, sw;
};
Env environment() {
  Env e{};
  e.flags = std::fetestexcept(FE_ALL_EXCEPT);
  e.round = std::fegetround();
  e.error = errno;
  e.mxcsr = _mm_getcsr();
  asm volatile("fnstcw %0" : "=m"(e.cw));
  asm volatile("fnstsw %0" : "=m"(e.sw));
  return e;
}
bool same(const Env &a, const Env &b) {
  return a.flags == b.flags && a.round == b.round && a.error == b.error &&
         a.mxcsr == b.mxcsr && a.cw == b.cw &&
         (a.sw & 0x383f) == (b.sw & 0x383f);
}
struct Run {
  Texture image;
  Env env;
};
Run run(Fn fn, const MotionSensorRenderPlan &p, const MotionSensorArtwork &a,
        Vec3 color, unsigned size, int rounding, unsigned tag) {
  fenv_t saved;
  std::fegetenv(&saved);
  std::fesetround(rounding);
  std::feclearexcept(FE_ALL_EXCEPT);
  if (tag & 1)
    std::feraiseexcept(FE_DIVBYZERO);
  errno = 17000 + tag % 101;
  auto image = fn(p, a, color, size);
  auto env = environment();
  std::fesetenv(&saved);
  return {std::move(image), env};
}
void image_equal(const Texture &a, const Texture &b) {
  require(a.width == b.width && a.height == b.height && a.format == b.format &&
              a.layers == b.layers && a.depth == b.depth &&
              a.mips.size() == b.mips.size(),
          "image metadata");
  for (std::size_t i = 0; i < a.mips.size(); ++i) {
    const auto &x = a.mips[i];
    const auto &y = b.mips[i];
    require(x.width == y.width && x.height == y.height && x.depth == y.depth &&
                x.offset == y.offset && x.size == y.size,
            "mip metadata");
  }
  require(a.pixels.size() == b.pixels.size() && a.pixels == b.pixels,
          "RGBA bytes");
  require(a.pixels.fingerprint() == b.pixels.fingerprint(), "fingerprint");
}
unsigned cow_checks = 0;
void cow(Texture page) {
  Texture copy = page;
  image_equal(page, copy);
  require(static_cast<const Texture &>(copy).pixels.data() !=
              static_cast<const Texture &>(page).pixels.data(),
          "exposed returned image must deep copy");
  ++cow_checks;
  const auto original = copy.pixels.fingerprint();
  auto *writer = page.pixels.data();
  writer[0] ^= std::byte{1};
  require(copy.pixels.fingerprint() == original &&
              page.pixels.fingerprint() != original,
          "copy before retained writer");
  ++cow_checks;
  Texture later = page;
  const auto laterhash = later.pixels.fingerprint();
  writer[1] ^= std::byte{2};
  require(later.pixels.fingerprint() == laterhash,
          "copy after retained writer");
  ++cow_checks;
  const auto pagehash = page.pixels.fingerprint();
  later.pixels[2] ^= std::byte{4};
  require(page.pixels.fingerprint() == pagehash, "copy mutation isolation");
  ++cow_checks;
}
unsigned cases = 0, comparisons = 0;
void check(const MotionSensorRenderPlan &p, const MotionSensorArtwork &a,
           unsigned size, int rounding = FE_TONEAREST) {
  const Vec3 color{.25f + .125f * (cases % 4), .6f, 1.f};
  auto reference = run(functions[0], p, a, color, size, rounding, cases);
  for (unsigned i = 1; i < functions.size(); ++i) {
    auto candidate = run(functions[i], p, a, color, size, rounding, cases);
    try {
      image_equal(reference.image, candidate.image);
      if (!same(reference.env, candidate.env)) {
        std::cerr << "env reference " << reference.env.flags << ' '
                  << reference.env.round << ' ' << reference.env.error << ' '
                  << reference.env.mxcsr << ' ' << reference.env.cw << ' '
                  << reference.env.sw << " candidate " << candidate.env.flags
                  << ' ' << candidate.env.round << ' ' << candidate.env.error
                  << ' ' << candidate.env.mxcsr << ' ' << candidate.env.cw
                  << ' ' << candidate.env.sw << '\n';
        throw std::runtime_error("FP/control/errno");
      }
      cow(std::move(candidate.image));
    } catch (const std::exception &e) {
      std::cerr << "case=" << cases << " size=" << size
                << " variant=" << names[i] << " error=" << e.what() << '\n';
      throw;
    }
    ++comparisons;
  }
  cow(std::move(reference.image));
  ++cases;
}
long long cpu() {
  timespec t{};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec * 1000000000ll + t.tv_nsec;
}
int main(int argc, char **argv) try {
  for (unsigned a = 0; a < 4; ++a) {
    const auto art = artwork(a);
    for (unsigned n = 64; n <= 512; n += 64)
      for (unsigned p = 0; p < 8; ++p)
        check(plan(p, 5 + p * 7), art, n);
  }
  const auto art = artwork(1);
  for (unsigned tick = 0; tick < 63; ++tick)
    check(plan(7, tick), art, 64);
  for (unsigned a = 0; a < 4; ++a)
    check(plan(8, 32), artwork(a), 64);
  for (int round : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO})
    for (unsigned p : {1u, 3u, 4u, 7u})
      check(plan(p, 23), art, 64, round);
  std::cout << "PASS " << cases
            << " complete RGBA/metadata/fingerprint/FP/errno cases, "
            << cow_checks << " copy isolation checks\n";
  if (argc > 1) {
    const bool require_gain = std::string(argv[1]) == "--require-speedup";
    const auto p = plan(5, 23);
    const auto verified = functions[0](p, art, {.5f, .6f, 1.f}, 128);
    const auto first_nonzero =
        std::find_if(verified.pixels.cbegin(), verified.pixels.cend(),
                     [](std::byte value) { return value != std::byte{}; });
    require(first_nonzero != verified.pixels.cend(),
            "Benchmark reference has no visible output");
    const auto consume_offset =
        std::size_t(first_nonzero - verified.pixels.cbegin());
    std::array<double, 7> reference{}, actual{};
    std::uint64_t checksum = 0;
    for (unsigned pair = 0; pair < 7; ++pair)
      for (unsigned order = 0; order < 2; ++order) {
        const auto i = (order + pair) % 2;
        const auto start = cpu();
        for (unsigned j = 0; j < 4; ++j) {
          auto image = functions[i](p, art, {.5f, .6f, 1.f}, 128);
          checksum += std::to_integer<unsigned char>(
              static_cast<const Texture &>(image).pixels[consume_offset]);
        }
        const double ns = double(cpu() - start) / 4;
        (i ? actual : reference)[pair] = ns;
        std::cout << "pair=" << pair << " variant=" << names[i]
                  << " cpu_ns=" << ns << '\n';
      }
    std::sort(reference.begin(), reference.end());
    std::sort(actual.begin(), actual.end());
    std::cout << "median_reference_cpu_ns=" << reference[3]
              << " median_actual_cpu_ns=" << actual[3]
              << " component_speedup=" << reference[3] / actual[3]
              << " checksum=" << checksum << '\n';
    require(checksum != 0, "Benchmark did not consume raster output");
    if (require_gain)
      require(actual[3] * 1.10 < reference[3],
              "Motion compositor has not met the component 1.10x gate");
  }
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
