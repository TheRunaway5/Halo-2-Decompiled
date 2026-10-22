#include "halo2/unit_channel_request.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu, current = 0x07000101u;
void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t p,
                   unsigned n = 4) {
  require(p <= b.size() && n <= b.size() - p,
          "Truncated retained source channel request");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[p + i])) << (i * 8);
  return v;
}
void write(std::span<std::byte> b, std::size_t p, unsigned n, std::uint32_t v,
           std::uint32_t ip, const UnitAnimationSelectionObserver &o) {
  auto before = read(b, p, n);
  for (unsigned i = 0; i < n; ++i)
    b[p + i] = std::byte((v >> (8 * i)) & 255);
  if (o)
    o({ip, p, n, before, read(b, p, n)});
}
void clear(std::span<std::byte> c, std::uint32_t one,
           const UnitAnimationSelectionObserver &o) {
  struct Row {
    std::size_t offset;
    unsigned size;
    std::uint32_t value, ip;
  };
  const Row rows[] = {{0, 4, none, 0x1ced3a},   {4, 2, none, 0x1ced3c},
                      {6, 2, none, 0x1ced40},   {0x1c, 4, 0, 0x1ced44},
                      {0x10, 1, 0, 0x1ced51},   {0x11, 1, 0, 0x1ced54},
                      {0x12, 2, 0, 0x1ced57},   {0x14, 2, 0, 0x1ced5b},
                      {0x16, 2, 0, 0x1ced5f},   {0x18, 4, one, 0x1ced67},
                      {8, 4, none, 0x1ced6c},   {0xc, 1, none, 0x1ced6f},
                      {0xd, 1, none, 0x1ced72}, {0xe, 2, none, 0x1ced75}};
  for (auto r : rows)
    write(c, r.offset, r.size, r.value, r.ip, o);
}
} // namespace
bool install_source_unit_requested_channel(
    std::span<std::byte> m, std::span<std::byte> c, std::uint32_t packed,
    std::uint32_t activity, std::uint32_t flags, std::uint32_t first,
    std::uint32_t second, std::uint32_t last,
    const SourceUnitChannelRequestServices &s,
    const UnitAnimationSelectionObserver &o) {
  require(m.size() >= 0x90 && c.size() >= 32,
          "Missing retained general channel installation storage");
  if (read(m, 0x68) == none)
    return false;
  if (flags & 0x10) {
    require(bool(s.variant), "Original1CEDA0 RNG/resource owner unavailable");
    packed = s.variant(m, packed);
  }
  if (!install_source_unit_animation_channel(c, 0, read(m, 0x68), flags, packed,
                                             activity, first, second, last,
                                             s.one_bits, o))
    return false;
  if (flags & 4) {
    require(bool(s.clamp_frame),
            "Original1C9F50 definition/clock owner unavailable");
    s.clamp_frame(c, 0);
  }
  if (flags & 8)
    write(c, 0x18, 4, s.one_bits, 0x1cee69, o);
  return true;
}
bool update_source_unit_channel_request(
    std::span<std::byte> m, std::span<std::byte> c, std::uint32_t wc,
    std::uint32_t weapon, const SourceUnitChannelRequestServices &s,
    const UnitAnimationSelectionObserver &o) {
  require(c.size() >= 32, "Missing retained32-byte request channel");
  if (read(c, 0) == none || read(c, 6, 2) == 65535)
    return false;
  require(m.size() >= 0x90, "Missing retained90-byte request manager");
  require(bool(s.graphs.graph_address),
          "Original request graph identity owner unavailable");
  const auto graph = s.graphs.graph_address(read(m, 0x68));
  const auto previous = read(c, 4), activity = read(c, 8);
  const auto type = read(c, 0xe, 1);
  if (wc == current)
    wc = read(m, 0x74);
  if (weapon == current)
    weapon = read(m, 0x78);
  const SourceAnimationIdentifiers ids{read(m, 0x70), wc, weapon, activity};
  std::uint32_t packed = none;
  switch (type) {
  case 0: {
    std::array<std::byte, 4> output{};
    lookup_source_animation_request(graph, ids, output, {}, s.graphs,
                                    s.resources);
    packed = read(output, 0);
    break;
  }
  case 1:
    require(bool(s.alternate_lookup),
            "Original1DF670 alternative request/resource owner unavailable");
    packed = s.alternate_lookup(graph, ids);
    break;
  case 4:
  case 5: {
    const auto bank = graph + (type == 4 ? 0x3cu : 0x44u);
    require(bank >= graph, "Escaped source animation bank");
    auto p = find_source_animation_record(bank, activity,
                                          type == 4 ? 0x28u : 0x14u, s.graphs);
    if (p) {
      require(p <= 0xfffffffbu, "Escaped source animation record");
      require(bool(s.graphs.read),
              "Original channel record backing unavailable");
      packed = read(s.graphs.read(p + 4, 4), 0);
    }
    break;
  }
  default:
    clear(c, s.one_bits, o);
    return false;
  }
  if ((packed >> 16) != 65535) {
    if (packed == previous)
      return true;
    if (install_source_unit_requested_channel(
            m, c, packed, activity, read(c, 0x12, 2), none, none, type, s, o))
      return true;
  }
  clear(c, s.one_bits, o);
  return false;
}
} // namespace halo2
