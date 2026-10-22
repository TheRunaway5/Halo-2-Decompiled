#include "halo2/biped_body_placement.hpp"
#include "halo2/physics_ragdoll_constraints.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return x;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t x,
       unsigned n = 4) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(x >> (8 * i));
  m.write(p, s.written);
  if (o)
    o(s);
}
void copy16(M &m, const O &o, std::uint32_t site, std::uint32_t to,
            std::uint32_t from) {
  std::array<std::byte, 16> bytes{};
  m.read(from, bytes);
  // Xbox stores a MOVAPS as two ordered eight-byte bus writes.
  for (unsigned i = 0; i < 2; ++i) {
    SourcePhysicsContactRecordStore s{site, to + i * 8, {}, {}};
    s.before.resize(8);
    m.read(s.address, s.before);
    s.written.assign(bytes.begin() + i * 8, bytes.begin() + i * 8 + 8);
    m.write(s.address, s.written);
    if (o)
      o(s);
  }
}
template <class F, class... A>
decltype(auto) need(const F &f, const char *s, A... a) {
  if (!f)
    throw std::runtime_error(s);
  return f(a...);
}
__m128 s(std::uint32_t x) { return _mm_set_ss(std::bit_cast<float>(x)); }
std::uint32_t bits(__m128 x) {
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x));
}
__m128 at(const M &m, std::uint32_t p) { return s(r(m, p)); }
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 63u) != 63u || (fp.mxcsr & 0x1f80u) != 0x1f80u)
      throw std::runtime_error(
          "Required original placement FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original placement x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::array<std::uint32_t, 4> distance(const M &m, std::uint32_t p,
                                      std::uint32_t a) {
  float px = std::bit_cast<float>(r(m, p)),
        py = std::bit_cast<float>(r(m, p + 4)),
        pz = std::bit_cast<float>(r(m, p + 8));
  float ax = std::bit_cast<float>(r(m, a)),
        ay = std::bit_cast<float>(r(m, a + 4)),
        az = std::bit_cast<float>(r(m, a + 8));
  float x, y, z, d;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile(
      "flds %4\n\tfsubs %7\n\tfstps %0\n\tflds %5\n\tfsubs %8\n\tfstps "
      "%1\n\tflds %6\n\tfsubs %9\n\tfsts %2\n\tfmuls %2\n\tflds %1\n\tfmuls "
      "%1\n\tfaddp\n\tflds %0\n\tfmuls %0\n\tfaddp\n\tfsqrt\n\tfstps %3"
      : "=m"(x), "=m"(y), "=m"(z), "=m"(d)
      : "m"(px), "m"(py), "m"(pz), "m"(ax), "m"(ay), "m"(az)
      : "memory");
#else
  throw std::runtime_error(
      "Required original placement FSQRT backend unavailable");
#endif
  return {std::bit_cast<std::uint32_t>(x), std::bit_cast<std::uint32_t>(y),
          std::bit_cast<std::uint32_t>(z), std::bit_cast<std::uint32_t>(d)};
}
std::uint32_t normalize(M &m, std::uint32_t p, const O &o) {
  float x = std::bit_cast<float>(r(m, p)),
        y = std::bit_cast<float>(r(m, p + 4)),
        z = std::bit_cast<float>(r(m, p + 8));
  float threshold = std::bit_cast<float>(r(m, 0x46504c)), length;
  unsigned char small;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile(
      "flds %4\n\tflds %3\n\tflds %2\n\tfld %%st(0)\n\tfmul "
      "%%st(1),%%st\n\tfld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp\n\tfld "
      "%%st(3)\n\tfmul %%st(4),%%st\n\tfaddp\n\tfsqrt\n\tfstp %%st(3)\n\tfstp "
      "%%st(0)\n\tfstp %%st(0)\n\tfsts %0\n\tfabs\n\tflds %5\n\tfcomip "
      "%%st(1),%%st\n\tfstp %%st(0)\n\tseta %1"
      : "=m"(length), "=qm"(small)
      : "m"(x), "m"(y), "m"(z), "m"(threshold)
      : "cc", "memory");
#else
  throw std::runtime_error(
      "Required original placement normalization x87 backend unavailable");
#endif
  if (small)
    return 0;
  const auto inverse = _mm_div_ss(at(m, 0x465030), _mm_set_ss(length));
  const auto nx = _mm_mul_ss(at(m, p), inverse);
  w(m, o, 0x30c1b, p, bits(nx));
  const auto ny = _mm_mul_ss(inverse, at(m, p + 4)),
             nz = _mm_mul_ss(inverse, at(m, p + 8));
  w(m, o, 0x30c2c, p + 4, bits(ny));
  w(m, o, 0x30c31, p + 8, bits(nz));
  return std::bit_cast<std::uint32_t>(length);
}
std::array<std::uint32_t, 3> transformed(const M &m, std::uint32_t matrix,
                                         std::uint32_t point, bool scale,
                                         bool reverse) {
  __m128 q[3]{at(m, point), at(m, point + 4), at(m, point + 8)};
  if (scale && _mm_ucomineq_ss(at(m, matrix), at(m, 0x465030)))
    for (auto &v : q)
      v = _mm_mul_ss(at(m, matrix), v);
  std::array<std::uint32_t, 3> out{};
  for (unsigned i = 0; i < 3; ++i) {
    auto x = _mm_mul_ss(at(m, matrix + 4 + i * 4), q[0]);
    auto y = _mm_mul_ss(at(m, matrix + 16 + i * 4), q[1]);
    auto z = _mm_mul_ss(at(m, matrix + 28 + i * 4), q[2]);
    auto value = reverse ? _mm_add_ss(_mm_add_ss(z, y), x)
                         : _mm_add_ss(_mm_add_ss(x, y), z);
    if (scale)
      value = _mm_add_ss(value, at(m, matrix + 40 + i * 4));
    out[i] = bits(value);
  }
  return out;
}
} // namespace

