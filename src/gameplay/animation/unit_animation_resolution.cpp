#include "halo2/unit_animation_resolution.hpp"
#include <bit>
#include <set>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu, fallback = 0x030000d9u;
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(n <= 4 && at <= b.size() && n <= b.size() - at,
          "Truncated original animation resolution storage");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
std::uint32_t word(const SourceAnimationGraphMemory &m, std::uint32_t p,
                   unsigned n = 4) {
  require(bool(m.read), "Original graph/resource backing unavailable");
  return read(m.read(p, n), 0, n);
}
std::int32_t signed_word(std::uint32_t v) {
  return std::bit_cast<std::int32_t>(v);
}
std::int32_t signed_short(std::uint32_t v) {
  return std::bit_cast<std::int16_t>(std::uint16_t(v));
}
void write(std::span<std::byte> b, std::size_t at, unsigned n, std::uint32_t v,
           std::uint32_t ip, AnimationResolutionOutput output,
           const AnimationResolutionObserver &o) {
  auto old = read(b, at, n);
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((v >> (8 * i)) & 255);
  if (o)
    o({ip, output, at, n, old, read(b, at, n)});
}
std::uint32_t inherited(std::uint32_t graph, std::int32_t index,
                        const SourceAnimationGraphMemory &m) {
  if (index < signed_word(word(m, graph + 0x4c))) {
    const auto datum =
        word(m, word(m, graph + 0x50) + std::uint32_t(index) * 0x20 + 4);
    if (datum != none) {
      require(bool(m.graph_address), "Inherited graph identity unavailable");
      return m.graph_address(datum);
    }
  }
  return 0;
}
void request(std::uint32_t p, const SourceAnimationResourceServices &s) {
  require(bool(s.request), "Original124e40 resource demand owner unavailable");
  s.request(p, 1);
}
void bank(std::uint32_t graph, std::array<std::uint32_t, 3> ids,
          const SourceAnimationGraphMemory &m,
          const SourceAnimationResourceServices &s) {
  const auto p = find_source_animation_weapon(graph, ids[0], ids[1], ids[2], m);
  if (!p)
    return;
  // Signed counts; x86 address multiplication wraps, host resolver bounds it.
  for (std::int32_t i = 0; i < signed_word(word(m, p + 0x24)); ++i) {
    const auto index = word(m, word(m, p + 0x28) + std::uint32_t(i) * 4);
    const auto descriptor = word(m, graph + 0xb0) + index * 0x14;
    if (word(m, descriptor + 0x10, 1))
      request(descriptor, s);
  }
}
void graph_sweep(std::uint32_t graph, std::array<std::uint32_t, 3> ids,
                 const SourceAnimationGraphMemory &m,
                 const SourceAnimationResourceServices &s) {
  // The five calls are unconditional and deliberately repeat equal tuples.
  bank(graph, ids, m, s);
  bank(graph, {ids[0], ids[1], fallback}, m, s);
  bank(graph, {ids[0], fallback, ids[2]}, m, s);
  bank(graph, {ids[0], fallback, fallback}, m, s);
  bank(graph, {fallback, fallback, fallback}, m, s);
}
void sweep(std::uint32_t graph, std::array<std::uint32_t, 3> ids,
           const SourceAnimationGraphMemory &m,
           const SourceAnimationResourceServices &s) {
  graph_sweep(graph, ids, m, s);
  for (std::int32_t i = 0; i < signed_word(word(m, graph + 0x4c)); ++i)
    graph_sweep(inherited(graph, i, m), ids, m, s);
}
bool valid(std::uint32_t packed) { return (packed >> 16) != 0xffff; }
bool overlaps(std::span<std::byte> a, std::span<std::byte> b) {
  const auto x = reinterpret_cast<std::uintptr_t>(a.data());
  const auto y = reinterpret_cast<std::uintptr_t>(b.data());
  return !a.empty() && !b.empty() &&
         (x <= y ? y - x < a.size() : x - y < b.size());
}
std::uint32_t resource_word(const SourceAnimationDemandServices &s,
                            std::uint32_t p, unsigned n = 4) {
  require(bool(s.memory.read), "Original resource runtime backing unavailable");
  return read(s.memory.read(p, n), 0, n);
}
void resource_write(const SourceAnimationDemandServices &s, std::uint32_t p,
                    std::uint32_t v, std::uint32_t ip,
                    const AnimationResourceObserver &o) {
  auto old = resource_word(s, p);
  require(bool(s.memory.write),
          "Original mutable resource backing unavailable");
  std::array<std::byte, 4> raw;
  for (unsigned i = 0; i < 4; ++i)
    raw[i] = std::byte((v >> (8 * i)) & 255);
  s.memory.write(p, raw);
  if (o)
    o({ip, p, old, resource_word(s, p)});
}
} // namespace

