#include "halo2/biped_controller.hpp"
#include "halo2/biped_ragdoll_admission.hpp"
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
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
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
__m128 scalar(std::uint32_t v) { return _mm_set_ss(std::bit_cast<float>(v)); }
std::uint32_t low(__m128 v) {
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));
}
__m128 vector(const M &m, std::uint32_t p) {
  std::array<std::uint32_t, 4> raw{};
  m.read(p, std::as_writable_bytes(std::span(raw)));
  return _mm_castsi128_ps(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(raw.data())));
}
void vector(M &m, const O &o, std::uint32_t site, std::uint32_t p, __m128 v) {
  std::array<std::uint64_t, 2> raw{};
  _mm_storeu_ps(reinterpret_cast<float *>(raw.data()), v);
  w(m, o, site, p, 8, raw[0]);
  w(m, o, site, p + 8, 8, raw[1]);
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error("Required unmasked controller floating-point "
                               "exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original controller x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t x87_store(std::uint32_t word) {
  const float input = std::bit_cast<float>(word);
  float output;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfstps %0" : "=m"(output) : "m"(input) : "memory");
#else
  throw std::runtime_error(
      "Required original controller x87 store unavailable");
#endif
  return std::bit_cast<std::uint32_t>(output);
}
// Complete E23A0; signed animation height clock uses the current game tick
// duration. A zero source byte does not write the output at all.
bool height(M &m, std::uint32_t datum, std::uint32_t output, const O &o) {
  const auto head = r(m, r(m, 0x4e78d0) + 0x44) + (datum & 0xffffu) * 12u;
  const auto amount = std::int8_t(r(m, r(m, head + 8) + 0x3aa, 1));
  if (!amount)
    return false;
  auto value =
      _mm_mul_ss(scalar(r(m, r(m, 0x518444) + 4)), scalar(r(m, 0x446df8)));
  value = _mm_mul_ss(value, _mm_cvtsi32_ss(_mm_setzero_ps(), amount));
  w(m, o, 0xe23e2, output, 4, low(value));
  return true;
}
} // namespace
void source_construct_biped_controller_1_1(
    M &m, const SourceBipedControllerRequest &in,
    const SourceBipedControllerScratch &scratch, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &estimate, const O &o) {
  if (!scratch.entry_esp)
    throw std::runtime_error(
        "Required original controller caller storage unavailable");
  Scope scope(fp);
  const auto component = in.component, definition = in.definition,
             state = in.physical_state;
  const auto frame = ((scratch.entry_esp - 4u) & ~15u) - 0xe0u;
  w(m, o, in.dead ? 0x1da715 : 0x1da709, component + 4, 4,
    in.dead ? r(m, component + 4) & 0xfffbffffu
            : r(m, component + 4) | 0x40000u);
  auto state_class = std::uint32_t(in.state_class),
       controller_class = std::uint32_t(in.controller_class);
  if (r(m, definition) & 0x40u) {
    state_class = controller_class = 10;
    w(m, o, 0x1da730, scratch.entry_esp + 16, 4, 10);
    w(m, o, 0x1da733, scratch.entry_esp + 20, 4, 10);
  }
  if (r(m, definition) & 0x20u) {
    w(m, o, 0x1da73d, component + 0x18, 1, 0xff);
    return;
  }
  const auto datum = r(m, component + 8),
             header_offset = (datum & 0xffffu) * 12u;
  const auto headers = r(m, r(m, 0x4e78d0) + 0x44);
  const auto motion_class =
      ((~(r(m, headers + header_offset + 2, 1) << 1u)) & 2u) | 5u;
  w(m, o, 0x1da75f, frame + 0x1c, 4, datum);
  const auto identity = r(m, 0x46fc70);
  for (unsigned i = 0; i < 13; ++i)
    w(m, o, 0x1da783, frame + 0x3c + i * 4, 4, r(m, identity + i * 4));
  w(m, o, 0x1da78d, frame + 0x38, 4, header_offset);
  w(m, o, 0x1da791, frame + 0x1b, 1, 0xff);
  for (unsigned i = 0; i < 4; ++i)
    w(m, o, 0x1da796 + i * 6, frame + 0x20 + i * 4, 4, 0);
  const auto shape =
      in.dead ? r(m, definition + 0x24) + r(m, state + 0x23, 1) * 0x80u + 0x30u
      : r(m, definition + 0x28)
          ? r(m, definition + 0x2c) + std::uint32_t(in.constrained) * 0x50u +
                0x20u
          : r(m, definition + 0x34) + std::uint32_t(in.constrained) * 0x80u +
                0x30u;
  for (unsigned i = 0; i < 3; ++i)
    vector(m, o, 0x1da800 + i * 8, frame + 0xb0 + i * 16, _mm_setzero_ps());
  auto flags = in.dead ? r(m, component + 4) & ~4u : r(m, component + 4) | 4u;
  w(m, o, 0x1da822, component + 4, 4, flags);
  flags |= 0x203u;
  w(m, o, 0x1da82a, component + 4, 4, flags);
  const auto physical = r(m, state, 1);
  const bool anchored = physical == 4 || physical == 5;
  flags = anchored ? flags | 0x100u : flags & ~0x100u;
  w(m, o, 0x1da846, component + 4, 4, flags);
  auto z = std::uint32_t{};
  if (anchored) {
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, 0x1da865 + i * 4, frame + 0x64 + i * 4, 4,
        r(m, state + 0x10 + i * 4));
    z = r(m, frame + 0x6c);
  } else {
    source_biped_world_position_1_1(m, datum, frame + 0x64, fp, o);
    z = r(m, frame + 0x6c);
    if ((r(m, headers + header_offset + 3, 1) & 31u) == 0 &&
        height(m, datum, frame + 0x38, o))
      z = low(_mm_sub_ss(scalar(z), scalar(r(m, frame + 0x38))));
  }
  constexpr std::array<std::uint32_t, 9> basis_sites{
      0x1da87d, 0x1da889, 0x1da895, 0x1da8a1, 0x1da8b0,
      0x1da8bf, 0x1da8ce, 0x1da8dd, 0x1da8ec};
  for (unsigned i = 0; i < 9; ++i)
    w(m, o, basis_sites[i], frame + 0x70 + (i / 3) * 16 + (i % 3) * 4, 4,
      r(m, frame + 0x40 + i * 4));
  w(m, o, 0x1da8fb, frame + 0x20, 4, r(m, frame + 0x64));
  w(m, o, 0x1da907, frame + 0x24, 4, r(m, frame + 0x68));
  w(m, o, 0x1da90d, frame + 0x28, 4, z);
  w(m, o, 0x1da913, frame + 0x2c, 4, 0);
  for (unsigned i = 0; i < 3; ++i)
    w(m, o,
      i == 0   ? 0x1da91e
      : i == 1 ? 0x1da924
               : 0x1da92d,
      frame + 0x7c + i * 16, 4, 0);
  vector(m, o, 0x1da936, frame + 0xa0, vector(m, frame + 0x20));
  const auto shape_type = source_physics_shape_type_1_1(m, shape);
  if (shape_type == 7) {
    const auto ends =
        _mm_add_ps(vector(m, shape + 0x10), vector(m, shape + 0x20));
    vector(m, o, 0x1da967, frame + 0x20,
           _mm_mul_ps(_mm_shuffle_ps(scalar(r(m, 0x46502c)),
                                     scalar(r(m, 0x46502c)), 0),
                      ends));
  } else if (source_physics_shape_type_1_1(m, shape) == 21) {
    vector(m, o, 0x1da9d7, frame + 0x20, vector(m, shape + 0x40));
  } else {
    const auto zero = r(m, 0x46fc28);
    constexpr std::array<std::uint32_t, 3> sites{0x1da9e7, 0x1da9f2, 0x1da9fd};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, sites[i], frame + 0x20 + i * 4, 4, r(m, zero + i * 4));
    w(m, o, 0x1daa06, frame + 0x2c, 4, 0);
  }
  const auto mass_word = in.dead ? r(m, 0x465030) : 0u;
  w(m, o, 0x1daa4a, frame + 0x1c, 4, mass_word);
  const std::array<std::uint32_t, 14> arguments{shape,
                                                0,
                                                0,
                                                frame + 0x20,
                                                mass_word,
                                                0,
                                                frame + 0x70,
                                                r(m, definition + 0x10),
                                                motion_class,
                                                frame + 0x70,
                                                in.dead ? controller_class
                                                        : state_class,
                                                0,
                                                0,
                                                0};
  auto body = source_create_physics_rigid_body_1_1(
      m, component, arguments, scratch.factory, fp, estimate, o);
  if (r(m, body + 0x40, 1) == 0) {
    const auto motion = r(m, body + 0x3c);
    if (r(m, r(m, motion) + 0x30) != 0x3213e0)
      throw std::runtime_error(
          "Required original controller inertia virtual owner unavailable");
    const auto old_mass = x87_store(r(m, motion + 0xec));
    const auto diagonal_z = x87_store(r(m, frame + 0xb0 + 0x28));
    const auto diagonal_y = x87_store(r(m, frame + 0xb0 + 0x14));
    w(m, o, 0x3213f2, motion + 0xe0, 4, r(m, frame + 0xb0));
    w(m, o, 0x3213f8, motion + 0xe4, 4, diagonal_y);
    w(m, o, 0x3213fe, motion + 0xe8, 4, diagonal_z);
    w(m, o, 0x321404, motion + 0xec, 4, old_mass);
  }
  if (!(r(m, component + 4) & 0x100u))
    body = source_commit_physics_shape_wrapper_1_1(
        m, component, body, 0x3f59999a, scratch.wrapper, fp, o);
  source_commit_physics_body_slot_1_1(m, {component, body, frame + 0x1b, 1, 1},
                                      o);
  w(m, o, 0x1daaa9, component + 0x18, 1, 0);
}
} // namespace halo2
