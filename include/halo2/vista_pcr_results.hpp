#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <optional>

namespace halo2 {
// Supplied Vista image33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Actual retained statistics owner8B1C80. This PC bank differs from the Xbox
// SourceRetainedResults layout. Neither bank may be cast to the other.
inline constexpr std::size_t vista_pcr_source_size=0xdc78;
struct VistaPcrResults {
  std::uint32_t variant{},player_count{},team_count{};
  std::array<std::array<std::byte,0x110>,16> players{};
  std::array<std::array<std::byte,0x84>,16> teams{};
  bool operator==(const VistaPcrResults&) const = default;
};
struct VistaPcrOrdering {
  std::array<std::uint32_t,16> players{},teams{};
  unsigned player_count{},team_count{};
};
// Literal468E88 with468DFD/468E3F and the actual48D12B sort, including its
// tie ordering. Returns no indices unless actual current/finalized bytes admit.
VistaPcrOrdering vista_pcr_order(std::span<const std::byte> source);

struct VistaPcrServices {
  // Actual46AD98 and46E361 results. An absent engine preserves the old bank.
  bool engine_present{},team_play{};
  // Actual45C341,470182 and46C675 services, in original invocation order.
  // Formatting/locale are explicit owner callbacks, never guessed here.
  std::function<std::u16string(std::uint32_t)> team_name;
  std::function<std::u16string(std::int32_t)> score,placement;
  // Actual466419/40E8B0 identity comparison for the FFA achievement branch.
  // Required for a reached FFA lookup. A null returned record is the actual
  // source lookup result; omitting the callback does not manufacture absence.
  std::function<std::optional<std::array<std::byte,8>>(unsigned)> live_identity;
  std::function<std::array<std::byte,8>()> selected_identity;
  std::function<void(unsigned)> medal_achievement;
};
// Literal40C8CE through40CDD7, including all retained numeric getters, ordered
// opponent columns, original medal group deduplication,16-byte appearance and
// 64-bit identity. An engine absence leaves every output byte unchanged. A
// successful pass clears both complete display banks, even with zero players.
// Source callbacks may throw; their exceptions propagate, and earlier source
// writes remain visible. The later4498DF optional saved-report owner is separate.
void vista_pcr_build(VistaPcrResults&,std::span<const std::byte>,const VistaPcrServices&);
} // namespace halo2
