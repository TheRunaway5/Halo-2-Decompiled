#include "halo2/physics_body_grouping.hpp"
#include <algorithm>
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
    v |= std::to_integer<std::uint32_t>(b[i]) << (i * 8);
  return v;
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::uint32_t sx(const M &m, std::uint32_t p, unsigned n) {
  const auto v = r(m, p, n);
  return n == 1 ? (v & 128u ? v | 0xffffff00u : v)
                : (v & 0x8000u ? v | 0xffff0000u : v);
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint64_t v) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(v >> (i * 8));
  m.write(p, s.written);
  if (o)
    o(s);
}
void copy(M &m, const O &o, std::uint32_t site, std::uint32_t dst,
          std::uint32_t src, unsigned n) {
  for (unsigned offset = 0; offset < n; offset += 8) {
    const auto size = std::min(8u, n - offset);
    std::array<std::byte, 8> b{};
    m.read(src + offset, std::span(b).first(size));
    std::uint64_t v = 0;
    for (unsigned j = 0; j < size; ++j)
      v |= std::uint64_t(std::to_integer<unsigned>(b[j])) << (8 * j);
    w(m, o, site, dst + offset, size, v);
  }
}
[[noreturn]] void missing(const char *s) { throw std::runtime_error(s); }
std::uint32_t object_header(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12u;
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, object_header(m, d) + 8);
}
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 0xffffu) * 16u + 8);
}
struct FpScope {
  unsigned mx;
  std::fenv_t env;
  explicit FpScope(FP p) : mx(_mm_getcsr()) {
    if ((p.x87_control_word & 0x3f) != 0x3f || (p.mxcsr & 0x1f80) != 0x1f80)
      missing("Required unmasked source FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(p.mxcsr);
  }
  ~FpScope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
__m128 scalar(const M &m, std::uint32_t p) {
  return _mm_set_ss(std::bit_cast<float>(r(m, p)));
}
std::uint32_t word(__m128 x) {
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x));
}
void root_matrix(M &m, std::uint32_t p, std::uint32_t forward, std::uint32_t up,
                 std::uint32_t point, FP fp, const O &o) {
  FpScope scope(fp);
  w(m, o, 0x143868, p, 4, r(m, 0x465030));
  constexpr std::array forward_sites{0x143876u, 0x14387bu, 0x143881u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, forward_sites[i], p + 4 + i * 4, 4, r(m, forward + i * 4));
  auto a = _mm_mul_ss(scalar(m, forward + 4), scalar(m, up));
  auto b = _mm_mul_ss(scalar(m, forward), scalar(m, up + 4));
  const auto z = _mm_sub_ss(a, b);
  a = _mm_mul_ss(scalar(m, up + 8), scalar(m, forward));
  b = _mm_mul_ss(scalar(m, forward + 8), scalar(m, up));
  const auto y = _mm_sub_ss(a, b);
  a = _mm_mul_ss(scalar(m, forward + 8), scalar(m, up + 4));
  b = _mm_mul_ss(scalar(m, up + 8), scalar(m, forward + 4));
  w(m, o, 0x1438c4, p + 0x18, 4, word(z));
  w(m, o, 0x1438cc, p + 0x14, 4, word(y));
  w(m, o, 0x1438d5, p + 0x10, 4, word(_mm_sub_ss(a, b)));
  constexpr std::array up_sites{0x1438dfu, 0x1438e4u, 0x1438eau};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, up_sites[i], p + 0x1c + i * 4, 4, r(m, up + i * 4));
  constexpr std::array zero_sites{0x1438f0u, 0x1438f4u, 0x1438f9u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, zero_sites[i], p + 0x28 + i * 4, 4, 0);
  constexpr std::array point_sites{0x143900u, 0x143906u, 0x14390du};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, point_sites[i], p + 0x28 + i * 4, 4, r(m, point + i * 4));
}
struct MotionClass {
  std::uint32_t value, edx;
};
MotionClass motion_mode(const M &m, std::uint32_t component,
                        std::uint32_t context, std::uint32_t index,
                        std::uint32_t mode, std::uint32_t request) {
  const auto type = r(m, object_header(m, r(m, component + 8)) + 3, 1);
  const auto data = r(m, r(m, 0x4e78d0) + 0x44);
  if (mode == 7)
    return {type == 1 ? 12u : 1u, data};
  const auto target = r(m, 0x1da4a0 + r(m, 0x1da4b4 + type, 1) * 4);
  const auto row = r(m, r(m, context + 0x48) + 0x3c) + index * 0x90u;
  switch (target) {
  case 0x1da422:
    return {si(request) <= 1 ? 4u : (si(request) >= 5 ? 5u : 3u), data};
  case 0x1da444:
    return {((r(m, row + 0x18, 1) & 8u) | 0x30u) >> 2, data};
  case 0x1da457: {
    const auto unit = object(m, r(m, component + 8));
    return {((r(m, unit + 0x10a, 1) >> 2) & 1u) * 2u | 9u, unit};
  }
  case 0x1da47d:
    return {mode == 6 ? 8u : ((r(m, row + 0x18, 1) & 8u) | 0x30u) >> 3, data};
  default:
    missing("Required original other object motion-class owner unavailable");
  }
}
std::uint32_t damage_info(const M &m, std::uint32_t datum) {
  if (datum == 0xffffffffu)
    return 0;
  const auto model_datum = r(m, tag(m, r(m, object(m, datum))) + 0x38);
  if (model_datum == 0xffffffffu)
    return 0;
  const auto model = tag(m, model_datum);
  return si(r(m, model + 0x60)) > 0 ? r(m, model + 0x64) : 0;
}
bool damage_constraint(const M &m, std::uint32_t datum, std::uint32_t type,
                       std::uint32_t index, bool destroyed) {
  const auto info = damage_info(m, datum);
  if (!info)
    return false;
  for (std::uint32_t i = 0; si(i) < si(r(m, info + 0xe0)); ++i) {
    const auto row = r(m, info + 0xe4) + i * 20u;
    if (sx(m, row + 0x12, 2) == index && sx(m, row + 0x10, 2) == type)
      return (r(m, object(m, datum) + (destroyed ? 0xe2u : 0xe0u), 2) &
              (1u << (i & 31u))) != 0;
  }
  return false;
}
void slot_map(M &m, std::uint32_t datum, std::uint32_t out, const O &o) {
  const auto ob = object(m, datum), model = r(m, tag(m, r(m, ob)) + 0x38);
  w(m, o, 0x183684, out, 4, 0);
  w(m, o, 0x18368b, out + 0x104, 1, 255);
  if (model == 0xffffffffu)
    return;
  const auto phmod = r(m, tag(m, model) + 0x24);
  if (phmod == 0xffffffffu)
    return;
  const auto phmo = tag(m, phmod),
             component = r(m, r(m, 0x526248) + 0x44) +
                         (r(m, ob + 0xb4) & 0xffffu) * 0xa0u;
  w(m, o, 0x1836e1, out, 4, r(m, phmo + 0xc8));
  for (unsigned i = 0; i < 64; ++i)
    w(m, o, 0x1836ef, out + 4 + i * 4, 4, 0xffffffffu);
  for (std::uint32_t n = 0; si(n) < si(r(m, component + 0x74)); ++n) {
    const auto slot = r(m, component + 0x70) + n * 0x60u;
    for (std::uint32_t i = 0; si(i) < si(r(m, slot + 0x4c)); ++i) {
      const auto index = sx(m, r(m, slot + 0x48) + i, 1);
      if (index == 0xffffffffu)
        continue;
      const auto node = sx(m, r(m, phmo + 0x3c) + index * 0x90u, 2);
      w(m, o, node == 0xffffffffu ? 0x183739 : 0x183733,
        node == 0xffffffffu ? out + 0x104 : out + 4 + node, 1, n);
    }
  }
}
std::uint32_t iterator_bank(M &m, std::uint32_t phmo, std::uint32_t type,
                            std::uint32_t stride, const O &o) {
  constexpr std::array offsets{0xb0u, 0xe8u, 0xb8u, 0xf8u, 0xf0u, 0x100u};
  constexpr std::array strides{0x78u, 0x84u, 0x94u, 0x7cu, 0x78u, 0x84u};
  constexpr std::array sites{0x1eeb7fu, 0x1eeb8bu, 0x1eeb97u,
                             0x1eeba3u, 0x1eebafu, 0x1eebbbu};
  if (type >= 6)
    missing("Required original constraint iterator class unavailable");
  w(m, o, sites[type], stride, 4, strides[type]);
  return phmo + offsets[type];
}
float source_trig_product(float value, float multiplier, bool cosine) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  float result;
  if (cosine)
    asm volatile("flds %1\n\tfmuls %2\n\tfcos\n\tfstps %0"
                 : "=m"(result)
                 : "m"(value), "m"(multiplier)
                 : "memory");
  else
    asm volatile("flds %1\n\tfmuls %2\n\tfsin\n\tfstps %0"
                 : "=m"(result)
                 : "m"(value), "m"(multiplier)
                 : "memory");
  return result;
