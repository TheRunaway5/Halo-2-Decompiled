#include "halo2/native_tu15_seat_look.hpp"
#include "halo2/bytes.hpp"
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <span>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using FP = SourcePhysicsFactoryFloatingPoint;

std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return x;
}
float f(const M &m, std::uint32_t p) { return std::bit_cast<float>(r(m, p)); }
Vec3 v3(const M &m, std::uint32_t p) { return {f(m, p), f(m, p + 4), f(m, p + 8)}; }

// Single-precision SSE operations under the source MXCSR.
float add(float a, float b) { return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a), _mm_set_ss(b))); }
float sub(float a, float b) { return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a), _mm_set_ss(b))); }
float mul(float a, float b) { return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a), _mm_set_ss(b))); }
float div(float a, float b) { return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a), _mm_set_ss(b))); }
// COMISS a,b: JA (a > b), JB (a < b or unordered), JBE (!(a > b)).
bool above(float a, float b) { return _mm_comigt_ss(_mm_set_ss(a), _mm_set_ss(b)) != 0; }
bool below(float a, float b) { return !_mm_comige_ss(_mm_set_ss(a), _mm_set_ss(b)); }
// UCOMISS x,0 / LAHF / TEST AH,44 / JNP: ordered equality with zero.
bool zero(float x) { return x == 0.f; }

struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error("Native TU1.5 seat look requires the x87 backend");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;
};
// x87 operations at the active control word's precision and rounding.
long double x87_sub(long double a, long double b) {
  volatile long double v = a - b;
  return v;
}
long double x87_add(long double a, long double b) {
  volatile long double v = a + b;
  return v;
}
long double x87_mul(long double a, long double b) {
  volatile long double v = a * b;
  return v;
}
long double x87_sqrt(long double x) {
  long double v;
  asm volatile("fsqrt" : "=t"(v) : "0"(x));
  return v;
}
long double x87_sin(long double x) {
  long double v;
  asm volatile("fsin" : "=t"(v) : "0"(x));
  return v;
}
long double x87_cos(long double x) {
  long double v;
  asm volatile("fcos" : "=t"(v) : "0"(x));
  return v;
}
long double x87_atan2(long double y, long double x) { // FPATAN: atan(ST1/ST0)
  long double v;
  asm volatile("fpatan" : "=t"(v) : "0"(x), "u"(y) : "st(1)");
  return v;
}
float x87_float(long double x) { // FSTP m32
  volatile float v = static_cast<float>(x);
  return v;
}
std::uint16_t x87_control() {
  std::uint16_t cw;
  asm volatile("fnstcw %0" : "=m"(cw));
  return cw;
}
void x87_load_control(std::uint16_t cw) { asm volatile("fldcw %0" ::"m"(cw) : "memory"); }

// CRT acos 32CCD8 for |c| <= 1: 332099 keeps the precision control, rounds
// to nearest with every exception masked; |c| == 1 loads 0 or FLDPI; else
// FPATAN(sqrt((1 + c)(1 - c)), c). 33212F restores the caller's word.
long double crt_acos(float c) {
  const auto caller = x87_control();
  x87_load_control(std::uint16_t((caller & 0x300u) | 0x7fu));
  long double result;
  const auto bits = std::bit_cast<std::uint32_t>(c);
  if ((bits & 0x7fffffffu) == 0x3f800000u) {
    if (bits & 0x80000000u)
      asm volatile("fldpi" : "=t"(result));
    else
      result = 0.0L;
  } else {
    const long double x = c;
    const auto s = x87_sqrt(x87_mul(x87_add(1.0L, x), x87_sub(1.0L, x)));
    result = x87_atan2(s, x);
  }
  x87_load_control(caller);
  return result;
}

float vector_angle(Vec3 a, Vec3 b, float pi) { // 11F7E0 (ECX=a, EDX=b), inside a Scope
  const float bb = add(add(mul(b.x, b.x), mul(b.y, b.y)), mul(b.z, b.z));
  const float aa = add(add(mul(a.x, a.x), mul(a.y, a.y)), mul(a.z, a.z));
  const float product = mul(bb, aa);
  if (zero(product))
    return 0.f;
  const float d = add(add(mul(b.z, a.z), mul(b.y, a.y)), mul(b.x, a.x));
  float c = sub(mul(mul(div(d, product), d), 2.f), 1.f);
  if (above(-1.f, c))
    c = -1.f;
  else if (above(c, 1.f))
    c = 1.f;
  const float half = x87_float(x87_mul(crt_acos(c), 0.5L));
  return above(0.f, d) ? sub(pi, half) : half;
}

std::uint32_t header(const M &m, std::uint32_t object) { return r(m, r(m, 0x4ecbd0u) + 0x44) + (object & 0xffffu) * 12u; }
std::uint32_t data(const M &m, std::uint32_t object) { return r(m, header(m, object) + 8); }
std::uint32_t tag(const M &m, std::uint32_t o) { return r(m, r(m, 0x4f0414u) + (r(m, o) & 0xffffu) * 16u + 8u); }
} // namespace