void demand_source_animation(std::uint32_t graph, std::uint32_t packed,
                             const SourceAnimationGraphMemory &m,
                             const SourceAnimationResourceServices &s) {
  require(bool(s.state),
          "Original animation resource state producer unavailable");
  const auto flags = s.state();
  if (!flags[0] || !flags[1] || !valid(packed))
    return;
  if (signed_short(packed) != -1)
    graph = inherited(graph, signed_short(packed), m);
  const auto definitions = word(m, graph + 0x30);
  auto index = signed_short(packed >> 16);
  auto p = definitions + std::uint32_t(index) * 0x6c;
  const auto selected = p;
  const auto base = signed_short(word(m, p + 0x30, 2));
  if (base != -1) {
    index = base;
    p = definitions + std::uint32_t(index) * 0x6c;
  }
  auto demand = [&](std::uint32_t definition) {
    if (word(m, definition + 0x16, 1) & 0x10) {
      const auto descriptor =
          word(m, graph + 0xb0) + word(m, definition + 0x24) * 0x14;
      if (word(m, descriptor + 0x10, 1))
        request(descriptor, s);
    }
  };
  // EAX retains the originally selected index; ESI holds linked base.
  demand(selected); // Complete1e1230 precedes the base-linked variant walk.
  std::set<std::uint32_t> seen{p};
  for (;;) {
    index = signed_short(word(m, p + 0x32, 2));
    if (index == -1)
      return;
    p = definitions + std::uint32_t(index) * 0x6c;
    require(seen.insert(p).second, "Cyclic original animation resource links");
    demand(p);
  }
}
void lookup_source_animation_request(
    std::uint32_t graph, SourceAnimationIdentifiers ids,
    std::span<std::byte> packed, std::array<std::span<std::byte>, 3> optional,
    const SourceAnimationGraphMemory &m,
    const SourceAnimationResourceServices &s,
    const AnimationResolutionObserver &o) {
  require(packed.size() >= 4, "Truncated original packed output");
  for (const auto output : optional)
    require(output.empty() || output.size() >= 4,
            "Truncated optional original resolution output");
  if (ids[1] == none) {
    write(packed, 0, 4, none, 0x1df660, AnimationResolutionOutput::packed, o);
    return;
  }
  std::array<std::byte, 28> state{};
  auto init = [&](unsigned at, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
      state[at + i] = std::byte((v >> (8 * i)) & 255);
  };
  init(0, graph);
  for (unsigned i = 0; i < 3; ++i)
    init(4 + 4 * i, ids[i]);
  std::array<std::uint32_t, 3> resolved{none, none, none};
  std::uint32_t found = none;
  for (;;) {
    const auto weapon = next_source_animation_weapon(state, resolved, m);
    if (!weapon)
      break;
    const auto action = find_source_animation_record(weapon + 4, ids[3], 8, m);
    found = action ? word(m, action + 4) : none;
    if (valid(found))
      break;
  }
  constexpr std::uint32_t ips[]{0x1dec22, 0x1dec31, 0x1dec3f};
  for (unsigned i = 0; i < 3; ++i)
    if (!optional[i].empty()) {
      auto translated = [&](const AnimationResolutionStore &store) {
        if (o) {
          auto at = store;
          at.offset += 4 * i;
          o(at);
        }
      };
      write(optional[i], 0, 4, resolved[i], ips[i],
            AnimationResolutionOutput::resolved, translated);
    }
  std::uint32_t store_ip = 0x1df63e;
  if (valid(found)) {
    demand_source_animation(graph, found, m, s);
    sweep(graph, {ids[0], ids[1], ids[2]}, m, s);
    sweep(graph, resolved, m, s);
    if (signed_word(word(m, graph + 0x4c)) < 1)
      store_ip = 0x1df652;
  }
  write(packed, 0, 4, found, store_ip, AnimationResolutionOutput::packed, o);
}

