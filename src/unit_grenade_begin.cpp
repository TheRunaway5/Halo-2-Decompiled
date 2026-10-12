#include "halo2/unit_grenade_begin.hpp"
#include <bit>
#include <initializer_list>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
void require(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(n <= 4 && at <= b.size() && n <= b.size() - at,
          "Truncated original grenade storage");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
std::int16_t s16(std::uint32_t v) {
  return std::bit_cast<std::int16_t>(std::uint16_t(v));
}
std::int8_t s8(std::uint32_t v) {
  return std::bit_cast<std::int8_t>(std::uint8_t(v));
}
void write(std::span<std::byte> b, std::size_t at, unsigned n,
           std::uint32_t value, std::uint32_t ip, std::uint32_t datum,
           GrenadeBeginBuffer kind, const GrenadeBeginObserver &o) {
  auto before = read(b, at, n);
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((value >> (8 * i)) & 255);
  if (o)
    o({ip, datum, kind, at, n, before, read(b, at, n)});
}
GrenadeBeginLeafResult call(const SourceGrenadeBeginServices &s,
                            std::uint32_t e,
                            std::initializer_list<std::uint32_t> a,
                            std::span<std::byte> output = {},
                            std::span<const std::byte> point = {},
                            std::span<const std::byte> direction = {}) {
  require(bool(s.invoke), "Original grenade begin lower owner unavailable");
  require(a.size() <= 6, "Too many grenade owner arguments");
  GrenadeBeginLeaf c{e, {}, unsigned(a.size())};
  unsigned i = 0;
  for (auto v : a)
    c.arguments[i++] = v;
  return s.invoke(c, output, point, direction);
}
bool al(GrenadeBeginLeafResult r) { return (r.eax & 255) != 0; }
std::span<std::byte> body(const SourceGrenadeBeginServices &s,
                          std::uint32_t unit) {
  require(bool(s.unit_body), "Original retained unit storage unavailable");
  return s.unit_body(unit);
}
} // namespace