void source_biped_constraint_pivots_1_1(M &m, std::uint32_t record,
                                        std::uint32_t first,
                                        std::uint32_t second,
                                        std::uint32_t packet, const O &o) {
  const auto kind =
      std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(m, record, 2))));
  if (kind < 0 || kind > 4 || kind == 3) {
    const auto zero = r(m, 0x46fc28);
    constexpr std::array sites{0x1d3a32u, 0x1d3a37u, 0x1d3a3du,
                               0x1d3a42u, 0x1d3a47u, 0x1d3a4du};
    for (unsigned i = 0; i < 6; ++i)
      w(m, o, sites[i], (i < 3 ? first : second) + (i % 3) * 4,
        r(m, zero + (i % 3) * 4));
    return;
  }
  if (packet & 15u)
    throw std::invalid_argument(
        "Original1D3970 requires aligned scratch packet");
  const auto joint = r(m, record + 8);
  if (kind == 1 || kind == 2)
    source_initialize_ragdoll_constraint_packet_1_1(
        m, packet,
        kind == 1 ? SourceRagdollConstraintKind::limited_hinge
                  : SourceRagdollConstraintKind::ragdoll,
        o);
  else {
    constexpr std::array base_sites{0x31f834u, 0x31f836u, 0x31f839u};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, base_sites[i], packet + i * 4, 0);
    for (unsigned i = 0; i < (kind == 0 ? 5u : 2u); ++i) {
      const auto site = (kind == 0 ? 0x31a0dbu : 0x31a66bu) + i * 4;
      for (unsigned half = 0; half < 2; ++half) {
        SourcePhysicsContactRecordStore row{
            site, packet + 16 + i * 16 + half * 8, {}, {}};
        row.before.resize(8);
        row.written.resize(8);
        m.read(row.address, row.before);
        m.write(row.address, row.written);
        if (o)
          o(row);
      }
    }
  }
  constexpr std::array base_sites{0x321f17u, 0x321f1cu, 0x321f22u},
      base_offsets{12u, 16u, 24u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, base_sites[i], packet + i * 4, r(m, joint + base_offsets[i]));
  if (kind == 2) {
    constexpr std::array input{0x20u, 0x50u, 0x40u, 0x70u, 0x30u, 0x60u},
        output{0x10u, 0x20u, 0x50u, 0x60u, 0x30u, 0x40u},
        sites{0x319022u, 0x31902au, 0x319032u, 0x31903au, 0x319042u, 0x31904au};
    for (unsigned i = 0; i < input.size(); ++i)
      copy16(m, o, sites[i], packet + output[i], joint + input[i]);
    constexpr std::array scalar_sites{0x319054u, 0x31905du, 0x319066u,
                                      0x31906fu, 0x319078u, 0x319084u};
    for (unsigned i = 0; i < 6; ++i)
      w(m, o, scalar_sites[i], packet + 0x70 + i * 4,
        r(m, joint + 0x80 + i * 4));
  } else if (kind == 1) {
    constexpr std::array input{0x20u, 0x60u, 0x30u, 0x70u, 0x40u, 0x50u, 0x80u},
        sites{0x319a52u, 0x319a5au, 0x319a62u, 0x319a6au,
              0x319a72u, 0x319a7au, 0x319a85u};
    for (unsigned i = 0; i < input.size(); ++i)
      copy16(m, o, sites[i], packet + 16 + i * 16, joint + input[i]);
    constexpr std::array scalar_sites{0x319a8fu, 0x319a9bu, 0x319aa7u};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, scalar_sites[i], packet + 0x80 + i * 4,
        r(m, joint + 0xc8 + i * 4));
  } else if (kind == 0) {
    constexpr std::array input{0x20u, 0x50u, 0x30u, 0x40u, 0x60u},
        output{0x10u, 0x20u, 0x40u, 0x50u, 0x30u},
        sites{0x31a202u, 0x31a20au, 0x31a212u, 0x31a21au, 0x31a222u};
    for (unsigned i = 0; i < input.size(); ++i)
      copy16(m, o, sites[i], packet + output[i], joint + input[i]);
  } else
    for (unsigned i = 0; i < 2; ++i)
      copy16(m, o, 0x31a782u + i * 8, packet + 16 + i * 16,
             joint + 0x20 + i * 16);
  constexpr std::array sites{0x1d3a08u, 0x1d3a0cu, 0x1d3a11u,
                             0x1d3a16u, 0x1d3a1au, 0x1d3a1fu};
  std::array<std::uint32_t, 3> pivot{r(m, packet + 16), r(m, packet + 20),
                                     r(m, packet + 24)};
  for (unsigned i = 0; i < 6; ++i)
    w(m, o, sites[i], (i < 3 ? first : second) + (i % 3) * 4, pivot[i % 3]);
}

void source_biped_motion_transform_1_1(M &m, std::uint32_t motion,
                                       std::uint32_t matrix, FP fp,
                                       const O &o) {
  Scope scope(fp);
  if ((matrix | motion) & 15u)
    throw std::invalid_argument(
        "Original31FFC0 requires aligned motion and transform");
  float clock = std::bit_cast<float>(r(m, motion + 0x6c)),
        increment = std::bit_cast<float>(r(m, 0x418460)), changed;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfadds %2\n\tfstps %0"
               : "=m"(changed)
               : "m"(clock), "m"(increment)
               : "memory");
