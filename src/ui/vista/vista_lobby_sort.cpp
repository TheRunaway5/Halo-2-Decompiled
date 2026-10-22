#include "halo2/vista_lobby_sort.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <bit>
#include <utility>

namespace halo2 {
std::int32_t vista_lobby_compare_players(std::uint32_t a, std::uint32_t b,
    const VistaLobbySortConfiguration& configuration) {
  if (!configuration) throw FormatError("Missing Vista lobby sort configuration owner");
  const auto first = configuration(a);
  const auto second = configuration(b);
  if (first.size() <= 0x7c || second.size() <= 0x7c)
    throw FormatError("Unowned Vista lobby sort configuration team byte");
  const auto left = std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(first[0x7c]));
  const auto right = std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(second[0x7c]));
  return left > right ? 1 : left < right ? -1 : 0;
}

void vista_lobby_sort_players(std::span<std::uint32_t> players,
    const VistaLobbySortConfiguration& configuration) {
  if (players.size() > 16) throw FormatError("Vista lobby sort exceeds its16 player slots");
  if (players.size() < 2) return;
  using Index = std::ptrdiff_t;
  const auto compare = [&](Index a, Index b) {
    return vista_lobby_compare_players(players[std::size_t(a)], players[std::size_t(b)], configuration);
  };
  const auto swap = [&](Index a, Index b) {
    std::swap(players[std::size_t(a)], players[std::size_t(b)]);
  };
  std::array<std::pair<Index, Index>, 30> pending;
  std::size_t depth{};
  Index low{}, high = Index(players.size()) - 1;
  for (;;) {
    const auto count = high - low + 1;
    if (count <= 8) {
      //68B0AE selects the first strictly greatest element and swaps it
      //with the high endpoint on each pass, including equal-team records.
      for (auto end = high; end > low; --end) {
        auto greatest = low;
        for (auto cursor = low + 1; cursor <= end; ++cursor)
          if (compare(cursor, greatest) > 0) greatest = cursor;
        swap(greatest, end);
      }
    } else {
      auto middle = low + count / 2;
      if (compare(low, middle) > 0) swap(low, middle);
      if (compare(low, high) > 0) swap(low, high);
      if (compare(middle, high) > 0) swap(middle, high);
      auto lower = low, upper = high;
      for (;;) {
        if (middle > lower) {
          do { ++lower; }
          while (lower < middle && compare(lower, middle) <= 0);
        }
        if (middle <= lower) {
          do { ++lower; }
          while (lower <= high && compare(lower, middle) <= 0);
        }
        do { --upper; }
        while (upper > middle && compare(upper, middle) > 0);
        if (lower > upper) break;
        swap(lower, upper);
        if (middle == upper) middle = lower;
      }
      ++upper;
      if (middle < upper) {
        do { --upper; }
        while (upper > middle && compare(upper, middle) == 0);
      }
      if (middle >= upper) {
        do { --upper; }
        while (upper > low && compare(upper, middle) == 0);
      }
      // Retain the source's partition choice and LIFO order. Comparison
      // callbacks can observe this order through the configuration owner.
      if (upper - low >= high - lower) {
        if (low < upper) pending[depth++] = {low, upper};
        if (lower < high) { low = lower; continue; }
      } else {
        if (lower < high) pending[depth++] = {lower, high};
        if (low < upper) { high = upper; continue; }
      }
    }
    if (!depth) return;
    const auto range = pending[--depth]; low = range.first; high = range.second;
  }
}
} // namespace halo2