#else
  throw std::runtime_error(
      "Required original ragdoll x87 trigonometric backend unavailable");
#endif
}
void constraints(M &m, std::uint32_t component, std::uint32_t context,
                 const SourcePhysicsBodyGroupingScratch &s, FP fp,
                 const SourcePhysicsReciprocalSqrtEstimate &estimate,
                 const O &o) {
  FpScope scope(fp);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#endif
  const auto frame = s.constraints_frame;
  slot_map(m, r(m, component + 8), frame + 0x358, o);
  const auto phmo = tag(m, r(m, r(m, context + 0x44) + 0x24));
  w(m, o, 0x1db201, frame + 0x1c, 4, phmo);
  const auto iterator = frame + 0x1c;
  w(m, o, 0x1eead7, iterator + 4, 2, 0);
  for (unsigned type = 0;; ++type) {
    w(m, o, 0x1eeae4, iterator + 6, 2, 0);
    const auto bank = iterator_bank(m, phmo, type, s.iterator_stride, o);
    if (si(sx(m, iterator + 6, 2)) < si(r(m, bank)) &&
        r(m, bank + 4) + sx(m, iterator + 6, 2) * r(m, s.iterator_stride) != 0)
      break;
    if (type == 5)
      break;
    w(m, o, 0x1eeb11, iterator + 4, 2, type + 1);
  }
  for (;;) {
    const auto type = sx(m, iterator + 4, 2), index = sx(m, iterator + 6, 2);
    const auto bank = iterator_bank(m, phmo, type, frame + 0x38, o);
    w(m, o, 0x1db228, frame + 0x3c, 4, index);
    if (si(index) >= si(r(m, bank)))
      break;
    const auto row = r(m, bank + 4) + index * r(m, frame + 0x38);
    if (row == 0)
      break;
    w(m, o, 0x1db24e, frame + 0x2c, 4, type);
    if (!damage_constraint(m, r(m, component + 8), type, index, false) &&
        sx(m, row + 4, 2) != 0xffffffffu) {
      const auto node_a = sx(m, row + 4, 2), node_b = sx(m, row + 6, 2);
      const auto slot_a = sx(
          m, frame + 0x358 + (node_a == 0xffffffffu ? 0x104u : 4u + node_a), 1);
      const auto slot_b = sx(
          m, frame + 0x358 + (node_b == 0xffffffffu ? 0x104u : 4u + node_b), 1);
      if (slot_a != slot_b && slot_a != 0xffffffffu &&
          r(m, r(m, r(m, component + 0x70) + slot_a * 0x60u + 0x40) + 0x40,
            1) == 0 &&
          ((slot_b == 0xffffffffu) == (node_b == 0xffffffffu))) {
        w(m, o, 0x1db325, frame + 0x24, 4, 0);
        source_ragdoll_constraint_frames_1_1(m,
                                             {component, context, frame + 0x358,
                                              row, frame + 0x40, frame + 0x80,
                                              frame + 0x18, frame + 0x14},
                                             fp, {}, o);
        const auto property =
            r(m, r(m, context + 0x48) + 0x34) + sx(m, row + 0x70, 2) * 0x18u;
        for (std::uint32_t i = 0; si(i) < si(r(m, property + 8)); ++i) {
          const auto dependency = r(m, property + 0xc) + i * 12u;
          if (sx(m, dependency, 2) == type &&
              sx(m, dependency + 2, 2) == index) {
            w(m, o, 0x1db364, frame + 0x24, 4, r(m, dependency + 8));
            break;
          }
        }
        if (type != 1 && type != 2)
          missing("Required original additional authored constraint factory "
                  "unavailable");
        const auto kind = type == 1 ? SourceRagdollConstraintKind::limited_hinge
                                    : SourceRagdollConstraintKind::ragdoll;
        const auto packet = frame + (type == 1 ? 0x1c0u : 0xf0u);
        source_initialize_ragdoll_constraint_packet_1_1(m, packet, kind, o);
        source_set_ragdoll_constraint_axes_1_1(
            m,
            {packet,
             r(m, frame + 0x18),
             r(m, frame + 0x14),
             {frame + 0x70, frame + 0xb0, frame + 0x60, frame + 0xa0,
              frame + 0x50, frame + 0x90}},
            kind, fp, estimate, o);
        const auto data = r(m, bank + 4) + index * (type == 1 ? 0x84u : 0x94u);
        const auto angle = _mm_set_ss(std::bit_cast<float>(r(m, 0x4650c4)));
        auto load = [&](std::uint32_t at) {
          return _mm_set_ss(std::bit_cast<float>(r(m, at)));
        };
        auto store = [&](std::uint32_t site, std::uint32_t at, __m128 value) {
          w(m, o, site, at, 4, word(value));
        };
        if (type == 1) {
          auto friction = load(frame + 0x24);
          if (_mm_cvtss_f32(friction) == 0.f)
            friction = load(data + 0x78);
          store(0x1db454, packet + 0x88, friction);
          auto low = _mm_sub_ss(_mm_setzero_ps(), load(data + 0x7c)),
               high = _mm_sub_ss(_mm_setzero_ps(), load(data + 0x80));
          const auto lower =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? high : low;
          store(0x1db484, packet + 0x80, _mm_mul_ss(lower, angle));
          low = _mm_sub_ss(_mm_setzero_ps(), load(data + 0x7c));
          high = _mm_sub_ss(_mm_setzero_ps(), load(data + 0x80));
          const auto upper =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? low : high;
          store(0x1db4b1, packet + 0x84, _mm_mul_ss(upper, angle));
        } else {
          auto low = load(data + 0x78), high = load(data + 0x7c);
          const auto smaller =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? high : low;
          store(0x1db562, packet + 0x7c, _mm_mul_ss(smaller, angle));
          low = load(data + 0x78);
          high = load(data + 0x7c);
          const auto larger =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? low : high;
          store(0x1db581, packet + 0x80, _mm_mul_ss(larger, angle));
          low = load(data + 0x88);
          high = load(data + 0x8c);
          const auto plane_min =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? high : low;
          store(0x1db5ad, frame + 0x30, plane_min);
          w(m, o, 0x1db5bf, packet + 0x74, 4,
            std::bit_cast<std::uint32_t>(source_trig_product(
                _mm_cvtss_f32(plane_min), _mm_cvtss_f32(angle), false)));
          low = load(data + 0x88);
          high = load(data + 0x8c);
          const auto plane_max =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? low : high;
          store(0x1db5e1, frame + 0x34, plane_max);
          w(m, o, 0x1db5f3, packet + 0x78, 4,
            std::bit_cast<std::uint32_t>(source_trig_product(
                _mm_cvtss_f32(plane_max), _mm_cvtss_f32(angle), false)));
          low = load(data + 0x80);
          high = load(data + 0x84);
          const auto cone =
              _mm_cvtss_f32(low) > _mm_cvtss_f32(high) ? low : high;
          store(0x1db615, frame + 0x28, cone);
          w(m, o, 0x1db638, packet + 0x70, 4,
            std::bit_cast<std::uint32_t>(source_trig_product(
                _mm_cvtss_f32(cone), _mm_cvtss_f32(angle), true)));
          auto friction = load(frame + 0x24);
          if (_mm_cvtss_f32(friction) == std::bit_cast<float>(r(m, 0x465048)))
            friction = load(data + 0x90);
          store(0x1db651, packet + 0x84, friction);
        }
        const auto adapter = r(m, 0x487638), backend = r(m, 0x480da0);
        if (r(m, r(m, adapter) + 0x10) != 0x148a10u || r(m, 0x480d98) != 2u ||
            r(m, r(m, backend) + 0x10) != 0x22e950u)
          missing("Required original ragdoll allocator owner unavailable");
        const auto constraint =
            source_allocate_physics_body_small_block_1_1(m, backend, 0xe0, o);
        if (!constraint)
          missing("Required original ragdoll alternate allocator unavailable");
        w(m, o, type == 1 ? 0x1db4ce : 0x1db66e, constraint + 4, 2, 0xe0);
        source_construct_ragdoll_constraint_1_1(m, constraint, packet, kind, fp,
                                                {}, o);
        source_commit_ragdoll_constraint_1_1(m, component, constraint,
                                             std::uint16_t(type),
                                             std::uint16_t(index), o);
      }
    }
    w(m, o, 0x1eeb23, iterator + 6, 2, r(m, iterator + 6, 2) + 1u);
    for (;;) {
      const auto next_type = sx(m, iterator + 4, 2),
                 next_index = sx(m, iterator + 6, 2);
      const auto next_bank =
          iterator_bank(m, phmo, next_type, s.iterator_stride, o);
      if (si(next_index) < si(r(m, next_bank)) &&
          r(m, next_bank + 4) + next_index * r(m, s.iterator_stride) != 0)
        break;
      if (next_type >= 5)
        break;
      w(m, o, 0x1eeb5f, iterator + 4, 2, next_type + 1u);
      w(m, o, 0x1eeb62, iterator + 6, 2, 0);
    }
  }
  w(m, o, 0x1db843, component + 4, 4, r(m, component + 4) & 0xfffffbffu);
  for (std::uint32_t i = 0; si(i) < si(r(m, component + 0x80)); ++i) {
    const auto constraint = r(m, r(m, component + 0x7c) + i * 12u + 8);
    if (r(m, constraint + 0x10) != r(m, r(m, 0x526234) + 0x38))
      continue;
    const auto primary = sx(m, component + 0x18, 1);
    if (primary != 0xffffffffu &&
        r(m, r(m, component + 0x70) + primary * 0x60u + 0x40) ==
            r(m, constraint + 0xc)) {
      w(m, o, 0x1db8ac, component + 0x18, 1, 255);
      w(m, o, 0x1db8b0, component + 0x19, 1, 255);
    }
    w(m, o, 0x1db8b4, component + 4, 4, r(m, component + 4) | 0x400u);
    break;
  }
}
void primary_slot(M &m, std::uint32_t component, std::uint32_t context,
                  const SourcePhysicsBodyGroupingScratch &s, FP fp,
                  const SourcePhysicsReciprocalSqrtEstimate &estimate,
                  const O &o) {
  const auto count = r(m, component + 0x74);
  std::uint32_t lowest = 0x7fffffffu;
  w(m, o, 0x1da4ca, component + 0x18, 1, 255);
  w(m, o, 0x1da4ce, component + 0x19, 1, 255);
  for (std::uint32_t i = 0; si(i) < si(r(m, component + 0x74)); ++i) {
    const auto slot = r(m, component + 0x70) + i * 0x60u;
    if (si(r(m, slot + 0x4c)) <= 0)
      continue;
    const auto row =
        r(m, r(m, context + 0x48) + 0x3c) + sx(m, r(m, slot + 0x48), 1) * 0x90u;
    const auto node = sx(m, row, 2);
    if (node == 0xffffffffu) {
      w(m, o, 0x1da527, component + 0x18, 1, i);
      w(m, o, 0x1da52c, component + 0x19, 1, r(m, row, 1));
      break;
    }
    if (si(node) < si(lowest)) {
      w(m, o, 0x1da50d, component + 0x18, 1, i);
      lowest = node;
      w(m, o, 0x1da517, component + 0x19, 1, node);
    }
  }
  const auto only_root = count == 1 && r(m, component + 0x19, 1) == 255;
  w(m, o, only_root ? 0x1da547 : 0x1da55b, component + 4, 4,
    only_root ? r(m, component + 4) | 0x200u
              : r(m, component + 4) & 0xfffffdffu);
  constraints(m, component, context, s, fp, estimate, o);
}
// Measured1D98B0→1DA020 one-row owner; count0 returns without overwriting
// mass/inertia/request. Compound aggregation and positive constraints stop.
std::uint32_t collect(M &m, std::uint32_t component, std::uint32_t context,
                      std::uint32_t modes, std::uint32_t group,
                      std::uint32_t visited, std::uint32_t bank,
                      std::uint32_t request,
                      const SourcePhysicsBodyGroupingScratch &s, const O &o) {
  const auto f = s.selection_frame, c = s.collection_frame,
             phmo = r(m, context + 0x48);
  w(m, o, 0x1d98d6, f + 0x1c, 4, 0);
  w(m, o, 0x1d98dd, f + 0x16, 1, 0);
  w(m, o, 0x1d98e1, f + 0x15, 1, 0);
  source_physics_body_property_mask_1_1(m, component, context, c + 0x260, o);
  source_physics_collect_body_groups_1_1(m,
                                         {component, context, modes, c + 0x260,
                                          bank, group, f + 0x90, f + 0x1c,
                                          visited, f + 0x16},
                                         o);
  const auto n = r(m, f + 0x1c);
  if (si(n) <= 0)
    return 0;
  const auto index = sx(m, f + 0x90, 1), mode = r(m, modes + index, 1);
  w(m, o, mode == 7 || mode == 6 ? 0x1d993a : 0x1d9933, c + 0xe, 1,
    mode == 7 || mode == 6 ? 1 : 0);
  if (n != 1)
    missing("Required original multi-body shape/inertia aggregation owner "
            "unavailable");
  const auto row = r(m, phmo + 0x3c) + index * 0x90u;
  w(m, o, 0x1d9958, f + 0x24, 4, r(m, row + 0x3c));
  copy(m, o, 0x1d9961, f + 0x80, row + 0x40, 16);
  copy(m, o, 0x1d996b, f + 0xd0, row + 0x50, 16);
  copy(m, o, 0x1d9972, f + 0xe0, row + 0x60, 16);
  copy(m, o, 0x1d997a, f + 0xf0, row + 0x70, 16);
  w(m, o, 0x1d9985, request, 4, sx(m, row + 0x1e, 2));
  const auto shape = r(m, row + 0x38);
  w(m, o, 0x1d9994, shape + 6, 2, r(m, shape + 6, 2) + 1u);
  const auto flag_count = (r(m, row + 0x18, 1) >> 2) & n;
  w(m, o, 0x1d999f, c + 0x20, 4, flag_count);
  if (r(m, row + 0x8e, 1) & 1u)
    w(m, o, 0x1d99ac, f + 0x14, 1, 1);
  w(m, o, 0x1d9f32, f + 0x15, 1, flag_count != n ? 1 : 0);
  return shape;
}
bool select(M &m, std::uint32_t component, std::uint32_t context,
            std::uint32_t modes, std::uint32_t wrap, std::uint32_t group,
            std::uint32_t request, std::uint32_t visited, std::uint32_t bank,
            std::uint32_t result, const SourcePhysicsBodyGroupingScratch &s,
            FP fp, const SourcePhysicsReciprocalSqrtEstimate &rsqrt, const O &o,
            const SourcePhysicsBodyGroupingCallObserver &call) {
  const auto f = s.selection_frame;
  w(m, o, 0x1d963d, f + 0x1c, 4, 0);
  w(m, o, 0x1d9645, f + 0x16, 1, 0);
  w(m, o, 0x1d9649, f + 0x15, 1, 1);
  w(m, o, 0x1d964e, f + 0x14, 1, 0);
  const auto shape = collect(m, component, context, modes, group, visited, bank,
                             request, s, o);
  if (shape == 0)
    return false;
  const auto phmo = r(m, context + 0x48), index = sx(m, f + 0x90, 1),
             row = r(m, phmo + 0x3c) + index * 0x90u;
  w(m, o, 0x1d9678, f + 0x2c, 4, phmo);
  const auto mode = r(m, f + 0x16, 1) ? 7u : sx(m, modes + index, 1);
  w(m, o, 0x1d969e, f + 0x18, 4, mode);
  const auto motion =
      motion_mode(m, component, context, index, mode, r(m, request));
  w(m, o, 0x1d96ad, f + 0x20, 4, motion.value);
  const auto node = sx(m, row, 2);
  const auto matrix =
      node == 0xffffffffu ? context + 8 : r(m, context + 0x50) + node * 0x34u;
  for (const auto [site, offset, src] :
       std::array<std::array<std::uint32_t, 3>, 12>{
           {{0x1d96d3, 0x48, 0xc},
            {0x1d96dc, 0x4c, 0xffffffff},
            {0x1d96e2, 0x40, 4},
            {0x1d96e8, 0x44, 8},
            {0x1d96fd, 0x5c, 0xffffffff},
            {0x1d9703, 0x50, 0x10},
            {0x1d9709, 0x54, 0x14},
            {0x1d970f, 0x58, 0x18},
            {0x1d9724, 0x6c, 0xffffffff},
            {0x1d972a, 0x60, 0x1c},
            {0x1d9730, 0x64, 0x20},
            {0x1d9736, 0x68, 0x24}}})
    w(m, o, site, f + offset, 4, src == 0xffffffffu ? 0 : r(m, matrix + src));
  w(m, o, 0x1d9741, f + 0x30, 4, r(m, matrix + 0x28));
  w(m, o, 0x1d974c, f + 0x34, 4, r(m, matrix + 0x2c));
  w(m, o, 0x1d9757, f + 0x38, 4, r(m, matrix + 0x30));
  w(m, o, 0x1d975d, f + 0x3c, 4, 0);
  copy(m, o, 0x1d9768, f + 0x70, f + 0x30, 16);
  const auto flags = r(m, row + 0x18, 2),
             type = r(m, object_header(m, r(m, component + 8)) + 3, 1);
  // Non-root node selection retains EDX=context; root selection retains
  // 1DA3C0's actual returned register. MOV DX replaces only its low16 bits.
  const auto flag_register =
      ((node == 0xffffffffu ? motion.edx : context) & 0xffff0000u) | flags;
  if (!(flags & 1u)) {
    w(m, o, 0x1d9794, f + 0x28, 4, flag_register);
    w(m, o, 0x1d979f, f + 0x17, 1, 0);
  }
  if ((flags & 1u) || ((1u << (type & 31u)) & 2u))
    w(m, o, 0x1d97ad, f + 0x17, 1, 1);
  // SETE CL and MOV AL preserve their actual register upper24 words.
  const auto header_data = r(m, r(m, 0x4e78d0) + 0x44);
  std::array<std::uint32_t, 14> args{
      shape,
      ((f + 0xd0) & 0xffffff00u) | (r(m, phmo + 0x38) == 1 ? 1u : 0u),
      ((flags & 1u ? motion.value : header_data) & 0xffffff00u) |
          (mode == 7 || mode == 6 ? 1u : 0u),
      f + 0x80,
      0x3f800000u,
      0,
      f + 0xd0,
      r(m, f + 0x24),
      mode,
      f + 0x40,
      r(m, f + 0x20),
      (flag_register & 0xffffff00u) | ((flags >> 1) & 1u),
      r(m, row + 0x24),
      r(m, row + 0x28)};
  if (call)
    call(args);
  auto body = source_create_physics_rigid_body_1_1(m, component, args,
                                                   s.factory, fp, rsqrt, o);
  w(m, o, 0x1d9812, shape + 6, 2, r(m, shape + 6, 2) - 1u);
  if (r(m, shape + 6, 2) == 0)
    missing("Required original deferred shape-delete owner unavailable");
  if (wrap & 255u)
    body = source_commit_physics_shape_wrapper_1_1(
        m, component, body, 0x3f59999au, s.wrapper, fp, o);
  w(m, o, 0x1d9857, result, 4, body);
  const auto count = r(m, f + 0x1c);
  std::uint32_t i = 0;
  for (; i < (count & ~3u); i += 4)
    w(m, o, 0x1d986a, result + 4 + i, 4, r(m, f + 0x90 + i));
  for (; i < count; ++i)
    w(m, o, 0x1d9871, result + 4 + i, 1, r(m, f + 0x90 + i, 1));
  w(m, o, 0x1d9877, result + 0x48, 1, r(m, f + 0x15, 1));
  w(m, o, 0x1d9880, result + 0x44, 4, count);
  if (r(m, f + 0x14, 1))
    w(m, o, 0x1d988a, component + 4, 4, r(m, component + 4) | 0x4000u);
  return true;
}
} // namespace

