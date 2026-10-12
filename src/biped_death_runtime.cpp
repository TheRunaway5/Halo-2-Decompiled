#include "halo2/biped_death_runtime.hpp"
#include "halo2/physics_constraint_membership.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
[[noreturn]] void missing(const char *s) { throw std::runtime_error(s); }
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (i * 8);
  return v;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint32_t v) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(v >> (8 * i));
  m.write(p, s.written);
  if (o)
    o(s);
}
std::int32_t si(std::uint32_t x) { return std::bit_cast<std::int32_t>(x); }
std::int32_t sx(const M &m, std::uint32_t p, unsigned n) {
  const auto x = r(m, p, n);
  return n == 1 ? std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x)))
                : std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x)));
}
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t u(float v) { return std::bit_cast<std::uint32_t>(v); }
__m128 scalar(std::uint32_t v) { return _mm_set_ss(f(v)); }
std::uint32_t word(__m128 v) { return u(_mm_cvtss_f32(v)); }
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12u + 8);
}
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 0xffffu) * 16u + 8);
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP p) : mx(_mm_getcsr()) {
    if ((p.x87_control_word & 0x3f) != 0x3f || (p.mxcsr & 0x1f80) != 0x1f80)
      missing("Required original biped FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(p.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(p.x87_control_word) : "memory");
#else
    missing("Required original biped x87 arithmetic backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::int32_t ticks(const M &m) { return sx(m, r(m, 0x518444) + 2, 2); }
__m128 source_ticks(const M &m) {
  return _mm_cvtsi32_ss(_mm_setzero_ps(), ticks(m));
}
std::int32_t rounded_ticks(__m128 value) {
  const auto input = _mm_cvtss_f32(value);
  std::int32_t result;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfistpl %0" : "=m"(result) : "m"(input) : "memory");
#else
  missing("Required original biped FISTP backend unavailable");
#endif
  return result;
}
std::uint32_t trig(std::uint32_t angle, std::uint32_t scale,
                   std::uint32_t radians, bool cosine) {
  const auto a = f(angle), b = f(scale), c = f(radians);
  float out;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if (cosine)
    asm volatile("flds %1\n\tfmuls %2\n\tfmuls %3\n\tfcos\n\tfstps %0"
                 : "=m"(out)
                 : "m"(a), "m"(b), "m"(c)
                 : "memory");
  else
    asm volatile("flds %1\n\tfmuls %2\n\tfmuls %3\n\tfsin\n\tfstps %0"
                 : "=m"(out)
                 : "m"(a), "m"(b), "m"(c)
                 : "memory");
#else
  missing("Required original biped FSIN/FCOS backend unavailable");
#endif
  return u(out);
}
void requirements(M &m, std::uint32_t constraint, std::uint32_t out,
                  const O &o) {
  if (r(m, r(m, constraint) + 0x28) != 0x319090)
    missing("Required original ragdoll requirement virtual owner unavailable");
  w(m, o, 0x31909a, out, 4, 0);
  w(m, o, 0x3190a0, out + 8, 4, 0x54);
  w(m, o, 0x3190a7, out + 4, 4, 0xf0);
  w(m, o, 0x3190ae, out + 12, 4, 6);
  if (f(r(m, constraint + 0x94)) != f(r(m, 0x465048))) {
    w(m, o, 0x3190c4, out + 8, 4, 0x6c);
    w(m, o, 0x3190cb, out + 4, 4, 0x150);
    w(m, o, 0x3190d2, out + 12, 4, 9);
  }
}
void friction(M &m, std::uint32_t constraint, std::uint32_t value,
              std::uint32_t scratch, const O &o) {
  const auto island = r(m, constraint + 0x14);
  if (!island) {
    w(m, o, 0x319136, constraint + 0x94, 4, value);
    return;
  }
  requirements(m, constraint, scratch, o);
  if (r(m, r(m, island) + 0xc) != 0x31c570)
    missing("Required original island requirement subtraction virtual owner "
            "unavailable");
  w(m, o, 0x31c578, island + 0xc, 4, r(m, island + 0xc) - r(m, scratch + 4));
  w(m, o, 0x31c586, island + 0x10, 4, r(m, island + 0x10) - r(m, scratch + 8));
  w(m, o, 0x31c58e, island + 0x18, 4, r(m, island + 0x18) - r(m, scratch + 12));
  w(m, o, 0x319111, constraint + 0x94, 4, value);
  requirements(m, constraint, scratch, o);
  if (r(m, r(m, island) + 8) != 0x31c520)
    missing("Required original island requirement addition virtual owner "
            "unavailable");
  source_accumulate_physics_constraint_requirements_1_1(m, island, constraint,
                                                        scratch, o);
}
} // namespace
bool source_biped_airborne_restriction_1_1(const M &m, std::uint32_t d, FP fp) {
  const auto p = object(m, d);
  if ((r(m, p + 0xc1, 1) & 1u) || !(r(m, p + 0x348, 1) & 1u))
    return false;
  Scope scope(fp);
  const auto limit =
      rounded_ticks(_mm_mul_ss(source_ticks(m), scalar(r(m, 0x4653b8))));
  if (sx(m, p + 0x399, 1) < limit)
    return false;
  const auto state = r(m, p + 0x3dc, 1);
  return (state == 1 || state == 3) && r(m, p + 0x14) == 0xffffffffu;
}
bool source_biped_contact_ragdoll_request_1_1(const M &m, std::uint32_t p,
                                              FP fp) {
  if (!r(m, p + 0x11, 1) || r(m, p + 0x12, 1))
    return false;
  if (r(m, p + 0xc, 1) && r(m, p + 0xe, 1))
    return true;
  Scope scope(fp);
  const auto limit =
      rounded_ticks(_mm_mul_ss(source_ticks(m), scalar(r(m, 0x4650cc))));
  return si(r(m, p + 0x10, 1)) > limit;
}
std::uint32_t source_biped_constraint_elapsed_1_1(const M &m, std::uint32_t p,
                                                  FP fp) {
  Scope scope(fp);
  const auto last = r(m, p + 0x5c);
  const auto one = scalar(r(m, 0x465030));
  if (last == 0xffffffffu)
    return word(one);
  const auto clock = r(m, 0x518444);
  auto t = _mm_cvtsi32_ss(_mm_setzero_ps(), si(r(m, clock + 8) - last));
  t = _mm_mul_ss(t, scalar(r(m, clock + 4)));
  t = _mm_mul_ss(t, scalar(r(m, 0x451ff8)));
  if (_mm_comigt_ss(_mm_setzero_ps(), t))
    return 0;
  return word(_mm_comigt_ss(t, one) ? one : t);
}
void source_update_biped_ragdoll_constraint_1_1(M &m, std::uint32_t component,
                                                std::uint32_t index,
                                                std::uint32_t scale_word,
                                                std::uint32_t scratch, FP fp,
                                                const O &o) {
  Scope scope(fp);
  const auto retained = r(m, component + 0x7c) + index * 12u;
  const auto phmo = tag(
      m,
      r(m, tag(m, r(m, tag(m, r(m, object(m, r(m, component + 8)))) + 0x38)) +
               0x24));
  auto property = std::uint32_t{0};
  for (std::uint32_t i = 0; si(i) < si(r(m, phmo + 0x30)); ++i) {
    const auto row = r(m, phmo + 0x34) + i * 0x18u;
    for (std::uint32_t j = 0; si(j) < si(r(m, row + 8)); ++j) {
      const auto dep = r(m, row + 0xc) + j * 12u;
      if (r(m, dep, 2) == r(m, retained, 2) &&
          r(m, dep + 2, 2) == r(m, retained + 2, 2)) {
        property = r(m, dep + 8);
        break;
      }
    }
  }
  const auto type = sx(m, retained, 2), which = sx(m, retained + 2, 2);
  const auto p = r(m, retained + 8);
  const auto scale = scalar(scale_word), radians = scalar(r(m, 0x4650c4));
  const auto scaled = [&](__m128 angle) {
    return word(_mm_mul_ss(_mm_mul_ss(angle, scale), radians));
  };
  const auto lower = [&](std::uint32_t a, std::uint32_t b) {
    return _mm_comigt_ss(scalar(a), scalar(b)) ? b : a;
  };
  const auto upper = [&](std::uint32_t a, std::uint32_t b) {
    return _mm_comigt_ss(scalar(a), scalar(b)) ? a : b;
  };
  if (type == 1) {
    const auto row = r(m, phmo + 0xec) + which * 0x84u;
    if (f(property) == 0.f)
      property = r(m, row + 0x78);
    w(m, o, 0x1d71fa, p + 0xd0, 4, word(_mm_mul_ss(scalar(property), scale)));
    const auto a = word(_mm_sub_ss(_mm_setzero_ps(), scalar(r(m, row + 0x7c))));
    const auto b = word(_mm_sub_ss(_mm_setzero_ps(), scalar(r(m, row + 0x80))));
    w(m, o, 0x1d722d, p + 0xc8, 4, scaled(scalar(lower(a, b))));
    w(m, o, 0x1d7255, p + 0xcc, 4, scaled(scalar(upper(a, b))));
  } else if (type == 2) {
    const auto row = r(m, phmo + 0xbc) + which * 0x94u;
    w(m, o, 0x1d70ca, p + 0x8c, 4,
      scaled(scalar(lower(r(m, row + 0x78), r(m, row + 0x7c)))));
    w(m, o, 0x1d70ec, p + 0x90, 4,
      scaled(scalar(upper(r(m, row + 0x78), r(m, row + 0x7c)))));
    w(m, o, 0x1d712d, p + 0x84, 4,
      trig(lower(r(m, row + 0x88), r(m, row + 0x8c)), scale_word,
           r(m, 0x4650c4), false));
    w(m, o, 0x1d7164, p + 0x88, 4,
      trig(upper(r(m, row + 0x88), r(m, row + 0x8c)), scale_word,
           r(m, 0x4650c4), false));
    w(m, o, 0x1d71a5, p + 0x80, 4,
      trig(upper(r(m, row + 0x80), r(m, row + 0x84)), scale_word,
           r(m, 0x4650c4), true));
    if (f(property) == 0.f)
      property = r(m, row + 0x90);
    friction(m, p, word(_mm_mul_ss(scalar(property), scale)), scratch, o);
  }
}
void source_update_biped_death_constraints_1_1(M &m, std::uint32_t d,
                                               std::uint32_t scratch, FP fp,
                                               const O &o) {
  const auto p = object(m, d);
  if (r(m, p + 0x3dc, 1) != 3)
    return;
  const auto datum = r(m, p + 0xb4);
  if (datum == 0xffffffffu)
    return;
  const auto component =
      r(m, r(m, 0x526248) + 0x44) + (datum & 0xffffu) * 0xa0u;
  if (!(r(m, component + 4) & 0x800u))
    return;
  Scope scope(fp);
  const std::array<std::uint32_t, 4> factors{r(m, 0x46507c), r(m, 0x465030),
                                             r(m, 0x465554), r(m, 0x465550)};
  const auto elapsed = source_biped_constraint_elapsed_1_1(m, p + 0x3ec, fp);
  const auto one = scalar(r(m, 0x465030));
  const auto value = _mm_add_ss(
      _mm_mul_ss(_mm_sub_ss(one, scalar(elapsed)), scalar(factors[d & 3u])),
      one);
  for (std::uint32_t i = 0; si(i) < si(r(m, component + 0x80)); ++i)
    source_update_biped_ragdoll_constraint_1_1(m, component, i, word(value),
                                               scratch, fp, o);
}
} // namespace halo2
