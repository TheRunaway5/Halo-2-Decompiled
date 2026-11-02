#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
namespace halo2 {
// Unchanged TU1.5 c_matchmaking_screen::get_title, 0x257F2D.
inline constexpr std::uint32_t original_matchmaking_title(int state) {
  constexpr std::array<std::uint32_t,9> titles{0x1400043a,0x1200043b,0x1300043c,
    0x1300043d,0x0e00043e,0x0c00043f,0x0d000440,0x0d000440,0x16000441};
  return state>=0&&unsigned(state)<titles.size()?titles[unsigned(state)]:0;
}
// TU1.5 0x2580B2 explicitly supplies category-list indices0..5 to 0x256C3A.
// Screen child indices include its two shared texts: child6..11 are pane
// records4..9. PG0x2501C0 supplies only five rows to this same format owner.
inline constexpr std::array<unsigned,6> original_matchmaking_option_rows{4,5,6,7,8,9};
inline constexpr std::array<unsigned,6> original_matchmaking_option_selectors{0,1,2,3,4,5};
inline constexpr std::optional<unsigned> original_matchmaking_option_category(int type) {
  switch(type){case 1:return 22;case 2:return 23;case 3:return 24;case 4:return 25;
    case 7:return 28;case 8:return 29;case 9:return 30;default:return {};}
}
// TU1.5 0x256D51 (retail250A43), actual source variant+44 game type.
inline constexpr unsigned original_matchmaking_game_icon(int type) {
  switch(type){case 1:return 6;case 2:return 0;case 3:return 3;case 4:return 1;
    case 7:return 4;case 8:return 8;case 9:return 7;default:return 10;}
}
enum class OriginalMatchmakingNeeded { hidden, players_needed, no_players_needed };
// TU1.5 0x2582BB writes pane1 child5/raw3; unavailable progress hides it.
inline constexpr OriginalMatchmakingNeeded original_matchmaking_needed(bool available,int count,int required) {
  return !available?OriginalMatchmakingNeeded::hidden:count<required?
    OriginalMatchmakingNeeded::players_needed:OriginalMatchmakingNeeded::no_players_needed;
}
// TU1.5 0x2582BB requests a new original tip at first update and strictly
// more than15 seconds later. Choosing its actual localized tip is a separate
// TU1.5 6F150/6F310 (retail8CA70/8CC20) owner; this clock never invents a
// tip or random selection.
struct OriginalMatchmakingTipClock {
  std::uint32_t last{};
  bool initialized{};
  bool update(std::uint32_t now) {
    if(std::bit_cast<std::int32_t>(now-last)>15000){initialized=false;last=now;}
    if(initialized)return false;initialized=true;return true;
  }
};
// TU1.5 6F238..6F25E, called by6F150 after reading the actual downloaded
// message_of_the_day.dat list. The primary RNG owner is an explicit input;
// this never seeds a substitute UI/effects stream or fabricates a tip list.
inline std::optional<unsigned> original_matchmaking_tip_index(std::uint32_t &seed,unsigned count) {
  if(count>32)return {};
  seed=seed*0x19660du+0x3c6ef35fu;
  return ((seed>>16)*count)>>16;
}
} // namespace halo2
