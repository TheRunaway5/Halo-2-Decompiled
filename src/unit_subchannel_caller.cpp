#include "halo2/unit_subchannel_caller.hpp"
#include <array>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t p,
                   unsigned n = 4) {
  require(p <= b.size() && n <= b.size() - p,
          "Truncated actual subchannel owner storage");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[p + i])) << (8 * i);
  return v;
}
std::size_t component(std::span<const std::byte> b, std::size_t p,
                      std::size_t n) {
  const auto signed_offset =
      std::bit_cast<std::int16_t>(std::uint16_t(read(b, p, 2)));
  require(signed_offset >= 0 && std::size_t(signed_offset) <= b.size() &&
              n <= b.size() - std::size_t(signed_offset),
          "Missing source-created subchannel component");
  return std::size_t(signed_offset);
}
} // namespace
void update_source_unit_subchannels(
    std::uint32_t unit, const UnitConstructorObjectPool &pool,
    const SourceUnitChannelRequestServices &s,
    const SourceUnitSubchannelObserver &o,
    const UnitAnimationSelectionObserver &stores) {
  const auto index = unit & 65535u;
  require(index < pool.capacity && bool(pool.body),
          "Missing actual subchannel body owner");
  const auto header = std::size_t(index) * 12;
  require(header <= pool.headers.size() && 12 <= pool.headers.size() - header,
          "Truncated actual subchannel header backing");
  require(read(pool.headers, header + 8) != 0,
          "Missing actual source header body address");
  auto b = pool.body(std::uint16_t(index));
  const auto extent = read(pool.headers, header + 6, 2);
  require(extent <= b.size(), "Truncated owned subchannel body extent");
  b = b.first(extent);
  const auto m = component(b, 0x12a, 0x90), e = component(b, 0x33e, 0xfc);
  struct Row {
    std::size_t channel;
    std::uint32_t instruction;
  };
  const Row order[] = {
      {e + 0x9c, 0x114f38}, {e + 0xdc, 0x114f4b}, {e + 0xbc, 0x114f62}};
  for (unsigned i = 0; i < 3; ++i) {
    const auto wc = i == 2 ? 0x0400054bu : 0x07000101u;
    // The third weapon is deliberately read after both preceding lower calls.
    const auto weapon = i == 2 ? read(b, e + 0x98) : 0x07000101u;
    const auto row = order[i];
    SourceUnitSubchannelCall call{row.instruction, unit,  m,    row.channel, wc,
                                  weapon,          false, false};
    if (o)
      o(call);
    auto forward = [&](const UnitAnimationSelectionStore &v) {
      if (stores) {
        auto at = v;
        at.offset += row.channel;
        stores(at);
      }
    };
    const auto result = update_source_unit_channel_request(
        b.subspan(m, 0x90), b.subspan(row.channel, 32), wc, weapon, s, forward);
    call.returning = true;
    call.result = result;
    if (o)
      o(call);
  }
}
} // namespace halo2
