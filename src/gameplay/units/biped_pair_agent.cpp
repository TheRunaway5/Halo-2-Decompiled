#include "halo2/physics_pair_manager.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using S = SourcePhysicsPairManagerServices;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint64_t v) {
  SourcePhysicsContactRecordStore x{site, p, {}, {}};
  x.before.resize(n);
  x.written.resize(n);
  m.read(p, x.before);
  for (unsigned i = 0; i < n; ++i)
    x.written[i] = std::byte(v >> (8 * i));
  m.write(p, x.written);
  if (o)
    o(x);
}
__m128 v(const M &m, std::uint32_t p) {
  std::array<std::uint32_t, 4> b{};
  for (unsigned i = 0; i < 4; ++i)
    b[i] = r(m, p + i * 4);
  return _mm_castsi128_ps(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(b.data())));
}
void vw(M &m, const O &o, std::uint32_t site, std::uint32_t p, __m128 value) {
  std::array<float, 4> b{};
  _mm_storeu_ps(b.data(), value);
  const auto words = std::bit_cast<std::array<std::uint32_t, 4>>(b);
  for (unsigned i = 0; i < 2; ++i)
    w(m, o, site, p + i * 8u, 8,
      std::uint64_t(words[i * 2]) | (std::uint64_t(words[i * 2 + 1]) << 32));
}
std::uint32_t required(M &m, const S &s, std::uint32_t target,
                       std::uint32_t receiver, std::vector<std::uint32_t> args,
                       bool cdecl, const O &o) {
  if (!s.required_owner)
    throw std::runtime_error(
        "Required original transformed-agent owner unavailable at " +
        std::to_string(target));
  return s.required_owner(m, {target, receiver, std::move(args), cdecl}, o);
}
std::uint32_t type(M &m, std::uint32_t shape, const S &s, const O &o) {
  const auto target = r(m, r(m, shape) + 0x14);
  if (target == 0xa0120)
    return 23;
  if (target == 0xa47b0)
    return 20;
  if (target == 0x2e4d20)
    return 21;
  return required(m, s, target, shape, {}, false, o);
}
struct Scope {
  unsigned old;
  explicit Scope(SourcePhysicsFactoryFloatingPoint fp) : old(_mm_getcsr()) {
    if ((fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original transformed-agent unmasked FP unavailable");
    _mm_setcsr(fp.mxcsr);
  }
  ~Scope() { _mm_setcsr(old); }
};
} // namespace
std::uint32_t source_allocate_physics_transformed_pair_agent_1_1(
    M &m, const std::array<std::uint32_t, 4> &a, std::uint32_t esp,
    SourcePhysicsFactoryFloatingPoint fp, const S &s, const O &o) {
  Scope scope(fp);
  const auto frame = ((esp - 4u) & ~15u) - 0xa0u, adapter = r(m, 0x487638),
             backend = r(m, 0x480da0);
  if (r(m, r(m, adapter) + 0x10) != 0x148a10 || r(m, 0x480d98) != 2 ||
      r(m, r(m, backend) + 0x10) != 0x22e950)
    throw std::runtime_error(
        "Required original transformed-agent allocator unavailable");
  const auto p =
      source_allocate_physics_body_small_block_1_1(m, backend, 0x10, o);
  if (!p)
    throw std::runtime_error(
        "Required original transformed-agent fallback allocator unavailable");
  w(m, o, 0x2f3765, p + 4, 2, 0x10);
  w(m, o, 0x2f377d, p + 8, 4, a[3]);
  w(m, o, 0x2f3783, p, 4, 0x4191e4);
  const auto input = r(m, a[0] + 8), shape = r(m, a[0]),
             child = r(m, shape + 12), packet = frame + 0x20u,
             blob = frame + 0x30u;
  constexpr std::array<std::uint32_t, 6> sites{0x2f3794, 0x2f379d, 0x2f37a6,
                                               0x2f37af, 0x2f37bb, 0x2f37d5};
  for (unsigned i = 0; i < 6; ++i)
    vw(m, o, sites[i], blob + i * 16u, v(m, input + i * 16u));
  const auto left = input + 0x20u, right = shape + 0x10u, out = blob + 0x20u;
  const __m128 basis[4]{v(m, left), v(m, left + 16),
                                    v(m, left + 32), v(m, left + 48)};
  for (unsigned i = 0; i < 4; ++i) {
    const auto q = v(m, right + i * 16u);
    const auto x = _mm_mul_ps(basis[0], _mm_shuffle_ps(q, q, 0)),
               y = _mm_mul_ps(basis[1], _mm_shuffle_ps(q, q, 0x55)),
               z = _mm_mul_ps(basis[2], _mm_shuffle_ps(q, q, 0xaa));
    vw(m, o, 0x2e0a32, out + i * 16u, _mm_add_ps(_mm_add_ps(x, y), z));
  }
  vw(m, o, 0x2e0a48, out + 48u, _mm_add_ps(v(m, out + 48u), basis[3]));
  w(m, o, 0x2f37e9, packet + 8, 4, blob);
  w(m, o, 0x2f37ef, packet + 12, 4, a[0]);
  w(m, o, 0x2f37f6, packet + 4, 4, r(m, a[0] + 4));
  w(m, o, 0x2f37fa, packet, 4, child);
  const auto first = type(m, child, s, o), second = type(m, r(m, a[1]), s, o),
             target = r(m, r(m, a[2]) + (first * 32u + second) * 4u + 0x18c);
  const std::array<std::uint32_t, 4> args{packet, a[1], a[2], a[3]};
  std::uint32_t nested;
  if (target == 0x300170)
    nested = source_allocate_physics_pair_agent_1_1(m, args, o);
  else if (target == 0x2f3730)
    nested = source_allocate_physics_transformed_pair_agent_1_1(
        m, args, frame - 20u, fp, s, o);
  else
    nested = required(m, s, target, 0, {args.begin(), args.end()}, true, o);
  w(m, o, 0x2f383f, p + 12, 4, nested);
  return p;
}
} // namespace halo2
