#include "halo2/unit_alternate_request.hpp"
#include <bit>
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
void write(std::span<std::byte> b, std::size_t at, unsigned n, std::uint32_t v,
           std::uint32_t ip, AnimationResolutionOutput output,
           const AnimationResolutionObserver &o) {
  auto old = o ? read(b, at, n) : 0;
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
} // namespace
void lookup_source_alternate_animation_request(
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
    write(packed, 0, 4, none, 0x1e0170, AnimationResolutionOutput::packed, o);
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
    const auto action =
        find_source_animation_record(weapon + 0xc, ids[3], 8, m);
    found = action ? word(m, action + 4) : none;
    if (valid(found))
      break;
  }
  constexpr std::uint32_t ips[]{0x1df732, 0x1df741, 0x1df74f};
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
  std::uint32_t store_ip = 0x1e014e;
  if (valid(found)) {
    demand_source_animation(graph, found, m, s);
    sweep(graph, {ids[0], ids[1], ids[2]}, m, s);
    sweep(graph, resolved, m, s);
    if (signed_word(word(m, graph + 0x4c)) < 1)
      store_ip = 0x1e0162;
  }
  write(packed, 0, 4, found, store_ip, AnimationResolutionOutput::packed, o);
}

bool source_unit_alternate_animation_available(
    std::span<const std::byte> manager, const SourceAnimationGraphMemory &m,
    const SourceAnimationResourceServices &s,
    const SourceAlternateAvailabilityObserver &o) {
  require(manager.size() >= 0x90, "Missing retained availability manager");
  require(bool(m.graph_address), "Missing original graph identity owner");
  const auto graph = m.graph_address(read(manager, 0x68));
  struct Row {
    std::uint32_t activity, call;
  };
  constexpr Row rows[] = {
      {0x0e000044, 0x1cfac4}, {0x0e000045, 0x1cfaee}, {0x0b000046, 0x1cfb21}};
  for (auto row : rows) {
    const SourceAnimationIdentifiers ids{read(manager, 0x70),
                                         read(manager, 0x74),
                                         read(manager, 0x78), row.activity};
    if (o)
      o({row.call, graph, ids, {}});
    // Private scratch's incoming bytes are not read by the source body. No
    // observer is installed on that scratch store; no Unit field is filled.
    std::array<std::byte, 4> packed;
    lookup_source_alternate_animation_request(graph, ids, packed, {}, m, s);
    const auto result = read(packed, 0);
    if (o)
      o({row.call, graph, ids, result});
    if (valid(result))
      return true;
  }
  return false;
}
} // namespace halo2