void source_physics_body_constraints_1_1(
    M &m, std::uint32_t component, std::uint32_t context,
    const SourcePhysicsBodyGroupingScratch &s, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &estimate, const O &o) {
  constraints(m, component, context, s, fp, estimate, o);
}

void source_physics_body_property_mask_1_1(M &m, std::uint32_t component,
                                           std::uint32_t context,
                                           std::uint32_t output, const O &o) {
  for (std::uint32_t i = 0; si(i) < si(r(m, r(m, context + 0x48) + 0x30));
       ++i) {
    const auto phmo = r(m, context + 0x48), row = r(m, phmo + 0x34) + i * 0x18u;
    const auto bit = 1u << (i & 31u), word = output + (i >> 5) * 4;
    w(m, o, 0x1d9f9e, word, 4, r(m, word) & ~bit);
    bool enabled = r(m, row + 8) == 0;
    for (std::uint32_t k = 0; !enabled && si(k) < si(r(m, row + 8)); ++k) {
      const auto dependency = r(m, row + 0xc) + k * 12u;
      if ((r(m, dependency + 4, 1) & 1u) &&
          !damage_constraint(m, r(m, component + 8), sx(m, dependency, 2),
                             sx(m, dependency + 2, 2), true))
        enabled = true;
    }
    if (enabled)
      w(m, o, 0x1d9ff8, word, 4, r(m, word) | bit);
  }
}
void source_physics_collect_body_groups_1_1(
    M &m, const SourcePhysicsGroupCollectionRequest &in, const O &o) {
  const auto group_count = [&](std::uint32_t group) {
    return r(m, in.groups +
                    (group == 0xffffffffu ? 0x4444u : (group + 1u) * 0x44u));
  };
  const auto group_start = [&](std::uint32_t group) {
    return in.groups + (group == 0xffffffffu ? 0x4404u : 4u + group * 0x44u);
  };
  if (si(group_count(in.group)) <= 0)
    return;
  const auto flags = r(m, in.component + 4);
  for (std::uint32_t i = 0; si(i) < si(group_count(in.group)); ++i) {
    auto index = sx(m, group_start(in.group) + i, 1);
    const auto phmo = r(m, in.context + 0x48),
               row = r(m, phmo + 0x3c) + index * 0x90u;
    bool redirected = false, redirectable = false;
    if (r(m, row + 0x18, 1) & 0x20u) {
      const auto destination = sx(m, row + 0x1c, 2);
      const auto clamped = si(destination) < 0
                               ? 0u
                               : std::min(destination, r(m, phmo + 0x38) - 1u);
      if (clamped == destination) {
        redirectable = true;
        if (!(flags & 0x2000u)) {
          redirected = true;
          index = destination;
        }
      }
    }
    w(m, o, 0x1da0fd, in.indices + r(m, in.count), 1, index);
    w(m, o, 0x1da105, in.count, 4, r(m, in.count) + 1u);
    if (redirected)
      w(m, o, 0x1da10d, in.component + 4, 4,
        r(m, in.component + 4) | 0x100000u);
    if (redirectable)
      w(m, o, 0x1da118, in.component + 4, 4, r(m, in.component + 4) | 0x80000u);
  }
  if (in.group != 0xffffffffu) {
    const auto at = in.visited + (in.group >> 5) * 4;
    w(m, o, 0x1da14a, at, 4, r(m, at) | (1u << (in.group & 31u)));
  }
  for (std::uint32_t i = 0; si(i) < si(r(m, r(m, in.context + 0x48) + 0x30));
       ++i) {
    const auto row = r(m, r(m, in.context + 0x48) + 0x34) + i * 0x18u;
    if (!(r(m, in.properties + (i >> 5) * 4) & (1u << (i & 31u))))
      continue;
    const auto a = sx(m, row + 4, 2), b = sx(m, row + 6, 2);
    if (a != in.group && b != in.group)
      continue;
    const auto other = a == in.group ? b : a;
    auto first = 0xffffffffu;
    if (other != 0xffffffffu && si(group_count(other)) >= 1) {
      first = sx(m, group_start(other), 1);
      if (first != 0xffffffffu &&
          (r(m, in.modes + first, 1) == 6 ||
           (r(m, in.visited + (other >> 5) * 4) & (1u << (other & 31u)))))
        continue;
    }
    const auto unbroken = [&]() {
      const auto datum = r(m, in.component + 8);
      for (std::uint32_t k = 0; si(k) < si(r(m, row + 8)); ++k) {
        const auto dependency = r(m, row + 0xc) + k * 12u,
                   type = sx(m, dependency, 2),
                   index = sx(m, dependency + 2, 2);
        if (!damage_constraint(m, datum, type, index, true) &&
            !damage_constraint(m, datum, type, index, false))
          return true;
      }
      return false;
    };
    if (other == 0xffffffffu) {
      if (unbroken())
        w(m, o, 0x1da35d, in.force_mode7, 1, 1);
      continue;
    }
    bool merge = si(group_count(other)) < 1;
    if (!merge && r(m, row + 8) == 0) {
      const auto current = sx(m, group_start(in.group), 1);
      merge = r(m, in.modes + current, 1) != 7 &&
              r(m, in.modes + sx(m, group_start(other), 1), 1) != 7;
    } else if (!merge)
      merge = unbroken();
    if (merge && first != 0xffffffffu) {
      auto nested = in;
      nested.group = other;
      source_physics_collect_body_groups_1_1(m, nested, o);
    }
  }
}
bool source_physics_body_runtime_context_1_1(M &m, std::uint32_t datum,
                                             std::uint32_t out, FP fp,
                                             const O &o) {
  const auto ob = object(m, datum), definition = tag(m, r(m, ob)),
             model_datum = r(m, definition + 0x38);
  if (model_datum == 0xffffffffu)
    return false;
  const auto model = tag(m, model_datum), phmo_datum = r(m, model + 0x24);
  if (phmo_datum == 0xffffffffu)
    return false;
  const auto phmo = tag(m, phmo_datum), nodes = ob + sx(m, ob + 0x116, 2),
             regions = ob + sx(m, ob + 0x11a, 2);
  w(m, o, 0x20e3e9, out, 4, datum);
  w(m, o, 0x20e3f5, out + 4, 4, r(m, ob + 0xb4));
  root_matrix(m, out + 8, ob + 0x70, ob + 0x7c, ob + 0x64, fp, o);
  if (r(m, ob + 0x14) != 0xffffffffu)
    missing(
        "Required original parent current-node transform owner unavailable");
  w(m, o, 0x20e408, out + 0x3c, 4, r(m, definition + 0x38));
  w(m, o, 0x20e40e, out + 0x44, 4, model);
  w(m, o, 0x20e412, out + 0x4c, 4, regions);
  w(m, o, 0x20e416, out + 0x48, 4, phmo);
  w(m, o, 0x20e419, out + 0x50, 4, nodes);
  w(m, o, 0x20e41c, out + 0x40, 4, r(m, model + 0x24));
  return true;
}
void source_physics_body_region_permutation_1_1(
    M &m, std::uint32_t model_datum, std::uint32_t datum, std::uint32_t region,
    std::uint32_t permutation, std::uint32_t ro, std::uint32_t po, const O &o) {
  const auto model = tag(m, model_datum), ob = object(m, datum);
  const auto count = si(sx(m, ob + 0x118, 2)) / 10;
  const auto stored = ob + sx(m, ob + 0x11a, 2);
  w(m, o, 0x1837bf, ro, 4, 0xffffffffu);
  w(m, o, 0x1837c5, po, 4, 0xffffffffu);
  for (std::uint32_t i = 0; si(i) < si(r(m, model + 0x70)); ++i) {
    const auto row = r(m, model + 0x74) + i * 0x10u;
    if (sx(m, row + 5, 1) != region)
      continue;
    w(m, o, 0x1837fa, ro, 4, i);
    const auto clamp = si(i) < 0 ? 0 : count - 1;
    if (clamp != si(i) && si(i) > clamp)
      return;
    const auto index = sx(m, stored + i, 1);
    if (index == 0xffffffffu)
      return;
    if (sx(m, r(m, row + 0xc) + index * 8u + 6, 1) == permutation)
      w(m, o, 0x183828, po, 4, index);
    return;
  }
}
void source_physics_body_groups_1_1(M &m, std::uint32_t datum,
                                    std::uint32_t out, std::uint32_t bypass,
                                    std::uint32_t ro, std::uint32_t po,
                                    const O &o) {
  const auto ob = object(m, datum), model_datum = r(m, tag(m, r(m, ob)) + 0x38);
  w(m, o, 0x18353e, out, 4, 0);
  if (model_datum == 0xffffffffu)
    return;
  const auto phmod = r(m, tag(m, model_datum) + 0x24);
  if (phmod == 0xffffffffu)
    return;
  const auto phmo = tag(m, phmod);
  w(m, o, 0x183578, out, 4, r(m, phmo + 0xc8));
  for (std::uint32_t i = 0; si(i) < si(r(m, out)); ++i)
    w(m, o, 0x183580, out + 0x44 + i * 0x44u, 4, 0);
  w(m, o, 0x18358c, out + 0x4444, 4, 0);
  for (std::uint32_t i = 0; si(i) < si(r(m, phmo + 0x38)); ++i) {
    const auto row = r(m, phmo + 0x3c) + i * 0x90u, node = sx(m, row, 2);
    const auto group =
        out + (node == 0xffffffffu ? 0x4404u : 4u + node * 0x44u);
    if (!(bypass & 255u)) {
      source_physics_body_region_permutation_1_1(m, model_datum, datum,
                                                 sx(m, row + 2, 2),
                                                 sx(m, row + 4, 2), ro, po, o);
      if (r(m, ro) == 0xffffffffu || r(m, po) == 0xffffffffu)
        continue;
    }
    w(m, o, 0x18360f, group + r(m, group + 0x40), 1, i);
    w(m, o, 0x183612, group + 0x40, 4, r(m, group + 0x40) + 1u);
  }
}
std::uint32_t source_group_physics_bodies_1_1(
    M &m, const SourcePhysicsBodyGroupingRequest &in,
    const SourcePhysicsBodyGroupingScratch &s, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &rsqrt, const O &o,
    const SourcePhysicsBodyGroupingCallObserver &call) {
  const auto p = in.component, f = s.grouping_frame, context = f + 0x38,
             bank = f + 0xa4c;
  auto force = in.force_mode7_word & 255u;
  const auto header = object_header(m, r(m, p + 8)), type = r(m, header + 3, 1);
  w(m, o, 0x1d9363, f + 0x14, 4, 0);
  if (!(r(m, header + 2, 1) & 1u) && !((1u << (type & 31u)) & 128u))
    force = 1;
  if (!source_physics_body_runtime_context_1_1(m, r(m, p + 8), context, fp, o))
    return r(m, f + 0x14);
  source_physics_body_groups_1_1(m, r(m, p + 8), bank, 0, s.region_result,
                                 s.permutation_result, o);
  constexpr std::array sites{0x1d93afu, 0x1d93b3u, 0x1d93b7u, 0x1d93bbu,
                             0x1d93bfu, 0x1d93c3u, 0x1d93c7u, 0x1d93cbu};
  for (unsigned i = 0; i < 8; ++i)
    w(m, o, sites[i], f + 0x18 + i * 4, 4, 0);
  const auto phmo = r(m, context + 0x48);
  for (std::uint32_t i = 0; si(i) < si(r(m, phmo + 0x38)); ++i) {
    const auto row = r(m, phmo + 0x3c) + i * 0x90u, kind = sx(m, row + 0x1a, 2);
    if (kind >= 6)
      missing("Required original authored motion-table class unavailable");
    auto mode = kind + 2u;
    if (force)
      mode = 7;
    if ((in.override_mode6_word & 255u) && mode != 7) {
      const auto node = sx(m, row, 2);
      if (node == 0xffffffffu ||
          !(r(m, r(m, phmo + 0xcc) + node * 12u + 4, 1) & 1u))
        mode = 6;
    }
    w(m, o, 0x1d945a, f + 0x8c + i, 1, mode);
  }
  w(m, o, 0x1d94a6, f + 0x10, 4, 0);
  std::uint32_t count =
      select(m, p, context, f + 0x8c, in.wrap_shape_word, 0xffffffffu, f + 0x10,
             f + 0x18, bank, f + 0xcc, s, fp, rsqrt, o, call)
          ? 1u
          : 0u;
  if (si(r(m, f + 0x10)) >= 0)
    w(m, o, 0x1d94c2, f + 0x14, 4, r(m, f + 0x10));
  for (std::uint32_t i = 0; si(i) < si(r(m, phmo + 0xc8)); ++i) {
    if (r(m, f + 0x18 + (i >> 5) * 4) & (1u << (i & 31)))
      continue;
    w(m, o, 0x1d9525, f + 0x10, 4, 0);
    if (select(m, p, context, f + 0x8c, in.wrap_shape_word, i, f + 0x10,
               f + 0x18, bank, f + 0xcc + count * 0x4cu, s, fp, rsqrt, o, call))
      ++count;
    if (si(r(m, f + 0x14)) <= si(r(m, f + 0x10)))
      w(m, o, 0x1d9544, f + 0x14, 4, r(m, f + 0x10));
  }
  if (si(count) > 0) {
    if (si(r(m, p + 0x78) & 0x7fffffffu) < si(count))
      source_grow_physics_body_bank_1_1(m, p + 0x70, count, 0x60, o);
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto result = f + 0xcc + i * 0x4cu;
      source_commit_physics_body_slot_1_1(m,
                                          {p, r(m, result), result + 4,
                                           r(m, result + 0x44),
                                           r(m, result + 0x48, 1)},
                                          o);
    }
  }
  primary_slot(m, p, context, s, fp, rsqrt, o);
  w(m, o, 0x1d95ac, p + 4, 4, r(m, p + 4) | 0x800u);
  return r(m, f + 0x14);
}
} // namespace halo2
