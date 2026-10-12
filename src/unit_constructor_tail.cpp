#include "halo2/unit_constructor_tail.hpp"
#include <algorithm>
#include <bit>
#include <initializer_list>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu, inherit = 0x07000101u;
void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(at <= b.size() && n <= b.size() - at,
          "Truncated unit constructor backing");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
std::uint32_t ptr(std::uint32_t p, std::size_t at) {
  require(at <= none - p, "Escaped original metadata address");
  return p + std::uint32_t(at);
}
std::span<const std::byte> metadata(const UnitConstructorTailMetadata &s,
                                    std::uint32_t p, std::size_t n) {
  require(bool(s.read), "Missing literal tag read owner");
  auto b = s.read(p, n);
  require(b.size() == n, "Truncated literal tag read");
  return b;
}
std::uint32_t field(const UnitConstructorTailMetadata &s, std::uint32_t p,
                    std::size_t at) {
  return read(metadata(s, ptr(p, at), 4), 0);
}
std::uint32_t address(const UnitConstructorTailMetadata &s, std::uint32_t d) {
  require(bool(s.address), "Missing literal tag address owner");
  return s.address(d);
}
std::int32_t count(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::span<std::byte> body(std::uint32_t unit,
                          const UnitConstructorObjectPool &s) {
  auto i = unit & 65535u;
  require(i < s.capacity && bool(s.body), "Invalid constructor pool index");
  auto at = std::size_t(i) * 12;
  require(at <= s.headers.size() && 12 <= s.headers.size() - at,
          "Truncated constructor headers");
  auto b = s.body(std::uint16_t(i));
  auto n = read(s.headers, at + 6, 2);
  require(n <= b.size(), "Truncated source unit body");
  return b.first(n);
}
std::size_t component(std::span<const std::byte> b, std::size_t word,
                      std::size_t extent) {
  auto raw = read(b, word, 2);
  auto offset = raw < 0x8000 ? std::int32_t(raw) : std::int32_t(raw) - 0x10000;
  require(offset >= 0 && std::size_t(offset) <= b.size() &&
              extent <= b.size() - std::size_t(offset),
          "Missing source-created unit component");
  return std::size_t(offset);
}
void write(std::span<std::byte> b, std::size_t at, unsigned n, std::uint32_t v,
           std::uint32_t ip, UnitConstructorTailBuffer k,
           const UnitConstructorTailObserver &o) {
  require(at <= b.size() && n <= b.size() - at,
          "Truncated unit constructor write backing");
  // Private packed scratch has no source-defined prior value. Read prior
  // bytes only for an explicitly requested observable store comparison.
  auto old = o ? read(b, at, n) : 0;
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((v >> (8 * i)) & 255);
  if (o)
    o({ip, k, at, n, old, read(b, at, n)});
}
void store(std::span<std::byte> b, std::size_t at, unsigned n, std::uint32_t v,
           std::uint32_t ip, const UnitConstructorTailObserver &o) {
  write(b, at, n, v, ip, UnitConstructorTailBuffer::Unit, o);
}
std::uint32_t graph_address(std::uint32_t d,
                            const SourceAnimationGraphMemory &g) {
  require(bool(g.graph_address),
          "Missing literal animation graph address owner");
  return g.graph_address(d);
}
std::uint32_t graph_field(const SourceAnimationGraphMemory &g, std::uint32_t p,
                          std::size_t off) {
  require(bool(g.read), "Missing literal animation graph read owner");
  auto b = g.read(ptr(p, off), 4);
  require(b.size() == 4, "Truncated animation graph read");
  return read(b, 0);
}
std::uint32_t named(std::uint32_t unit, std::uint32_t name,
                    const UnitConstructorTailServices &s) {
  auto b = body(unit, s.objects);
  auto m = component(b, 0x12a, 0x90);
  auto g = graph_address(read(b, m + 0x68), s.graphs);
  // Scratch before bytes are never read by the original lookup.
  std::array<std::byte, 4> out;
  find_source_named_animation(g, name, out, s);
  return read(out, 0);
}
std::uint8_t invoke(std::uint32_t unit, std::span<std::byte> m,
                    const UnitConstructorTailServices &s, std::uint32_t entry,
                    std::uint32_t eax, bool has_eax,
                    std::initializer_list<std::uint32_t> args) {
  require(bool(s.invoke), "Missing complete constructor-tail lower owner");
  UnitConstructorTailCall c{entry,   unit, eax,
                            has_eax, {},   unsigned(args.size())};
  require(args.size() <= c.arguments.size(), "Invalid constructor-tail packet");
  std::copy(args.begin(), args.end(), c.arguments.begin());
  return s.invoke(m, c);
}
} // namespace