bool resolve_source_animation_request(std::span<const std::byte> manager,
                                      SourceAnimationIdentifiers ids,
                                      std::uint32_t flags,
                                      SourceAnimationResolutionBuffers out,
                                      const SourceAnimationGraphMemory &m,
                                      const SourceAnimationResourceServices &s,
                                      const AnimationResolutionObserver &o) {
  require(manager.size() >= 0x90 && out.requested.size() >= 16 &&
              out.resolved.size() >= 16 && out.packed.size() >= 4,
          "Truncated original resolution manager/output");
  require(!overlaps(out.requested, out.resolved) &&
              !overlaps(out.requested, out.packed) &&
              !overlaps(out.resolved, out.packed),
          "Aliased host animation resolution outputs unsupported");
  require(bool(m.graph_address), "Original graph identity unavailable");
  const auto graph = m.graph_address(read(manager, 0x68));
  auto set_requested = [&](unsigned i, std::uint32_t v, std::uint32_t ip) {
    write(out.requested, 4 * i, 4, v, ip, AnimationResolutionOutput::requested,
          o);
  };
  // Complete1cfce0 output stores, including unconditional class map store.
  constexpr std::uint32_t initial[]{0x1cfcef, 0x1cfcf1, 0x1cfcf4, 0x1cfcf7};
  for (unsigned i = 0; i < 4; ++i)
    set_requested(i, ids[i], initial[i]);
  if (ids[0] == 0x07000101)
    set_requested(0, read(manager, 0x70), 0x1cfd02);
  if ((ids[0] == 0x07000101 && read(out.requested, 0) == none) ||
      read(out.requested, 0) == 0x07000001)
    set_requested(0, 0x06000086, 0x1cfd0e);
  if (ids[1] == 0x07000101)
    set_requested(1, read(manager, 0x74), 0x1cfd1f);
  if (read(out.requested, 4) == 0x07000001 || read(out.requested, 4) == none)
    set_requested(1, 0x07000083, 0x1cfd31);
  if (ids[2] == 0x07000101)
    set_requested(2, read(manager, 0x78), 0x1cfd43);
  if (read(out.requested, 8) == 0x07000001 || read(out.requested, 8) == none)
    set_requested(2, fallback, 0x1cfd55);
  auto wc = read(out.requested, 4);
  const auto map =
      find_source_animation_record(graph + 0x54, read(out.requested, 8), 8, m);
  if (map) {
    const auto value = word(m, map + 4);
    if (value != none && value != fallback && wc != 0x0400054b)
      wc = value;
  }
  set_requested(1, wc, 0x1cfda7);
  if (ids[3] == 0x07000101) {
    set_requested(3, read(manager, 0x7c), 0x1cfdb8);
    if (read(out.requested, 12) == none)
      set_requested(3, 0x0400000c, 0x1cfdbd);
  }
  if (read(out.requested, 12) == 0x07000001)
    set_requested(3, 0x0400000c, 0x1cfdcb);
  SourceAnimationIdentifiers requested;
  constexpr std::uint32_t copy[]{0x1cfe27, 0x1cfe2c, 0x1cfe32, 0x1cfe38};
  for (unsigned i = 0; i < 4; ++i) {
    requested[i] = read(out.requested, 4 * i);
    write(out.resolved, 4 * i, 4, requested[i], copy[i],
          AnimationResolutionOutput::resolved, o);
  }
  std::array<std::byte, 4> local_packed{};
  const std::array<std::span<std::byte>, 3> optional{
      out.resolved.subspan(0, 4), out.resolved.subspan(4, 4),
      out.resolved.subspan(8, 4)};
  auto lookup = [&](SourceAnimationIdentifiers input, std::uint32_t ip) {
    // Rebase nested resolved stores; local packed stores are not outer outputs.
    auto nested = [&](const AnimationResolutionStore &st) {
      if (!o || st.output != AnimationResolutionOutput::resolved)
        return;
      auto translated = st;
      translated.offset = st.instruction == 0x1dec22   ? 0
                          : st.instruction == 0x1dec31 ? 4
                                                       : 8;
      o(translated);
    };
    lookup_source_animation_request(graph, input, local_packed, optional, m, s,
                                    nested);
    write(out.packed, 0, 4, read(local_packed, 0), ip,
          AnimationResolutionOutput::packed, o);
  };
  lookup(requested, 0x1cfe67);
  bool admitted = valid(read(out.packed, 0));
  if (!admitted && (flags & 2)) {
    auto input = requested;
    input[3] = 0x0400000c;
    lookup(input, 0x1cfea9);
    admitted = valid(read(out.packed, 0));
    if (admitted)
      write(out.resolved, 12, 4, input[3], 0x1cfeb2,
            AnimationResolutionOutput::resolved, o);
  }
  if (!valid(read(out.packed, 0)) && (flags & 4)) {
    const std::array<std::array<std::uint32_t, 3>, 5> choices{
        {{requested[0], requested[1], requested[2]},
         {requested[0], requested[1], fallback},
         {requested[0], fallback, requested[2]},
         {requested[0], fallback, fallback},
         {fallback, fallback, fallback}}};
    for (unsigned i = 0; i < choices.size(); ++i)
      if (find_source_animation_weapon(graph, choices[i][0], choices[i][1],
                                       choices[i][2], m)) {
        admitted = true;
        if (i == 1)
          write(out.resolved, 8, 4, fallback, 0x1cff21,
                AnimationResolutionOutput::resolved, o);
        if (i == 2)
          write(out.resolved, 4, 4, fallback, 0x1cff4b,
                AnimationResolutionOutput::resolved, o);
        if (i == 3 || i == 4) {
          if (i == 4)
            write(out.resolved, 0, 4, fallback, 0x1cffa1,
                  AnimationResolutionOutput::resolved, o);
          write(out.resolved, 4, 4, fallback, i == 4 ? 0x1cffa7 : 0x1cff73,
                AnimationResolutionOutput::resolved, o);
          write(out.resolved, 8, 4, fallback, i == 4 ? 0x1cffad : 0x1cff79,
                AnimationResolutionOutput::resolved, o);
        }
        break;
      }
  }
  if (admitted)
    sweep(graph,
          {read(out.resolved, 0), read(out.resolved, 4), read(out.resolved, 8)},
          m, s);
  return admitted;
}