#else
  throw std::runtime_error("Required original31FFC0 x87 backend unavailable");
#endif
  w(m, o, 0x31ffdb, motion + 0x6c, std::bit_cast<std::uint32_t>(changed));
  constexpr std::array matrix_sites{0x31ffe1u, 0x31ffe8u, 0x31fff0u, 0x31fff8u};
  for (unsigned i = 0; i < 4; ++i)
    copy16(m, o, matrix_sites[i], motion + 0x80 + i * 16, matrix + i * 16);
  float d[3]{std::bit_cast<float>(r(m, matrix)),
             std::bit_cast<float>(r(m, matrix + 0x14)),
             std::bit_cast<float>(r(m, matrix + 0x28))};
  float zero = std::bit_cast<float>(r(m, 0x465048)),
        one = std::bit_cast<float>(r(m, 0x465030)),
        half = std::bit_cast<float>(r(m, 0x46502c));
  unsigned char positive;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %2\n\tfadds %1\n\tfadds %3\n\tflds %4\n\tfcomip "
               "%%st(1),%%st\n\tfstp %%st(0)\n\tsetb %0"
               : "=qm"(positive)
               : "m"(d[0]), "m"(d[1]), "m"(d[2]), "m"(zero)
               : "cc", "memory");
#else
  throw std::runtime_error("Required original2E0860 x87 backend unavailable");
#endif
  std::array<float, 4> quaternion{};
  auto mf = [&](unsigned i) {
    return std::bit_cast<float>(r(m, matrix + i * 4));
  };
  if (positive) {
    float a = mf(6), b = mf(9), c = mf(8), e = mf(2), g = mf(1), h = mf(4);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile(
        "flds %5\n\tfadds %4\n\tfadds %6\n\tfadds %7\n\tfsqrt\n\tflds "
        "%8\n\tfdiv %%st(1),%%st\n\tflds %9\n\tfsubs %10\n\tfmul "
        "%%st(1),%%st\n\tfstps %0\n\tflds %11\n\tfsubs %12\n\tfmul "
        "%%st(1),%%st\n\tfstps %1\n\tflds %13\n\tfsubs %14\n\tfmul "
        "%%st(1),%%st\n\tfstps %2\n\tfstp %%st(0)\n\tfmuls %8\n\tfstps %3"
        : "=m"(quaternion[0]), "=m"(quaternion[1]), "=m"(quaternion[2]),
          "=m"(quaternion[3])
        : "m"(d[0]), "m"(d[1]), "m"(d[2]), "m"(one), "m"(half), "m"(a), "m"(b),
          "m"(c), "m"(e), "m"(g), "m"(h)
        : "memory");
#endif
    constexpr std::array sites{0x2e0897u, 0x2e08a1u, 0x2e08acu, 0x2e08b7u};
    for (unsigned i = 0; i < 4; ++i)
      w(m, o, sites[i], motion + 0x30 + i * 4,
        std::bit_cast<std::uint32_t>(quaternion[i]));
  } else {
    unsigned i = d[1] > d[0] ? 1u : 0u;
    if (d[2] > d[i])
      i = 2;
    const auto j = (i + 1) % 3, k = (j + 1) % 3;
    float a = mf(k + j * 4), b = mf(j + k * 4), c = mf(i + j * 4),
          e = mf(j + i * 4), g = mf(i + k * 4), h = mf(k + i * 4);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile(
        "flds %6\n\tfadds %5\n\tfsubrs %4\n\tfadds %7\n\tfsqrt\n\tflds "
        "%8\n\tfdiv %%st(1),%%st\n\tfxch %%st(1)\n\tfmuls %8\n\tfstps "
        "%0\n\tflds %9\n\tfsubs %10\n\tfmul %%st(1),%%st\n\tfstps %1\n\tflds "
        "%11\n\tfadds %12\n\tfmul %%st(1),%%st\n\tfstps %2\n\tflds "
        "%13\n\tfadds %14\n\tfmul %%st(1),%%st\n\tfstps %3\n\tfstp %%st(0)"
        : "=m"(quaternion[i]), "=m"(quaternion[3]), "=m"(quaternion[j]),
          "=m"(quaternion[k])
        : "m"(d[i]), "m"(d[j]), "m"(d[k]), "m"(one), "m"(half), "m"(a), "m"(b),
          "m"(c), "m"(e), "m"(g), "m"(h)
        : "memory");
#endif
    constexpr std::array sites{0x2e093eu, 0x2e094fu, 0x2e095au, 0x2e096bu};
    const std::array order{i, 3u, j, k};
    for (unsigned n = 0; n < 4; ++n)
      w(m, o, sites[n], motion + 0x30 + order[n] * 4,
        std::bit_cast<std::uint32_t>(quaternion[order[n]]));
  }
  std::array<float, 4> row{};
  auto vector = [&](std::uint32_t p) {
    for (unsigned i = 0; i < 4; ++i)
      row[i] = std::bit_cast<float>(r(m, p + i * 4));
    return _mm_loadu_ps(row.data());
  };
  const auto center = vector(motion + 0x20);
  const auto z =
      _mm_mul_ps(vector(motion + 0xa0), _mm_shuffle_ps(center, center, 0xaa));
  const auto zt = _mm_add_ps(z, vector(motion + 0xb0));
  const auto y =
      _mm_mul_ps(vector(motion + 0x90), _mm_shuffle_ps(center, center, 0x55));
  const auto x =
      _mm_mul_ps(vector(motion + 0x80), _mm_shuffle_ps(center, center, 0));
  const auto com = _mm_add_ps(_mm_add_ps(x, y), zt);
  _mm_storeu_ps(row.data(), com);
  for (unsigned n = 0; n < 2; ++n) {
    SourcePhysicsContactRecordStore store{
        0x2e0be5, motion + 0x70 + n * 8, {}, {}};
    store.before.resize(8);
    m.read(store.address, store.before);
    store.written.resize(8);
    for (unsigned i = 0; i < 8; ++i)
      store.written[i] = std::byte(
          std::bit_cast<std::uint32_t>(row[n * 2 + i / 4]) >> (8 * (i % 4)));
    m.write(store.address, store.written);
    if (o)
      o(store);
  }
  w(m, o, 0x32000f, motion + 0x7c, 0);
}