std::uint32_t find_source_model_node(std::uint32_t model, std::uint32_t name,
                                     const UnitConstructorTailMetadata &s) {
  if (model == none)
    return none;
  auto p = address(s, model);
  auto n = count(field(s, p, 0x78));
  if (n <= 0)
    return none;
  auto bank = field(s, p, 0x7c);
  for (std::int32_t i = 0; i < n; ++i)
    if (field(s, bank, std::size_t(i) * 0x5c) == name)
      return std::uint32_t(i);
  return none;
}

void find_source_named_animation(std::uint32_t root, std::uint32_t name,
                                 std::span<std::byte> out,
                                 const UnitConstructorTailServices &s,
                                 const UnitConstructorTailObserver &o) {
  require(out.size() == 4, "Packed animation output must have four bytes");
  if (!root) {
    write(out, 0, 4, none, 0x1e0b91, UnitConstructorTailBuffer::Packed, o);
    return;
  }
  auto current = root;
  std::int32_t inherited = -1;
  std::uint32_t packed = none;
  for (;;) {
    auto n = count(graph_field(s.graphs, current, 0x2c));
    auto bank = n > 0 ? graph_field(s.graphs, current, 0x30) : 0;
    for (std::int32_t i = 0; i < n; ++i) {
      if (graph_field(s.graphs, bank, std::size_t(i) * 0x6c) != name)
        continue;
      // The source packs low16 iteration indices. Prevent malformed host
      // metadata from driving its signed16 wrap into an unbounded walk.
      require(i < 0x7fff && inherited < 0x7fff,
              "Unsupported wrapped animation bank index");
      packed = (std::uint32_t(i) << 16) | (std::uint32_t(inherited) & 65535u);
      require(bool(s.resource), "Missing complete animation resource owner");
      s.resource(root, packed);
      write(out, 0, 4, packed, 0x1e0b7d, UnitConstructorTailBuffer::Packed, o);
      return;
    }
    ++inherited;
    if (inherited >= count(graph_field(s.graphs, root, 0x4c)))
      break;
    require(inherited < 0x7fff, "Unsupported wrapped inheritance bank index");
    auto p = graph_field(s.graphs, root, 0x50);
    auto d = graph_field(s.graphs, p, std::size_t(inherited) * 0x20 + 4);
    current = d == none ? 0 : graph_address(d, s.graphs);
    if (!current)
      break;
  }
  write(out, 0, 4, packed, 0x1e0ba5, UnitConstructorTailBuffer::Packed, o);
}

