#include "halo2/object_current_nodes.hpp"
#include "halo2/item_transform_math.hpp"
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
    v |= std::to_integer<unsigned>(b[i]) << (8 * i);
  return v;
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::int32_t sx(const M &m, std::uint32_t p, unsigned n = 2) {
  const auto v = r(m, p, n);
  return n == 1 ? std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))
                : std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v)));
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t v,
       unsigned n = 4) {
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
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 65535u) * 12 + 8);
}
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 65535u) * 16 + 8);
}
__m128 load(const M &m, std::uint32_t p) {
  return _mm_set_ss(std::bit_cast<float>(r(m, p)));
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original current-node FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original current-node x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
} // namespace
PhysicsNodeDeliveryScratch
source_physics_node_delivery_scratch_1_1(std::uint32_t esp) {
  const auto frame = ((esp - 4u) & ~15u) - 0x190u;
  // 1D42D0 is called directly at frame-4, then saves EBP and aligns.
  return {frame + 0x3c, ((frame - 8u) & ~15u) - 0x40u};
}
std::uint32_t source_host_x86_rcpss_estimate(std::uint32_t input) {
  return std::bit_cast<std::uint32_t>(
      _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(std::bit_cast<float>(input)))));
}
std::uint32_t source_biped_physical_height_1_1(const M &m, std::uint32_t d,
                                               FP fp) {
  Scope scope(fp);
  const auto p = object(m, d);
  auto value = _mm_setzero_ps();
  if (r(m, p + 0x34b, 1) == 1 && r(m, p + 0x3dc, 1) == 3) {
    const auto start = r(m, p + 0x448);
    if (start != 0xffffffffu) {
      const auto clock = r(m, 0x518444);
      const auto one = load(m, 0x465030);
      value = _mm_cvtsi32_ss(_mm_setzero_ps(), si(r(m, clock + 8) - start));
      value = _mm_mul_ss(value, load(m, clock + 4));
      value = _mm_mul_ss(value, load(m, 0x465274));
      value = _mm_sub_ss(one, value);
      if (_mm_comigt_ss(_mm_setzero_ps(), value))
        value = _mm_setzero_ps();
      else if (_mm_comigt_ss(value, one))
        value = one;
    }
    value = _mm_mul_ss(value, load(m, 0x465370));
  }
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));
}
void source_update_object_current_nodes_1_1(
    M &m, std::uint32_t d, std::uint32_t esp, FP fp,
    const SourceObjectCurrentNodeServices &s, const O &o) {
  Scope scope(fp);
  const auto frame = esp - 0xf8u, p = object(m, d),
             definition = tag(m, r(m, p));
  const auto current = p + std::uint32_t(sx(m, p + 0x116));
  const auto count = std::uint32_t(sx(m, p + 0x114)) / 0x34u;
  w(m, o, 0xbdfd8, p + 4, r(m, p + 4) & 0xbfffffffu);
  const bool biped = (r(m, definition, 1) & 31u) == 0;
  if (biped && r(m, object(m, d) + 0x34b, 1) == 2)
    return;
  const auto mask = frame + 0x70;
  const auto words = si(count + 31u) >> 5;
  for (std::int32_t i = 0; i < words; ++i)
    w(m, o, 0xbe02c, mask + std::uint32_t(i) * 4, 0);
  if (r(m, p + 0xb4) != 0xffffffffu && (r(m, p + 0xc0, 1) & 0x40u)) {
    const auto component =
        r(m, r(m, 0x526248) + 0x44) + (r(m, p + 0xb4) & 65535u) * 0xa0u;
    const auto height = biped ? source_biped_physical_height_1_1(m, d, fp) : 0u;
    const auto entry = frame - 16;
    SourceUnitAnimationRuntimeServices runtime;
    runtime.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &observer) {
      if(call.instruction==0x1d7476u&&s.physical.motion_type) {
        abi.gp[0]=s.physical.motion_type(abi.gp[1],call.target);abi.gp[4]+=4;return;
      }
      if(s.physical.runtime_owner) {
        s.physical.runtime_owner(memory,call,abi,observer);return;
      }
      throw std::runtime_error("Required original physical-node virtual owner unavailable");
    };
    source_call_unit_animation_pose_1_1(m,
        {.entry=0x1d7310,.entry_esp=entry,.arguments={component,mask,height}},
        fp,runtime,o);
  }
  const auto actual = object(m, d);
  const bool local_owned = sx(m, actual + 0x112) != -1;
  const auto local =
      local_owned ? p + std::uint32_t(sx(m, p + 0x112)) : 0x4e38c0u;
  auto invoke = [&](SourceBipedPhaseCall c, std::optional<std::uint8_t> bl = {},
                    std::optional<std::uint8_t> dl = {}) {
    const auto deliver = [&](std::uint32_t output,
                             const SourceMatrixMathStore &event) {
      const auto at = output + std::uint32_t(event.offset);
      SourcePhysicsContactRecordStore store{
          event.instruction, at, {}, event.after};
      store.before.resize(event.after.size());
      m.read(at, store.before);
      m.write(at, store.written);
      if (o)
        o(store);
    };
    if(c.entry==0x10d260)
      return source_call_unit_animation_pose_1_1(m,c,fp,s.animation.value_or(SourceUnitAnimationRuntimeServices{}),o);
    if (s.animation && (c.entry == 0xbea50 || c.entry == 0x1c9e30))
      return source_call_unit_animation_pose_1_1(m, c, fp, *s.animation, o);
    if (c.entry == 0x143960 && s.reciprocal) {
      const auto output = c.arguments[0], input = c.arguments[1];
      if (!(r(m, 0x566764, 1) & 1u)) {
        w(m, o, 0x143982, 0x566764, r(m, 0x566764) | 1u);
        w(m, o, 0x143988, 0x566760, r(m, 0x46504c));
      }
      const auto q = _mm_set_ps(std::bit_cast<float>(r(m, input + 12)),
                                std::bit_cast<float>(r(m, input + 8)),
                                std::bit_cast<float>(r(m, input + 4)),
                                std::bit_cast<float>(r(m, input)));
      auto squared = _mm_mul_ps(q, q);
      squared = _mm_add_ps(squared, _mm_movehl_ps(squared, squared));
      const auto norm =
          _mm_add_ss(squared, _mm_shuffle_ps(squared, squared, 0x55));
      if (!_mm_comigt_ss(norm, load(m, 0x566760))) {
        const auto one = r(m, 0x465030);
        const auto pair_store = [&](std::uint32_t site, std::uint32_t at,
                                    std::uint32_t first) {
          SourcePhysicsContactRecordStore store{
              site, at, {}, std::vector<std::byte>(8)};
          store.before.resize(8);
          m.read(at, store.before);
          for (unsigned i = 0; i < 4; ++i)
            store.written[i] = std::byte(first >> (8 * i));
          m.write(at, store.written);
          if (o)
            o(store);
        };
        pair_store(0x1439c5, output + 4, one);
        pair_store(0x1439c9, output + 12, 0);
        pair_store(0x1439cd, output + 20, one);
        pair_store(0x1439d1, output + 28, 0);
        w(m, o, 0x1439d5, output + 36, one);
        w(m, o, 0x143aa7, output, r(m, input + 28));
        std::array<std::byte, 8> position{};
        m.read(input + 16, position);
        SourcePhysicsContactRecordStore store{
            0x143aaf,
            output + 40,
            {},
            std::vector<std::byte>(position.begin(), position.end())};
        store.before.resize(8);
        m.read(output + 40, store.before);
        m.write(output + 40, store.written);
        if (o)
          o(store);
        w(m, o, 0x143ab8, output + 48, r(m, input + 24));
      } else {
        std::array<std::byte, 32> node{};
        std::array<std::byte, 0x34> out{};
        std::array<std::byte, 8> lazy{};
        m.read(input, node);
        m.read(output, out);
        m.read(0x566760, std::span(lazy).first(4));
        lazy[4] = std::byte{1};
        std::array<std::uint32_t, 4> sign{};
        for (unsigned i = 0; i < 4; ++i)
          sign[i] = r(m, 0x45a8c0 + i * 4);
        const SourceNodeMatrixInputs inputs{
            std::bit_cast<float>(r(m, 0x465030)), sign, lazy, [&](float value) {
              return std::bit_cast<float>(
                  s.reciprocal(std::bit_cast<std::uint32_t>(value)));
            }};
        source_item_node_matrix(node, out, inputs, [&](const auto &event) {
          deliver(output, event);
        });
      }
      return std::uint32_t(0); // The caller consumes no143960 return value.
    }
    if (c.entry == 0xbeb30 && !c.arguments[3] && !(c.arguments[2] & 255u)) {
      std::array<std::byte, 12> up{}, forward{}, point{};
      std::array<std::byte, 0x34> out{};
      m.read(*c.eax, up);
      m.read(*c.ecx, forward);
      m.read(c.arguments[0], point);
      m.read(c.arguments[4], out);
      source_item_root_matrix(
          up, forward, point, std::bit_cast<float>(c.arguments[1]),
          c.arguments[2], std::bit_cast<float>(r(m, 0x465030)), out,
          [&](const auto &event) { deliver(c.arguments[4], event); });
      return std::uint32_t(0);
    }
    if (c.entry == 0x1441d0) {
      std::array<std::byte, 0x34> left{}, right{}, out{};
      m.read(*c.ecx, left);
      m.read(*c.edx, right);
      m.read(c.arguments[0], out);
      source_compose_item_matrix(
          left, right, out, [&](const SourceMatrixMathStore &event) {
            const auto at = c.arguments[0] + std::uint32_t(event.offset);
            SourcePhysicsContactRecordStore store{
                event.instruction, at, {}, event.after};
            store.before.resize(event.after.size());
            m.read(at, store.before);
            m.write(at, store.written);
            if (o)
              o(store);
          });
      return std::uint32_t(0); // BDF80 does not consume this owner's EAX.
    }
    if (!s.required_owner)
      throw std::runtime_error(
          "Required original object current-node callee unavailable");
    return s.required_owner(m, {std::move(c), bl, dl}, o);
  };
  const auto model_datum = r(m, definition + 0x38);
  const auto model = model_datum == 0xffffffffu ? 0u : tag(m, model_datum);
  if (model && r(m, model + 4) != 0xffffffffu) {
    const auto render = tag(m, r(m, model + 4));
    std::uint32_t parent_matrix = 0;
    std::uint8_t parent_handedness = 0;
    if (r(m, p + 0x14) != 0xffffffffu) {
      const auto parent = object(m, r(m, p + 0x14));
      parent_matrix = parent + std::uint32_t(sx(m, parent + 0x116)) +
                      std::uint32_t(sx(m, p + 0x18, 1)) * 0x34u;
      parent_handedness = std::uint8_t((r(m, parent + 4) >> 10) & 1u);
    }
    bool root_local = false;
    if (!(r(m, actual + 4) & 0x20000000u) &&
        (!biped || !r(m, actual + 0x34b, 1))) {
      constexpr std::array<std::uint32_t, 8> sites{0x16f3ef, 0x16f3f4, 0x16f3fa,
                                                   0x16f400, 0x16f40b, 0x16f410,
                                                   0x16f416, 0x16f41d};
      for (std::int32_t node = 0; node < si(r(m, render + 0x48)); ++node) {
        const auto at = r(m, render + 0x4c) + std::uint32_t(node) * 0x60u,
                   out = local + std::uint32_t(node) * 32u;
        for (unsigned i = 0; i < 8; ++i)
          w(m, o, sites[i], out + i * 4,
            i < 4   ? r(m, at + 0x18 + i * 4)
            : i < 7 ? r(m, at + 0xc + (i - 4) * 4)
                    : r(m, 0x465030));
      }
      if (sx(m, object(m, d) + 0x12a) != -1 &&
          (d == 0xffffffffu || !r(m, object(m, d) + 0xb3, 1))) {
        const auto manager = p + std::uint32_t(sx(m, p + 0x12a));
        if (r(m, manager + 0x68) != 0xffffffffu) {
          if (!(r(m, 0x566c8c, 1) & 1u))
            w(m, o, 0xbe1da, 0x566c8c, r(m, 0x566c8c) | 1u);
          std::uint32_t blend_mask = 0;
          bool empty_mask = true;
          for (unsigned i = 0; i < 8; ++i)
            empty_mask &= r(m, model + 0xa4 + i * 4) == 0;
          if (local_owned && !empty_mask) {
            for (unsigned i = 0; i < 8; ++i)
              w(m, o, 0xbe211, 0x566c6c + i * 4, r(m, model + 0xa4 + i * 4));
            constexpr std::array<std::uint32_t, 8> inverse_sites{
                0xc0d37, 0xc0d3e, 0xc0d46, 0xc0d4e,
                0xc0d56, 0xc0d5e, 0xc0d66, 0xc0d73};
            for (unsigned i = 0; i < 8; ++i)
              w(m, o, inverse_sites[i], 0x566c6c + i * 4,
                ~r(m, 0x566c6c + i * 4));
            w(m, o, 0xc0d76, 0x566c88, r(m, 0x566c88) & 255u);
            const auto last = (si(count + 31u) >> 5) - 1;
            for (auto i = last + 1; i < 8; ++i)
              w(m, o, 0xc0d9f, 0x566c6c + std::uint32_t(i) * 4, 0);
            const auto bits = count & 31u,
                       limit = bits ? 0xffffffffu >> (32u - bits) : 0xffffffffu;
            w(m, o, 0xc0db7, 0x566c6c + std::uint32_t(last) * 4,
              r(m, 0x566c6c + std::uint32_t(last) * 4) & limit);
            blend_mask = 0x566c6c;
          }
          invoke({.entry = 0xbea50,
                  .entry_esp = frame - 16,
                  .eax = count,
                  .ecx = blend_mask,
                  .edx = d,
                  .arguments = {render, manager, local}});
          // 194CE0 modifies the selected bit from actual DL, not a whole
          // register.
          w(m, o, blend_mask ? 0x194ceb : 0x194cf6, p + 4,
            blend_mask ? r(m, p + 4) | 0x40000000u
                       : r(m, p + 4) & ~0x40000000u);
          if (r(m, manager + 0x68) != 0xffffffffu &&
              r(m, manager) != 0xffffffffu && sx(m, manager + 6) != -1) {
            const auto selected = invoke(
                {.entry = 0x1c9e30, .entry_esp = frame - 4, .ecx = manager});
            root_local = (r(m, selected + 0x16, 1) >> 1) & 1u;
          }
        }
      }
    }
    if (!r(m, render + 0x48)) {
      const auto defaults = r(m, 0x46fc78);
      for (unsigned i = 0; i < 8; ++i)
        w(m, o, 0xbe297, local + i * 4, r(m, defaults + i * 4));
    }
    if (!(r(m, mask) & 1u)) {
      if (root_local)
        invoke({.entry = 0x143960,
                .entry_esp = frame - 12,
                .arguments = {current, local}});
      else {
        constexpr std::array<std::uint32_t, 3> ps{0xbe2cb, 0xbe2cf, 0xbe2dc},
            fs{0xbe2d8, 0xbe2e9, 0xbe2ef}, us{0xbe304, 0xbe308, 0xbe2f9};
        for (unsigned i = 0; i < 3; ++i) {
          w(m, o, ps[i], frame + 0x44 + i * 4, r(m, p + 0x64 + i * 4));
          w(m, o, fs[i], frame + 0x50 + i * 4, r(m, p + 0x70 + i * 4));
          w(m, o, us[i], frame + 0x38 + i * 4, r(m, p + 0x7c + i * 4));
        }
        if (r(m, p + 0xd4) != 0xffffffffu && (r(m, p + 0xd8, 1) & 1u)) {
          if (invoke({.entry = 0xaa690,
                      .entry_esp = frame - 4,
                      .ecx = d,
                      .esi = frame + 0x2c}) &
              255u)
            for (unsigned i = 0; i < 3; ++i)
              w(m, o, 0xbe337 + i * 4, frame + 0x44 + i * 4,
                r(m, frame + 0x2c + i * 4));
          if (invoke({.entry = 0xaa720,
                      .entry_esp = frame - 4,
                      .ecx = d,
                      .esi = frame + 0x64,
                      .edi = frame + 0x2c}) &
              255u) {
            for (unsigned i = 0; i < 3; ++i) {
              w(m, o, 0xbe367 + i * 8, frame + 0x50 + i * 4,
                r(m, frame + 0x2c + i * 4));
              w(m, o, 0xbe37f + i * 4, frame + 0x38 + i * 4,
                r(m, frame + 0x64 + i * 4));
            }
          }
        }
        invoke({.entry = 0xbeb30,
                .entry_esp = frame - 24,
                .eax = frame + 0x38,
                .ecx = frame + 0x50,
                .arguments = {frame + 0x44, r(m, p + 0xa0),
                              (r(m, p + 4) >> 10) & 0xffffff01u, parent_matrix,
                              frame + 0xc4}},
               parent_handedness);
        invoke({.entry = 0x143960,
                .entry_esp = frame - 12,
                .arguments = {frame + 0x90, local}});
        invoke({.entry = 0x1441d0,
                .entry_esp = frame - 8,
                .ecx = frame + 0xc4,
                .edx = frame + 0x90,
                .arguments = {current}});
      }
      invoke({.entry = 0x10d260,
              .entry_esp = frame - 8,
              .ebx = d,
              .arguments = {current}});
    }
    const auto nodes = si(r(m, render + 0x48));
    for (std::int32_t i = 1; i < nodes; ++i) {
      const auto index = std::uint32_t(i), out = current + index * 0x34u;
      if (r(m, mask + (index >> 5) * 4) & (1u << (index & 31)))
        continue;
      invoke({.entry = 0x143960,
              .entry_esp = frame - 12,
              .arguments = {out, local + index * 0x20u}});
      const auto parent = sx(m, r(m, render + 0x4c) + index * 0x60u + 4);
      invoke({.entry = 0x1441d0,
              .entry_esp = frame - 8,
              .ecx = current + std::uint32_t(parent) * 0x34u,
              .edx = out,
              .arguments = {out}});
    }
    return;
  }
  std::uint32_t parent_matrix = 0;
  std::uint8_t parent_handedness = 0;
  if (r(m, p + 0x14) != 0xffffffffu) {
    const auto parent = object(m, r(m, p + 0x14));
    parent_matrix = parent + std::uint32_t(sx(m, parent + 0x116)) +
                    std::uint32_t(sx(m, p + 0x18, 1)) * 0x34u;
    parent_handedness = std::uint8_t((r(m, parent + 4) >> 10) & 1u);
  }
  invoke({.entry = 0xbeb30,
          .entry_esp = frame - 24,
          .eax = p + 0x7c,
          .ecx = p + 0x70,
          .arguments = {p + 0x64, r(m, p + 0xa0),
                        (r(m, p + 4) >> 10) & 0xffffff01u, parent_matrix,
                        current}},
         parent_handedness);
}
} // namespace halo2
