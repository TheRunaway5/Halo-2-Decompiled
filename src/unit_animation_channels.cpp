#include "halo2/unit_animation_runtime.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <set>
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
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t v,
       unsigned n = 4) {
  SourcePhysicsContactRecordStore e{site, p, {}, {}};
  e.before.resize(n);
  e.written.resize(n);
  m.read(p, e.before);
  for (unsigned i = 0; i < n; ++i)
    e.written[i] = std::byte(v >> (8 * i));
  m.write(p, e.written);
  if (o)
    o(e);
}
std::int32_t sx(const M &m, std::uint32_t p) {
  return std::bit_cast<std::int16_t>(std::uint16_t(r(m, p, 2)));
}
__m128 load(const M &m, std::uint32_t p) {
  return _mm_set_ss(std::bit_cast<float>(r(m, p)));
}
std::uint32_t word(__m128 v) {
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));
}
bool below(__m128 a, __m128 b) {
  // Source JB consumes COMISS carry, which is also set for unordered input.
  // The C++ COMILT intrinsic may lower to reversed COMISS/SETA and exclude
  // unordered input; it cannot represent this original branch.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned char carry;
  asm volatile("comiss %2,%1\n\tsetb %0" : "=qm"(carry) : "x"(a), "x"(b)
               : "cc");
  return carry;
#else
  throw std::runtime_error("Original animation COMISS flags unavailable");
#endif
}
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 65535u) * 16 + 8);
}
bool active(const M &m, std::uint32_t p) {
  return r(m, p) != 0xffffffffu && sx(m, p + 6) != -1;
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original animation clock FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original animation clock x87 owner unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t call(M &m, const SourceRuntimeOwner &owner, SourceRuntimeCall q,
                   const O &o) {
  if (q.entry == 0x1ceda0 || q.entry == 0x1cefe0) {
    FP fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error("Required original animation FP backend unavailable");
#endif
    return source_call_unit_animation_selection_1_1(m, q, fp, owner, o);
  }
  if (!owner)
    throw std::runtime_error(
        "Required original animation channel owner unavailable");
  return owner(m, q, o);
}
void layers(M &m, std::uint32_t p, const O &o) {
  auto primary = r(m, p + 0x12, 2), secondary = r(m, p + 0x32, 2);
  const bool lower = !r(m, 0x4770dc, 1) || !active(m, p + 0x20) ||
                     _mm_comigt_ss(load(m, 0x465290), load(m, p + 0x80));
  if (!lower && !_mm_comigt_ss(load(m, p + 0x80), load(m, 0x465194)))
    return;
  if (lower) {
    w(m, o, 0x1cf0ca, p + 0x6c, r(m, p + 0x6c, 1) & 0xdfu, 1);
    primary &= 0xfffffc7fu;
    secondary |= 0x380u;
  } else {
    w(m, o, 0x1cf0b6, p + 0x6c, r(m, p + 0x6c, 1) | 0x20u, 1);
    primary |= 0x380u;
    secondary &= 0xfffffc7fu;
  }
  if (active(m, p))
    w(m, o, 0x1cf0e8, p + 0x12, primary, 2);
  if (active(m, p + 0x20))
    w(m, o, 0x1cf0f7, p + 0x32, secondary, 2);
}
} // namespace
void source_demand_unit_animation_1_1(M &m, std::uint32_t graph,
                                      std::uint32_t packed, std::uint32_t esp,
                                      const SourceRuntimeOwner &owner,
                                      const O &o) {
  if (!r(m, 0x518410, 1) || !r(m, 0x518411, 1))
    return;
  const auto index = std::bit_cast<std::int16_t>(std::uint16_t(packed >> 16));
  if (index == -1)
    return;
  const auto inherited = std::bit_cast<std::int16_t>(std::uint16_t(packed));
  if (inherited != -1) {
    auto next = 0u;
    if (inherited < std::bit_cast<std::int32_t>(r(m, graph + 0x4c))) {
      const auto datum =
          r(m, r(m, graph + 0x50) + std::uint32_t(inherited) * 32 + 4);
      if (datum != 0xffffffffu)
        next = tag(m, datum);
    }
    graph = next;
  }
  const auto definitions = r(m, graph + 0x30);
  const auto selected = definitions + std::uint32_t(std::int32_t(index)) * 0x6c;
  auto base = selected;
  const auto link = sx(m, selected + 0x30);
  if (link != -1)
    base = definitions + std::uint32_t(link) * 0x6c;
  auto demand = [&](std::uint32_t definition) {
    if (!(r(m, definition + 0x16, 1) & 0x10u))
      return;
    const auto descriptor = r(m, graph + 0xb0) + r(m, definition + 0x24) * 20;
    if (r(m, descriptor + 0x10, 1))
      call(m, owner,
           {.entry = 0x124e40,
            .entry_esp = esp - 12,
            .eax = descriptor,
            .cl = 1u},
           o);
  };
  // The1E1230 tail call still demands the originally selected definition;
  // only the subsequent variant walk starts from its optional base link.
  demand(selected);
  std::set<std::uint32_t> seen{base};
  while (sx(m, base + 0x32) != -1) {
    base = definitions + std::uint32_t(sx(m, base + 0x32)) * 0x6c;
    if (!seen.insert(base).second)
      throw std::runtime_error("Cyclic original animation resource links");
    demand(base);
  }
}
std::uint32_t source_traverse_unit_animation_events_1_1(
    M &m, std::uint32_t graph, std::uint32_t definition, std::uint32_t options,
    std::uint32_t from_word, std::uint32_t to_word, std::uint32_t callback,
    std::uint32_t context, std::uint32_t esp, FP fp,
    const SourceRuntimeOwner &owner, const O &o) {
  Scope scope(fp);
  const auto frame = esp - 0x34;
  auto from = _mm_set_ss(std::bit_cast<float>(from_word)),
       to = _mm_set_ss(std::bit_cast<float>(to_word));
  const bool reversed = _mm_comigt_ss(from, to);
  const auto lo = reversed ? to : from, hi = reversed ? from : to;
  w(m, o, 0x1cae57, frame + 0xc, 0);
  w(m, o, 0x1cae5b, frame + 0x10, to_word);
  if (!reversed)
    w(m, o, 0x1cae63, frame + 0x10, from_word);
  w(m, o, 0x1cae6c, frame + 0x14, from_word);
  if (!reversed)
    w(m, o, 0x1cae74, frame + 0x14, to_word);
  auto in_range = [&](std::int32_t event_frame) {
    const auto value = _mm_cvtsi32_ss(_mm_setzero_ps(), event_frame);
    return !below(value, lo) && _mm_comigt_ss(hi, value);
  };
  auto emit = [&](std::int32_t event_frame) {
    call(m, owner,
         {.entry = callback,
          .entry_esp = frame - 16,
          .arguments = {context,
                        word(_mm_cvtsi32_ss(_mm_setzero_ps(), event_frame)),
                        frame + 0x20}},
         o);
    // Source reloads the two value arguments after the callback, preserving
    // their original interval while observing all retained bank mutations.
    from = _mm_set_ss(std::bit_cast<float>(from_word));
    to = _mm_set_ss(std::bit_cast<float>(to_word));
  };
  if (!(options & 0x80u)) {
    const auto count = std::bit_cast<std::int32_t>(r(m, definition + 0x4c));
    for (std::int32_t i = 0; i < count; ++i) {
      const auto event = r(m, definition + 0x50) + std::uint32_t(i) * 4;
      const auto event_frame = sx(m, event + 2);
      w(m, o, 0x1caeaf, frame + 0x18,
        word(_mm_cvtsi32_ss(_mm_setzero_ps(), event_frame)));
      if (!in_range(event_frame))
        continue;
      const auto type = std::uint32_t(sx(m, event));
      w(m, o, 0x1caed4, frame + 0xc, r(m, frame + 0xc) | (1u << (type & 31u)));
      if (!callback)
        continue;
      w(m, o, 0x1caedc, frame + 0x28, type);
      w(m, o, 0x1caee4, frame + 0x22, event_frame, 2);
      w(m, o, 0x1caeee, frame + 0x20, 0, 2);
      w(m, o, 0x1caef3, frame + 0x2c, 0);
      w(m, o, 0x1caefd, frame + 0x24, 0xffffffffu);
      w(m, o, 0x1caf05, frame + 0x30, 0x600008a);
      emit(event_frame);
    }
  }
  auto admits = [&](std::uint32_t flags) {
    return (!(flags & 2u) || (options & 0x2000u)) &&
           (!(flags & 4u) || (options & 0x1000u)) &&
           (!(flags & 0x10u) || !_mm_comigt_ss(from, to)) &&
           (!(flags & 0x20u) || !_mm_comigt_ss(to, from));
  };
  if (!(options & 0x100u)) {
    const auto count = std::bit_cast<std::int32_t>(r(m, definition + 0x54));
    w(m, o, 0x1caf3c, frame + 0x1c, count);
    for (std::int32_t i = 0; i < count; ++i) {
      const auto event = r(m, definition + 0x58) + std::uint32_t(i) * 8;
      const auto event_frame = sx(m, event + 2);
      w(m, o, 0x1caf6b, frame + 0x18,
        word(_mm_cvtsi32_ss(_mm_setzero_ps(), event_frame)));
      if (!in_range(event_frame))
        continue;
      const auto reference = sx(m, event);
      if (reference == -1)
        continue;
      const auto ref = r(m, graph + 0x18) + std::uint32_t(reference) * 12;
      if (!ref)
        continue;
      const auto datum = r(m, ref + 4);
      if (datum == 0xffffffffu || !callback)
        continue;
      const auto flags = r(m, ref + 8, 2);
      if (!admits(flags))
        continue;
      w(m, o, 0x1caffc, frame + 0x24, datum);
      w(m, o, 0x1cb00a, frame + 0x30, r(m, event + 4));
      w(m, o, 0x1cb013, frame + 0x2c, flags);
      w(m, o, 0x1cb01d, frame + 0x20, 1, 2);
      w(m, o, 0x1cb024, frame + 0x28, 0xffffffffu);
      w(m, o, 0x1cb02c, frame + 0x22, event_frame, 2);
      emit(event_frame);
    }
  }
  if (!(options & 0x200u)) {
    const auto count = std::bit_cast<std::int32_t>(r(m, definition + 0x5c));
    for (std::int32_t i = 0; i < count; ++i) {
      const auto event = r(m, definition + 0x60) + std::uint32_t(i) * 4;
      const auto event_frame = sx(m, event + 2);
      w(m, o, 0x1cb08b, frame + 0x1c,
        word(_mm_cvtsi32_ss(_mm_setzero_ps(), event_frame)));
      if (!in_range(event_frame))
        continue;
      const auto reference = sx(m, event);
      if (reference == -1)
        continue;
      const auto ref = r(m, graph + 0x20) + std::uint32_t(reference) * 12;
      if (!ref)
        continue;
      const auto datum = r(m, ref + 4);
      if (datum == 0xffffffffu)
        continue;
      const auto flags = r(m, ref + 8, 2);
      if (!admits(flags) || !callback)
        continue;
      w(m, o, 0x1cb120, frame + 0x24, datum);
      w(m, o, 0x1cb12c, frame + 0x2c, flags);
      w(m, o, 0x1cb136, frame + 0x20, 2, 2);
      w(m, o, 0x1cb13d, frame + 0x28, 0xffffffffu);
      w(m, o, 0x1cb145, frame + 0x22, event_frame, 2);
      w(m, o, 0x1cb14a, frame + 0x30, 0x600008a);
      emit(event_frame);
    }
  }
  return r(m, frame + 0xc);
}
std::uint32_t source_unit_animation_definition_1_1(
    M &m, std::uint32_t master, std::uint32_t packed, std::uint32_t esp,
    const SourceRuntimeOwner &owner, const O &o) {
  const auto index = std::bit_cast<std::int16_t>(std::uint16_t(packed >> 16));
  if (index == -1)
    return 0;
  const auto inherited = std::bit_cast<std::int16_t>(std::uint16_t(packed));
  auto graph = master;
  if (inherited != -1) {
    graph = 0;
    if (inherited < std::bit_cast<std::int32_t>(r(m, master + 0x4c))) {
      const auto datum =
          r(m, r(m, master + 0x50) + std::uint32_t(inherited) * 32 + 4);
      if (datum != 0xffffffffu)
        graph = tag(m, datum);
    }
  }
  const auto definition =
      r(m, graph + 0x30) + std::uint32_t(std::int32_t(index)) * 0x6cu;
  if (definition)
    source_demand_unit_animation_1_1(m, master, packed, esp - 16, owner, o);
  return definition;
}
std::uint32_t source_unit_animation_channel_graph_1_1(const M &m,
                                                      std::uint32_t channel) {
  auto graph = tag(m, r(m, channel));
  const auto inherited = sx(m, channel + 4);
  if (inherited != -1) {
    auto next = 0u;
    if (inherited < std::bit_cast<std::int32_t>(r(m, graph + 0x4c))) {
      const auto datum =
          r(m, r(m, graph + 0x50) + std::uint32_t(inherited) * 32 + 4);
      if (datum != 0xffffffffu)
        next = tag(m, datum);
    }
    graph = next;
  }
  return graph;
}
std::uint32_t source_unit_animation_channel_definition_1_1(
    M &m, std::uint32_t channel, std::uint32_t esp,
    const SourceRuntimeOwner &owner, const O &o) {
  if (sx(m, channel + 6) == -1)
    return 0;
  return source_unit_animation_definition_1_1(
      m, tag(m, r(m, channel)), r(m, channel + 4), esp - 8, owner, o);
}
namespace {
void future(M &m, std::uint32_t channel, std::uint32_t esp, FP fp,
            const SourceRuntimeOwner &owner, const O &o) {
  Scope scope(fp);
  w(m, o, 0x1ca3a9, channel + 0x16, 0, 2);
  if (sx(m, channel + 6) == -1)
    return;
  const auto definition = source_unit_animation_definition_1_1(
      m, tag(m, r(m, channel)), r(m, channel + 4), esp - 20, owner, o);
  auto target = _mm_setzero_ps();
  if (_mm_comigt_ss(load(m, channel + 0x18), target))
    target =
        _mm_add_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)),
                   load(m, 0x465030));
  const auto current = load(m, channel + 0x1c);
  const bool reversed = _mm_comigt_ss(current, target);
  const auto lo = reversed ? target : current, hi = reversed ? current : target;
  const auto count = std::bit_cast<std::int32_t>(r(m, definition + 0x4c));
  auto examine = [&](std::uint32_t i, std::uint32_t equal_pc,
                     std::uint32_t unequal_pc) {
    const auto event = r(m, definition + 0x50) + 4 * i;
    const auto frame = _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, event + 2));
    // UCOMISS/LAHF/TEST44 admits the first branch only for ordered equality.
    const bool equal =
        (_mm_movemask_ps(_mm_cmpeq_ss(lo, load(m, channel + 0x1c))) & 1) != 0;
    const bool admitted =
        equal ? _mm_comigt_ss(frame, lo) && !below(hi, frame)
              : !below(frame, lo) && _mm_comigt_ss(hi, frame);
    if (admitted)
      w(m, o, equal ? equal_pc : unequal_pc, channel + 0x16,
        r(m, channel + 0x16, 2) | (1u << (r(m, event, 1) & 31u)), 2);
  };
  std::int32_t i = 0;
  if (count >= 4)
    for (; i < count - 3; i += 4) {
      examine(i, 0x1ca45e, 0x1ca45e);
      examine(i + 1, 0x1ca49b, 0x1ca49b);
      examine(i + 2, 0x1ca4db, 0x1ca4db);
      examine(i + 3, 0x1ca509, 0x1ca522);
    }
  for (; i < count; ++i)
    examine(i, 0x1ca56c, 0x1ca585);
}
} // namespace
void source_clamp_unit_animation_frame_1_1(M &m, std::uint32_t channel,
                                           std::uint32_t requested,
                                           std::uint32_t esp, FP fp,
                                           const SourceRuntimeOwner &owner,
                                           const O &o) {
  Scope scope(fp);
  auto definition = 0u;
  if (sx(m, channel + 6) != -1)
    definition = source_unit_animation_definition_1_1(
        m, tag(m, r(m, channel)), r(m, channel + 4), esp - 12, owner, o);
  auto current = _mm_setzero_ps();
  const auto desired = _mm_set_ss(std::bit_cast<float>(requested));
  const auto epsilon = load(m, 0x46504c);
  if (!_mm_comigt_ss(current, desired)) {
    current = _mm_sub_ss(
        _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)), epsilon);
    if (!_mm_comigt_ss(desired, current))
      current = desired;
  }
  w(m, o, 0x1c9fa7, channel + 0x1c, word(current));
  if ((r(m, channel + 0x12, 1) & 1u) && !(r(m, channel + 0x11, 1) & 9u)) {
    auto next =
        _mm_mul_ss(load(m, r(m, 0x518444) + 4), load(m, channel + 0x18));
    next = _mm_add_ss(_mm_mul_ss(next, load(m, 0x46516c)), current);
    const auto end = _mm_sub_ss(
        _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)), epsilon);
    if (!below(next, end))
      w(m, o, 0x1c9fe6, channel + 0x11, r(m, channel + 0x11, 1) | 2u, 1);
    else
      w(m, o, 0x1c9ff5, channel + 0x11, r(m, channel + 0x11, 1) & 0xfdu, 1);
  }
  future(m, channel, esp - 8, fp, owner, o);
}
void source_set_unit_animation_frame_1_1(
    M &m, std::uint32_t channel, std::uint32_t requested, std::uint32_t manager,
    std::uint32_t callback, std::uint32_t context, std::uint32_t esp, FP fp,
    const SourceRuntimeOwner &owner, const O &o) {
  Scope scope(fp);
  const auto frame = esp - 0x1c;
  auto packed = r(m, channel + 4);
  w(m, o, 0x1ca0a0, frame + 0x10, packed);
  const auto graph = source_unit_animation_channel_graph_1_1(m, channel);
  w(m, o, 0x1ca0b2, frame + 0x18, graph);
  auto definition = 0u;
  if (sx(m, channel + 6) != -1)
    definition = source_unit_animation_definition_1_1(
        m, tag(m, r(m, channel)), packed, frame - 8, owner, o);
  const auto old = r(m, channel + 0x1c);
  w(m, o, 0x1ca0e4, channel + 0x14, 0, 2);
  w(m, o, 0x1ca0e8, channel + 0x16, 0, 2);
  w(m, o, 0x1ca0f0, frame + 0x14, old);
  const auto end = word(
      _mm_sub_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)),
                 load(m, 0x46504c)));
  w(m, o, 0x1ca102, frame + 0x24, end);
  w(m, o, 0x1ca10e, frame + 0x20, requested);
  if (_mm_comigt_ss(_mm_setzero_ps(),
                    _mm_set_ss(std::bit_cast<float>(requested))))
    w(m, o, 0x1ca116, frame + 0x20, 0);
  w(m, o, 0x1ca130, channel + 0x11, r(m, channel + 0x11, 1) & 0xf1u, 1);
  auto events = [&](std::uint32_t source_graph, std::uint32_t from,
                    std::uint32_t to) {
    return source_traverse_unit_animation_events_1_1(
        m, source_graph, definition, r(m, channel + 0x12, 2), from, to,
        callback, context, frame - 28, fp, owner, o);
  };
  const auto emitted = events(graph, old, r(m, frame + 0x20));
  auto unchanged = [&] { return r(m, channel + 4) == packed; };
  if (!unchanged())
    return;
  w(m, o, 0x1ca15d, channel + 0x14, r(m, channel + 0x14, 2) | emitted, 2);
  if (!below(load(m, frame + 0x20), load(m, frame + 0x24))) {
    const auto remainder =
        events(r(m, frame + 0x18), r(m, frame + 0x14), 0x7f7fffff);
    if ((r(m, channel + 4, 2) != (packed & 65535u)))
      return;
    if (r(m, channel + 6, 2) == (packed >> 16)) {
      w(m, o, 0x1ca1aa, channel + 0x14, r(m, channel + 0x14, 2) | remainder, 2);
      const auto flags = r(m, channel + 0x12, 2);
      if (flags & 2u) {
        if ((flags & 0x20u) && sx(m, definition + 0x2e) == 0) {
          const auto output = call(m, owner,
                                   {.entry = 0x1ceda0,
                                    .entry_esp = frame - 8,
                                    .esi = frame + 0x28,
                                    .edi = manager,
                                    .arguments = {r(m, channel + 4)}},
                                   o);
          packed = r(m, output);
          w(m, o, 0x1ca1e1, channel + 4, packed);
          if (r(m, channel + 0x12, 1) & 8u)
            w(m, o, 0x1ca1ee, channel + 0x18, r(m, 0x465030));
          w(m, o, 0x1ca1f5, frame + 0x10, packed);
          definition = source_unit_animation_channel_definition_1_1(
              m, channel, frame - 4, owner, o);
        }
        const auto loop =
            _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x2e));
        const auto adjusted = _mm_add_ss(
            _mm_sub_ss(load(m, frame + 0x20), load(m, frame + 0x24)), loop);
        w(m, o, 0x1ca225, frame + 0x20, word(adjusted));
        w(m, o, 0x1ca22b, frame + 0x14, word(loop));
        const auto loop_events =
            events(r(m, frame + 0x18), word(loop), word(adjusted));
        if ((r(m, channel + 4, 2) != (packed & 65535u)))
          return;
        if (r(m, channel + 6, 2) == (packed >> 16)) {
          w(m, o, 0x1ca258, channel + 0x14,
            r(m, channel + 0x14, 2) | loop_events, 2);
          w(m, o, 0x1ca25c, channel + 0x11, r(m, channel + 0x11, 1) | 4u, 1);
        }
      } else {
        w(m, o, 0x1ca262, channel + 0x11, r(m, channel + 0x11, 1) | 8u, 1);
        w(m, o, 0x1ca276, frame + 0x20,
          word(_mm_sub_ss(
              _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)),
              load(m, 0x46504c))));
      }
    }
  }
  if (unchanged())
    source_clamp_unit_animation_frame_1_1(m, channel, r(m, frame + 0x20),
                                          frame - 8, fp, owner, o);
}
void source_advance_unit_animation_channel_1_1(
    M &m, std::uint32_t channel, std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context, std::uint32_t esp, FP fp,
    const SourceRuntimeOwner &owner, const O &o) {
  Scope scope(fp);
  const auto delay = r(m, channel + 0x10, 1);
  w(m, o, 0x1ca2b7, channel + 0x14, 0, 2);
  w(m, o, 0x1ca2bb, channel + 0x16, 0, 2);
  if (delay) {
    w(m, o, 0x1ca2c3, channel + 0x10, delay - 1, 1);
    return;
  }
  if ((r(m, channel + 0x12, 1) & 1u) && !(r(m, channel + 0x11, 1) & 9u)) {
    const auto seconds = load(m, r(m, 0x518444) + 4);
    const auto frame =
        word(_mm_add_ss(_mm_mul_ss(_mm_mul_ss(seconds, load(m, channel + 0x18)),
                                   load(m, 0x46516c)),
                        load(m, channel + 0x1c)));
    w(m, o, 0x1ca302, esp - 16, frame);
    source_set_unit_animation_frame_1_1(m, channel, frame, manager, callback,
                                        context, esp - 24, fp, owner, o);
  }
}
bool source_update_unit_animation_manager_1_1(
    M &m, std::uint32_t manager, std::uint32_t callback, std::uint32_t context,
    std::uint32_t nodes, std::uint32_t saved, std::uint32_t current,
    std::uint32_t esp, FP fp, const SourceRuntimeOwner &owner, const O &o) {
  Scope scope(fp);
  const auto frame = esp - 16;
  w(m, o, 0x1cf106, frame + 0xe, 1, 1);
  for (unsigned i = 0; i < 3; ++i) {
    const auto channel = manager + 32 * i;
    if (active(m, channel)) {
      bool valid = false;
      if (r(m, manager + 0x68) != 0xffffffffu) {
        if (!(r(m, manager + 0x6c, 1) & 1u))
          source_advance_unit_animation_channel_1_1(
              m, channel, manager, callback, context, frame - 16, fp, owner, o);
        valid = true;
      }
      w(m, o, 0x1cf142, frame + 0xe,
        r(m, frame + 0xe, 1) & std::uint32_t(valid), 1);
    }
    if (!r(m, frame + 0xe, 1))
      break;
  }
  if (active(m, manager + 0x40) && (r(m, manager + 0x51, 1) & 10u)) {
    bool capture = true;
    if (active(m, manager + 0x40)) {
      const auto definition = source_unit_animation_definition_1_1(
          m, tag(m, r(m, manager + 0x40)), r(m, manager + 0x44), frame - 8,
          owner, o);
      capture = !(r(m, definition + 0x18, 1) & 2u);
      w(m, o, 0x1cf1a3, frame + 0xf, capture, 1);
    }
    if (capture)
      call(m, owner,
           {.entry = 0x1cefe0,
            .entry_esp = frame - 20,
            .eax = nodes,
            .arguments = {manager, 0x3e88b439, saved, current}},
           o);
    w(m, o, 0x1cf1cf, manager + 0x40, 0xffffffffu);
    w(m, o, 0x1cf1d2, manager + 0x44, 0xffff, 2);
    w(m, o, 0x1cf1d6, manager + 0x46, 0xffff, 2);
    w(m, o, 0x1cf1da, manager + 0x5c, 0);
    w(m, o, 0x1cf1df, manager + 0x50, 0, 1);
    w(m, o, 0x1cf1e3, manager + 0x51, 0, 1);
    w(m, o, 0x1cf1e7, manager + 0x52, 0, 2);
    w(m, o, 0x1cf1eb, manager + 0x58, r(m, 0x465030));
    w(m, o, 0x1cf1f0, manager + 0x54, 0, 2);
    w(m, o, 0x1cf1f4, manager + 0x56, 0, 2);
    w(m, o, 0x1cf1f8, manager + 0x48, 0xffffffffu);
    w(m, o, 0x1cf1fb, manager + 0x4c, 0xff, 1);
    w(m, o, 0x1cf1fe, manager + 0x4d, 0xff, 1);
    w(m, o, 0x1cf201, manager + 0x4e, 0xffff, 2);
    if (active(m, manager) && (r(m, manager + 0x12, 1) & 1u))
      w(m, o, 0x1cf215, manager + 0x11, r(m, manager + 0x11, 1) & 0xfeu, 1);
  }
  layers(m, manager, o);
  if (r(m, 0x4770dc, 1) && active(m, manager + 0x20)) {
    auto definition = 0u;
    if (sx(m, manager + 6) != -1)
      definition = source_unit_animation_definition_1_1(
          m, tag(m, r(m, manager)), r(m, manager + 4), frame - 8, owner, o);
    const auto count = sx(m, definition + 0x14);
    auto fraction = _mm_setzero_ps();
    w(m, o, 0x1cf279, frame + 0x24, 0);
    if (count > 0) {
      fraction = _mm_div_ss(load(m, manager + 0x1c),
                            _mm_cvtsi32_ss(_mm_setzero_ps(), count));
      w(m, o, 0x1cf294, frame + 0x24, word(fraction));
      if (_mm_comigt_ss(_mm_setzero_ps(), fraction))
        w(m, o, 0x1cf2a9, frame + 0x24, 0);
      else if (_mm_comigt_ss(fraction, load(m, 0x465030)))
        w(m, o, 0x1cf2a9, frame + 0x24, r(m, 0x465030));
    }
    definition = 0;
    if (sx(m, manager + 0x26) != -1)
      definition = source_unit_animation_definition_1_1(
          m, tag(m, r(m, manager + 0x20)), r(m, manager + 0x24), frame - 8,
          owner, o);
    const auto target = word(
        _mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)),
                   load(m, frame + 0x24)));
    // Original pushes the context word as storage, then overwrites that same
    // outgoing argument cell with the computed float before pushing channel.
    w(m, o, 0x1cf2ee, frame - 16, target);
    source_set_unit_animation_frame_1_1(m, manager + 0x20, target, manager,
                                        callback, context, frame - 24, fp,
                                        owner, o);
  }
  return r(m, frame + 0xe, 1) != 0;
}
} // namespace halo2