float native_tu15_vector_angle(Vec3 a, Vec3 b, float pi, FP fp) {
  Scope scope(fp);
  return vector_angle(a, b, pi);
}

float native_tu15_look_yaw_delta(float raw, float previous) {
  return std::remainder(raw - previous, native_tu15_look_two_pi);
}

void native_tu15_seat_look(NativeSeatLookRecord &record, const NativeSeatLookOperands &o, float dy, float dp, FP fp,
                           bool advance) {
  Scope scope(fp);
  constexpr float bound = native_tu15_look_pitch_bound;
  float limit_minimum = -bound, limit_maximum = bound; // 18A30C/18A335
  record.yaw = add(record.yaw, dy);                   // 18A350..18A370
  // Yaw limits around the seat marker (18A3B4..18A503).
  if (o.seated && !(zero(o.yaw_minimum) && zero(o.yaw_maximum))) {
    const float base = x87_float(x87_atan2(o.marker_forward.y, o.marker_forward.x)); // 18A3EF..18A41F
    const float low = add(o.yaw_minimum, base), high = add(o.yaw_maximum, base);
    const auto fold = [&](float v) { // (pi, 2pi) normalization, two independent tests
      if (!below(v, o.pi))
        v = sub(v, o.two_pi);
      if (!below(sub(0.f, o.pi), v))
        v = add(v, o.two_pi);
      return v;
    };
    float range = fold(sub(high, low));
    const float to_high = fold(sub(high, record.yaw));
    const float from_low = fold(sub(record.yaw, low));
    if (above(0.f, range)) // 18A4BB..18A4C4: ADDSS executes before the JA
      range = add(range, native_tu15_look_two_pi);
    const bool inside = (!below(to_high, 0.f) && above(range, to_high)) ||
                        (!below(from_low, 0.f) && above(range, from_low));
    if (!inside) {
      // 18A4DE..18A4FE: FCOMI |to_high| with |from_low|, JBE keeps high.
      const long double near_high = std::fabs(static_cast<long double>(to_high));
      const long double near_low = std::fabs(static_cast<long double>(from_low));
      record.yaw = near_high > near_low ? low : high;
    }
  }
  // 18A51D..18A560: wrap into [0, 2pi].
  if (above(0.f, record.yaw)) {
    float y = record.yaw;
    do
      y = add(y, native_tu15_look_two_pi);
    while (above(0.f, y));
    record.yaw = y;
  }
  if (above(record.yaw, native_tu15_look_two_pi)) {
    float y = record.yaw;
    do
      y = sub(y, native_tu15_look_two_pi);
    while (above(y, native_tu15_look_two_pi));
    record.yaw = y;
  }
  if (o.constraints) { // 18A565
    float level = o.level_pitch;
    if (!(zero(o.pitch_maximum) && zero(o.pitch_minimum))) {
      limit_minimum = o.pitch_minimum;
      limit_maximum = o.pitch_maximum;
      if (o.seated && above(o.up.z, .2f)) { // 18A5D1: 0x46A16C
        // 18A5E2..18A612: horizontal look direction (pitch 0.0 from 0x46A0F0).
        const long double yaw = record.yaw;
        const long double cos_pitch = x87_cos(0.0L);
        const Vec3 look{x87_float(x87_mul(x87_cos(yaw), cos_pitch)), x87_float(x87_mul(x87_sin(yaw), cos_pitch)),
                        x87_float(x87_sin(0.0L))};
        const float tilt = sub(std::bit_cast<float>(0x3fc90fdbu), vector_angle(o.up, look, o.acos_pi)); // 0x41C4CC
        limit_minimum = sub(limit_minimum, tilt);
        limit_maximum = sub(limit_maximum, tilt);
        level = sub(level, tilt);
      }
      // 18A660..18A6A6: clamp into the global bounds.
      if (above(-bound, limit_minimum))
        limit_minimum = -bound;
      else if (above(limit_minimum, bound))
        limit_minimum = bound;
      if (above(-bound, limit_maximum))
        limit_maximum = -bound;
      else if (above(limit_maximum, bound))
        limit_maximum = bound;
    }
    if (advance && (!zero(level) || record.idle)) { // 18A6AC: auto-level
      const float rate = x87_float(x87_mul(std::fabs(x87_sub(record.pitch, level)), std::bit_cast<float>(0x3f22f983u)));
      const float difference = sub(level, record.pitch);
      const long double vx = o.velocity.x, vy = o.velocity.y, vz = o.velocity.z;
      const float speed =
          x87_float(x87_mul(x87_sqrt(x87_add(x87_add(x87_mul(vx, vx), x87_mul(vy, vy)), x87_mul(vz, vz))), o.seconds));
      const float step = zero(level) ? mul(mul(o.level_scale, speed), rate)
                                     : mul(mul(speed, rate), std::bit_cast<float>(0x3da3d70au)); // 0x46A7F0
      float change = difference;
      if (above(sub(0.f, step), difference))
        change = sub(0.f, step);
      else if (above(difference, step))
        change = step;
      record.pitch = add(change, record.pitch);
    }
  }
  if (advance) { // 18A763..18A7CD: soft limits, at most pi/256 per call
    constexpr float rate = 0.012271846644580364f; // 0x46A988 (and -0x46A98C)
    float d = sub(limit_minimum, record.minimum);
    if (above(-rate, d))
      d = -rate;
    else if (above(d, rate))
      d = rate;
    record.minimum = add(record.minimum, d);
    d = sub(limit_maximum, record.maximum);
    float e = d;
    if (above(-rate, d))
      e = -rate;
    else if (above(d, rate))
      e = rate;
    record.maximum = add(e, record.maximum);
  }
  float pitch = add(record.pitch, dp); // 18A7D5..18A81A
  if (above(record.minimum, pitch))
    pitch = record.minimum;
  else if (above(pitch, record.maximum))
    pitch = record.maximum;
  record.pitch = pitch;
}