void source_biped_body_transform40_1_1(
    M &m, std::uint32_t component, std::uint32_t slot, std::uint32_t matrix,
    std::uint32_t entry, FP fp,
    const SourceBipedBodyTransformServices &services, const O &o) {
  Scope scope(fp);
  const auto frame = ((entry - 4) & ~15u) - 0x90;
  if (matrix & 15u)
    throw std::invalid_argument("Original1D46E0 requires aligned transform");
  constexpr std::array sites{0x1d46efu, 0x1d46f8u, 0x1d4701u, 0x1d4716u};
  for (unsigned i = 0; i < 4; ++i)
    copy16(m, o, sites[i], frame + 0x10 + i * 16, matrix + i * 16);
  if (r(m, component + 4) & 0x40u)
    need(services.inverse_correction,
         "Required original2E0980/2E0B60 inverse correction unavailable",
         frame + 0x10, component + 0x30, frame + 0x50);
  auto body = r(m, r(m, component + 0x70) + slot * 0x60 + 0x40);
  if (r(m, body + 0x40, 1) && (r(m, component + 4) & 0x20u)) {
    need(services.remove,
         "Required original1D4C50 component removal unavailable", component);
    body = r(m, r(m, component + 0x70) + slot * 0x60 + 0x40);
    need(services.static_transform,
         "Required original1DC870 static body transform unavailable", body,
         frame + 0x10);
    need(services.insert,
         "Required original1D4F30 component insertion unavailable", component);
    return;
  }
  const auto island = r(m, body + 0x44);
  const auto active = island ? r(m, island + 0x29, 1) : 0;
  w(m, o, island ? 0x3121b8 : 0x3121ab, frame + 0xf, active, 1);
  if (!active && island)
    need(services.activate,
         "Required original3121C0 body activation unavailable", body);
  if (!r(m, body + 0x40, 1) || !r(m, body + 8)) {
    const auto motion = r(m, body + 0x3c), method = r(m, r(m, motion) + 0x4c);
    if (method != 0x31ffc0)
      throw std::runtime_error(
          "Required original motion-class transform owner unavailable");
    source_biped_motion_transform_1_1(m, motion, frame + 0x10, fp, o);
  }
}
void source_biped_body_transform34_1_1(
    M &m, std::uint32_t component, std::uint32_t slot, std::uint32_t matrix,
    std::uint32_t entry, FP fp,
    const SourceBipedBodyTransformServices &services, const O &o) {
  const auto frame = ((entry - 4) & ~15u) - 0x60;
  constexpr std::array input{0x10u, 0x14u, 0x18u, 0x1cu, 0x20u, 4u,
                             0x24u, 8u,    0x28u, 12u,   0x2cu},
      output{0x30u, 0x34u, 0x38u, 0x40u, 0x44u, 0x20u,
             0x48u, 0x24u, 0x10u, 0x28u, 0x14u},
      sites{0x1d4624u, 0x1d462fu, 0x1d463au, 0x1d4645u, 0x1d4650u, 0x1d465bu,
            0x1d4666u, 0x1d4671u, 0x1d467cu, 0x1d4687u, 0x1d4690u};
  for (unsigned i = 0; i < input.size(); ++i)
    w(m, o, sites[i], frame + output[i], r(m, matrix + input[i]));
  const auto z = r(m, matrix + 0x30);
  w(m, o, 0x1d469e, frame + 0x2c, 0);
  w(m, o, 0x1d46a4, frame + 0x3c, 0);
  w(m, o, 0x1d46aa, frame + 0x4c, 0);
  w(m, o, 0x1d46b0, frame + 0x18, z);
  w(m, o, 0x1d46b6, frame + 0x1c, 0);
  copy16(m, o, 0x1d46c5, frame + 0x50, frame + 0x10);
  source_biped_body_transform40_1_1(m, component, slot, frame + 0x20, frame - 4,
                                    fp, services, o);
}

