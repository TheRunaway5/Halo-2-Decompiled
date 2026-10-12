#include "halo2/physics_ragdoll_constraints.hpp"
#include "halo2/transform.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
using K = SourceRagdollConstraintKind;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
std::int32_t sh(const M &m, std::uint32_t p) {
  return std::bit_cast<std::int16_t>(std::uint16_t(r(m, p, 2)));
}
std::int32_t sb(const M &m, std::uint32_t p) {
  return std::bit_cast<std::int8_t>(std::uint8_t(r(m, p, 1)));
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint64_t v) {
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
float f(const M &m, std::uint32_t p) { return std::bit_cast<float>(r(m, p)); }
__m128 v(const M &m, std::uint32_t p) {
  std::array<std::uint32_t, 4> a{};
  for (unsigned i = 0; i < 4; ++i)
    a[i] = r(m, p + 4 * i);
  return _mm_castsi128_ps(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(a.data())));
}
void vec(M &m, const O &o, std::uint32_t site, std::uint32_t p, __m128 value) {
  std::array<std::uint64_t, 2> a{};
  _mm_storeu_ps(reinterpret_cast<float *>(a.data()), value);
  w(m, o, site, p, 8, a[0]);
  w(m, o, site, p + 8, 8, a[1]);
}
struct FpScope {
  unsigned mx;
  std::fenv_t env;
  explicit FpScope(FP p) : mx(_mm_getcsr()) {
    if ((p.x87_control_word & 63u) != 63u || (p.mxcsr & 0x1f80u) != 0x1f80u)
      throw std::runtime_error(
          "Required original ragdoll unmasked FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(p.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(p.x87_control_word) : "memory");
#endif
  }
  ~FpScope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
__m128 dot(__m128 a, __m128 b) {
  const auto q = _mm_mul_ps(a, b),
             y = _mm_add_ss(_mm_shuffle_ps(q, q, 0x55), q);
  return _mm_add_ss(_mm_shuffle_ps(q, q, 0xaa), y);
}
__m128 normalized(const M &m, __m128 a,
                  const SourcePhysicsReciprocalSqrtEstimate &estimate) {
  const auto squared = dot(a, a);
  if (!estimate)
    throw std::runtime_error(
        "Required Xbox RSQRTSS estimate provider unavailable");
  const auto inverse = _mm_set_ss(std::bit_cast<float>(
      estimate(std::bit_cast<std::uint32_t>(_mm_cvtss_f32(squared)))));
  const auto error =
      _mm_sub_ss(_mm_set_ss(f(m, 0x465044)),
                 _mm_mul_ss(_mm_mul_ss(squared, inverse), inverse));
  const auto scale =
      _mm_mul_ss(_mm_mul_ss(_mm_set_ss(f(m, 0x46502c)), inverse), error);
  return _mm_mul_ps(_mm_shuffle_ps(scale, scale, 0), a);
}
void base(M &m, std::uint32_t p, std::uint32_t packet,
          const SourcePhysicsPairManagerServices &s, const O &o) {
  w(m, o, 0x322193, p + 6, 2, 1);
  w(m, o, 0x32219d, p + 8, 4, 0x41b980);
  w(m, o, 0x3221a8, p, 4, 0x41b9a0);
  w(m, o, 0x3221ae, p + 8, 4, 0x41b990);
  w(m, o, 0x3221b5, p + 0x14, 4, 0);
  w(m, o, 0x3221b8, p + 0xc, 4, 0);
  w(m, o, 0x3221bb, p + 0x10, 4, 0);
  source_bind_physics_pair_constraint_body_1_1(m, p, r(m, packet), false, s, o);
  source_bind_physics_pair_constraint_body_1_1(m, p, r(m, packet + 4), true, s,
                                               o);
  w(m, o, 0x3221de, p + 0x18, 4, r(m, packet + 8));
}
void compose40(M &m, std::uint32_t output, std::uint32_t a, std::uint32_t b,
               const O &o) {
  // 2E09E0 deliberately reads each next column after the preceding store;
  //  this preserves the actual alias behavior of its original callers.
  for (unsigned i = 0; i < 4; ++i) {
    const auto q = v(m, b + i * 16);
    const auto x = _mm_mul_ps(v(m, a), _mm_shuffle_ps(q, q, 0));
    const auto y = _mm_mul_ps(v(m, a + 16), _mm_shuffle_ps(q, q, 0x55));
    const auto z = _mm_mul_ps(v(m, a + 32), _mm_shuffle_ps(q, q, 0xaa));
    vec(m, o, 0x2e0a32, output + i * 16, _mm_add_ps(_mm_add_ps(x, y), z));
  }
  vec(m, o, 0x2e0a48, output + 48, _mm_add_ps(v(m, output + 48), v(m, a + 48)));
}
} // namespace
void source_initialize_ragdoll_constraint_packet_1_1(M &m, std::uint32_t p,
                                                     K kind, const O &o) {
  w(m, o, 0x31f834, p, 4, 0);
  w(m, o, 0x31f836, p + 4, 4, 0);
  w(m, o, 0x31f839, p + 8, 4, 0);
  const bool hinge = kind == K::limited_hinge;
  for (unsigned i = 0; i < (hinge ? 7u : 6u); ++i)
    vec(m, o, (hinge ? 0x3198cbu : 0x318e8bu) + i * 4, p + 16 + i * 16,
        _mm_setzero_ps());
  if (hinge) {
    w(m, o, 0x3198e7, p + 0x80, 4, 0xc0490fdb);
    w(m, o, 0x3198f1, p + 0x84, 4, 0x40490fdb);
    w(m, o, 0x3198fb, p + 0x88, 4, 0);
  } else {
    constexpr std::array values{0x3f800000u, 0xbccccccdu, 0x3ccccccdu,
                                0xbf060aa6u, 0x3f060aa6u, 0u};
    constexpr std::array sites{0x318ea3u, 0x318eaau, 0x318eb1u,
                               0x318eb8u, 0x318ebfu, 0x318ec9u};
    for (unsigned i = 0; i < 6; ++i)
      w(m, o, sites[i], p + 0x70 + 4 * i, 4, values[i]);
  }
}
void source_set_ragdoll_constraint_axes_1_1(
    M &m, const SourceRagdollConstraintAxes &in, K kind, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &estimate, const O &o) {
  FpScope scope(fp);
  const auto p = in.packet;
  const bool hinge = kind == K::limited_hinge;
  w(m, o, hinge ? 0x1d1cec : 0x1d20fc, p, 4, in.body_a);
  w(m, o, hinge ? 0x1d1cf1 : 0x1d2101, p + 4, 4, in.body_b);
  vec(m, o, hinge ? 0x1d1cfd : 0x1d210d, p + 0x10, v(m, in.vectors[0]));
  vec(m, o, hinge ? 0x1d1d07 : 0x1d2114, p + 0x20, v(m, in.vectors[1]));
  vec(m, o, hinge ? 0x1d1d0e : 0x1d211b, p + 0x30, v(m, in.vectors[2]));
  if (hinge)
    vec(m, o, 0x1d1d15, p + 0x40, v(m, in.vectors[3]));
  vec(m, o, hinge ? 0x1d1d84 : 0x1d218d, p + 0x30,
      normalized(m, v(m, p + 0x30), estimate));
  if (!hinge)
    vec(m, o, 0x1d2194, p + 0x40, v(m, in.vectors[3]));
  vec(m, o, hinge ? 0x1d1de0 : 0x1d21f0, p + 0x40,
      normalized(m, v(m, p + 0x40), estimate));
  vec(m, o, hinge ? 0x1d1de7 : 0x1d21f7, p + (hinge ? 0x60 : 0x50),
      v(m, in.vectors[4]));
  vec(m, o, hinge ? 0x1d1e46 : 0x1d2256, p + (hinge ? 0x60 : 0x50),
      normalized(m, v(m, p + (hinge ? 0x60 : 0x50)), estimate));
  vec(m, o, hinge ? 0x1d1e4d : 0x1d225d, p + (hinge ? 0x70 : 0x60),
      v(m, in.vectors[5]));
  vec(m, o, hinge ? 0x1d1ea3 : 0x1d22b3, p + (hinge ? 0x70 : 0x60),
      normalized(m, v(m, p + (hinge ? 0x70 : 0x60)), estimate));
  if (hinge) {
    const auto a = v(m, p + 0x30), b = v(m, p + 0x60);
    const auto lhs =
        _mm_mul_ps(_mm_shuffle_ps(a, a, 0xd2), _mm_shuffle_ps(b, b, 0xc9));
    const auto rhs =
        _mm_mul_ps(_mm_shuffle_ps(b, b, 0xd2), _mm_shuffle_ps(a, a, 0xc9));
    vec(m, o, 0x1d1ed4, p + 0x50, _mm_sub_ps(lhs, rhs));
  }
}
bool source_validate_limited_hinge_1_1(const M &m, std::uint32_t p, FP fp) {
  FpScope scope(fp);
  const auto tolerance = static_cast<long double>(f(m, 0x41891c));
  auto orthogonal = [&](unsigned a, unsigned b) {
    return std::fabs(static_cast<long double>(
               _mm_cvtss_f32(dot(v(m, p + a), v(m, p + b))))) < tolerance;
  };
  if (!orthogonal(0x40, 0x50) || !orthogonal(0x80, 0x70))
    return false;
  for (auto offset : {0x40u, 0x50u, 0x70u, 0x30u}) {
    volatile long double error = static_cast<long double>(_mm_cvtss_f32(
                                     dot(v(m, p + offset), v(m, p + offset)))) -
                                 static_cast<long double>(f(m, 0x465030));
    if (!(std::fabs(error) < tolerance))
      return false;
  }
  const long double low = f(m, p + 0xc8), high = f(m, p + 0xcc),
                    zero = f(m, 0x465048);
  volatile long double span =
      low <= zero && high <= zero ? std::fabs(low) + high : high - low;
  return span <= static_cast<long double>(f(m, 0x418a28));
}
std::uint32_t source_construct_ragdoll_constraint_1_1(
    M &m, std::uint32_t p, std::uint32_t packet, K kind, FP fp,
    const SourcePhysicsPairManagerServices &s, const O &o) {
  base(m, p, packet, s, o);
  const bool hinge = kind == K::limited_hinge;
  w(m, o, hinge ? 0x319b1a : 0x3192c0, p, 4, hinge ? 0x41aea0 : 0x41ad2c);
  w(m, o, hinge ? 0x319b20 : 0x3192c6, p + 8, 4, 0x41ae90);
  constexpr std::array rag_sites{
      0x3192cdu, 0x3192d3u, 0x3192dfu, 0x3192e2u, 0x3192e8u, 0x3192ebu,
      0x3192f1u, 0x3192f4u, 0x3192fau, 0x3192fdu, 0x319306u, 0x319308u,
      0x31930eu, 0x319310u, 0x319316u, 0x319318u, 0x31931eu, 0x319320u};
  constexpr std::array hinge_sites{0x319b27u, 0x319b2du, 0x319b39u, 0x319b3cu,
                                   0x319b42u, 0x319b45u, 0x319b4eu, 0x319b50u,
                                   0x319b56u, 0x319b58u, 0x319b5eu, 0x319b60u,
                                   0x319b6au, 0x319b6cu};
  for (unsigned i = 0; i < (hinge ? 14u : 18u); ++i)
    w(m, o, hinge ? hinge_sites[i] : rag_sites[i],
      p + (hinge ? 0x90 : 0x98) + i * 4, 4, 0);
  if (hinge) {
    w(m, o, 0x3199ca, p + 0xc8, 4, r(m, packet + 0x80));
    w(m, o, 0x3199d6, p + 0xcc, 4, r(m, packet + 0x84));
    w(m, o, 0x3199e2, p + 0xd0, 4, r(m, packet + 0x88));
    constexpr std::array<std::array<std::uint32_t, 3>, 7> moves{
        {{0x3199ec, 0x20, 0x10},
         {0x3199f4, 0x30, 0x30},
         {0x3199fc, 0x40, 0x50},
         {0x319a04, 0x50, 0x60},
         {0x319a0c, 0x60, 0x20},
         {0x319a14, 0x70, 0x40},
         {0x319a23, 0x80, 0x70}}};
    for (const auto &x : moves)
      vec(m, o, x[0], p + x[1], v(m, packet + x[2]));
    w(m, o, 0x319a2a, p + 0xd4, 4, 0);
    if (r(m, r(m, p) + 0x1c) != 0x319b90)
      throw std::runtime_error(
          "Required original limited-hinge validator virtual unavailable");
    (void)source_validate_limited_hinge_1_1(m, p, fp);
  } else {
    constexpr std::array<std::array<std::uint32_t, 3>, 6> moves{
        {{0x318f98, 0x20, 0x10},
         {0x318fa0, 0x50, 0x20},
         {0x318fa8, 0x40, 0x50},
         {0x318fb0, 0x70, 0x60},
         {0x318fb8, 0x30, 0x30},
         {0x318fc0, 0x60, 0x40}}};
    for (const auto &x : moves)
      vec(m, o, x[0], p + x[1], v(m, packet + x[2]));
    constexpr std::array sites{0x318fc7u, 0x318fd0u, 0x318fd9u,
                               0x318fe2u, 0x318feeu, 0x318ffau};
    for (unsigned i = 0; i < 6; ++i)
      w(m, o, sites[i], p + 0x80 + i * 4, 4, r(m, packet + 0x70 + i * 4));
  }
  return p;
}
void source_commit_ragdoll_constraint_1_1(M &m, std::uint32_t component,
                                          std::uint32_t constraint,
                                          std::uint16_t type,
                                          std::uint16_t index, const O &o) {
  const auto bank = component + 0x7c, count = r(m, bank + 4);
  if (count == (r(m, bank + 8) & 0x7fffffffu))
    source_double_physics_body_bank_1_1(m, bank, 12, o);
  const auto n = r(m, bank + 4), out = r(m, bank) + n * 12;
  w(m, o, 0x1d3b88, bank + 4, 4, n + 1);
  w(m, o, 0x1d3b96, out, 4, std::uint32_t(type) | (std::uint32_t(index) << 16));
  w(m, o, 0x1d3b98, out + 4, 4, 0xffffffffu);
  w(m, o, 0x1d3b9c, out + 8, 4, constraint);
}
void source_ragdoll_constraint_frames_1_1(
    M &m, const SourceRagdollConstraintFrameRequest &in, FP fp,
    const SourceRagdollRelativeFrame &relative, const O &o) {
  FpScope scope(fp);
  const auto row = in.authored_constraint, slots = r(m, in.component + 0x70);
  auto slot_for = [&](std::int32_t node) {
    return sb(m,
              in.node_slots + (node == -1 ? 0x104u : 4u + std::uint32_t(node)));
  };
  const auto a_node = sh(m, row + 4), a_slot = slot_for(a_node);
  const auto a = slots + std::uint32_t(a_slot) * 0x60u;
  auto frame = [&](std::uint32_t slot, std::int32_t node,
                   std::uint32_t authored, std::uint32_t out, bool second) {
    const auto body_slot = slots + slot * 0x60u, phmo = r(m, in.context + 0x48),
               rows = r(m, phmo + 0x3c);
    const auto indices = r(m, body_slot + 0x48);
    const auto first_node = sh(m, rows + std::uint32_t(sb(m, indices)) * 0x90u);
    // The original scans all rigid indices but compares the first row below.
    for (std::uint32_t i = 0;
         std::bit_cast<std::int32_t>(i) <
         std::bit_cast<std::int32_t>(r(m, body_slot + 0x4c));
         ++i)
      if (sh(m, rows + std::uint32_t(sb(m, indices + i)) * 0x90u) == node)
        break;
    Transform transformed{};
    bool direct = first_node == node;
    if (!direct) {
      if (!relative)
        throw std::runtime_error("Required original compound ragdoll "
                                 "relative-frame owner unavailable");
      const auto transform = std::bit_cast<Transform>(
          relative(in.component, slot, first_node, node));
      Transform local;
      m.read(authored, std::as_writable_bytes(std::span{&local, 1}));
      transformed = compose_transform(transform, local);
    }
    constexpr std::array a_direct{0x1dbac9u, 0x1dbacbu, 0x1dbad0u, 0x1dbad8u,
                                  0x1dbaeau, 0x1dbaedu, 0x1dbaf2u, 0x1dbaf7u,
                                  0x1dbb09u, 0x1dbb0cu, 0x1dbb11u, 0x1dbb16u};
    constexpr std::array b_direct{0x1dbd05u, 0x1dbd07u, 0x1dbd0cu, 0x1dbd11u,
                                  0x1dbd23u, 0x1dbd26u, 0x1dbd2bu, 0x1dbd30u,
                                  0x1dbd42u, 0x1dbd45u, 0x1dbd4au, 0x1dbd4fu};
    constexpr std::array a_relative{0x1dba0cu, 0x1dba16u, 0x1dba28u, 0x1dba30u,
                                    0x1dba35u, 0x1dba40u, 0x1dba4bu, 0x1dba56u,
                                    0x1dba5bu, 0x1dba66u, 0x1dba71u, 0x1dbaabu};
    constexpr std::array b_relative{0x1dbc6au, 0x1dbc74u, 0x1dbc7fu, 0x1dbc87u,
                                    0x1dbc8cu, 0x1dbc97u, 0x1dbca2u, 0x1dbcadu,
                                    0x1dbcb2u, 0x1dbcbdu, 0x1dbcc8u, 0x1dbcf1u};
    const auto values =
        std::bit_cast<std::array<std::uint32_t, 13>>(transformed);
    for (unsigned i = 0; i < 12; ++i) {
      const auto site = direct ? (second ? b_direct[i] : a_direct[i])
                               : (second ? b_relative[i] : a_relative[i]);
      w(m, o, site, out + i * 4, 4,
        (i & 3u) == 3u
            ? 0u
            : (direct ? r(m, authored + 4 + (i / 4) * 12 + (i % 4) * 4)
                      : values[1 + (i / 4) * 3 + i % 4]));
    }
    std::array<std::uint32_t, 4> point{};
    for (unsigned i = 0; i < 3; ++i)
      point[i] = direct ? r(m, authored + 0x28 + 4 * i) : values[10 + i];
    vec(m, o, second ? 0x1dbd83 : (direct ? 0x1dbb47 : 0x1dbab0), out + 48,
        _mm_castsi128_ps(
            _mm_loadu_si128(reinterpret_cast<const __m128i *>(point.data()))));
  };
  frame(std::uint32_t(a_slot), a_node, row + 8, in.frame_a, false);
  const auto body_a = r(m, a + 0x40);
  w(m, o, 0x1dbb5b, in.body_a_output, 4, body_a);
  const auto b_node = sh(m, row + 6),
             b_slot = b_node == -1 ? -1 : slot_for(b_node);
  if (b_node != -1 && b_slot != -1) {
    frame(std::uint32_t(b_slot), b_node, row + 0x3c, in.frame_b, true);
    w(m, o, 0x1dbd91, in.body_b_output, 4,
      r(m, slots + std::uint32_t(b_slot) * 0x60u + 0x40));
  } else {
    compose40(m, in.frame_b, r(m, body_a + 0x3c) + 0x80, in.frame_a, o);
    w(m, o, 0x1dbdbe, in.body_b_output, 4, r(m, r(m, 0x526234) + 0x38));
  }
}
} // namespace halo2