NativeSeatLookRecord native_tu15_seat_look_record(const M &m, std::uint32_t at) {
  NativeSeatLookRecord record;
  record.yaw = f(m, at + 0x10);
  record.pitch = f(m, at + 0x14);
  record.idle = r(m, at + 0x30, 1) != 0;
  record.minimum = f(m, at + 0x80);
  record.maximum = f(m, at + 0x84);
  return record;
}

NativeSeatLookContext native_tu15_seat_look_context(const M &m, std::uint32_t unit) { // 189FD0
  NativeSeatLookContext c;
  c.object = unit;
  if (unit == 0xffffffffu)
    return c;
  const auto u = data(m, unit);
  const auto seat = std::bit_cast<std::int16_t>(std::uint16_t(r(m, u + 0x1fc, 2)));
  if (seat != -1) {
    const auto parent = r(m, u + 0x14);
    const auto definition = r(m, tag(m, data(m, parent)) + 0x1cc) + std::uint32_t(std::int32_t(seat)) * 0xb0u;
    c.object = parent;
    c.seat = seat;
    c.seat_definition = definition;
    c.constraints = definition + 0x60u;
    c.marker = r(m, definition + 8);
  } else
    c.constraints = tag(m, u) + 0xd4u;
  return c;
}

NativeSeatLookOperands native_tu15_seat_look_operands(const M &m, std::uint32_t unit, std::optional<Vec3> marker) {
  const auto c = native_tu15_seat_look_context(m, unit);
  NativeSeatLookOperands o;
  o.seated = c.seat != -1;
  if (o.seated) {
    o.yaw_minimum = f(m, c.seat_definition + 0x88);
    o.yaw_maximum = f(m, c.seat_definition + 0x8c);
  }
  if (marker)
    o.marker_forward = *marker;
  o.constraints = c.constraints != 0;
  if (o.constraints) {
    o.level_pitch = f(m, c.constraints + 8);
    o.pitch_minimum = f(m, c.constraints + 0xc);
    o.pitch_maximum = f(m, c.constraints + 0x10);
  }
  if (c.object != 0xffffffffu) {
    const auto d = data(m, c.object);
    o.up = v3(m, d + 0x7c);
    o.velocity = v3(m, d + 0x88);
  }
  o.level_scale = f(m, r(m, r(m, 0x4ecc1cu) + 0xf4u) + 0x5cu);
  o.seconds = f(m, r(m, 0x51df54u) + 4);
  o.pi = f(m, 0x5564c8u);
  o.two_pi = f(m, 0x5564c4u);
  o.acos_pi = f(m, 0x556554u);
  return o;
}

NativeSeatLookDefinition native_seat_look_definition(const Cache &cache, std::uint32_t definition, int seat) {
  const auto metadata = cache.load_source_tag_metadata(definition);
  const Bytes b(metadata.bytes_at(metadata.source_address(), metadata.size()));
  NativeSeatLookDefinition d;
  if (seat < 0) {
    d.constraints = true;
    d.level_pitch = b.f32(0xdc);
    d.pitch_minimum = b.f32(0xe0);
    d.pitch_maximum = b.f32(0xe4);
    return d;
  }
  const auto count = b.u32(0x1c8);
  if (unsigned(seat) >= count)
    throw FormatError("Seat look definition is outside the authored seats");
  const Bytes s(metadata.bytes_at(b.u32(0x1cc) + std::uint32_t(seat) * 0xb0u, 0xb0));
  d.constraints = true;
  d.marker = s.u32(8);
  d.level_pitch = s.f32(0x68);
  d.pitch_minimum = s.f32(0x6c);
  d.pitch_maximum = s.f32(0x70);
  d.yaw_minimum = s.f32(0x88);
  d.yaw_maximum = s.f32(0x8c);
  return d;
}
} // namespace halo2