void source_biped_body_velocity_1_1(
    M &m, std::uint32_t component, std::uint32_t slot, std::uint32_t velocity,
    std::uint32_t entry, const SourceBipedBodyTransformServices &services,
    const O &o) {
  const auto frame = ((entry - 4) & ~15u) - 0x20;
  const auto body = r(m, r(m, component + 0x70) + slot * 0x60 + 0x40);
  if (r(m, body + 0x40, 1))
    return;
  constexpr std::array sites{0x1d47e5u, 0x1d47f0u, 0x1d47ffu};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], frame + 0x10 + i * 4, r(m, velocity + i * 4));
  w(m, o, 0x1d480b, frame + 0x1c, 0);
  const auto island = r(m, body + 0x44),
             active = island ? r(m, island + 0x29, 1) : 0;
  w(m, o, island ? 0x3121b8 : 0x3121ab, frame + 0xf, active, 1);
  if (!active && island)
    need(services.activate,
         "Required original3121C0 velocity activation unavailable", body);
  const auto motion = r(m, body + 0x3c), method = r(m, r(m, motion) + 0x50);
  if (method != 0x31fe90)
    throw std::runtime_error(
        "Required original motion-class velocity owner unavailable");
  copy16(m, o, 0x31fe97, motion + 0x40, frame + 0x10);
}
void source_add_biped_body_velocity_1_1(
    M &m, std::uint32_t component, std::uint32_t slot, std::uint32_t delta,
    std::uint32_t entry, FP fp,
    const SourceBipedBodyTransformServices &services, const O &o) {
  Scope scope(fp);
  const auto frame = entry - 0x14;
  const auto body = r(m, r(m, component + 0x70) + slot * 0x60 + 0x40);
  std::array<std::uint32_t, 3> current{};
  if (!r(m, body + 0x40, 1)) {
    const auto motion = r(m, body + 0x3c);
    for (unsigned i = 0; i < 3; ++i)
      current[i] = r(m, motion + 0x40 + i * 4);
  } else {
    const auto zero = r(m, 0x46fc44);
    constexpr std::array sites{0x1d4a37u, 0x1d4a47u, 0x1d4a51u};
    for (unsigned i = 0; i < 3; ++i) {
      current[i] = r(m, zero + i * 4);
      w(m, o, sites[i], frame + 8 + i * 4, current[i]);
    }
  }
  std::array<std::uint32_t, 3> next{};
  for (unsigned i = 0; i < 3; ++i)
    next[i] = bits(_mm_add_ss(at(m, delta + i * 4), s(current[i])));
  constexpr std::array sites{0x1d4a7au, 0x1d4a80u, 0x1d4a86u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], frame + 8 + i * 4, next[i]);
  source_biped_body_velocity_1_1(m, component, slot, frame + 8, frame - 8,
                                 services, o);
}

void source_biped_placement_basis_product_1_1(M &m, std::uint32_t left,
                                              std::uint32_t right,
                                              std::uint32_t output, FP fp,
                                              const O &o) {
  Scope scope(fp);
  std::array<std::uint32_t, 9> a{}, b{};
  for (unsigned i = 0; i < 9; ++i) {
    a[i] = r(m, left + i * 4);
    b[i] = r(m, right + i * 4);
  }
  constexpr std::array sites{0x144676u, 0x14469fu, 0x1446c9u,
                             0x1446f3u, 0x14471eu, 0x144749u,
                             0x144773u, 0x14479eu, 0x1447c9u};
  for (unsigned col = 0; col < 3; ++col)
    for (unsigned row = 0; row < 3; ++row) {
      const auto x = _mm_mul_ss(s(a[row]), s(b[col * 3]));
      const auto y = _mm_mul_ss(s(a[3 + row]), s(b[col * 3 + 1]));
      const auto z = _mm_mul_ss(s(a[6 + row]), s(b[col * 3 + 2]));
      // Retain each original entry's addition order, including the final
      // column's z+y+x. Reassociation changes actual corpse pose words.
      constexpr std::array order{0u, 1u, 1u, 1u, 1u, 1u, 0u, 2u, 2u};
      const auto which = order[col * 3 + row];
      const auto q =
          which == 2
              ? _mm_add_ss(_mm_add_ss(z, y), x)
              : _mm_add_ss(which == 1 ? _mm_add_ss(y, x) : _mm_add_ss(x, y), z);
      w(m, o, sites[col * 3 + row], output + (col * 3 + row) * 4, bits(q));
    }
}
void source_biped_placement_rotation_1_1(M &m, std::uint32_t from,
                                         std::uint32_t to, std::uint32_t output,
                                         FP fp, const O &o) {
  Scope scope(fp);
  auto dot = _mm_add_ss(_mm_add_ss(_mm_mul_ss(at(m, from + 4), at(m, to + 4)),
                                   _mm_mul_ss(at(m, from), at(m, to))),
                        _mm_mul_ss(at(m, to + 8), at(m, from + 8)));
  if (_mm_comigt_ss(at(m, 0x46503c), dot))
    dot = at(m, 0x46503c);
  else if (_mm_comigt_ss(dot, at(m, 0x465030)))
    dot = at(m, 0x465030);
  float cosine = _mm_cvtss_f32(dot), one = std::bit_cast<float>(r(m, 0x465030)),
        half = std::bit_cast<float>(r(m, 0x46502c)), cw, sw;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %2\n\tfadds %3\n\tfmuls %4\n\tfsqrt\n\tfstps %0\n\tflds "
               "%3\n\tfsubs %2\n\tfmuls %4\n\tfsqrt\n\tfstps %1"
               : "=m"(cw), "=m"(sw)
               : "m"(cosine), "m"(one), "m"(half)
               : "memory");
#else
  throw std::runtime_error("Required original142F20 x87 backend unavailable");