std::uint32_t
find_source_queued_animation(std::uint32_t descriptor,
                             const SourceAnimationDemandServices &s) {
  if (resource_word(s, descriptor + 0xc) != none)
    return none;
  const auto pool = resource_word(s, s.bindings.queued_pool_pointer);
  for (std::int32_t i = 0; i < signed_word(resource_word(s, pool + 0x38));
       ++i) {
    const auto bitmap = resource_word(s, pool + 0x48);
    if (!(resource_word(s, bitmap + (std::uint32_t(i) >> 5) * 4) &
          (1u << (std::uint32_t(i) & 31))))
      continue;
    const auto slot = resource_word(s, pool + 0x44) +
                      resource_word(s, pool + 0x24) * std::uint32_t(i);
    if (resource_word(s, slot + 8) == descriptor)
      return (resource_word(s, slot, 2) << 16) | std::uint32_t(i);
  }
  return none;
}
void request_source_animation_resource(std::uint32_t descriptor,
                                       std::uint8_t demand,
                                       const SourceAnimationDemandServices &s,
                                       const AnimationResourceObserver &o) {
  if (!resource_word(s, s.bindings.active_byte, 1))
    return;
  const auto priority = demand ? 1u : 2u;
  const auto datum = resource_word(s, descriptor + 0xc);
  if (datum == none) {
    if (find_source_queued_animation(descriptor, s) != none)
      return;
    require(bool(s.allocate_queued),
            "Original resource pool allocator unavailable");
    const auto requested_pool =
        resource_word(s, s.bindings.queued_pool_pointer);
    const auto allocation = s.allocate_queued(requested_pool);
    if (allocation.datum != none) {
      const auto slot = resource_word(s, allocation.pool_address + 0x44) +
                        (allocation.datum & 65535) * 0xc;
      resource_write(s, slot + 4, priority, 0x124e8b, o);
      resource_write(s, slot + 8, descriptor, 0x124e8f, o);
      require(bool(s.schedule),
              "Original125120 cache/IO scheduler unavailable");
      s.schedule();
      return;
    }
    require(bool(s.tick_count), "Original resource warning clock unavailable");
    const auto now = s.tick_count();
    if (resource_word(s, s.bindings.warning_deadline) < now) {
      const auto later = s.tick_count();
      resource_write(s, s.bindings.warning_deadline, later + 30000u, 0x124eb0,
                     o);
    }
  } else {
    const auto threshold =
        resource_word(s, s.bindings.priority_table + priority * 0xc);
    const auto cache = resource_word(s, s.bindings.cache_pointer);
    const auto pool = resource_word(s, cache + 0x64);
    const auto elements = resource_word(s, pool + 0x44);
    const auto counter = resource_word(s, cache + 0x38);
    const auto age_slot = elements + (datum & 65535) * 0x18 + 0x14;
    if (signed_word(threshold) <
        signed_word(counter - resource_word(s, age_slot)))
      resource_write(s, age_slot, counter - threshold, 0x124ee9, o);
  }
}
} // namespace halo2