void reset_source_unit_subchannels(std::uint32_t unit,
                                   const UnitConstructorTailServices &s,
                                   const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto e = component(b, 0x33e, 0x124);
  struct Op {
    std::uint32_t ip;
    std::size_t off;
    unsigned size;
    std::uint32_t value;
  };
  const Op ops[]{{0x11082d, 0x98, 4, none},
                 {0x110833, 0x94, 4, none},
                 {0x110839, 0x9c, 4, none},
                 {0x11083f, 0xa0, 2, none},
                 {0x110846, 0xa2, 2, none},
                 {0x11084d, 0xb8, 4, 0},
                 {0x110855, 0xb4, 4, s.constants.one_bits},
                 {0x11085d, 0xa4, 4, none},
                 {0x110863, 0xa8, 1, none},
                 {0x110869, 0xa9, 1, none},
                 {0x11086f, 0xaa, 2, none},
                 {0x110878, 0xac, 1, 0},
                 {0x11087e, 0xad, 1, 0},
                 {0x110884, 0xae, 2, 0},
                 {0x11088b, 0xb0, 2, 0},
                 {0x110892, 0xb2, 2, 0},
                 {0x110899, 0xbc, 4, none},
                 {0x11089f, 0xc0, 2, none},
                 {0x1108a6, 0xc2, 2, none},
                 {0x1108ad, 0xcc, 1, 0},
                 {0x1108b3, 0xcd, 1, 0},
                 {0x1108b9, 0xce, 2, 0},
                 {0x1108c0, 0xd0, 2, 0},
                 {0x1108c7, 0xd2, 2, 0},
                 {0x1108ce, 0xd8, 4, 0},
                 {0x1108d6, 0xd4, 4, s.constants.one_bits},
                 {0x1108de, 0xc4, 4, none},
                 {0x1108e4, 0xc8, 1, none},
                 {0x1108ea, 0xc9, 1, none},
                 {0x1108f0, 0xca, 2, none},
                 {0x1108f7, 0xdc, 4, none},
                 {0x1108fd, 0xe0, 2, none},
                 {0x110904, 0xe2, 2, none},
                 {0x11090b, 0xec, 1, 0},
                 {0x110911, 0xed, 1, 0},
                 {0x110917, 0xee, 2, 0},
                 {0x11091e, 0xf0, 2, 0},
                 {0x110925, 0xf2, 2, 0},
                 {0x11092c, 0xf8, 4, 0},
                 {0x110934, 0xf4, 4, s.constants.one_bits},
                 {0x11093c, 0xe4, 4, none},
                 {0x110942, 0xe8, 1, none},
                 {0x110948, 0xe9, 1, none},
                 {0x11094e, 0xea, 2, none},
                 {0x110955, 3, 1, 0}};
  for (const auto &r : ops)
    store(b, e + r.off, r.size, r.value, r.ip, o);
  for (unsigned i = 0; i < 7; ++i) {
    store(b, e + 0xfc + 4 * i, 2, none, 0x110963, o);
    store(b, e + 0xfe + 4 * i, 2, none, 0x110966, o);
  }
  for (unsigned i = 0; i < 3; ++i) {
    store(b, e + 0x118 + 4 * i, 2, none, 0x110980, o);
    store(b, e + 0x11a + 4 * i, 2, none, 0x110983, o);
  }
  store(b, e, 1, read(b, e, 1) & 0xfd, 0x11098d, o);
}

