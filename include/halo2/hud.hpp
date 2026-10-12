#pragma once
#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/hud_bitmap_geometry.hpp"
#include "halo2/source_hud_score.hpp"
#include "halo2/motion_sensor.hpp"
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
  float multiplayer_scale = 1;
  int clip_cutoff{}, ammo_cutoff{}, clip_capacity{};
  bool score_scenario_present{};
  std::vector<SourceHudScoreColor> score_appearance_colors, score_team_colors;
  struct Sensor {
    float range{},minimum_speed{},sneak_speed{};
    std::array<std::uint8_t,4> blip_sizes{};
    MotionSensorArtwork artwork;
  } sensor;
};
// The current match snapshot carries individual/team signed-short scores and
// source membership/flags. Round-win and saved appearance producers are open;
// this adapter uses the original default appearance constructor.
SourceHudScoreDisplay multiplayer_hud_score_display(const MultiplayerState&,
    unsigned player, int score_limit, unsigned time_ticks, const HudContent&);
struct HudViewport {
  UiRect outer, safe;
  unsigned mode{};
};
HudViewport hud_viewport(unsigned players, unsigned view,
                         bool widescreen = false);
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
class MultiplayerHud {
public:
  MultiplayerHud(HudContent, const std::filesystem::path &fonts, Mesh &,
                 unsigned local_players, std::string variant = "Slayer",
                 std::optional<HudBitmapGeometryConstants> geometry = {},
                 const std::filesystem::path &ui_assets = {},unsigned hand = 0);
  // Consume each actual accepted simulation snapshot once, before drawing.
  void update_motion_sensor(const MultiplayerState &,unsigned player,unsigned view);
  std::optional<unsigned> motion_sensor_material(unsigned view)const;
  void voice_text(std::string text){voice_text_=std::move(text);}
  void prompt_gamepad(unsigned view,bool enabled){prompt_gamepad_.at(view)=enabled;}
  void prompt_gamepad_buttons(unsigned view,std::array<std::uint32_t,2> buttons){for(unsigned i=0;i<2;++i)prompt_buttons_.at(view)[i]=buttons[i];}
  void prompt_keyboard_buttons(unsigned view,std::array<std::string,2> buttons){for(unsigned i=0;i<2;++i)prompt_keyboard_.at(view)[i]=std::move(buttons[i]);}
  void prompt_boarding_buttons(unsigned view,const PcKeyboardSettings &,const ControllerSettings &);
  std::string action_text(const MultiplayerState &,unsigned player,unsigned view)const;
  std::vector<Vertex> draw(const MultiplayerState &, unsigned player,
                           unsigned view, unsigned players, unsigned width,
                           unsigned height, int score_limit,
                           unsigned time_ticks,
                           std::optional<Vec2> player_aim_anchor_pixels = {});
  // Per-viewport shader copies bind the original widget inputs independently.
  void animate(const MultiplayerState &, unsigned player, unsigned view,
               int score_limit, unsigned time_ticks, double seconds);

private:
  HudContent content_;
  unsigned hand_{};
  std::string voice_text_;
  std::array<RetailFont, 2> fonts_;
  std::array<RetailFont,2> action_fonts_;
  std::array<unsigned,2> action_materials_{};
  std::array<RetailFont,2> action_button_fonts_;
  std::array<unsigned,2> action_button_materials_{};
  bool vista_action_buttons_{};
  std::array<bool,4> prompt_gamepad_{};
  std::array<std::array<std::uint32_t,4>,4> prompt_buttons_{{
      {0xe102,0xe104,0xe106,0xe101},{0xe102,0xe104,0xe106,0xe101},{0xe102,0xe104,0xe106,0xe101},{0xe102,0xe104,0xe106,0xe101}}};
  std::array<std::array<std::string,4>,4> prompt_keyboard_{{
      {"E","X","Mouse 2","Q"},{"E","X","Mouse 2","Q"},{"E","X","Mouse 2","Q"},{"E","X","Mouse 2","Q"}}};
  Mesh &mesh_;
  std::vector<std::vector<std::vector<unsigned>>> materials_;
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
};
} // namespace halo2
