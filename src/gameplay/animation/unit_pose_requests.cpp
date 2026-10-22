#include "halo2/unit_pose_requests.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu, current = 0x07000101u;
void require(bool b, const char *why) {
  if (!b) throw std::runtime_error(why);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(n <= 4 && at <= b.size() && n <= b.size() - at,
          "Truncated retained pose-request storage");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
void assign(std::span<std::byte> b, std::size_t at, unsigned n,
            std::uint32_t v) {
  require(at <= b.size() && n <= b.size() - at,
          "Truncated retained pose-request output");
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((v >> (8 * i)) & 255);
}
void write(std::span<std::byte> b, std::size_t at, unsigned n,
           std::uint32_t v, std::uint32_t ip,
           const UnitAnimationSelectionObserver &o) {
  const auto before = o ? read(b, at, n) : 0;
  assign(b, at, n, v);
  if (o) o({ip, at, n, before, read(b, at, n)});
}
void event(const SourceUnitPoseCallObserver &o, std::uint32_t site,
           std::uint32_t entry, std::array<std::uint32_t, 4> args,
           std::optional<std::uint32_t> result = {}, bool void_return = false) {
  if (o) o({site, entry, args, result.has_value() || void_return, result});
}
void manager(std::span<const std::byte> m) {
  require(m.size() >= 0x90, "Missing retained pose-request manager");
}
std::uint32_t graph(std::span<const std::byte> m,
                    const SourceUnitPoseRequestServices &s) {
  require(bool(s.channels.graphs.graph_address),
          "Missing actual pose-request graph identity");
  return s.channels.graphs.graph_address(read(m, 0x68));
}
bool valid(std::uint32_t p) { return (p >> 16) != 65535; }
struct Body { std::span<std::byte> bytes; std::uint32_t address; };
Body body(std::uint32_t unit, const UnitConstructorObjectPool &pool) {
  const auto index = unit & 65535u;
  require(index < pool.capacity && bool(pool.body),
          "Missing actual pose-request body owner");
  const auto h = std::size_t(index) * 12;
  require(h <= pool.headers.size() && 12 <= pool.headers.size() - h,
          "Truncated actual pose-request header backing");
  const auto address = read(pool.headers, h + 8);
  require(address != 0, "Missing actual source body address");
  auto b = pool.body(std::uint16_t(index));
  const auto extent = read(pool.headers, h + 6, 2);
  require(extent <= b.size(), "Truncated actual pose-request body extent");
  return {b.first(extent), address};
}
std::int16_t offset(std::span<const std::byte> b, std::size_t field) {
  return std::bit_cast<std::int16_t>(std::uint16_t(read(b, field, 2)));
}
std::size_t component(std::span<const std::byte> b, std::size_t field,
                      std::size_t extent) {
  const auto at = offset(b, field);
  require(at >= 0 && std::size_t(at) <= b.size() &&
              extent <= b.size() - std::size_t(at),
          "Missing source-created pose-request component");
  return std::size_t(at);
}
std::array<std::uint32_t, 2> activities(std::uint32_t a,
                                     std::span<const std::byte> b) {
  // The compact114EF0 table sends9000015/16 to the second group and
  // 900001F/20 to the first. All other table entries preserveNONE.
  switch (a) {
  case 0x0400004a: case 0x05000049:
    if (read(b, 0x14) != none) break;
    [[fallthrough]];
  case 0x0400000c: case 0x0700005c: case 0x0700005d:
  case 0x0800001e: case 0x0900000e: case 0x090006b2:
  case 0x0900001f: case 0x09000020: case 0x0a00000f:
  case 0x0b000059: case 0x0b00005a: case 0x0c000058:
  case 0x0c00005b:
    return {0x0c000026, 0x0e000028};
  case 0x09000015: case 0x09000016: case 0x0a000014:
  case 0x0a000017: case 0x0a000019: case 0x0a00001a:
  case 0x0a00002d: case 0x0b000018: case 0x0b00001b:
  case 0x0b00002e: case 0x0c000033: case 0x0c000034:
  case 0x0d000032: case 0x0d000035:
    return {0x0b000027, 0x0d000029};
  default: break;
  }
  return {none, none};
}
} // namespace
void construct_source_unit_pose_channel(std::span<std::byte> c,
                                         std::uint32_t one_bits) {
  require(c.size() == 32, "Source pose scratch channel must have32 bytes");
  assign(c, 0, 4, none); assign(c, 4, 2, none); assign(c, 6, 2, none);
  assign(c, 8, 4, none); assign(c, 0xc, 1, none); assign(c, 0xd, 1, none);
  assign(c, 0xe, 2, none); assign(c, 0x10, 1, 0); assign(c, 0x11, 1, 0);
  assign(c, 0x12, 2, 0); assign(c, 0x14, 2, 0); assign(c, 0x16, 2, 0);
  assign(c, 0x18, 4, one_bits); assign(c, 0x1c, 4, 0);
}
bool install_source_unit_alternate_pose_channel(
    std::span<std::byte> m, std::span<std::byte> c,
    std::uint32_t wc, std::uint32_t w, std::uint32_t activity,
    std::uint32_t flags, const SourceUnitPoseRequestServices &s,
    const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  manager(m);
  if (wc == current || wc == none) wc = read(m, 0x74);
  if (w == current || w == none) w = read(m, 0x78);
  if (read(m, 0x68) == none) return false;
  const auto mode = read(m, 0x70), g = graph(m, s);
  std::array<std::byte, 4> p;
  event(o, 0x1cef62, 0x1df670, {mode, wc, w, activity});
  lookup_source_alternate_animation_request(g, {mode, wc, w, activity}, p, {},
                                           s.channels.graphs,
                                           s.channels.resources);
  const auto packed = read(p, 0);
  event(o, 0x1cef62, 0x1df670, {mode, wc, w, activity}, packed);
  if (!valid(packed)) return false;
  event(o, 0x1cef86, 0x1cee00, {packed, activity, flags, 1});
  const auto result = install_source_unit_requested_channel(
      m, c, packed, activity, flags, none, none, 1, s.channels, stores);
  event(o, 0x1cef86, 0x1cee00, {packed, activity, flags, 1}, result);
  return result;
}
void lookup_source_unit_alternate_pose(
    std::span<std::byte> m, std::uint32_t wc, std::uint32_t w,
    std::uint32_t activity, std::span<std::byte> p,
    const SourceUnitPoseRequestServices &s, const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  manager(m); require(p.size() >= 4, "Truncated alternate pose output");
  if (read(m, 0x68) == none) {
    write(p, 0, 4, none, 0x1cf462, stores); return;
  }
  std::array<std::byte, 32> c;
  // Complete source-owned scratch initializer; every channel byte is written.
  construct_source_unit_pose_channel(c, s.channels.one_bits);
  event(o, 0x1cf437, 0x1cef10, {wc, w, activity, 0});
  const auto result = install_source_unit_alternate_pose_channel(
      m, c, wc, w, activity, 0, s, o);
  event(o, 0x1cf437, 0x1cef10, {wc, w, activity, 0}, result);
  write(p, 0, 4, result ? read(c, 4) : none,
        result ? 0x1cf444 : 0x1cf453, stores);
}
void lookup_source_unit_resolved_pose(
    std::span<std::byte> m, std::uint32_t wc, std::uint32_t w,
    std::uint32_t activity, std::span<std::byte> p,
    const SourceUnitPoseRequestServices &s, const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  manager(m); require(p.size() >= 4, "Truncated resolved pose output");
  // These two full packets are output-only source scratch. Host initialization
  // avoids indeterminate C++ reads by the existing store observer helper; it
  // is not a guest Unit field or gameplay input. Source writes all16 bytes
  // before consuming either packet.
  std::array<std::byte, 16> requested{}, resolved{};
  std::array<std::byte, 4> local;
  assign(local, 0, 2, none); assign(local, 2, 2, none);
  const auto mode = read(m, 0x70);
  event(o, 0x1cf396, 0x1cfde0, {mode, wc, w, activity});
  const auto result = resolve_source_animation_request(
      m, {mode, wc, w, activity}, 0, {requested, resolved, local},
      s.channels.graphs, s.channels.resources);
  event(o, 0x1cf396, 0x1cfde0, {mode, wc, w, activity}, result);
  write(p, 0, 4, result ? read(local, 0) : none,
        result ? 0x1cf3a2 : 0x1cf3b0, stores);
}
void lookup_source_unit_pose_request(
    std::span<std::byte> m, std::uint32_t wc, std::uint32_t w,
    std::uint32_t activity, std::span<std::byte> p,
    const SourceUnitPoseRequestServices &s, const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  manager(m); require(p.size() >= 4, "Truncated pose request output");
  if (wc == current || wc == none) wc = read(m, 0x74);
  if (w == current || w == none) w = read(m, 0x78);
  std::array<std::byte, 4> local;
  event(o, 0x1cf4fd, 0x1cf3c0, {wc, w, activity, 0});
  lookup_source_unit_alternate_pose(m, wc, w, activity, local, s, o);
  auto packed = read(local, 0);
  event(o, 0x1cf4fd, 0x1cf3c0, {wc, w, activity, 0}, packed);
  if (!valid(packed)) {
    event(o, 0x1cf520, 0x1cf360, {wc, w, activity, 0});
    lookup_source_unit_resolved_pose(m, wc, w, activity, local, s, o);
    packed = read(local, 0);
    event(o, 0x1cf520, 0x1cf360, {wc, w, activity, 0}, packed);
  }
  write(p, 0, 4, packed, 0x1cf52d, stores);
}
void lookup_source_unit_pose_index(
    std::span<const std::byte> m, std::uint32_t index,
    std::span<std::byte> p, const SourceUnitPoseRequestServices &s,
    const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  manager(m); require(p.size() >= 4, "Truncated pose index output");
  const auto g = graph(m, s);
  std::uint32_t activity;
  if (index == 1) activity = 0x0e000045;
  else if (index == 0) activity = 0x0e000044;
  else if (index == 2) activity = 0x0b000046;
  else { write(p, 0, 4, none, 0x1cfc42, stores); return; }
  const SourceAnimationIdentifiers ids{read(m, 0x70), read(m, 0x74),
                                       read(m, 0x78), activity};
  const auto site = index == 1 ? 0x1cfbd8u : index == 0 ? 0x1cfc03u : 0x1cfc32u;
  std::array<std::byte, 4> local;
  event(o, site, 0x1df670, ids);
  lookup_source_alternate_animation_request(g, ids, local, {},
                                           s.channels.graphs,
                                           s.channels.resources);
  event(o, site, 0x1df670, ids, read(local, 0));
  write(p, 0, 4, read(local, 0),
        index == 1 ? 0x1cfbdf : index == 0 ? 0x1cfc0a : 0x1cfc39, stores);
}
void update_source_unit_posture_channel(
    std::uint32_t unit, const UnitConstructorObjectPool &pool,
    const SourceUnitPoseRequestServices &s, const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  auto b = body(unit, pool).bytes;
  const auto m = component(b, 0x12a, 0x90), e = component(b, 0x33e, 0xfc);
  auto manager_span = b.subspan(m, 0x90);
  const auto wc = read(manager_span, 0x74), w = read(manager_span, 0x78);
  if (read(manager_span, 0x68) == none) return;
  const auto mode = read(manager_span, 0x70), g = graph(manager_span, s);
  constexpr std::uint32_t activity = 0x10000551;
  std::array<std::byte, 4> p;
  event(o, 0x115b53, 0x1df670, {mode, wc, w, activity});
  lookup_source_alternate_animation_request(g, {mode, wc, w, activity}, p, {},
                                           s.channels.graphs,
                                           s.channels.resources);
  const auto packed = read(p, 0);
  event(o, 0x115b53, 0x1df670, {mode, wc, w, activity}, packed);
  if (!valid(packed)) return;
  auto forward = [&](const UnitAnimationSelectionStore &v) {
    if (stores) { auto a = v; a.offset += e + 0xdc; stores(a); }
  };
  event(o, 0x115b7f, 0x1cee00, {packed, activity, 0x803f, 1});
  const auto result = install_source_unit_requested_channel(
      manager_span, b.subspan(e + 0xdc, 32), packed, activity, 0x803f,
      none, none, 1, s.channels, forward);
  event(o, 0x115b7f, 0x1cee00, {packed, activity, 0x803f, 1}, result);
  if (result) write(b, e, 1, read(b, e, 1) & 0x7f, 0x115b89, stores);
}
void update_source_unit_pose_requests(
    std::uint32_t unit, const UnitConstructorObjectPool &pool,
    const SourceUnitPoseRequestServices &s, const SourceUnitPoseCallObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  const auto owner = body(unit, pool); auto b = owner.bytes;
  const auto signed_manager = offset(b, 0x12a);
  if (owner.address + std::uint32_t(std::int32_t(signed_manager)) == 0) return;
  const auto m = component(b, 0x12a, 0x90), e = component(b, 0x33e, 0x124);
  auto ms = b.subspan(m, 0x90);
  // The original snapshots these inputs before its first lower call.
  const auto alternate_weapon = read(b, e + 0x98), wc = read(ms, 0x74),
             w = read(ms, 0x78), activity = read(ms, 0x7c);
  const auto chosen = activities(activity, b);
  auto lookup = [&](std::uint32_t site, std::uint32_t cls,
                    std::uint32_t weapon, std::uint32_t action,
                    std::size_t target, std::uint32_t store_ip) {
    std::array<std::byte, 4> p;
    event(o, site, 0x1cf4c0, {cls, weapon, action, 0});
    lookup_source_unit_pose_request(ms, cls, weapon, action, p, s, o);
    const auto packed = read(p, 0);
    event(o, site, 0x1cf4c0, {cls, weapon, action, 0}, packed);
    write(b, e + target, 4, packed, store_ip, stores);
  };
  lookup(0x114d5e, wc, w, chosen[0], 0xfc, 0x114d72);
  lookup(0x114d78, wc, w, chosen[1], 0x100, 0x114d85);
  lookup(0x114d93, wc, w, 0x0400004b, 0x114, 0x114da0);
  if (wc == 0x0400054b) {
    lookup(0x114db6, wc, w, 0x0b00054a, 0x104, 0x114dd2);
    lookup(0x114dd8, 0x0400054b, alternate_weapon, 0x0b00054a, 0x108, 0x114de5);
    lookup(0x114df6, 0x0400054b, w, 0x0400060b, 0x10c, 0x114e03);
    lookup(0x114e14, 0x0400054b, alternate_weapon, 0x0400060b, 0x110, 0x114e1b);
  } else {
    write(b, e + 0x104, 2, none, 0x114e26, stores);
    write(b, e + 0x106, 2, none, 0x114e2d, stores);
    write(b, e + 0x108, 2, none, 0x114e34, stores);
    write(b, e + 0x10a, 2, none, 0x114e3b, stores);
    lookup(0x114e4f, wc, w, 0x0400060b, 0x10c, 0x114e5c);
    lookup(0x114e6a, wc, w, 0x0400060b, 0x110, 0x114e71);
  }
  event(o, 0x114e7b, 0x115af0, {unit, 0, 0, 0});
  update_source_unit_posture_channel(unit, pool, s, o, stores);
  event(o, 0x114e7b, 0x115af0, {unit, 0, 0, 0}, {}, true);
  bool has_any = false;
  for (std::uint32_t i = 0; i < 3; ++i) {
    std::array<std::byte, 4> p;
    event(o, 0x114e9d, 0x1cfb90, {i, 0, 0, 0});
    lookup_source_unit_pose_index(ms, i, p, s, o);
    const auto packed = read(p, 0);
    event(o, 0x114e9d, 0x1cfb90, {i, 0, 0, 0}, packed);
    write(b, e + 0x118 + 4 * i, 4, packed, 0x114ea4, stores);
    if (valid(read(b, e + 0x118 + 4 * i))) has_any = true;
  }
  write(b, e, 1, has_any ? read(b, e, 1) | 2 : read(b, e, 1) & 0xfdu,
        has_any ? 0x114ecd : 0x114ed8, stores);
}
} // namespace halo2