void construct_source_grenade_begin_request(std::span<std::byte> b,
                                            const GrenadeBeginObserver &o) {
  require(b.size() >= 0x20, "Truncated original grenade request");
  constexpr std::uint32_t ips[]{0x151515, 0x151519, 0x15151d, 0x151521,
                                0x151525, 0x151529, 0x15152d, 0x151535};
  for (unsigned i = 0; i < 8; ++i)
    write(b, 4 * i, 4, 0, ips[i], none, GrenadeBeginBuffer::request, o);
  write(b, 0, 4, 22, 0x15153c, none, GrenadeBeginBuffer::request, o);
}
std::span<std::byte>
source_grenade_begin_action_storage(std::span<std::byte> b) {
  const auto offset = s16(read(b, 0x346, 2));
  require(offset >= 0 && std::size_t(offset) <= b.size() &&
              0x44 <= b.size() - std::size_t(offset),
          "Original signed unit action component unavailable");
  return b.subspan(std::size_t(offset), 0x44);
}
bool begin_source_grenade(std::uint32_t unit, std::span<const std::byte> req,
                          const SourceGrenadeBeginServices &s,
                          const GrenadeBeginObserver &o) {
  require(req.size() >= 0x20, "Truncated original grenade request");
  auto u = body(s, unit);
  auto action = source_grenade_begin_action_storage(u);
  const auto hand = s8(read(u, 0x212, 1));
  const auto weapon =
      hand == -1
          ? none
          : read(u, std::size_t(std::int64_t(0x218) + std::int64_t(hand) * 4));
  const auto selected = s8(read(u, 0x23c, 1));
  std::span<const std::byte> definition;
  if (selected != -1) {
    require(bool(s.grenade_definition),
            "Original grenade definition owner unavailable");
    definition = s.grenade_definition(selected);
    require(definition.empty() || definition.size() >= 0x2c,
            "Truncated original grenade definition");
  }
  if (read(u, 0xaa, 1))
    return false;
  if (al(call(s, 0x1055f0, {weapon})))
    return false; // ECX actual current weapon.
  const auto blocked = al(call(s, 0x112270, {unit}));
  if ((blocked || (read(body(s, unit), 0x134) & 0x02000000u)) &&
      !read(req, 5, 1))
    return false;
  bool prefer = true;
  const auto parent = read(u, 0x14);
  const auto seat = s16(read(u, 0x1fc, 2));
  if (parent != none && seat != -1) {
    require(bool(s.seat_definition),
            "Original parent seat metadata unavailable");
    auto record = s.seat_definition(parent, seat);
    const auto flags = read(record, 0);
    prefer = (flags & 0x20) != 0;
    if (flags & 0x800) {
      const auto marker = read(record, 0x10);
      if (!marker)
        return false;
      std::array<std::byte, 12> point;
      if (s16(call(s, 0xb9c20, {parent, marker, 1, 0}, point).eax) != 1)
        return false;
    }
  }
  auto missing = [&]() {
    if (prefer) {
      if (call(s, 0x14f5d0, {unit}).eax != none) {
        const auto player = call(s, 0x14f5d0, {unit}).eax;
        require(bool(s.player_body),
                "Original player/local-user storage unavailable");
        const auto local = s16(read(s.player_body(player), 0x28, 2));
        if (local != -1)
          call(s, 0x204173, {0, std::uint32_t(std::int32_t(local)), 1, none});
      }
    }
    return false;
  };
  if (definition.empty())
    return missing();
  if (s16(call(s, 0xcf440,
               {unit,
                std::uint32_t(std::int32_t(s8(read(body(s, unit), 0x23c, 1))))})
              .eax) <= 0)
    return missing();
  if (!read(req, 5, 1) && (read(action, 4) & 0x00400000u))
    return false;
  prefer = al(call(s, 0xefee0, {unit, 0}));
  for (auto type : {22u, 8u, 18u, 0u, 10u, 27u})
    call(s, 0xe7ff0, {unit, type});
  call(s, 0xe7ef0, {unit, 19});
  auto put = [&](std::size_t at, unsigned n, std::uint32_t v,
                 std::uint32_t ip) {
    write(action, at, n, v, ip, unit, GrenadeBeginBuffer::action, o);
  };
  if (read(req, 5, 1)) {
    put(0xc, 1, 1, 0xe981e);
    put(0xe, 1, 0, 0xe9822);
    put(0xd, 1, 0, 0xe9826);
    call(s, 0xe8d60, {unit});
    call(s, 0xe8f30, {unit, 0}, {}, req.subspan(8, 12), req.subspan(0x14, 12));
    return false;
  }
  std::array<std::byte, 4> channel;
  bool channel_ok = false;
  if (prefer)
    channel_ok =
        al(call(s, 0x115650, {unit, 0x1000006c, 0x3e08b439, 0}, channel));
  if (!channel_ok)
    channel_ok =
        al(call(s, 0x115650, {unit, 0x0d000021, 0x3e08b439, 0}, channel));
  if (!channel_ok)
    return false;
  const auto channel_token = read(channel, 0);
  put(0xc, 1, 1, 0xe9896);
  put(0xe, 1, 0, 0xe989a);
  put(0xf, 1, 0, 0xe989e);
  put(0xd, 1, read(req, 4, 1), 0xe98a5);
  const auto duration = call(s, 0x1ca8d0, {channel_token}).xmm0_bits;
  if (std::bit_cast<float>(duration) >= std::bit_cast<float>(s.zero_bits))
    put(0xf, 1, call(s, 0x148160, {duration}).eax, 0xe98c6);
  if (read(u, 0x14) == none) {
    call(s, 0xef630, {unit});
    call(s, 0xefe30, {unit, 26u + unsigned(prefer)});
    call(s, 0xc9a40, {unit, 0});
  }
  const auto effect = read(definition, 8);
  if (effect != none)
    call(s, 0x1781f0,
         {effect, unit, 0, 0, 0, 0}); // EBX0, stack zeros, XMM0 zero.
  const auto callback = read(definition, 0x20);
  if (callback != none)
    call(s, 0xbcd80, {callback});
  call(s, 0xa99f0, {unit});
  put(4, 4, read(action, 4) | 0x00400000u, 0xe993b);
  return true;
}
} // namespace halo2
