#include "halo2/biped_world_insertion.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using S = SourceBipedWorldInsertionServices;
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
       std::uint32_t v) {
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
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::uint32_t required(M &m, const S &s, std::uint32_t site,
                       std::uint32_t target,
                       std::optional<std::uint32_t> receiver, std::uint32_t esp,
                       std::vector<std::uint32_t> args, const O &o) {
  if (!s.lower.required_owner)
    throw std::runtime_error(
        "Required original biped insertion owner unavailable at " +
        std::to_string(target));
  return s.lower.required_owner(
      m, {site, target, esp, receiver, std::move(args)}, o);
}
std::uint32_t object(M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12u + 8u);
}
std::uint32_t tag(M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 0xffffu) * 16u + 8u);
}
std::uint32_t kind(M &m, std::uint32_t body, const S &s, std::uint32_t esp,
                   const O &o) {
  const auto motion = r(m, body + 0x3c), target = r(m, r(m, motion) + 0x18);
  if (target == 0x9d900)
    return 4;
  if (target == 0x9b560)
    return 5;
  if (target == 0x31b260)
    return 6;
  if (target == 0x31b250)
    return 7;
  return required(m, s, 0x1d4fcb, target, motion, esp, {}, o);
}
void listener(M &m, std::uint32_t body, std::uint32_t value, bool contact,
              const O &o) {
  const auto d = body + (contact ? 0x60u : 0x70u);
  if (r(m, d + 4) == (r(m, d + 8) & 0x7fffffffu))
    source_double_physics_body_bank_1_1(m, d, 4, o);
  w(m, o, contact ? 0x3123c7 : 0x312387, r(m, d) + r(m, d + 4) * 4u, 4, value);
  w(m, o, contact ? 0x3123ca : 0x31238a, d + 4, 4, r(m, d + 4) + 1u);
}
void release(M &m, std::uint32_t p, std::uint32_t site, std::uint32_t resident,
             const S &s, std::uint32_t esp, const O &o) {
  w(m, o, site, p + 6, 2, r(m, p + 6, 2) - 1u);
  if (r(m, p + 6, 2))
    return;
  if (source_physics_body_reference_is_resident_1_1(m, p))
    w(m, o, resident, p + 6, 2, 1);
  else {
    const auto target = r(m, r(m, p));
    if (target == 0x1d53c0 || target == 0x31ac60) {
      w(m, o, target == 0x1d53c0 ? 0x1d53cb : 0xdd97b, p, 4, 0x459d98);
      if (r(m, p + 4, 2) & 0x8000u) {
        w(m, o, target == 0x1d53c0 ? 0x1d53da : 0xdd98a, p + 4, 2,
          r(m, p + 4, 2) & 0x7fffu);
        if (!s.disposal.clear_reference_flag)
          throw std::runtime_error(
              "Required original auxiliary flag owner unavailable");
        s.disposal.clear_reference_flag(m, p, o);
      }
      w(m, o, target == 0x1d53c0 ? 0x1d53e8 : 0xdd993, p, 4, 0x459d94);
      const auto adapter = r(m, 0x487638), backend = r(m, 0x480da0),
                 bytes = r(m, p + 4, 2);
      if (r(m, r(m, adapter) + 0x14) != 0x148a70 ||
          r(m, r(m, backend) + 0x14) != 0x22eb20 ||
          si(p) < si(backend + 0x4340u) || si(p) > si(backend + 0xf0340u))
        throw std::runtime_error(
            "Required original auxiliary free owner unavailable");
      if (s.disposal.observe_free)
        s.disposal.observe_free({target == 0x1d53c0 ? 0x1d5400u : 0x31ac7fu,
                                 adapter, p, bytes, 0x26});
      source_free_physics_body_small_block_1_1(m, backend, p, bytes, o);
    } else
      required(m, s, site == 0x1d5148 ? 0x1d5174 : 0x1d519f, target, p, esp,
               {1}, o);
  }
}
struct Scope {
  std::fenv_t env;
  unsigned mx;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original insertion unmasked FP effects unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
} // namespace
void source_construct_biped_body_auxiliary_1_1(M &m, std::uint32_t aux,
                                               std::uint32_t body,
                                               std::uint32_t datum,
                                               std::uint32_t bounds, FP fp,
                                               const O &o) {
  source_construct_physics_body_auxiliary_1_1(m, aux, body, bounds, fp, o);
  w(m, o, 0x1d521f, aux + 0x60, 4, datum);
  w(m, o, 0x1d5222, aux, 4, 0x45cad0);
  for (std::uint32_t i = 0; si(i) < si(r(m, body + 0x34)); ++i)
    if (r(m, r(m, body + 0x30) + i * 8u) == 0x2002) {
      w(m, o, 0x1d5256, aux + 0x64, 4, r(m, r(m, body + 0x30) + i * 8u + 4));
      return;
    }
  w(m, o, 0x1d5249, aux + 0x64, 4, 0);
}
void source_insert_biped_world_broadphase_1_1(M &m, std::uint32_t world,
                                              std::uint32_t body,
                                              std::uint32_t esp, FP fp,
                                              const S &s, const O &o) {
  const auto frame = ((esp - 4u) & ~15u) - 0x80u;
  auto pairs = s.pairs;
  pairs.scratch = s.scratch;
  pairs.floating_point = fp;
  if (!pairs.required_owner)
    pairs.required_owner = [&](M &memory,
                               const SourcePhysicsPairOwnerCall &call,
                               const O &observer) {
      return required(memory, s, 0, call.entry,
                      call.cdecl ? std::nullopt : std::optional(call.receiver),
                      0, call.arguments, observer);
    };
  SourcePhysicsWorldPairDeliveryServices delivery;
  delivery.static_pairs = [&](M &memory, std::uint32_t manager,
                              const std::array<std::uint32_t, 5> &arguments,
                              const O &observer) {
    source_deliver_physics_pairs_1_1(memory, manager, arguments, {frame - 24u},
                                     pairs, observer);
  };
  delivery.dynamic_pairs = [&](M &memory, std::uint32_t island,
                               const std::array<std::uint32_t, 4> &arguments,
                               const O &observer) {
    source_update_physics_island_pairs_1_1(memory, island, arguments,
                                           frame - 64u, pairs, observer);
  };
  source_insert_physics_world_broadphase_1_1(
      m, world, body, {frame, {frame - 0x70u}}, fp, s.scratch, delivery, o);
}
void source_register_biped_world_body_1_1(M &m, std::uint32_t body,
                                          std::uint8_t inactive,
                                          std::uint32_t esp, FP fp, const S &s,
                                          const O &o) {
  if (si(r(m, 0x486558)) >= 0x352)
    throw std::runtime_error(
        "Required original1C8AF0 body-budget eviction owner unavailable");
  w(m, o, 0x1c736a, 0x486558, 4, r(m, 0x486558) + 1u);
  SourcePhysicsWorldInsertionServices world;
  world.broadphase = [&](M &memory, std::uint32_t wld, std::uint32_t b,
                         const O &obs) {
    if (s.broadphase)
      s.broadphase(memory, wld, b, esp - 32u, obs);
    else
      source_insert_biped_world_broadphase_1_1(memory, wld, b, esp - 32u, fp, s,
                                               obs);
  };
  world.world_listener = [&](M &memory, std::uint32_t target,
                             std::uint32_t receiver, std::uint32_t b,
                             const O &obs) {
    required(memory, s, 0x31e62e, target, receiver, esp - 56u, {b}, obs);
  };
  source_insert_physics_world_body_1_1(m, r(m, 0x526234), body, world, o);
  if (inactive && r(m, r(m, body + 0x44) + 0x40) == 1)
    required(m, s, 0x1c7388, 0x3121f0, body, esp - 8u, {}, o);
  if (r(m, 0x5660b0, 1) && !r(m, 0x486563, 1))
    required(m, s, 0x1c738d, 0x27f190, std::nullopt, esp - 8u, {}, o);
}
void source_insert_biped_component_world_1_1(M &m, std::uint32_t p,
                                             std::uint32_t esp, FP fp,
                                             const S &s, const O &o) {
  const auto frame = ((esp - 4u) & ~15u) - 0x30u;
  for (const auto offset : {0x9cu, 0x98u})
    if (const auto phantom = r(m, p + offset))
      required(m, s, offset == 0x9c ? 0x1d4f51 : 0x1d4f67, 0x314a00,
               r(m, 0x526234), frame - 8u, {phantom}, o);
  const auto unit = object(m, r(m, p + 8));
  for (std::uint32_t i = 0; si(i) < si(r(m, p + 0x74)); ++i) {
    const auto body = r(m, r(m, p + 0x70) + i * 0x60u + 0x40);
    listener(m, body, r(m, 0x526538), false, o);
    if ((r(m, p + 4, 1) & 1u) && !r(m, body + 0x40, 1) &&
        kind(m, body, s, frame - 4u, o) != 6) {
      const auto v = r(m, 0x526538);
      listener(m, body, v ? v + 4u : 0u, true, o);
      w(m, o, 0x1d4feb, body + 0x5c, 2, 0);
    }
    source_register_biped_world_body_1_1(
        m, body, std::uint8_t(r(m, unit + 0xc1, 1) & 1u), frame - 12u, fp, s,
        o);
    source_biped_body_velocity_1_1(m, p, i, unit + 0x88, frame - 8u, s.velocity,
                                   o);
    source_biped_body_angular_velocity_1_1(m, p, i, unit + 0x94, frame - 8u,
                                           s.velocity, o);
    const auto adapter = r(m, 0x487638), backend = r(m, 0x480da0);
    if (r(m, r(m, adapter) + 0x10) != 0x148a10 || r(m, 0x480d98) != 2 ||
        r(m, r(m, backend) + 0x10) != 0x22e950)
      throw std::runtime_error(
          "Required original biped auxiliary allocator unavailable");
    const auto aux =
        source_allocate_physics_body_small_block_1_1(m, backend, 0x70, o);
    if (!aux)
      throw std::runtime_error(
          "Required original biped auxiliary fallback allocator unavailable");
    w(m, o, 0x1d5037, aux + 4, 2, 0x70);
    source_construct_biped_body_auxiliary_1_1(m, aux, body, r(m, p + 8),
                                              frame - 0x70u, fp, o);
    if (r(m, p + 4) & 0x800u) {
      const auto object_definition = tag(m, r(m, object(m, r(m, p + 8))));
      const auto model = tag(m, r(m, object_definition + 0x38));
      const auto definition = tag(m, r(m, model + 0x24));
      Scope scope(fp);
      const auto a = std::bit_cast<float>(r(m, definition + 8)),
                 b = std::bit_cast<float>(r(m, definition + 12)),
                 one = std::bit_cast<float>(r(m, 0x465030));
      if (a != one || b != one) {
        const auto temp = frame + 0x1c;
        const std::array<std::uint32_t, 5> defaults{
            0xbf800000, 0x3c23d70a, 0x3ba3d70a, 0x3dcccccd, 0x3e4ccccd};
        for (unsigned j = 0; j < 5; ++j)
          w(m, o,
            std::array{0x31a8a8u, 0x31a8aeu, 0x31a8b5u, 0x31a8bcu,
                       0x31a8c3u}[j],
            temp + j * 4u, 4, defaults[j]);
        for (unsigned j = 0; j < 4; ++j)
          w(m, o, std::array{0x31a9d7u, 0x31a9ddu, 0x31a9e3u, 0x31a9e9u}[j],
            temp + 4u + j * 4u, 4, r(m, aux + 0x50u + j * 4u));
        w(m, o, 0x31a9ef, temp, 4, r(m, aux + 8));
        for (unsigned j = 0; j < 4; ++j) {
          const auto v = _mm_mul_ss(
              _mm_set_ss(std::bit_cast<float>(r(m, temp + 4u + j * 4u))),
              _mm_set_ss(j < 2 ? a : b));
          w(m, o, std::array{0x1d50f0u, 0x1d5101u, 0x1d5112u, 0x1d512au}[j],
            temp + 4u + j * 4u, 4,
            std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v)));
        }
        for (unsigned j = 0; j < 4; ++j)
          w(m, o, std::array{0x31aa07u, 0x31aa0du, 0x31aa13u, 0x31aa19u}[j],
            aux + 0x50u + j * 4u, 4, r(m, temp + 4u + j * 4u));
        w(m, o, 0x31aa1e, aux + 8, 4, r(m, temp));
      }
    }
    w(m, o, 0x1d5139, aux + 6, 2, r(m, aux + 6, 2) + 1u);
    const auto previous = r(m, body + 0x54);
    if (previous)
      release(m, previous, 0x1d5148, 0x1d5164, s, frame - 8u, o);
    w(m, o, 0x1d5176, body + 0x54, 4, aux);
    release(m, aux, 0x1d5179, 0x1d5191, s, frame - 8u, o);
  }
  for (std::uint32_t i = 0; si(i) < si(r(m, p + 0x80)); ++i)
    source_insert_physics_constraint_into_world_1_1(
        m, r(m, 0x526234), r(m, r(m, p + 0x7c) + i * 12u + 8u), frame - 8u,
        s.lower, o);
  w(m, o, 0x1d51fa, p + 4, 4, (r(m, p + 4) & ~8u) | 0x20u);
}
} // namespace halo2