#endif
  __m128 q[3];
  auto qw = _mm_set_ss(cw), sine = _mm_set_ss(sw), two = at(m, 0x46507c);
  const auto denominator = _mm_mul_ss(_mm_mul_ss(sine, qw), two);
  if (_mm_ucomineq_ss(denominator, _mm_setzero_ps())) {
    const auto scale = _mm_div_ss(sine, denominator);
    q[0] = _mm_mul_ss(_mm_sub_ss(_mm_mul_ss(at(m, from + 4), at(m, to + 8)),
                                 _mm_mul_ss(at(m, from + 8), at(m, to + 4))),
                      scale);
    q[1] = _mm_mul_ss(_mm_sub_ss(_mm_mul_ss(at(m, to), at(m, from + 8)),
                                 _mm_mul_ss(at(m, to + 8), at(m, from))),
                      scale);
    q[2] = _mm_mul_ss(_mm_sub_ss(_mm_mul_ss(at(m, from), at(m, to + 4)),
                                 _mm_mul_ss(at(m, from + 4), at(m, to))),
                      scale);
  } else if (_mm_comigt_ss(_mm_setzero_ps(), dot)) {
    const auto abs = [](__m128 x) {
      return _mm_and_ps(x, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
    };
    const auto x = abs(at(m, from)), y = abs(at(m, from + 4)),
               z = abs(at(m, from + 8));
    if (_mm_comige_ss(y, x) && _mm_comige_ss(z, x)) {
      q[0] = _mm_setzero_ps();
      q[1] = at(m, from + 8);
      q[2] = _mm_sub_ss(_mm_setzero_ps(), at(m, from + 4));
    } else if (_mm_comige_ss(z, y)) {
      q[0] = _mm_sub_ss(_mm_setzero_ps(), at(m, from + 8));
      q[1] = _mm_setzero_ps();
      q[2] = at(m, from);
    } else {
      q[0] = at(m, from + 4);
      q[1] = _mm_sub_ss(_mm_setzero_ps(), at(m, from));
      q[2] = _mm_setzero_ps();
    }
    qw = _mm_setzero_ps();
  } else {
    const auto identity = r(m, 0x46fc6c);
    for (unsigned i = 0; i < 3; ++i)
      q[i] = at(m, identity + i * 4);
    qw = at(m, identity + 12);
  }
  const auto diagonal =
      _mm_sub_ss(_mm_mul_ss(_mm_mul_ss(qw, qw), two), at(m, 0x465030));
  const auto identity = r(m, 0x46fc70);
  for (unsigned i = 0; i < 13; ++i)
    w(m, o, 0x143099, output + i * 4, r(m, identity + i * 4));
  const auto two_w = _mm_mul_ss(qw, two);
  constexpr std::array col_order{0u, 2u, 1u};
  constexpr std::array sites{0x143149u, 0x14317bu, 0x1431aeu,
                             0x143244u, 0x14326fu, 0x143298u,
                             0x143330u, 0x143353u, 0x143377u};
  for (unsigned n = 0; n < 3; ++n) {
    const auto col = col_order[n], base = output + 4 + col * 12;
    __m128 a[3]{at(m, base), at(m, base + 4), at(m, base + 8)};
    const auto x = _mm_mul_ss(a[0], q[0]), y = _mm_mul_ss(a[1], q[1]),
               z = _mm_mul_ss(a[2], q[2]);
    const auto d = _mm_mul_ss(n == 2 ? _mm_add_ss(_mm_add_ss(z, y), x)
                                     : _mm_add_ss(_mm_add_ss(x, z), y),
                              two);
    const auto cross_x =
        _mm_sub_ss(_mm_mul_ss(a[2], q[1]), _mm_mul_ss(a[1], q[2]));
    const auto cross_y =
        _mm_sub_ss(_mm_mul_ss(a[0], q[2]), _mm_mul_ss(a[2], q[0]));
    const auto cross_z =
        _mm_sub_ss(_mm_mul_ss(a[1], q[0]), _mm_mul_ss(a[0], q[1]));
    const auto rx = _mm_add_ss(
        _mm_add_ss(_mm_mul_ss(cross_x, two_w), _mm_mul_ss(a[0], diagonal)),
        _mm_mul_ss(d, q[0]));
    const auto ry = n == 1 ? _mm_add_ss(_mm_add_ss(_mm_mul_ss(cross_y, two_w),
                                                   _mm_mul_ss(q[1], d)),
                                        _mm_mul_ss(diagonal, a[1]))
                           : _mm_add_ss(_mm_add_ss(_mm_mul_ss(a[1], diagonal),
                                                   _mm_mul_ss(q[1], d)),
                                        _mm_mul_ss(cross_y, two_w));
    const auto rz = n == 1 ? _mm_add_ss(_mm_add_ss(_mm_mul_ss(cross_z, two_w),
                                                   _mm_mul_ss(q[2], d)),
                                        _mm_mul_ss(diagonal, a[2]))
                           : _mm_add_ss(_mm_add_ss(_mm_mul_ss(a[2], diagonal),
                                                   _mm_mul_ss(q[2], d)),
                                        _mm_mul_ss(cross_z, two_w));
    w(m, o, sites[n * 3], base, bits(rx));
    w(m, o, sites[n * 3 + 1], base + 4, bits(ry));
    w(m, o, sites[n * 3 + 2], base + 8, bits(rz));
  }
  const auto close = [&](__m128 error) {
    const auto raw = bits(error);
    if ((raw & 0x7f800000u) == 0x7f800000u)
      return false;
    return _mm_comigt_ss(at(m, 0x4650e0), s(raw & 0x7fffffffu)) != 0;
  };
  const auto unit = [&](std::uint32_t p) {
    const auto x = at(m, p), y = at(m, p + 4), z = at(m, p + 8);
    return close(
        _mm_sub_ss(_mm_add_ss(_mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)),
                              _mm_mul_ss(z, z)),
                   at(m, 0x465030)));
  };
  const auto perpendicular = [&](std::uint32_t a, std::uint32_t b, bool xzy) {
    const auto x = _mm_mul_ss(at(m, a), at(m, b)),
               y = _mm_mul_ss(at(m, a + 4), at(m, b + 4)),
               z = _mm_mul_ss(at(m, b + 8), at(m, a + 8));
    return close(_mm_sub_ss(xzy ? _mm_add_ss(_mm_add_ss(x, z), y)
                                : _mm_add_ss(_mm_add_ss(x, y), z),
                            _mm_setzero_ps()));
  };
  const auto a = output + 4, b = output + 16, c = output + 28;
  if (unit(a) && unit(b) && unit(c) && perpendicular(a, b, true) &&
      perpendicular(b, c, false) && perpendicular(a, c, true))
    return;
  normalize(m, c, o);
  auto cross = [&](std::uint32_t first, std::uint32_t second,
                   std::uint32_t dest, const auto &ss) {
    const auto x = _mm_sub_ss(_mm_mul_ss(at(m, first + 4), at(m, second + 8)),
                              _mm_mul_ss(at(m, first + 8), at(m, second + 4)));
    const auto y = _mm_sub_ss(_mm_mul_ss(at(m, first + 8), at(m, second)),
                              _mm_mul_ss(at(m, first), at(m, second + 8)));
    const auto z = _mm_sub_ss(_mm_mul_ss(at(m, first), at(m, second + 4)),
                              _mm_mul_ss(at(m, first + 4), at(m, second)));
    w(m, o, ss[0], dest, bits(x));
    w(m, o, ss[1], dest + 4, bits(y));
    w(m, o, ss[2], dest + 8, bits(z));
  };
  cross(c, a, b, std::array{0x1433d6u, 0x1433dau, 0x1433dfu});
  normalize(m, b, o);
  cross(b, c, a, std::array{0x14342du, 0x143431u, 0x143436u});
}

