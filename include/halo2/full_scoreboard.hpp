#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
// Literal words imported from the selected original executable. No font or
// localized label payload is embedded in this module.
struct FullScoreboardConstants {
  std::uint32_t counter_seconds{}, fade_scale{}, stagger_fraction{}, stagger_scale{};
  std::uint32_t one{}, header_rgb{}, background_alpha{}, text_scale{}, text_bias{}, dim_scale{};
  std::uint32_t scroll_axis_scale{}, scroll_pixels{};
  std::uint64_t fade_exponent{};
};
struct FullScoreboardRect { std::int16_t top{}, left{}, bottom{}, right{}; };
struct FullScoreboardWidget {
  bool valid{}, right_justified{};
  FullScoreboardRect rectangle{}, text_rectangle{};
  std::array<float, 4> background{}, foreground{}; // A,R,G,B
  std::u16string text;
};
struct FullScoreboardPlayer {
  std::uint32_t datum{}, current_unit{0xffffffffu};
  std::uint16_t flags{};
  std::int8_t membership{-1};
  std::int16_t local_view{-1}, lives_remaining{};
  // Literal source stat shorts at engine+304+slot*1C+[4,6,8,A,10,12].
  std::int16_t score{}, total_score{}, stat8{}, stat_a{}, stat10{}, wins{};
  // Delivered output of the original name sanitizer (233CF1), before the
  // widget's own19-code-unit truncation. Raw markup/device substitution stays
  // with the caller's text provider.
  std::u16string_view display_name;
  // Original830C0 palette result, not a generated slot color.
  std::array<float, 3> color{};
  // Actual69E80 admission/quality result, when available. No ping conversion.
  std::optional<std::int16_t> connection_quality;
};
struct FullScoreboardOptions {
  bool engine_present{}, engine_active{}, team_mode{}, game_over{}, extended_round_comparison{};
  std::int32_t display_mode{}, score_limit{}, round_limit{}, life_limit{};
};
struct FullScoreboardTeam {
  // Array position is the original team designator0..7. Retained membership
  // selects the team rows; playing membership controls sorting and dimming.
  bool retained{}, playing{};
  // Literal source stat shorts at engine+304+team*12+[1C4,1C6,1C8,1CA,1D0,1D2].
  std::int16_t score{}, total_score{}, stat8{}, stat_a{}, stat10{}, wins{};
  std::u16string_view display_name;
  std::array<float, 3> color{}; // original83050 palette delivery
};
// Original roster sorting and encoded ranks, independent of font/layout.
// Entries are indexed by actual player slot/team designator; FF means absent.
struct FullScoreboardRanks {
  std::array<std::uint8_t,16> players;
  std::array<std::uint8_t,8> teams;
};
FullScoreboardRanks full_scoreboard_ranks(std::span<const FullScoreboardPlayer>,
    const FullScoreboardOptions&, std::span<const FullScoreboardTeam> = {});
struct FullScoreboardLabels {
  // Literal locale/title delivery. Empty is the original unavailable-locale
  // branch. Titles/Name/Wins/Score are not invented in this module.
  std::u16string_view title, name, wins, score;
};
// Original15BB50 decimal/time selection; callers own the output capacity.
std::u16string full_scoreboard_score_text(std::int32_t score,std::int32_t display_mode);
struct FullScoreboardRow {
  std::uint32_t datum{};
  std::uint8_t encoded_rank{};
  bool current_player{};
  std::int16_t icon_kind{};
  std::array<FullScoreboardWidget, 6> widgets; // place,icon,name,wins,score,connection
};
struct FullScoreboardTeamRow {
  std::uint8_t team{}, encoded_rank{};
  std::array<FullScoreboardWidget, 4> widgets; // place,name,wins,score
};
enum class FullScoreboardStatus { Ready, InvalidInput };
struct FullScoreboardLayout {
  FullScoreboardStatus status{FullScoreboardStatus::InvalidInput};
  std::array<bool, 3> columns{};
  float scroll{}, input_alpha{};
  FullScoreboardWidget title;
  std::array<FullScoreboardWidget, 4> headers; // place,Name,Wins,Score
  std::vector<FullScoreboardTeamRow> team_rows;
  std::vector<FullScoreboardRow> rows;
};
// Original163360/162B70/162E70 roster layout. Viewport, post-sanitizer names,
// resolved colors, locale/title and existing engine scores are explicit inputs.
// Team mode requires all eight stat records, including unretained teams whose
// statistics can still participate in player comparisons. Source score/world/
// graphics ownership remains with the caller.
FullScoreboardLayout full_scoreboard_layout(std::span<const FullScoreboardPlayer>,
    std::uint32_t current_player, const FullScoreboardOptions&,
    FullScoreboardRect outer, FullScoreboardRect inner, float scroll,
    float input_alpha, const FullScoreboardLabels&, const FullScoreboardConstants&,
    std::span<const FullScoreboardTeam> teams = {});

struct FullScoreboardClock { std::int16_t rate{}; float seconds_per_tick{}; std::int32_t tick{}; };
enum class FullScoreboardRounding { NearestEven, Down, Up, TowardZero };
struct FullScoreboardHeldState {
  std::array<std::uint8_t, 4> counters{};
  std::uint8_t inactive_counter{};
  std::array<float, 4> scroll{};
};
struct FullScoreboardHeldInput {
  std::int16_t local_view{-1};
  std::int32_t input_user{-1};
  bool processed_record_present{}, held{};
  std::int16_t scroll_axis{};
  bool engine_present{};
  std::int16_t lifecycle{};
  std::uint8_t options_state{};
  std::int32_t engine_status{};
  std::uint32_t current_unit{0xffffffffu};
  std::int32_t death_countdown{};
  std::uint16_t player_flags{};
  bool exhausted{}, death_observer_excluded{}; // owning160570/1656D0 results
};
// Original15E6BC tail. Host bounds reject invalid view/user/rate before mutation.
// x87 rounding mode is a caller input; no Xbox FP startup mode is inferred.
bool full_scoreboard_update_held(FullScoreboardHeldState&,
    const FullScoreboardHeldInput&, FullScoreboardClock,
    FullScoreboardRounding, const FullScoreboardConstants&);
// Original pre-power fade/1645E0 HUD predicate. Missing engine/view retains HUD.
float full_scoreboard_fade(std::uint8_t counter, FullScoreboardClock,
    const FullScoreboardConstants&);
bool full_scoreboard_normal_hud(bool engine_present, std::int16_t local_view,
    std::uint8_t counter, FullScoreboardClock, const FullScoreboardConstants&);
// Original15C2A0 calls an x87 CRT power routine with the imported literal double
// exponent, then rounds to binary32. That math provider remains explicit.
using FullScoreboardPower = float (*)(float, std::uint64_t);
float full_scoreboard_display_alpha(std::uint8_t, FullScoreboardClock,
    const FullScoreboardConstants&, FullScoreboardPower);
// Native libm adapter; equivalence to all original CRT power inputs is not
// claimed by the frozen full-fade (base1) differential.
float full_scoreboard_host_power(float, std::uint64_t exponent_bits);
} // namespace halo2
