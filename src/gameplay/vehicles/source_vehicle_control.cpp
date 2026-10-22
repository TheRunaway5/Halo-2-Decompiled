#include "halo2/source_vehicle_control.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;

struct Scope {
  std::fenv_t environment;
  unsigned mxcsr;
  explicit Scope(FP fp) : mxcsr(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3fu) != 0x3fu ||
        (fp.mxcsr & 0x1f80u) != 0x1f80u)
      throw std::runtime_error("Required vehicle FP exception owner unavailable");
    std::fegetenv(&environment);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" : : "m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error("Required vehicle x87 backend unavailable");
#endif
    _mm_setcsr(fp.mxcsr);
  }
  ~Scope() {
    std::fesetenv(&environment);
    _mm_setcsr(mxcsr);
  }
};
void validate(SourcePhysicsContactImage image) {
  if (image != SourcePhysicsContactImage::MapPack11 &&
      image != SourcePhysicsContactImage::TitleUpdate15)
    throw std::invalid_argument("Unverified vehicle executable image");
}
std::uint32_t read(const M &m, std::uint32_t p) {
  std::array<std::byte, 4> bytes{};
  m.read(p, bytes);
  std::uint32_t word{};
  for (unsigned n = 0; n < 4; ++n)
    word |= std::to_integer<std::uint32_t>(bytes[n]) << (n * 8);
  return word;
}
__m128 scalar(std::uint32_t word) {
  return _mm_set_ss(std::bit_cast<float>(word));
}
__m128 load(const M &m, std::uint32_t p) { return scalar(read(m, p)); }
void store(M &m, const O &o, SourcePhysicsContactImage image,
           std::uint32_t site, std::uint32_t p, __m128 value) {
  if (image == SourcePhysicsContactImage::TitleUpdate15)
    site += 0x4900;
  SourcePhysicsContactRecordStore event{site, p, {}, {}};
  event.before.resize(4);
  event.written.resize(4);
  m.read(p, event.before);
  const auto word = std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));
  for (unsigned n = 0; n < 4; ++n)
    event.written[n] = std::byte(word >> (n * 8));
  m.write(p, event.written);
  if (o)
    o(event);
}
void accelerate(M &m, std::uint32_t p, std::uint32_t rates,
                std::uint32_t direction_word, std::uint32_t dt_word,
                SourcePhysicsContactImage image, const O &o) {
  const auto zero = _mm_setzero_ps(), direction = scalar(direction_word);
  // UCOMISS/LAHF/TEST/JNP returns only for an ordered zero, including -0.
  if (_mm_movemask_ps(_mm_cmpeq_ss(direction, zero)) & 1)
    return;
  const float input = std::bit_cast<float>(direction_word);
  const float dt = std::bit_cast<float>(dt_word);
  const float positive_rate = std::bit_cast<float>(read(m, rates + 8));
  const float negative_rate = std::bit_cast<float>(read(m, rates + 12));
  float acceleration{}, deceleration{};
  // Original x87 multiplication and float-store boundaries. The selected
  // precision and rounding apply before each of the two single-precision stores.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %2\n\tfabs\n\tfld %%st(0)\n\tfmuls %3\n\tfmuls %5\n\t"
               "fstps %0\n\tfmuls %4\n\tfmuls %5\n\tfstps %1"
               : "=m"(acceleration), "=m"(deceleration)
               : "m"(input), "m"(positive_rate), "m"(negative_rate), "m"(dt)
               : "st", "st(1)", "memory");
#endif
  const auto a = _mm_set_ss(acceleration), d = _mm_set_ss(deceleration);
  const auto one = scalar(0x3f800000u);
  if (_mm_comigt_ss(direction, zero)) {
    __m128 result;
    if (_mm_comige_ss(_mm_sub_ss(zero, d), load(m, p)))
      result = _mm_add_ss(load(m, p), d);
    else if (_mm_comige_ss(load(m, p), zero))
      result = _mm_add_ss(load(m, p), a);
    else
      result = _mm_mul_ss(_mm_add_ss(_mm_div_ss(load(m, p), d), one), a);
    store(m, o, image, 0x208dfb, p, result);
    if (_mm_comigt_ss(result, load(m, rates))) {
      store(m, o, image, 0x208e08, p, load(m, rates));
      return;
    }
  } else {
    const auto current = load(m, p);
    if (_mm_comige_ss(current, d))
      store(m, o, image, 0x208e49, p, _mm_sub_ss(current, d));
    else if (_mm_comige_ss(zero, load(m, p)))
      store(m, o, image, 0x208e2d, p, _mm_sub_ss(load(m, p), a));
    else
      store(m, o, image, 0x208e49, p,
            _mm_mul_ss(_mm_sub_ss(_mm_div_ss(load(m, p), d), one), a));
    const auto lower = _mm_sub_ss(zero, load(m, rates + 4));
    if (!_mm_comigt_ss(load(m, p), lower)) {
      store(m, o, image, 0x208e5f, p, lower);
      return;
    }
  }
  store(m, o, image, 0x208e5f, p, load(m, p));
}
} // namespace

void source_vehicle_control_accelerate(M &m, std::uint32_t p,
    std::uint32_t rates, std::uint32_t direction, std::uint32_t dt,
    SourcePhysicsContactImage image, FP fp, const O &o) {
  validate(image);
  Scope scope(fp);
  accelerate(m, p, rates, direction, dt, image, o);
}
bool source_vehicle_control_approach(M &m, std::uint32_t p,
    std::uint32_t rates, std::uint32_t direction, std::uint32_t dt,
    std::uint32_t target, SourcePhysicsContactImage image, FP fp, const O &o) {
  validate(image);
  Scope scope(fp);
  const auto desired = scalar(target);
  if (_mm_comigt_ss(load(m, p), desired)) {
    const auto reverse = _mm_sub_ss(_mm_setzero_ps(), scalar(direction));
    accelerate(m, p, rates,
        std::bit_cast<std::uint32_t>(_mm_cvtss_f32(reverse)), dt, image, o);
    if (!_mm_comige_ss(desired, load(m, p)))
      return false;
  } else if (_mm_comigt_ss(desired, load(m, p))) {
    accelerate(m, p, rates, direction, dt, image, o);
    if (!_mm_comige_ss(load(m, p), desired))
      return false;
  } else {
    return true;
  }
  store(m, o, image, 0x208eb7, p, desired);
  return true;
}
} // namespace halo2