void reset_source_unit_named_animation(std::uint32_t unit,
                                       const UnitConstructorTailServices &s,
                                       const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto e = component(b, 0x33e, 0x124);
  const unsigned ip[]{0x110111, 0x110113, 0x110116, 0x110119,
                      0x11011c, 0x11011f, 0x110122, 0x110125};
  for (unsigned i = 0; i < 8; ++i)
    store(b, e + 0x5c + 4 * i, 4, 0, ip[i], o);
  store(b, e + 0x6c, 2, none, 0x11012d, o);
  auto v = named(unit, 0x0800004d, s);
  store(b, e + 0x5c, 4, v, 0x110157, o);
}
void reset_source_unit_aim_animation(std::uint32_t unit,
                                     const UnitConstructorTailServices &s,
                                     const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto e = component(b, 0x33e, 0x124);
  for (unsigned i = 0; i < 11; ++i)
    store(b, e + 0x30 + 4 * i, 4, 0, 0x10f3be, o);
  store(b, e + 0x34, 4, named(unit, 0x08000145, s), 0x10f3e3, o);
  auto v = named(unit, 0x0c00024d, s);
  store(b, e + 0x40, 4, 0, 0x10f412, o);
  store(b, e + 0x38, 4, none, 0x10f417, o);
  store(b, e + 0x30, 4, v, 0x10f41f, o);
}
void reset_source_unit_action_cache(std::uint32_t unit,
                                    const UnitConstructorTailServices &s,
                                    const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto e = component(b, 0x33e, 0x124);
  const unsigned off[]{0x81, 0x80, 0x83, 0x85, 0x84, 0x87, 0x89, 0x88,
                       0x8b, 0x8d, 0x8c, 0x8f, 0x91, 0x90, 0x93};
  for (unsigned i = 0; i < 15; ++i)
    store(b, e + off[i], 1, 0, 0x115050 + 6 * i, o);
  store(b, e + 0x82, 1, 1, 0x1150ac, o);
  store(b, e + 0x86, 1, 1, 0x1150b2, o);
  store(b, e + 0x8a, 1, 2, 0x1150b8, o);
  store(b, e + 0x8e, 1, 1, 0x1150bf, o);
  store(b, e + 0x92, 1, 1, 0x1150c5, o);
}
void initialize_source_unit_extension(std::uint32_t unit,
                                      const UnitConstructorTailServices &s,
                                      const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto e = component(b, 0x33e, 0x124);
  reset_source_unit_subchannels(unit, s, o);
  store(b, e + 0x28, 4, none, 0x110a4f, o);
  store(b, e, 2, 0, 0x110a5a, o);
  const unsigned ip[]{0x110a60, 0x110a62, 0x110a65, 0x110a68,
                      0x110a6e, 0x110a70, 0x110a73, 0x110a76};
  for (unsigned i = 0; i < 8; ++i)
    store(b, e + 4 + 4 * i, 4, 0, ip[i], o);
  store(b, e + 2, 1, 0, 0x110a7f, o);
  store(b, e + 3, 1, 0, 0x110a82, o);
  auto def = address(s.tags, read(b, 0));
  auto model = field(s.tags, def, 0x38);
  store(b, e + 0x7c, 2, find_source_model_node(model, 0x080001a2, s.tags),
        0x110aa0, o);
  // Source rereads the literal model reference before its second lookup.
  model = field(s.tags, def, 0x38);
  store(b, e + 0x7e, 2, find_source_model_node(model, 0x090001a3, s.tags),
        0x110ab1, o);
  auto m = component(b, 0x12a, 0x90);
  store(b, m + 0x74, 4, none, 0x110ac3, o);
  store(b, m + 0x78, 4, none, 0x110ac6, o);
  reset_source_unit_named_animation(unit, s, o);
  reset_source_unit_aim_animation(unit, s, o);
  reset_source_unit_action_cache(unit, s, o);
  store(b, e + 0x94, 4, none, 0x110ae8, o);
  store(b, e + 0x98, 4, none, 0x110aee, o);
}

void initialize_source_unit_variant_metadata(
    std::uint32_t unit, std::uint32_t variant,
    const UnitConstructorTailServices &s,
    const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto e = component(b, 0x342, 0x58);
  auto d = address(s.tags, read(b, 0));
  auto m = address(s.tags, field(s.tags, d, 0x38));
  store(b, e, 4, field(s.tags, m, 0x90), 0x1166d0, o);
  store(b, e + 8, 4, field(s.tags, m, 0xa0), 0x1166d8, o);
  auto n = count(field(s.tags, m, 0x50));
  if (n <= 0)
    return;
  require(n <= 0x7fff, "Unsupported wrapped unit variant index");
  for (std::int32_t i = 0; i < n; ++i) {
    auto p = ptr(field(s.tags, m, 0x54), std::size_t(i) * 0x38);
    auto matching = field(s.tags, p, 0) == variant;
    auto ref = field(s.tags, p, 0x34);
    if (ref == none || (!matching && read(b, e) != none))
      continue;
    auto value = field(s.tags, address(s.tags, ref), 0x14);
    // Source reacquires the header/component for the two reference writes.
    auto live = body(unit, s.objects);
    auto le = component(live, 0x342, 0x58);
    store(live, le, 4, ref, matching ? 0x1167b3 : 0x11674e, o);
    store(live, le + 4, 4, value, matching ? 0x1167b5 : 0x116750, o);
    auto name = field(s.tags, p, 0x2c);
    if (name && name != none)
      store(b, e + 8, 4, name, matching ? 0x1167c4 : 0x116763, o);
    if (matching)
      return;
  }
}