std::uint8_t source_place_biped_body_1_1(
    M &m, const SourceBipedPlacementRequest &q, std::uint32_t entry, FP fp,
    const SourceBipedBodyPlacementServices &services, const O &o) {
  Scope scope(fp);
  const auto frame = entry - 0x13c;
  const auto unit =
      r(m, r(m, r(m, 0x4e78d0) + 0x44) + (q.object_datum & 0xffffu) * 12 + 8);
  const auto component =
      r(m, r(m, 0x526248) + 0x44) + (r(m, unit + 0xb4) & 0xffffu) * 0xa0;
  const auto body = r(m, r(m, component + 0x70) + q.slot * 0x60 + 0x40),
             motion = r(m, body + 0x3c);
  constexpr std::array com_sites{0xe32e8u, 0xe32eeu, 0xe32f4u};
  std::array<std::uint32_t, 3> com{r(m, motion + 0x70), r(m, motion + 0x74),
                                   r(m, motion + 0x78)};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, com_sites[i], frame + 0x40 + i * 4, com[i]);
  const auto matrix_scratch = ((frame - 8) & ~15u) - 0x40;
  source_physics_body_matrix_1_1(m, component, q.slot, frame + 0x50,
                                 matrix_scratch, services.correction, o);
  if (!q.inherited_valid &&
      !need(services.point,
            "Required original16A9C0 placement query unavailable",
            frame + 0x40))
    return 0;
  if (q.parent_slot == 0xffffffffu) {
    const auto x = _mm_sub_ss(at(m, frame + 0x40), at(m, frame + 0x78));
    const auto y = _mm_sub_ss(at(m, frame + 0x44), at(m, frame + 0x7c));
    const auto z = _mm_sub_ss(at(m, frame + 0x48), at(m, frame + 0x80));
    const auto margin = _mm_sub_ss(s(q.height_word), at(m, 0x457a64));
    const auto ax = r(m, q.anchor), ay = r(m, q.anchor + 4),
               az = r(m, q.anchor + 8);
    w(m, o, 0xe398b, frame + 0x48, az);
    const auto adjusted_z = _mm_sub_ss(at(m, frame + 0x48), margin);
    w(m, o, 0xe3999, frame + 0x40, ax);
    const auto tx = _mm_sub_ss(at(m, frame + 0x40), x);
    w(m, o, 0xe39a7, frame + 0x44, ay);
    const auto ty = _mm_sub_ss(at(m, frame + 0x44), y);
    w(m, o, 0xe39b1, frame + 0x48, bits(adjusted_z));
    w(m, o, 0xe39b7, frame + 0x78, bits(tx));
    const auto tz = _mm_sub_ss(adjusted_z, z);
    w(m, o, 0xe39cc, frame + 0x80, bits(tz));
    w(m, o, 0xe39d5, frame + 0x7c, bits(ty));
  } else {
    w(m, o, 0xe3344, frame + 0x104, 0xffff, 2);
    source_physics_body_matrix_1_1(m, component, q.parent_slot, frame + 0x94,
                                   matrix_scratch, services.correction, o);
    const auto record = r(m, component + 0x7c) + q.constraint * 12;
    const auto packet = ((frame - 8) & ~15u) - 0x90;
    source_biped_constraint_pivots_1_1(m, record, frame + 0xc8, frame + 0xd4,
                                       packet, o);
    source_biped_constraint_slots_1_1(m, component, q.constraint, frame + 0x4c,
                                      frame + 0x84, o);
    const auto first = r(m, frame + 0x4c);
    const auto parent_pivot =
        first == q.parent_slot ? frame + 0xc8 : frame + 0xd4;
    const auto child_pivot =
        first == q.parent_slot ? frame + 0xd4 : frame + 0xc8;
    const auto parent_world =
        transformed(m, frame + 0x94, parent_pivot, true, true);
    constexpr std::array parent_sites{0xe342bu, 0xe3460u, 0xe34acu};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, parent_sites[i], frame + 0x10 + i * 4, parent_world[i]);
    const auto child_world =
        transformed(m, frame + 0x50, child_pivot, true, true);
    const auto delta = distance(m, frame + 0x10, q.anchor);
    constexpr std::array delta_sites{0xe34fbu, 0xe351cu, 0xe353fu};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, delta_sites[i], frame + 0x28 + i * 4, delta[i]);
    constexpr std::array child_sites{0xe35b5u, 0xe35bbu, 0xe35c1u};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, child_sites[i], frame + 0x34 + i * 4, child_world[i]);
    const auto margin = _mm_sub_ss(s(q.height_word), at(m, 0x457a64));
    w(m, o, 0xe35c7, frame + 0x84, bits(margin));
    w(m, o, 0xe35d0, frame + 0x4c, delta[3]);
    constexpr std::array new_sites{0xe35f7u, 0xe35ffu, 0xe3603u};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, new_sites[i], frame + 0x1c + i * 4, r(m, frame + 0x10 + i * 4));
    if (!_mm_comigt_ss(margin, s(delta[3])) &&
        need(services.segment,
             "Required original16B240 placement segment unavailable", q.anchor,
             frame + 0x28, frame + 0xe0)) {
      auto fraction = at(m, frame + 0xe4);
      const auto product = _mm_mul_ss(fraction, s(delta[3]));
      const bool retreat = _mm_comile_ss(margin, product);
      if (retreat)
        fraction =
            _mm_sub_ss(fraction, _mm_div_ss(at(m, 0x457a64), s(delta[3])));
      constexpr std::array straight_sites{0xe3699u, 0xe369fu, 0xe36a5u},
          retreat_sites{0xe36cau, 0xe36dfu, 0xe36f4u};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, (retreat ? retreat_sites : straight_sites)[i],
          frame + 0x1c + i * 4,
          bits(_mm_add_ss(_mm_mul_ss(at(m, frame + 0x28 + i * 4), fraction),
                          at(m, q.anchor + i * 4))));
    }
    std::array<std::uint32_t, 3> new_com{}, separation{};
    for (unsigned i = 0; i < 3; ++i) {
      const auto pivot = at(m, frame + 0x1c + i * 4);
      new_com[i] =
          bits(_mm_add_ss(_mm_sub_ss(pivot, at(m, frame + 0x34 + i * 4)),
                          at(m, frame + 0x40 + i * 4)));
      separation[i] = bits(_mm_sub_ss(s(new_com[i]), pivot));
    }
    w(m, o, 0xe3739, frame + 0x90, new_com[2]);
    w(m, o, 0xe3746, frame + 0x8c, new_com[1]);
    w(m, o, 0xe375a, frame + 0x18, separation[2]);
    w(m, o, 0xe3767, frame + 0x88, new_com[0]);
    w(m, o, 0xe3774, frame + 0x14, separation[1]);
    w(m, o, 0xe3790, frame + 0x10, separation[0]);
    const auto squared =
        _mm_add_ss(_mm_add_ss(_mm_mul_ss(s(separation[2]), s(separation[2])),
                              _mm_mul_ss(s(separation[1]), s(separation[1]))),
                   _mm_mul_ss(s(separation[0]), s(separation[0])));
    if (_mm_comigt_ss(squared, at(m, 0x4653b4)) &&
        need(services.point,
             "Required original16A9C0 placement query unavailable",
             frame + 0x88)) {
      constexpr std::array negate_sites{0xe37deu, 0xe37eeu, 0xe380eu};
      for (unsigned i = 0; i < 2; ++i)
        w(m, o, negate_sites[i], frame + 0x10 + i * 4,
          bits(_mm_mul_ss(at(m, frame + 0x28 + i * 4), at(m, 0x46503c))));
      w(m, o, 0xe37fa, frame + 0x3c, separation[2]);
      w(m, o, 0xe3806, frame + 0x34, separation[0]);
      w(m, o, 0xe380a, frame + 0x38, separation[1]);
      w(m, o, negate_sites[2], frame + 0x18,
        bits(_mm_mul_ss(at(m, frame + 0x30), at(m, 0x46503c))));
      if (_mm_ucomineq_ss(s(normalize(m, frame + 0x10, o)), at(m, 0x465048)) &&
          _mm_ucomineq_ss(s(normalize(m, frame + 0x34, o)), at(m, 0x465048))) {
        source_biped_placement_rotation_1_1(m, frame + 0x34, frame + 0x10,
                                            frame + 0x94, fp, o);
        source_biped_placement_basis_product_1_1(m, frame + 0x98, frame + 0x54,
                                                 frame + 0x54, fp, o);
        normalize(m, frame + 0x54, o);
        normalize(m, frame + 0x60, o);
        normalize(m, frame + 0x6c, o);
      }
    }
    const auto rotated =
        transformed(m, frame + 0x50, child_pivot, false, false);
    w(m, o, 0xe3931, frame + 0x78,
      bits(_mm_sub_ss(at(m, frame + 0x1c), s(rotated[0]))));
    const auto ty = _mm_sub_ss(at(m, frame + 0x20), s(rotated[1])),
               tz = _mm_sub_ss(at(m, frame + 0x24), s(rotated[2]));
    w(m, o, 0xe39cc, frame + 0x80, bits(tz));
    w(m, o, 0xe39d5, frame + 0x7c, bits(ty));
  }
  need(services.set_transform,
       "Required original1D4610 physical pose owner unavailable", component,
       q.slot, frame + 0x50);
  return 1;
}
} // namespace halo2
