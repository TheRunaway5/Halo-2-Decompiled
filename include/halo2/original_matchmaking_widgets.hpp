#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <memory>
namespace halo2 {
// Unchanged TU1.5 c_matchmaking_screen::get_title, 0x257F2D.
std::uint32_t original_matchmaking_title(int state);
// TU1.5 0x2580B2 explicitly supplies category-list indices0..5 to 0x256C3A.
// Screen child indices include its two shared texts: child6..11 are pane
// records4..9. PG0x2501C0 supplies only five rows to this same format owner.
inline constexpr std::array<unsigned,6> original_matchmaking_option_rows{4,5,6,7,8,9};
inline constexpr std::array<unsigned,6> original_matchmaking_option_selectors{0,1,2,3,4,5};
std::optional<unsigned> original_matchmaking_option_category(int type);
// TU1.5 0x256D51 (retail250A43), actual source variant+44 game type.
unsigned original_matchmaking_game_icon(int type);
enum class OriginalMatchmakingNeeded { hidden, players_needed, no_players_needed };
// TU1.5 0x2582BB writes pane1 child5/raw3; unavailable progress hides it.
OriginalMatchmakingNeeded original_matchmaking_needed(bool available,int count,int required);
// TU1.5 0x2582BB requests a new original tip at first update and strictly
// more than15 seconds later. Choosing its actual localized tip is a separate
// TU1.5 6F150/6F310 (retail8CA70/8CC20) owner; this clock never invents a
// tip or random selection.
class OriginalMatchmakingTipClock {
public:
  explicit OriginalMatchmakingTipClock(std::uint32_t last=0,bool initialized=false);
  ~OriginalMatchmakingTipClock();
  OriginalMatchmakingTipClock(const OriginalMatchmakingTipClock &);
  OriginalMatchmakingTipClock &operator=(const OriginalMatchmakingTipClock &);
  bool update(std::uint32_t now);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
// TU1.5 6F238..6F25E, called by6F150 after reading the actual downloaded
// message_of_the_day.dat list. The primary RNG owner is an explicit input;
// this never seeds a substitute UI/effects stream or fabricates a tip list.
std::optional<unsigned> original_matchmaking_tip_index(std::uint32_t &seed,unsigned count);
} // namespace halo2
