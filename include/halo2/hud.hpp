#pragma once
#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/hud_bitmap_geometry.hpp"
#include "halo2/weapon_hud.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_hud_score.hpp"
#include "halo2/motion_sensor.hpp"
#include "halo2/prompt_device.hpp"
#include "halo2/full_scoreboard_paint_constants.hpp"
#include <memory>
namespace halo2 {
struct HudEffect {
  unsigned flags{};
  std::array<ShaderOverlay, 5> functions;
};
struct HudWidget {
  std::string name;
  std::array<unsigned, 4> inputs{};
  std::array<std::uint16_t, 4> yes{}, no{};
  unsigned anchor{}, flags{}, special{}, weapon_type{};
  std::array<unsigned, 3> cutoffs{};
  std::uint32_t bitmap = null_datum, shader = null_datum, text{};
  std::array<int, 3> sequence{}, fonts{};
  std::array<Vec2, 3> offset{}, registration{};
  unsigned justification{};
  bool is_text{};
  std::vector<HudEffect> effects;
};
struct HudContent {
  std::vector<HudWidget> widgets;
  std::map<std::uint32_t, UiImage> images;
  std::map<std::uint32_t, Material> shaders;
  UiStrings strings;
  struct ActionMessages {
    bool enabled=false;
    std::uint32_t color{};
    // item+CC/D0/D4/D8/DC: pickup, swap, pickup-or-dual, swap-or-dual, dual.
    std::map<std::uint32_t,std::array<std::uint32_t,5>> weapons;
    std::map<std::uint32_t,std::uint32_t> weapon_trades;
    struct UnitMessages {std::vector<std::uint32_t> seats;std::uint32_t flip{};};
    std::map<std::uint32_t,UnitMessages> units;
  } action_messages;
  struct Messages {
    bool enabled{};
    float hold_seconds{},fade_seconds{},line_spacing{};
  } messages;
  float multiplayer_scale = 1;
  int clip_cutoff{}, ammo_cutoff{}, clip_capacity{}, reserve_capacity{};
  unsigned weapon_magazine_count{},weapon_trigger_count{};
  std::int16_t weapon_trigger_behavior{},weapon_charging_type{};
  std::optional<unsigned> unit_representation; // Actual matg representation for this biped.
  bool score_scenario_present{};
  std::vector<SourceHudScoreColor> score_appearance_colors, score_team_colors;
  struct Sensor {
    float range{},minimum_speed{},sneak_speed{};
    std::array<std::uint8_t,4> blip_sizes{};
    MotionSensorArtwork artwork;
  } sensor;
};
SourceWeaponHudState multiplayer_hud_weapon_state(const MultiplayerState::Player &,const HudContent &,unsigned hand);

// The current match snapshot carries individual/team signed-short scores and
// source membership/flags, round wins and saved appearance. A missing saved
// profile retains the original default appearance constructor.
SourceHudScoreDisplay multiplayer_hud_score_display(const MultiplayerState&,
    unsigned player, int score_limit, unsigned time_ticks, const HudContent&);
struct HudViewport {
  UiRect outer, safe;
  unsigned mode{};
};
// Omitted preferences retain the original aspect-selected grid for reference
// callers. Ordinary gameplay supplies its explicit two-player preference.
constexpr bool hud_layout_widescreen(unsigned players,bool widescreen,
    std::optional<TwoPlayerSplit> split = {}) {
  return players==2 && split ? *split==TwoPlayerSplit::Vertical : widescreen;
}
HudViewport hud_viewport(unsigned players, unsigned view,
                         bool widescreen = false,std::optional<TwoPlayerSplit> split = {});
bool hud_widget_visible(const HudWidget &, std::array<std::uint16_t, 4> state);
unsigned hud_screen_mode(unsigned width, unsigned height, unsigned anchor = 0,
                         bool widescreen = false);
struct HudShieldHistory {
  float previous = -1, pulse = -1;
  std::uint32_t tick = ~0u;
};
struct HudShieldLayer {
  bool visible{};
  std::array<float, 4> inputs{};
  std::array<ShaderVector, 3> colors{};
};
void update_hud_shield(HudShieldHistory &, float current, std::uint32_t tick);
HudShieldLayer hud_shield_layer(const HudShieldHistory &, float current,
                                unsigned layer, bool elite = false);
class GameplayFontBank;
class ControllerPromptArtwork;
class MultiplayerHud {
public:
  MultiplayerHud(HudContent, const std::filesystem::path &fonts, Mesh &,
                 unsigned local_players, std::string variant = "Slayer",
                 std::optional<HudBitmapGeometryConstants> geometry = {},
                 const std::filesystem::path &ui_assets = {},unsigned hand = 0,
                 std::shared_ptr<GameplayFontBank> font_bank = {},bool defusal_objectives = false);
  ~MultiplayerHud();
  MultiplayerHud(const MultiplayerHud &)=delete;
  MultiplayerHud &operator=(const MultiplayerHud &)=delete;
  MultiplayerHud(MultiplayerHud &&)=delete;
  MultiplayerHud &operator=(MultiplayerHud &&)=delete;
  class Checkpoint {
  public:bool valid()const noexcept;
  private:friend class MultiplayerHud;struct Data;std::shared_ptr<const Data> data_;
  };
  class PreparedRestore {
  public:
    ~PreparedRestore();PreparedRestore(PreparedRestore &&)noexcept;
    PreparedRestore &operator=(PreparedRestore &&)noexcept;
  private:friend class MultiplayerHud;PreparedRestore();struct Data;std::unique_ptr<Data> data_;
  };
  Checkpoint checkpoint()const;
  // Identity/executor only; preparation also admits current resources/bindings.
  bool owns_checkpoint(const Checkpoint &)const noexcept;
  // Exact CPU values only. Current resolved key/button bindings must still
  // match; actual device admission and material/GPU images are separate owners.
  PreparedRestore prepare_restore(const Checkpoint &)const;
  bool restore_ready(const PreparedRestore &)const noexcept;
  // Swap-only, same completed executor. Keep the consumed token until
  // displaced strings/history may retire outside the enclosing publication.
  bool commit_restore(PreparedRestore &)noexcept;
  // Completed-frame presentation replacement. Discard private clocks and
  // trails before observing the accepted world. Keep uploaded bitmap/font/
  // material identities, authored donor sampling and local input bindings.
  // The caller must update/animate every local view and invalidate its radar
  // upload tick before drawing. This does not change authoritative vitality,
  // scores or input.
  void reset_lifetimes()noexcept;
  // Consume each actual accepted simulation snapshot once, before drawing.
  void update_motion_sensor(const MultiplayerState &,unsigned player,unsigned view);
  std::optional<unsigned> motion_sensor_material(unsigned view)const;
  void voice_text(std::string text);
  void prompt_gamepad(unsigned view,bool enabled);
  void prompt_device(unsigned view,PromptDevice);
  void prompt_gamepad_buttons(unsigned view,std::array<std::uint32_t,2> buttons);
  void prompt_keyboard_buttons(unsigned view,std::array<std::string,2> buttons);
  void prompt_boarding_buttons(unsigned view,const PcKeyboardSettings &,const ControllerSettings &);
  std::string action_text(const MultiplayerState &,unsigned player,unsigned view)const;
  std::vector<Vertex> draw(const MultiplayerState &, unsigned player,
                           unsigned view, unsigned players, unsigned width,
                           unsigned height, int score_limit,
                           unsigned time_ticks,
                           std::optional<Vec2> player_aim_anchor_pixels = {},
                           std::optional<TwoPlayerSplit> split = {},
                           std::optional<SourceHudAimCamera> target_camera = {});
  // 25B53A..25B5BE: independent centered status line. Alpha is the caller's
  // actual per-view HUD opacity; the ordinary native caller supplies one.
  std::vector<Vertex> draw_player_status(std::u16string_view,const MultiplayerState&,
      unsigned player,unsigned view,unsigned players,unsigned width,unsigned height,
      const FullScoreboardPaintConstants&,std::optional<TwoPlayerSplit> split,float alpha);
  // Per-viewport shader copies bind the original widget inputs independently.
  void animate(const MultiplayerState &, unsigned player, unsigned view,
               int score_limit, unsigned time_ticks, double seconds);

private:
  struct Continuity;struct Values;class Call;
  std::string action_text_impl(const MultiplayerState &,unsigned,unsigned)const;
  HudContent content_;
  unsigned hand_{};
  std::string voice_text_;
  std::array<std::shared_ptr<const RetailFont>, 2> fonts_;
  std::array<std::shared_ptr<const RetailFont>,2> action_fonts_;
  std::array<unsigned,2> action_materials_{};
  std::array<std::shared_ptr<const RetailFont>,2> status_fonts_;
  std::array<unsigned,2> status_materials_{};
  std::array<std::int16_t,2> status_font_offsets_{};
  bool defusal_objectives_{};
  std::optional<unsigned> defusal_marker_material_;
  std::array<std::shared_ptr<const RetailFont>,2> action_button_fonts_;
  std::array<unsigned,2> action_button_materials_{};
  bool vista_action_buttons_{};
  std::array<bool,4> prompt_gamepad_{};
  std::array<PromptDevice,4> prompt_devices_{};
  std::shared_ptr<ControllerPromptArtwork> prompt_artwork_;
  std::array<std::array<std::uint32_t,4>,4> prompt_buttons_{};
  std::array<std::array<std::string,4>,4> prompt_keyboard_{};
  Mesh &mesh_;
  std::vector<std::vector<std::vector<unsigned>>> materials_;
  // Authored donor layer placement, independent of simulation/world lifetime.
  std::map<unsigned,std::array<float,2>> sampled_offsets_;
  double seconds_{};
  std::string variant_;
  std::optional<HudBitmapGeometryConstants> geometry_;
  struct Winning {
    float amount{};
    std::uint32_t tick{};
    bool initialized{};
  };
  std::array<Winning, 4> winning_{};
  std::array<HudShieldHistory, 4> shields_{};
  struct SensorView {
    MotionSensorHistory history{};
    std::uint32_t tick=~0u,unit=null_datum;
    std::optional<unsigned> material;
  };
  std::array<SensorView,4> sensors_{};
  std::shared_ptr<Continuity> continuity_;
};
} // namespace halo2
