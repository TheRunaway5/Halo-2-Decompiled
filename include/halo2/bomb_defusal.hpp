#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
struct UiVariant;
struct NetgameFlag;
// Native extension IDs never occupy the original 304-byte Assault variant.
inline constexpr unsigned native_defusal_parameter=0x80000001u;
inline constexpr unsigned native_defusal_fuse_parameter=0x80000002u;
inline constexpr unsigned native_defusal_plant_parameter=0x80000003u;
inline constexpr unsigned native_defusal_disarm_parameter=0x80000004u;
// Optional ordered site overrides reference existing scenario marker rows.
inline constexpr unsigned native_defusal_site_parameter=0x80000100u;
bool bomb_defusal_parameter(unsigned);
bool bomb_defusal_variant(const UiVariant &);
struct BombDefusalRules {
  unsigned round_ticks=150*30, fuse_ticks=45*30, plant_ticks=4*30,
      defuse_ticks=6*30, freeze_ticks=5*30, intermission_ticks=7*30;
  unsigned half_rounds=12, wins_needed=13;
};
BombDefusalRules bomb_defusal_settings(const UiVariant &);
struct BombSite {
  std::uint32_t marker{};
  Vec3 position;
  float radius=1.5f;
};
struct BombDefusalLayout {
  std::string name;
  unsigned recommended_team_size{};
  std::array<BombSite,2> sites;
  std::array<Vec3,2> spawn_anchors; // Attack, defense; teams exchange at halftime.
};
// Profiles require exact scenario identity and matching authored marker pins.
std::optional<BombDefusalLayout> load_bomb_defusal_layout(const std::filesystem::path &,
    std::string_view scenario,std::span<const NetgameFlag>);
std::optional<BombDefusalLayout> bundled_bomb_defusal_layout(std::string_view scenario,
    std::span<const NetgameFlag>);
// Use every authored defending-side Assault goal, across marker channels.
// Unlike vanilla Assault this has no first-two-goals/first-eight-query limit.
std::vector<BombSite> bomb_defusal_sites(std::span<const NetgameFlag>,const UiVariant * = nullptr);
enum class BombDefusalPhase : std::uint8_t { waiting, freeze, live, planted, ended, complete };
enum class BombDefusalOutcome : std::uint8_t { none, exploded, defused, eliminated, time };
struct BombDefusalPlayer {
  std::uint32_t datum=0xffffffffu;
  int team=-1;
  bool connected{},alive{},use{},moving{},grounded=true;
  Vec3 position;
  bool unobstructed=true;
};
struct BombDefusalState {
  BombDefusalPhase phase=BombDefusalPhase::waiting;
  BombDefusalOutcome outcome=BombDefusalOutcome::none;
  unsigned round{}, remaining{}, progress{};
  int attacking_team{}, winner=-1, match_winner=-1, site=-1;
  std::array<std::uint16_t,2> scores{};
  std::uint32_t carrier=0xffffffffu, actor=0xffffffffu;
  Vec3 bomb_position;
  std::vector<BombSite> sites;
  // Round roster uses full generations; late joins wait until the next round.
  std::array<std::uint32_t,16> roster{};
};
// Each call consumes exactly one authority tick. No client supplies progress,
// plant/defuse completion, round winner, or bomb position.
// true requests a fresh round's player bodies before the following tick.
bool bomb_defusal_tick(BombDefusalState &,const BombDefusalRules &,
    std::span<const BombDefusalPlayer>);
bool bomb_defusal_interacting(const BombDefusalState &,const BombDefusalPlayer &);
void validate_bomb_defusal_state(const BombDefusalState &);
std::string bomb_defusal_status(const BombDefusalState &,const BombDefusalRules &,
    const BombDefusalPlayer &);
struct BombDefusalMarkerVisual {
  float red_fill{}, brightness=1;
  bool planted{};
};
// Derived only from replicated fuse ticks, so views and network peers agree.
BombDefusalMarkerVisual bomb_defusal_marker_visual(const BombDefusalState &,
    const BombDefusalRules &,unsigned site);
} // namespace halo2