std::uint32_t
source_unit_transition_duration(std::uint32_t next, std::uint32_t prev,
                                std::uint8_t changed, std::uint8_t force,
                                std::uint32_t minimum,
                                const UnitConstructorTailConstants &s) {
  auto bits = (next == 0x09000020 || next == 0x0900001f || next == 0x090006b2)
                  ? s.special_blend_bits
                  : s.normal_blend_bits;
  auto member = [](std::uint32_t v) {
    return v == 0x0a000014 || v == 0x09000015 || v == 0x09000016 ||
           v == 0x0a000017;
  };
  if (changed || force || (prev == 0x0400000c && member(next)) ||
      (member(prev) && (member(next) || next == 0x0400000c)))
    bits = s.normal_blend_bits;
  // Original COMISS/JA: equal and unordered take the literal minimum operand.
  return std::bit_cast<float>(bits) > std::bit_cast<float>(minimum) ? bits
                                                                    : minimum;
}

std::uint8_t bind_source_unit_activity(std::uint32_t unit,
                                       const UnitActivityRequest &r,
                                       const UnitConstructorTailServices &s,
                                       const UnitConstructorTailObserver &o) {
  auto b = body(unit, s.objects);
  auto m = component(b, 0x12a, 0x90), e = component(b, 0x33e, 0x124);
  auto manager = b.subspan(m, 0x90);
  auto prev = read(b, m + 0x7c), mode = read(b, m + 0x70);
  auto flags = r.flags;
  if (prev != none) {
    if (prev == r.activity || r.activity == inherit)
      flags |= 0x20;
    else {
      auto equivalent = none;
      for (auto pair : s.constants.activity_pairs)
        if (pair[0] == r.activity) {
          equivalent = pair[1];
          break;
        }
      if (prev == equivalent)
        flags |= 0x40;
    }
  }
  if (read(b, 0x13c) != none) {
    require(bool(s.player_transition_gate),
            "Missing actual player transition gate owner");
    if (!s.player_transition_gate())
      flags |= 0x200;
  }
  std::uint8_t result = 1;
  if (r.mode != inherit || r.activity != inherit) {
    result = invoke(unit, manager, s, 0x1d0a60, r.activity, true,
                    {r.mode, r.weapon_class, r.weapon, flags, 0x3f});
    if (!result)
      return result;
  }
  bool mode_changed =
      ((mode == r.mode || r.mode == inherit) ? r.force != 0 : true);
  bool activity_changed =
      ((prev == r.activity || r.activity == inherit) ? r.force != 0 : true);
  if (mode_changed) {
    invoke(unit, manager, s, 0x114f00, unit, true, {});
    auto has = invoke(unit, manager, s, 0x1cfa90, 0, false, {});
    store(b, e, 1, has ? (read(b, e, 1) | 2) : (read(b, e, 1) & 0xfd),
          has ? 0x110d17 : 0x110d1c, o);
  } else if (!activity_changed)
    return result;
  auto duration = source_unit_transition_duration(
      r.activity, prev, mode_changed, 0, r.minimum_blend_bits, s.constants);
  if (std::bit_cast<float>(duration) >
          std::bit_cast<float>(s.constants.zero_bits) &&
      !(read(b, m + 0x6c, 1) & 0x10))
    invoke(unit, manager, s, 0xbb240, 0, false, {duration});
  invoke(unit, manager, s, 0x114bd0, 0, false, {unit});
  return result;
}
} // namespace halo2
