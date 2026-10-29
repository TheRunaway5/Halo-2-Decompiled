#include "halo2/action_prompt_bindings.hpp"
#include "halo2/hud.hpp"
#include "halo2/mcc_hud_bitmap_geometry.hpp"
#include "halo2/slayer.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_event_messages_tu15.hpp"
#include "halo2/full_scoreboard_providers.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/controller_prompt_artwork.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <string_view>
#include <thread>
#include <type_traits>

namespace halo2 {
SourceWeaponHudState multiplayer_hud_weapon_state(const MultiplayerState::Player &p,const HudContent &content,unsigned hand) {
  if(hand>2)throw FormatError("Invalid weapon HUD owner");
  if(hand==2)return p.source_vehicle_hud.weapon;
  const auto &shader=hand?p.secondary_weapon.shader:p.weapon_shader;
  const int loaded=hand?p.secondary_weapon.loaded:p.loaded;
  const int reserve=hand?p.secondary_weapon.reserve:p.reserve;
  const int available=hand?p.secondary_weapon.ammunition_available:p.ammunition_available;
  const auto datum=hand?p.secondary_weapon.datum:p.source_weapon;
  WeaponHudProjectionInput input;input.datum=datum;input.heat=shader.heat;input.age=shader.power_fraction;
  input.magazine_mode=hand?p.secondary_weapon.magazine_mode:p.source_zoom_magazine_mode;
  input.weapon_flags=std::uint16_t(shader.weapon_flags);input.magazine_present=content.weapon_magazine_count!=0;
  input.loaded=std::bit_cast<std::int16_t>(std::uint16_t(loaded));
  input.capacity=std::int16_t(content.clip_capacity);input.maximum_reserve=std::int16_t(content.reserve_capacity);
  //Real snapshots carry104CF0 (loaded plus shared reserve). Datum-less HUD
  //scalar fixtures provide the already-produced reserve word directly.
  input.shared_reserve=datum==null_datum?reserve:available-loaded;
  const auto &trigger=hand?p.secondary_weapon.trigger:p.weapon_trigger;
  input.input_flags=trigger.input_flags;input.charge=trigger.charge;
  input.trigger_present=content.weapon_trigger_count!=0;
  input.trigger_behavior=content.weapon_trigger_behavior;input.charging_type=content.weapon_charging_type;
  input.trigger_state=trigger.mode;input.trigger_ticks=trigger.ticks;
  input.acquire_target=trigger.target_acquired;
  if(trigger.target_position)input.target_position=Vec3{(*trigger.target_position)[0],
      (*trigger.target_position)[1],(*trigger.target_position)[2]};
  SourceWeaponHudState result;weapon_hud_project(result,input);return result;
}
namespace { std::vector<unsigned> codepoints(const std::string &); }
SourceHudScoreDisplay multiplayer_hud_score_display(const MultiplayerState &s,
    unsigned id,int limit,unsigned time_ticks,const HudContent &content) {
  if (id>=s.players.size()) throw FormatError("HUD player outside source table");
  if (!s.players[id].connected) return {};
  SourceHudScoreInput input;
  input.source_type=s.source_engine;input.round_mode=s.round_mode;
  input.teams=s.team_mode;input.limit=limit;input.time_limit=time_ticks ? 1 : 0;
  input.clock_seconds=s.clock_seconds;input.local_player=s.players[id].source_player;
  input.team_scores=s.team_scores;
  for(unsigned team=0;team<8;++team)input.team_wins[team]=s.team_wins[team];
  std::u16string title;
  for (auto c:codepoints(s.variant_name)) {
    if (c<=65535) title+=char16_t(c);
    else {c-=0x10000;title+=char16_t(0xd800+(c>>10));title+=char16_t(0xdc00+(c&1023));}
  }
  input.variant_name=title;
  const auto appearance=full_scoreboard_default_appearance();
  std::array<TeamRespawnPlayer,multiplayer_capacity> presence;
  unsigned count=0;
  for (unsigned n=0;n<s.players.size();++n) {
    const auto &p=s.players[n];if (!p.connected) continue;
    input.connected_players|=std::uint16_t(1u<<n);
    input.players[n]={p.source_player,std::int8_t(p.source_membership),
        std::bit_cast<std::int8_t>(p.source_configuration ? (*p.source_configuration)[0x40] : appearance[0]),p.source_flags,
        std::bit_cast<std::int16_t>(std::uint16_t(p.score)),p.wins};
    presence[count++]={std::int8_t(p.source_membership),p.source_flags,0};
  }
  input.playing_teams=source_team_presence(0,std::span(presence).first(count)).playing;
  const auto result=source_hud_score_display(input,{content.score_scenario_present,
      content.score_appearance_colors,content.score_team_colors});
  if (result.status==SourceHudScoreStatus::InvalidInput ||
      result.status==SourceHudScoreStatus::UnavailableRoundStatistics)
    throw FormatError("Original HUD score query requires unavailable source statistics or membership");
  return result;
}
namespace {
bool elite_hud(const HudContent &content,const MultiplayerState::Player &player) {
  const auto representation=content.unit_representation.value_or(multiplayer_representation(player.source_configuration));
  return representation==1 || representation==3;
}
std::array<std::uint16_t, 4> conditions(const HudWidget &w,const HudContent &content,
                                        const MultiplayerState &s, unsigned id,
                                        const SourceHudScoreDisplay &score,bool talking,unsigned hand) {
  const auto &p = s.players.at(id);
  // Retail13A163..13A18F sets shield availability and Elite appearance
  // independently. TU1.5's2316B4 admits representation1 or3 as Elite.
  std::uint16_t unit = 1 | (p.secondary_weapon.datum==null_datum?0x10:0x20) | p.source_hud_zoom_mask | 0x800 | 0x1000;
  if (elite_hud(content,p)) unit |= 0x2000;
  unit |= p.grenade_current == 0 ? 4 : p.grenade_current == 1 ? 8 : 2;
  const auto projected=multiplayer_hud_weapon_state(p,content,hand);
  const auto *item=(w.weapon_type==1 || w.weapon_type==4)?&projected:nullptr;
  const auto weapon=weapon_hud_cutoffs(item,w.cutoffs,weapon_hud_mask(item,w.weapon_type==1?(hand?2u:1u):w.weapon_type));
  const auto flags=p.source_hud_target_flags;
  const auto target=std::uint16_t((flags&1u)|((flags&8u)?2u:0u)|((flags&0x10u)?4u:0u)|
      ((flags&4u)?8u:0u)|((flags&2u)?0x10u:0u));
  return {unit, target, weapon, std::uint16_t(source_hud_score_mask(score) |
      (p.source_hud_objectives.charge_visible?0x400:0) | (talking?0x800:0))};
}
std::array<float, 4> widget_inputs(const HudWidget &w,
                                   const HudContent &content,
                                   const MultiplayerState &s, unsigned id,
                                   const SourceHudScoreDisplay &score, float winning,unsigned hand) {
  std::array<float, 4> out{};
  const auto &p = s.players.at(id);
  const auto projected=multiplayer_hud_weapon_state(p,content,hand);
  for (unsigned i = 0; i < 4; ++i)
    switch (w.inputs[i]) {
    case 1:
      out[i] = 1;
      break;
    case 2:
      out[i] = float(s.tick) * tick_seconds;
      break;
    case 3:
      //13AD48 selector3 reads1392A9's per-user HUD opacity, initialized to1
      //for ordinary gameplay.
      out[i] = 1;
      break;
    case 16:
      out[i] = p.shield;
      break;
    case 17:
      out[i] = p.health;
      break;
    case 18:
      out[i]=p.source_hud_autoaimed?1.f:0.f;
      break;
    case 19:
      out[i] = p.grenades[0] == 0 && p.grenades[1] == 0 ? 1.f : 0.f;
      break;
    case 20:
      out[i] = float(p.grenades[0]);
      break;
    case 21:
      out[i] = float(p.grenades[1]);
      break;
    case 22:
      out[i] = p.shield < .2f ? float(s.tick) * tick_seconds : 0;
      break;
    case 48:case 49:case 50:case 51:case 52:case 53:case 54:case 55:case 56:case 57:
      out[i]=weapon_hud_input((w.weapon_type==1 || w.weapon_type==4)?&projected:nullptr,w.inputs[i],float(s.tick)*tick_seconds);
      break;
    case 64:
      out[i] = score.score_fraction;
      break;
    case 65:
      out[i] = score.rival_fraction;
      break;
    case 66:
      out[i] = winning;
      break;
    case 67:
      out[i] = p.source_hud_objectives.charge_fraction;
      break;
    default:
      break;
    }
  if (w.special == 5)
    out[0] = out[1] = std::clamp(p.shield, 0.f, 1.f);
  return out;
}
float function(std::uint32_t id, const std::array<float, 4> &inputs,bool elite=false) {
  if (id >= 0x0a0005cf && id <= 0x0a0005d2)
    return inputs[id - 0x0a0005cf];
  if(id==0x090005ce)return elite?1.f:0.f;
  if (id == 0x0d0005ca || id == 0x0c0005cb ||
      id == 0x0c0005cc || id == 0x0e0005cd)
    return 0;
  return 1; // Original 22ACE7 default.
}
Vec2 anchor(unsigned a, const HudViewport &v) {
  switch (a) {
  case 0:
    return {v.safe.right, v.safe.top};
  case 1:
    return {v.safe.left, v.safe.top};
  case 2:
    return {v.safe.left, v.safe.bottom};
  case 3:
    return {v.safe.right, v.safe.bottom};
  case 4:
    return {std::floor((v.safe.left + v.safe.right) * .5f),
            std::floor((v.safe.top + v.safe.bottom) * .5f)};
  default:
    return {-10000, -10000}; // No lock target in the current sandbox.
  }
}
ShaderVector rgb(std::uint32_t c) {
  return {float((c >> 16) & 255) / 255, float((c >> 8) & 255) / 255,
          float(c & 255) / 255, 1};
}
Material hud_material(const Material &source, const Texture &texture) {
  auto m = source;
  if (m.passes.empty())
    throw FormatError("HUD shader has no original pixel pass");
  const auto &pass = m.passes[0];
  unsigned src = 0x302, dst = 0x303;
  for (const auto &state : pass.states) {
    if (state.index == 62)
      src = state.value;
    if (state.index == 63)
      dst = state.value;
  }
  if (src == 0x302 && dst == 0x303)
    m.forward_blend = 1;
  else if (src == 1 && dst == 0x303)
    m.forward_blend = 2;
  else if (src == 1 && dst == 1)
    m.forward_blend = 3;
  else if (src == 1 && dst == 0x302)
    m.forward_blend = 4;
  else
    throw FormatError("Unsupported HUD blend in " + source.name);
  for (const auto &binding : pass.textures)
    if (binding.stage != 0 ||
        (binding.external != 15 && binding.external != 16))
      throw FormatError("Unsupported HUD texture binding in " + source.name);
  m.diffuse = texture;
  m.hud_pass = true;
  m.forward_pass = true;
  m.static_lightmap_pass = false;
  m.forward_pass_index = 0;
  m.forward_program = pass.program;
  m.forward_vertex = 0;
  m.vertex_program.clear();
  m.program_constants = pass.constants;
  m.environment_pass.reset();
  m.accumulation_texture = false;
  if (!pass.texture_states.empty())
    m.texture_states[0] = pass.texture_states[0];
  return m;
}
bool supported(const HudWidget &w) {
  return w.special != 6;
}
FrontendAppearance score_icon_appearance(const MultiplayerState &s,std::uint32_t datum) {
  FrontendAppearance appearance;
  const auto player=std::find_if(s.players.begin(),s.players.end(),[&](const auto &p) {
    return p.connected && p.source_player==datum && datum!=null_datum;
  });
  if(player!=s.players.end() && player->source_configuration) {
    const auto &bytes=*player->source_configuration;
    for(unsigned i=0;i<4;++i)appearance.colors[i]=std::bit_cast<std::int8_t>(bytes[0x40+i]);
    appearance.species=std::bit_cast<std::int8_t>(bytes[0x44]);
    appearance.foreground=bytes[0x45];appearance.background=bytes[0x46];appearance.flags=bytes[0x47];
  }
  return appearance;
}
std::vector<unsigned> codepoints(const std::string &text) {
  std::vector<unsigned> out;
  for (std::size_t i = 0; i < text.size();) {
    unsigned c = static_cast<unsigned char>(text[i++]), n = 0;
    if (c >= 0xc0 && c < 0xe0) {
      c &= 31;
      n = 1;
    } else if (c >= 0xe0 && c < 0xf0) {
      c &= 15;
      n = 2;
    } else if (c >= 0xf0) {
      c &= 7;
      n = 3;
    }
    while (n-- && i < text.size())
      c = (c << 6) | (static_cast<unsigned char>(text[i++]) & 63);
    out.push_back(c);
  }
  return out;
}
std::string score_utf8(std::u16string_view value) {
  std::string out;
  for (std::size_t n=0;n<value.size();++n) {
    unsigned c=value[n];
    if (c>=0xd800 && c<=0xdbff && n+1<value.size() && value[n+1]>=0xdc00 && value[n+1]<=0xdfff)
      c=0x10000+((c-0xd800)<<10)+(value[++n]-0xdc00);
    if (c<128) out+=char(c);
    else if (c<2048) {out+=char(0xc0|(c>>6));out+=char(0x80|(c&63));}
    else if (c<65536) {out+=char(0xe0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
    else {out+=char(0xf0|(c>>18));out+=char(0x80|((c>>12)&63));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
  }
  return out;
}
std::string text_value(const HudWidget &w, const HudContent &content,
                       const std::array<float, 4> &inputs,
                       const SourceHudScoreDisplay &score,bool elite,const std::string &voice) {
  if (w.flags & 1) {
    int value = int(function(w.text, inputs,elite));
    unsigned digits = 0;
    if (w.flags & 2) {
      value = std::clamp(value, 0, 99);
      digits = 2;
    } else if (w.flags & 4) {
      value = std::clamp(value, 0, 999);
      digits = 3;
    }
    auto out = std::to_string(value);
    if (out.size() < digits)
      out.insert(0, digits - out.size(), '0');
    return out;
  }
  if (w.flags & 8)
    return voice;
  const auto found = content.strings.find(w.text);
  if (found == content.strings.end())
    return {};
  std::string out;
  for (const auto c : codepoints(found->second))
    switch (c) {
    case 0xe42d:
      out.append(score.score_text.begin(),score.score_text.end());
      break;
    case 0xe42e: {
      out.append(score.rival_text.begin(),score.rival_text.end());
      break;
    }
    case 0xe42f:
      out += score_utf8(score.name);
      break;
    case 0xe430: {
      out.append(score.timer_text.begin(),score.timer_text.end());
      break;
    }
    default:
      if (c < 128)
        out += char(c);
      else if (c < 2048) {
        out += char(0xc0 | (c >> 6));
        out += char(0x80 | (c & 63));
      } else {
        out += char(0xe0 | (c >> 12));
        out += char(0x80 | ((c >> 6) & 63));
        out += char(0x80 | (c & 63));
      }
      break;
    }
  return out;
}
} // namespace
// Original 13A838 shield history and 22AFF1 special type five. The previous
// amount is held for half a second; it is not a smoothed vitality value.
void update_hud_shield(HudShieldHistory &h, float current, std::uint32_t tick) {
  if (!std::isfinite(current))
    throw FormatError("Non-finite HUD shield amount");
  if (h.previous > current) {
    if (h.pulse < 0 || h.pulse > 1)
      h.tick = tick;
    const float elapsed = float(tick - h.tick) * tick_seconds;
    if (elapsed < .5f)
      h.pulse = 0;
    else {
      h.previous = current;
      h.pulse += elapsed;
      h.tick = tick;
    }
  } else if (h.previous < current) {
    h.previous = current;
    h.pulse = -1;
    h.tick = tick;
  } else {
    if (h.pulse > 0)
      h.pulse += float(tick - h.tick) * tick_seconds;
    h.tick = tick;
  }
}
HudShieldLayer hud_shield_layer(const HudShieldHistory &h, float current,
                                unsigned layer, bool elite) {
  if (layer >= 5)
    throw FormatError("HUD shield layer outside original range");
  HudShieldLayer out;
  out.inputs[0] = std::clamp(h.previous - float(layer), 0.f, 1.f);
  out.inputs[1] = std::clamp(current - float(layer), 0.f, 1.f);
  out.visible = out.inputs[0] > 0 || out.inputs[1] > 0;
  const float pulse = out.inputs[0] > out.inputs[1] ? h.pulse : -1;
  const float weight = pulse < 0 ? 0 : std::clamp(1 - pulse, 0.f, 1.f);
  out.colors[0] = {weight, weight, weight, 1};
  constexpr std::array<std::uint32_t, 4> colors{0xff0000, 0x00ff00, 0xffff00,
                                                0x7f00ff};
  out.colors[1] = rgb(layer ? colors[layer - 1] : elite ? 0x45059a : 0x0055aa);
  out.colors[2] = rgb(layer ? colors[layer - 1] : elite ? 0x9c46c1 : 0x0078f0);
  return out;
}
unsigned hud_screen_mode(unsigned width, unsigned height, unsigned anchor,
                         bool widescreen) {
  const unsigned mode = width < 640 && height < 480   ? 2
                        : width < 640 || height < 480 ? 1
                                                      : 0;
  // Original 13A690/16A90 promotes the bottom-right half layout on
  // widescreen displays. This selects each widget's authored layout slot.
  return mode == 1 && anchor == 3 && widescreen ? 2 : mode;
}
HudViewport hud_viewport(unsigned players, unsigned view, bool widescreen,std::optional<TwoPlayerSplit> split) {
  if (players < 1 || players > 4 || view >= players)
    throw FormatError("HUD viewport outside local player grid");
  if(split && *split!=TwoPlayerSplit::Horizontal && *split!=TwoPlayerSplit::Vertical)
    throw FormatError("Invalid two-player split-screen layout");
  // Original 2234E0 selects mode 1 (standard) or 2 (widescreen), then
  // 223080 calls 2F600, 2F640 and 2F6B0. Reproduce their grid and span rules;
  // three participants occupy the whole surface, with one spanning view.
  const auto layout=source_view_grid(players,view,hud_layout_widescreen(players,widescreen,split));
  const unsigned columns=layout.grid[0],rows=layout.grid[1],span_x=layout.span[0],span_y=layout.span[1];
  const unsigned x=layout.position[0],y=layout.position[1];
  const unsigned cell_w = (592 - 48) / columns, cell_h = (444 - 36) / rows;
  const float l = float(48 + x * cell_w), r = float(48 + (x + span_x) * cell_w),
              t = float(36 + y * cell_h), b = float(36 + (y + span_y) * cell_h);
  HudViewport v{{y ? t : 0, x ? l : 0, y + span_y == rows ? 480 : b,
                 x + span_x == columns ? 640 : r},
                {y ? t + 4 : t, x ? l + 4 : l, y + span_y == rows ? b : b - 4,
                 x + span_x == columns ? r : r - 4},
                0};
  v.mode = hud_screen_mode(unsigned(v.outer.right - v.outer.left),
                           unsigned(v.outer.bottom - v.outer.top));
  return v;
}
bool hud_widget_visible(const HudWidget &w,
                        std::array<std::uint16_t, 4> state) {
  bool yes = false;
  for (unsigned i = 0; i < 4; ++i) {
    yes |= (w.yes[i] & state[i]) != 0;
    if (w.no[i] & state[i])
      return false;
  }
  return yes;
}
struct MultiplayerHud::Continuity {
  std::thread::id executor=std::this_thread::get_id();std::uint64_t revision{};bool busy{},alive=true;
  bool own()const noexcept{return executor==std::this_thread::get_id()&&alive;}
};
class MultiplayerHud::Call {
  std::shared_ptr<Continuity> owner_;
public:
  Call(const MultiplayerHud &hud,bool write):owner_(hud.continuity_){
    if(!owner_||!owner_->own()||owner_->busy||(write&&owner_->revision==UINT64_MAX))
      throw FormatError("HUD continuation requires its completed owning executor");
    owner_->busy=true;if(write)++owner_->revision;
  }
  ~Call(){owner_->busy=false;}
  Call(const Call &)=delete;Call &operator=(const Call &)=delete;
};
struct MultiplayerHud::Values {
  double seconds{};std::array<Winning,4> winning;std::array<HudShieldHistory,4> shields;
  std::array<SensorView,4> sensors;std::string voice;
  std::array<bool,4> gamepad;std::array<std::array<std::uint32_t,4>,4> buttons;
  std::array<PromptDevice,4> devices;
  std::array<std::array<std::string,4>,4> keyboard;
  explicit Values(const MultiplayerHud &h):seconds(h.seconds_),winning(h.winning_),shields(h.shields_),sensors(h.sensors_),
    voice(h.voice_text_),gamepad(h.prompt_gamepad_),buttons(h.prompt_buttons_),devices(h.prompt_devices_),keyboard(h.prompt_keyboard_){}
  bool bindings_match(const MultiplayerHud &h)const noexcept{return buttons==h.prompt_buttons_&&keyboard==h.prompt_keyboard_;}
  bool sensor_links_match(const MultiplayerHud &h)const noexcept{
    for(unsigned n=0;n<4;++n)if(sensors[n].material!=h.sensors_[n].material)return false;return true;
  }
  void swap(MultiplayerHud &h)noexcept{
    static_assert(std::is_nothrow_swappable_v<Values>);using std::swap;
    swap(seconds,h.seconds_);winning.swap(h.winning_);shields.swap(h.shields_);sensors.swap(h.sensors_);
    voice.swap(h.voice_text_);gamepad.swap(h.prompt_gamepad_);buttons.swap(h.prompt_buttons_);keyboard.swap(h.prompt_keyboard_);
    devices.swap(h.prompt_devices_);
  }
};
struct MultiplayerHud::Checkpoint::Data {
  std::weak_ptr<Continuity> owner;const Mesh *mesh{};const Material *roots{};std::size_t materials{},views{};Values values;
};
struct MultiplayerHud::PreparedRestore::Data {
  std::weak_ptr<Continuity> owner;std::uint64_t revision{};const Mesh *mesh{};const Material *roots{};
  std::size_t materials{},views{};Values replacement;bool consumed{};
};
MultiplayerHud::MultiplayerHud(HudContent content,
                               const std::filesystem::path &directory,
                               Mesh &mesh, unsigned local_players,
                               std::string variant,std::optional<HudBitmapGeometryConstants> geometry,
                               const std::filesystem::path &ui_assets,unsigned hand,
                               std::shared_ptr<GameplayFontBank> font_bank,bool defusal_objectives)
    : content_(std::move(content)),hand_(hand), mesh_(mesh), materials_(local_players),
      variant_(std::move(variant)),geometry_(geometry),continuity_(std::make_shared<Continuity>()) {
  if(hand_>2)throw FormatError("Invalid HUD weapon owner");
  defusal_objectives_=defusal_objectives && !hand_;
  if(hand_)std::erase_if(content_.widgets,[&](const auto &widget){return widget.weapon_type!=(hand_==2?4u:1u);});
  hud_viewport(local_players, 0);
  // Keep first-frame/default prompts on the same adapter as live rebinding.
  // A second hand-written mapping had retained LB for dual pickup and the
  // wrong keyboard grenade/melee labels.
  const PcKeyboardSettings keyboard;
  const ControllerSettings controller;
  const auto keys=native_action_prompt_keyboard_buttons(keyboard);
  const auto buttons=native_action_prompt_controller_buttons(controller);
  const auto boarding_keys=native_boarding_prompt_keyboard_buttons(keyboard);
  const auto boarding_buttons=native_boarding_prompt_controller_buttons(controller);
  for(unsigned view=0;view<prompt_buttons_.size();++view)for(unsigned action=0;action<2;++action) {
    prompt_keyboard_[view][action]=keys[action];prompt_buttons_[view][action]=buttons[action];
    prompt_keyboard_[view][action+2]=boarding_keys[action];prompt_buttons_[view][action+2]=boarding_buttons[action];
  }
  if(!font_bank)font_bank=std::make_shared<GameplayFontBank>(directory);
  if(!font_bank->matches_directory(directory))throw FormatError("HUD font bank belongs to a different source");
  const auto& font_source=font_bank->source();
  const auto& names=font_source.names;
  const bool native_mcc_fonts=font_source.domain==GameplayFontDomain::Mcc;
  const auto prompt_assets=font_prompt_asset_directory(font_source.directory,ui_assets);
  if(!hand_) {
    prompt_artwork_=std::make_shared<ControllerPromptArtwork>(mesh_,font_source.directory);
    for(auto selector:{5u,6u,8u})prompt_artwork_->prepare(selector);
  }
  fonts_[0] = font_bank->font(6);
  fonts_[1] = font_bank->font(8);
  // Selected redraw packs get their own mutable copy. The usual native route
  // shares all glyph maps and atlas bytes across this match's HUDs.
  const auto writable=[](auto& retained)->RetailFont& {
    auto copy=std::make_shared<RetailFont>(*retained);
    retained=copy;return *copy;
  };
  if((content_.action_messages.enabled || content_.messages.enabled || defusal_objectives_) && !hand_)for(unsigned i=0;i<2;++i){
    const auto selector=i?5:6;
    action_fonts_[i]=font_bank->font(selector);
    const auto sampled_font=hud_sampled_font_path(ui_assets,selector,names[selector]);
    const bool sampled_text=!ui_assets.empty() && std::filesystem::is_regular_file(sampled_font);
    if(native_mcc_fonts || sampled_text) {
      // MCC's authenticated native fonts own the colored controller/keycap
      // glyphs. Keep their actual advances and physical atlas; optional legacy
      // redraws belong only to the separately selected non-MCC reference route.
      const auto button_source=native_mcc_fonts?font_source.directory:hud_menu_asset_directory(ui_assets);
      action_button_fonts_[i]=native_mcc_fonts?font_bank->raw(selector):
          std::make_shared<const RetailFont>(load_retail_font(hud_sampled_font_path(button_source,selector,names[selector])));
      // Use the selected redraw pack for action controls, while retaining
      // the original logical glyph metrics and live binding substitution.
      if(!native_mcc_fonts)apply_vista_prompt_sampling(writable(action_button_fonts_[i]),prompt_assets,selector);
      const auto& buttons=*action_button_fonts_[i];
      // Vista names E132 keyboard_button_background0. The actual decoded
      // control/keycap glyphs retain their native RGB without HUD tinting.
      for(const auto c:{0xe100u,0xe101u,0xe102u,0xe103u,0xe104u,0xe105u,0xe106u,0xe107u,
          0xe108u,0xe109u,0xe10au,0xe10bu,0xe10cu,0xe10du,0xe10eu,0xe10fu,
          0xe132u,0xe133u,0xe134u,0xe135u}) {
        if(buttons.glyph_record_indices.size()!=65536 ||
            std::count(buttons.glyph_record_indices.begin(),buttons.glyph_record_indices.end(),
                buttons.glyph_record_indices.at(c))!=1 || !buttons.glyphs.at(c).width)
          throw FormatError("Selected UI fonts lack native action button artwork");
      }
      if(sampled_text) {
        const auto sampled=load_retail_font(sampled_font);
        if(!native_mcc_fonts) {
          if(is_mcc_classic_hud_pack(ui_assets))apply_mcc_classic_font_sampling(writable(action_fonts_[i]),sampled);
          else apply_retail_font_sampling(writable(action_fonts_[i]),sampled);
        } else {
          if(!is_mcc_classic_hud_pack(ui_assets))throw FormatError("Custom MCC gameplay font redraw needs an explicit original-metric projection");
          require_mcc_gameplay_font_sample(font_source,selector,sampled_font);
        }
      }
      action_button_materials_[i]=unsigned(mesh_.materials.size());
      mesh_.materials.push_back({std::string(native_mcc_fonts?"action-message/mcc-buttons/font":"action-message/vista-buttons/font")+
          std::to_string(selector),buttons.atlas});
      auto &material=mesh_.materials.back();material.ui_full_surface=true;
      material.texture_states[0].address_u=material.texture_states[0].address_v=3;
      if(i==1)vista_action_buttons_=true;
    }
    if(!prompt_assets.empty())apply_vista_prompt_sampling(writable(action_fonts_[i]),prompt_assets,selector);
    action_materials_[i]=unsigned(mesh_.materials.size());
    mesh_.materials.push_back({"action-message/font"+std::to_string(selector),action_fonts_[i]->atlas});
    auto &material=mesh_.materials.back();material.ui_full_surface=true;
    material.texture_states[0].address_u=material.texture_states[0].address_v=3;
  }
  if(!hand_)for(unsigned i=0;i<2;++i) {
    const unsigned selector=i?5:6;
    status_font_offsets_[i]=std::int16_t(gameplay_font_horizontal_offset(font_source,selector));
    if(action_fonts_[i]) {
      status_fonts_[i]=action_fonts_[i];status_materials_[i]=action_materials_[i];
    } else {
      status_fonts_[i]=font_bank->font(selector);
      status_materials_[i]=unsigned(mesh_.materials.size());
      mesh_.materials.push_back({"player-status/font"+std::to_string(selector),status_fonts_[i]->atlas});
      auto& material=mesh_.materials.back();material.ui_full_surface=true;
      material.texture_states[0].address_u=material.texture_states[0].address_v=3;
    }
  }
  if(defusal_objectives_) {
    Texture solid;solid.width=solid.height=1;solid.format=11;solid.pixels=std::vector<std::byte>(4,std::byte{255});
    defusal_marker_material_=unsigned(mesh_.materials.size());
    mesh_.materials.push_back({"bomb-defusal/marker",std::move(solid)});
    mesh_.materials.back().ui_full_surface=true;
  }
  const auto sampled=load_ui_asset_library(content_.images,ui_assets,"hud.tsv");
  const auto sampled_emblems=!ui_assets.empty() && std::filesystem::is_regular_file(ui_assets/"scoreboard.tsv")?
      load_ui_asset_library(content_.images,ui_assets,"scoreboard.tsv"):FrontendUiAssetPack{};
  // MCC imports explicitly select their source radar pages. Ordinary Xbox
  // and Vista packs keep the original64x64 composition.
  const bool mcc=is_mcc_classic_hud_pack(ui_assets);
  if(mcc || (!ui_assets.empty() && std::filesystem::exists(ui_assets/"hud-reconstruction.tsv"))) {
    if(!mcc) {
      std::ifstream marker(ui_assets/"hud-reconstruction.tsv");
      std::string header,key,extra;unsigned size{};
      std::getline(marker,header);
      if(header!="H2_HUD_RECONSTRUCTION_V1" || !(marker>>key>>size) ||
          key!="radar-size" || size!=256 || marker>>extra)
        throw FormatError("Unsupported HUD reconstruction pack settings");
    }
    std::array<Texture*,4> pages{&content_.sensor.artwork.sweep,&content_.sensor.artwork.mask,
                                &content_.sensor.artwork.blip,&content_.sensor.artwork.custom};
    constexpr std::array names{"hud_sweeper","hud_sweeper_mask","hud_sensor_blip","hud_sensor_blip_custom"};
    std::map<std::uint32_t,UiImage> originals;
    for(unsigned n=0;n<pages.size();++n) {
      UiImage image;image.name=std::string("ui\\hud\\bitmaps\\")+names[n];image.images.push_back(*pages[n]);
      originals.emplace(n,std::move(image));
    }
    const auto replacements=load_ui_asset_library(originals,ui_assets,"hud.tsv");
    for(unsigned n=0;n<pages.size();++n) {
      const auto found=replacements.find(originals.at(n).name);
      if(found==replacements.end() || found->second.size()!=1 ||
         found->second[0].width!=pages[n]->width*(mcc?(n==3?1:2):8) ||
         found->second[0].height!=pages[n]->height*(mcc?(n==3?1:2):8))
        throw FormatError("Missing authored radar artwork for selected HUD source");
      *pages[n]=found->second[0];
      // MCC's custom contact page is native DXT1. The CPU radar composer
      // consumes decoded channels; use the production BC decoder unchanged.
      if(pages[n]->format>=14 && pages[n]->format<=16)
        *pages[n]=prepare_xemu_bc_texture(*pages[n]);
    }
  }
  if(!ui_assets.empty())for(const auto &[datum,image]:content_.images)
    if(image.name.starts_with("ui\\hud\\bitmaps\\new_hud\\crosshairs\\") && !sampled.contains(image.name))
      throw FormatError("Missing Vista crosshair artwork; run asset setup again: "+image.name);
  const auto sampled_offsets=load_hud_asset_sample_offsets(sampled,ui_assets);
  for(unsigned i=0;i<fonts_.size();++i) {
    const auto path=hud_sampled_font_path(ui_assets,i==0?6:8,names[i==0?6:8]);
    if(!ui_assets.empty() && std::filesystem::is_regular_file(path)) {
      const auto donor=ui_assets/"fonts-source"/names[i==0?6:8];
      if(!native_mcc_fonts) {
        if(mcc)apply_mcc_classic_font_sampling(writable(fonts_[i]),load_retail_font(path));
        else if(std::filesystem::exists(ui_assets/"hud-reconstruction.tsv"))
          apply_retail_font_sampling(writable(fonts_[i]),load_retail_font(path),load_retail_font(donor));
        else apply_retail_font_sampling(writable(fonts_[i]),load_retail_font(path));
      } else {
        if(!mcc)throw FormatError("Custom MCC gameplay font redraw needs an explicit original-metric projection");
        require_mcc_gameplay_font_sample(font_source,i==0?6:8,path);
      }
    }
    if(!prompt_assets.empty())apply_vista_prompt_sampling(writable(fonts_[i]),prompt_assets,i==0?6:8);
  }
  for (auto &view : materials_) {
    view.resize(content_.widgets.size());
    for (unsigned i = 0; i < view.size(); ++i) {
      const auto &w = content_.widgets[i];
      if (!supported(w) || w.shader == null_datum)
        continue;
      const auto &shader = content_.shaders.at(w.shader);
      auto add = [&](const Texture &t) {
        view[i].push_back(unsigned(mesh_.materials.size()));
        mesh_.materials.push_back(hud_material(shader, t));
      };
      if(shader.ui_emblem_pass) {
        auto emblem=shader;
        // Gameplay HUD vertices already use the full drawable's 640x480
        // coordinate range, including widescreen and split-screen anchors.
        emblem.ui_full_surface=true;
        Texture *stages[]{&emblem.diffuse,&emblem.detail};
        for(const auto &binding:emblem.passes.front().textures) {
          const auto &remaps=emblem.passes.front().bitmap_remaps;
          const auto remap=std::find_if(remaps.begin(),remaps.end(),[&](const auto &r) {
            return (r.destination&255)==binding.parameter;
          });
          if(binding.external || binding.stage>1 || remap==remaps.end() || remap->source>=emblem.bitmaps.size())
            throw FormatError("Unsupported HUD emblem image binding");
          const auto &bitmap=emblem.bitmaps[remap->source];
          const auto replacement=sampled_emblems.find(content_.images.at(bitmap.datum).name);
          if(replacement!=sampled_emblems.end()) {
            *stages[binding.stage]=replacement->second.at(bitmap.index);
            emblem.animated_textures[binding.stage]=replacement->second;
          }
        }
        view[i].push_back(unsigned(mesh_.materials.size()));
        mesh_.materials.push_back(std::move(emblem));
      } else if (w.is_text)
        for (const auto &font : fonts_)
          add(font->atlas);
      else if (w.bitmap != null_datum)
        for (unsigned layer = 0; layer < (w.special == 5 ? 5u : w.special == 7 ? 8u : 1u); ++layer)
          {
            const auto &image=content_.images.at(w.bitmap);
            const auto replacement=sampled.find(image.name);
            unsigned frame=0;
            for(const auto &t:replacement==sampled.end()?image.images:replacement->second) {
              // Relative fill/outline placement belongs to the donor artwork.
              // Apply it only to shield fill sampling, retaining source quads.
              if(w.special==5) {
                if(replacement!=sampled.end()) {
                  if(const auto offset=sampled_offsets.find({image.name,frame});offset!=sampled_offsets.end())
                    sampled_offsets_.emplace(unsigned(mesh_.materials.size()),offset->second);
                } else if(const auto offset=mcc_hud_shield_fill_sample_offset(image,frame))
                  sampled_offsets_.emplace(unsigned(mesh_.materials.size()),*offset);
              }
              add(t);
              ++frame;
            }
          }
    }
  }
  if(content_.sensor.range>0 && std::any_of(content_.widgets.begin(),content_.widgets.end(),
      [](const auto &w){return w.name=="motion_tracker_background" && (w.yes[0]&0x800);})) {
    for(unsigned view=0;view<local_players;++view) {
      Material m;m.name="original_motion_sensor";m.hud_pass=m.forward_pass=true;
      m.static_lightmap_pass=false;m.forward_blend=2;
      const unsigned size=mcc?128u:std::max(64u,content_.sensor.artwork.sweep.width/2);
      m.diffuse.width=m.diffuse.height=size;m.diffuse.format=11;m.diffuse.pixels.resize(size*size*4);
      // Literal final2548F0 pixel definition, opacity1. The radar input has
      // already passed the original contact/sweeper/mask composition.
      m.forward_program=source_motion_sensor_final_program_tu15();
      m.texture_states[0].address_u=m.texture_states[0].address_v=3;
      m.texture_states[0].mip_filter=0;
      sensors_[view].material=unsigned(mesh_.materials.size());
      mesh_.materials.push_back(std::move(m));
    }
  }
}
std::optional<unsigned> MultiplayerHud::motion_sensor_material(unsigned view)const {
  Call call(*this,false);
  return sensors_.at(view).material;
}
MultiplayerHud::~MultiplayerHud(){continuity_->alive=false;}
bool MultiplayerHud::Checkpoint::valid()const noexcept{const auto owner=data_?data_->owner.lock():nullptr;return owner&&owner->own();}
MultiplayerHud::Checkpoint MultiplayerHud::checkpoint()const {
  Call call(*this,false);Checkpoint out;out.data_=std::make_shared<const Checkpoint::Data>(Checkpoint::Data{
    continuity_,&mesh_,mesh_.materials.data(),mesh_.materials.size(),materials_.size(),Values(*this)});return out;
}
bool MultiplayerHud::owns_checkpoint(const Checkpoint &checkpoint)const noexcept{
  return checkpoint.valid()&&checkpoint.data_->owner.lock()==continuity_;
}
MultiplayerHud::PreparedRestore::PreparedRestore()=default;
MultiplayerHud::PreparedRestore::~PreparedRestore()=default;
MultiplayerHud::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MultiplayerHud::PreparedRestore &MultiplayerHud::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MultiplayerHud::PreparedRestore MultiplayerHud::prepare_restore(const Checkpoint &checkpoint)const {
  Call call(*this,false);
  if(!owns_checkpoint(checkpoint)||continuity_->revision==UINT64_MAX)
    throw FormatError("HUD checkpoint belongs to another owner or exhausted continuation");
  const auto &p=*checkpoint.data_;
  if(p.mesh!=&mesh_||p.roots!=mesh_.materials.data()||p.materials!=mesh_.materials.size()||p.views!=materials_.size()||
      !p.values.sensor_links_match(*this)||!p.values.bindings_match(*this))
    throw FormatError("HUD checkpoint has different resident resources or current resolved input bindings");
  PreparedRestore out;out.data_=std::make_unique<PreparedRestore::Data>(PreparedRestore::Data{
    continuity_,continuity_->revision,p.mesh,p.roots,p.materials,p.views,p.values,false});return out;
}
bool MultiplayerHud::restore_ready(const PreparedRestore &prepared)const noexcept{
  const auto *p=prepared.data_.get();return continuity_->own()&&!continuity_->busy&&p&&!p->consumed&&
    p->owner.lock()==continuity_&&p->revision==continuity_->revision&&continuity_->revision!=UINT64_MAX&&
    p->mesh==&mesh_&&p->roots==mesh_.materials.data()&&p->materials==mesh_.materials.size()&&p->views==materials_.size()&&
    p->replacement.sensor_links_match(*this)&&p->replacement.bindings_match(*this);
}
bool MultiplayerHud::commit_restore(PreparedRestore &prepared)noexcept{
  if(!restore_ready(prepared))return false;auto &p=*prepared.data_;p.replacement.swap(*this);p.consumed=true;++continuity_->revision;return true;
}
void MultiplayerHud::voice_text(std::string text){Call call(*this,true);voice_text_=std::move(text);}
void MultiplayerHud::prompt_gamepad(unsigned view,bool enabled){prompt_device(view,enabled?PromptDevice::Xbox:PromptDevice::KeyboardMouse);}
void MultiplayerHud::prompt_device(unsigned view,PromptDevice device){
  Call call(*this,true);prompt_gamepad_.at(view)=device!=PromptDevice::KeyboardMouse;prompt_devices_.at(view)=device;
}
void MultiplayerHud::prompt_gamepad_buttons(unsigned view,std::array<std::uint32_t,2> buttons){
  Call call(*this,true);for(unsigned n=0;n<2;++n)prompt_buttons_.at(view)[n]=buttons[n];
}
void MultiplayerHud::prompt_keyboard_buttons(unsigned view,std::array<std::string,2> buttons){
  Call call(*this,true);for(unsigned n=0;n<2;++n)prompt_keyboard_.at(view)[n]=std::move(buttons[n]);
}
void MultiplayerHud::reset_lifetimes()noexcept {
  if(!continuity_->own()||continuity_->busy||continuity_->revision==UINT64_MAX)return;
  Call call(*this,true);
  winning_={};shields_={};seconds_=0;voice_text_.clear();
  for(auto &sensor:sensors_) {
    sensor.history={};sensor.tick=~0u;sensor.unit=null_datum;
  }
}
void MultiplayerHud::update_motion_sensor(const MultiplayerState &s,unsigned id,unsigned view) {
  Call call(*this,true);
  if(id>=s.players.size() || view>=materials_.size())throw FormatError("Invalid radar local player");
  auto &sensor=sensors_[view];const auto &p=s.players[id];
  if(!sensor.material || (sensor.tick==s.tick && sensor.unit==p.source_unit))return;
  if(sensor.unit!=p.source_unit || sensor.tick>s.tick)source_motion_sensor_reset(sensor.history,view);
  else if(sensor.tick!=~0u && s.tick-sensor.tick>1) {
    // Missing LAN snapshots contain no observed contacts. Age the retained
    // trail in source ticks without inventing positions for the missing bank.
    std::array<std::optional<MotionSensorSample>,4> missing;
    missing[view]=MotionSensorSample{};
    for(unsigned n=0;n<std::min(10u,s.tick-sensor.tick-1);++n)
      source_motion_sensor_store_samples(sensor.history,missing);
  }
  sensor.unit=p.source_unit;sensor.tick=s.tick;
  std::array<unsigned,multiplayer_capacity> order{};
  for(unsigned n=0;n<order.size();++n)order[n]=n;
  //1A2336 enumerates the source player pool by ascending datum index.
  std::stable_sort(order.begin(),order.end(),[&](unsigned a,unsigned b){
    return (s.players[a].source_player&0xffff)<(s.players[b].source_player&0xffff);
  });
  std::vector<MotionSensorCandidate> candidates;
  for(unsigned n:order) {
    const auto &q=s.players[n];
    if(!q.connected || q.dead || q.source_unit==null_datum)continue;
    const float dx=q.position.x-p.position.x,dy=q.position.y-p.position.y;
    const float nearby=content_.sensor.range*1.5f;
    if(nearby*nearby<dx*dx+dy*dy)continue;
    // C6C50 consumes unit148 bits16/21 for the two weapon trigger holds.
    // 1A20E6 also calls E70E0: signed component+0C is the grenade phase,
    // not zoom or an animation-name estimate. Phases1/2 reveal the unit;
    // idle0 and post-release3 still require movement or trigger input.
    MotionSensorObject object;object.velocity=q.velocity;object.type=0;
    object.player_13c=q.source_player;object.control_14a=q.motion_sensor_control;
    object.estimate_mode=q.grenade_action.phase();
    const bool visible=source_motion_sensor_visible(object,content_.sensor.minimum_speed,
                                                   content_.sensor.sneak_speed,true,false);
    const bool enemy=s.source_engine_entity && p.connected?
        source_multiplayer_engine_entity_enemies_tu15(*s.source_engine_entity,unsigned(p.source_membership),unsigned(q.source_membership)):
        q.source_membership!=p.source_membership;
    const std::uint8_t type=n==id?1 : enemy?3:2;
    candidates.push_back({q.position,true,visible,type,
        content_.sensor.blip_sizes.at(multiplayer_representation(q.source_configuration))});
    if(candidates.size()==16)break;
  }
  std::array<std::optional<MotionSensorSample>,4> samples;
  samples[view]=source_motion_sensor_sample(p.connected && !p.dead && p.source_unit!=null_datum,
      p.position,p.yaw,content_.sensor.range,candidates);
  source_motion_sensor_store_samples(sensor.history,samples);
  const auto plan=source_motion_sensor_render_plan(sensor.history,view,0,{0,0},{0,0},s.tick,tick_seconds);
  const Vec3 color=multiplayer_representation(p.source_configuration)==3 ?
      Vec3{206.f/255,143.f/255,222.f/255}:Vec3{117.f/255,186.f/255,1};
  auto &image=mesh_.materials.at(*sensor.material).diffuse;
  image=source_motion_sensor_composite(plan,content_.sensor.artwork,color,image.width);
}
void MultiplayerHud::animate(const MultiplayerState &s, unsigned id,
                             unsigned view, int limit, unsigned time_ticks,
                             double seconds) {
  Call call(*this,true);
  if (id >= multiplayer_capacity || view >= materials_.size())
    throw FormatError("HUD player outside table");
  seconds_ = seconds;
  update_hud_shield(shields_[view], s.players[id].shield, s.tick);
  auto &win = winning_[view];
  const auto score=multiplayer_hud_score_display(s,id,limit,time_ticks,content_);
  if (score.status==SourceHudScoreStatus::Ready) {
    const float target=score.leading ? 1 : 0;
    const auto ticks = win.initialized ? s.tick - win.tick : 1;
    const float delta = float(std::min(ticks, 15u)) * (2.f / 30);
    win.amount = target > win.amount ? std::min(target, win.amount + delta)
                                     : std::max(target, win.amount - delta);
  }
  win.tick = s.tick;
  win.initialized = true;
  std::vector<Vec3> palette;
  for(const auto &color:content_.score_appearance_colors)palette.push_back(std::bit_cast<Vec3>(color));
  for (unsigned i = 0; i < content_.widgets.size(); ++i) {
    const auto &w = content_.widgets[i];
    const auto inputs =
        widget_inputs(w, content_, s, id, score, winning_[view].amount,hand_);
    for (unsigned k = 0; k < materials_[view][i].size(); ++k) {
      const auto material = materials_[view][i][k];
      auto resolved_inputs = inputs;
      HudShieldLayer shield;
      if (w.special == 5) {
        shield =
            hud_shield_layer(shields_[view], s.players[id].shield,
                             k / content_.images.at(w.bitmap).images.size(),
                             elite_hud(content_,s.players[id]));
        resolved_inputs = shield.inputs;
      }
      if(w.special==7) {
        const auto territory=k/content_.images.at(w.bitmap).images.size();
        resolved_inputs[0]=s.players[id].source_hud_objectives.territories.at(territory).fraction;
      }
      auto &m = mesh_.materials.at(material);
      const auto &pass = m.passes.at(m.forward_pass_index);
      m.forward_program = pass.program;
      if(m.ui_emblem_pass) {
        //Special1/2 selects the same salted own/rival datum as the score
        //query; the other view and the previous rival retain no ownership.
        const auto appearance=score_icon_appearance(s,w.special==1?score.player:score.rival_player);
        apply_object_colors(m,frontend_appearance_colors(appearance,palette));
        const auto input=[&](std::uint32_t name) {
          switch(name) {
          case 0x0d0005ff:case 0x170005fa:case 0x170005fb:
          case 0x180005fc:case 0x1d0005fd:case 0x1d0005fe:
            return frontend_emblem_input(appearance,name,1);
          default:return function(name,resolved_inputs,elite_hud(content_,s.players[id]));
          }
        };
        animate_forward_material(m,seconds,input);
        animate_material_bitmaps(m,seconds,input);
        continue; // Object-change palette externs belong to this appearance.
      }
      animate_forward_material(m, seconds, [&](auto name) {
        return function(name, resolved_inputs,elite_hud(content_,s.players[id]));
      });
      std::array<float, 256> scalars{};
      scalars[40] = 1;
      std::array<ShaderVector, 256> colors{};
      if (w.special == 5) {
        for (unsigned c = 0; c < 3; ++c)
          colors[36 + c] = shield.colors[c];
      }
      else if (w.special >= 1 && w.special <= 4) {
        const auto &bits=(w.special==1 || w.special==3) ? score.color : score.rival_color;
        colors[36]=colors[37]={std::bit_cast<float>(bits[0]),
            std::bit_cast<float>(bits[1]),std::bit_cast<float>(bits[2]),1};
      }
      else if(w.special==7) {
        const auto territory=k/content_.images.at(w.bitmap).images.size();
        const auto color=s.players[id].source_hud_objectives.territories.at(territory).color;
        colors[36]=rgb(color);
        colors[37]={0,0,0,1}; //22B349/231D49: original second meter color is black.
      }
      remap_pixel_externs(m.forward_program, scalars, colors,
                          pass.pixel_externs, pass.fragments,
                          m.shader_switches);
    }
  }
}
void MultiplayerHud::prompt_boarding_buttons(unsigned view,const PcKeyboardSettings &keys,const ControllerSettings &pad) {
  Call call(*this,true);
  const auto k=native_boarding_prompt_keyboard_buttons(keys);const auto p=native_boarding_prompt_controller_buttons(pad);
  for(unsigned i=0;i<2;++i){prompt_keyboard_.at(view)[i+2]=k[i];prompt_buttons_.at(view)[i+2]=p[i];}
}
std::string MultiplayerHud::action_text(const MultiplayerState &s,unsigned id,unsigned view)const {
  Call call(*this,false);return action_text_impl(s,id,view);
}
std::string MultiplayerHud::action_text_impl(const MultiplayerState &s,unsigned id,unsigned view)const {
  if(id>=s.players.size() || view>=prompt_gamepad_.size())throw FormatError("Invalid action prompt view");
  const auto &p=s.players[id];
  if(s.defusal && s.defusal_rules && defusal_objectives_ && p.connected) {
    const BombDefusalPlayer player{p.source_player,p.source_membership,p.connected,!p.dead,false,false,true,p.position};
    auto text=bomb_defusal_status(*s.defusal,*s.defusal_rules,player);
    const auto button=prompt_gamepad_[view]?std::string("[Use / Reload]"):"["+prompt_keyboard_[view][0]+"]";
    for(std::size_t at=0;(at=text.find("USE",at))!=std::string::npos;at+=button.size())text.replace(at,3,button);
    return text;
  }
  if(!content_.action_messages.enabled || hand_ || !p.connected || p.dead || s.finished)return {};
  const auto &prompt=p.action_prompt;auto message=prompt.message;
  if(prompt.target.action==1 || prompt.target.action==7){
    const auto found=content_.action_messages.weapons.find(prompt.definition);
    if(found==content_.action_messages.weapons.end())return {};
    const auto flags=std::uint16_t(prompt.target.index);
    if(!flags)return {};
    const bool primary=flags&3,dual=flags&12;
    const auto selector=primary ? unsigned(!(flags&1))+(dual?2:0) : 4u;
    message=found->second[selector];
  }else if(prompt.target.action==2) {
    const auto found=content_.action_messages.weapon_trades.find(prompt.definition);
    if(found==content_.action_messages.weapon_trades.end())return {};
    message=found->second;
  }else if(prompt.target.action>=4 && prompt.target.action<=6) {
    const auto found=content_.action_messages.units.find(prompt.definition);
    if(found==content_.action_messages.units.end() || prompt.target.index<0 ||
        unsigned(prompt.target.index)>=found->second.seats.size())return {};
    message=found->second.seats[unsigned(prompt.target.index)];
  }else if(prompt.target.action==8) {
    const auto found=content_.action_messages.units.find(prompt.definition);
    if(found==content_.action_messages.units.end())return {};
    message=found->second.flip;
  }else if(prompt.target.action!=3 && !(prompt.target.action==0 && prompt.message))return {};
  if(prompt.target.object==null_datum || !message)return {};
  const auto found=content_.strings.find(message);
  if(found==content_.strings.end())return {};
  auto text=found->second;
  // Actual MCC action templates use a separate callback namespace. Resolve
  // only complete qualified templates, preserving raw locale data and original
  // E45A thumbstick/training tokens outside these authored action messages.
  #include "ui/hud/mcc_action_prompt_templates.inc"
  const auto compatible=std::lower_bound(mcc_action_templates.begin(),mcc_action_templates.end(),std::string_view(text),
      [](const auto &row,std::string_view value){return row.first<value;});
  if(compatible!=mcc_action_templates.end() && compatible->first==text)text=compatible->second;
  //22D2EE's action tokens are resolved against the local input owner. The PC
  //host's requested bindings differ from the Xbox controller configuration.
  const auto button=[&](unsigned action) {
    if(!prompt_gamepad_[view])return vista_action_buttons_?std::string("\xee\x84\xb2")+prompt_keyboard_[view][action]:
        "["+prompt_keyboard_[view][action]+"]";
    const auto glyph=prompt_buttons_[view][action];
    if(glyph<0xe100 || glyph>0xe10f)throw FormatError("Unsupported action button binding");
    if(vista_action_buttons_)return std::string{"\xee\x84"}+char(0x80+(glyph-0xe100));
    constexpr std::array<const char*,16> names{"[A]","[B]","[X]","[Y]","[LB]","[RB]","[LT]","[RT]",
        "[D-pad up]","[D-pad down]","[D-pad left]","[D-pad right]","[Start]","[Back]","[LS]","[RS]"};
    return std::string(names[glyph-0xe100]);
  };
  const auto primary=button(0),secondary=button(1);
  for(const auto &[token,label]:std::array<std::pair<std::string,std::string>,4>{{
      {"\xee\x91\x88",primary},{"\xee\x91\x89",secondary},{"\xee\x91\x8c",button(2)},{"\xee\x91\x8a",button(3)}}}) {
    for(std::size_t at=0;(at=text.find(token,at))!=std::string::npos;at+=label.size())
      text.replace(at,token.size(),label);
  }
  return text;
}
std::vector<Vertex> MultiplayerHud::draw_player_status(std::u16string_view text,
    const MultiplayerState& state,unsigned id,unsigned view,unsigned players,
    unsigned width,unsigned height,const FullScoreboardPaintConstants& constants,
    std::optional<TwoPlayerSplit> split,float alpha) {
  Call call(*this,false);
  if(id>=state.players.size() || view>=materials_.size() || !width || !height ||
      !std::isfinite(alpha) || alpha<0 || alpha>1)
    throw FormatError("Invalid player status view");
  if(hand_ || !state.players[id].connected || text.empty() || alpha==0)return {};
  const bool widescreen=hud_layout_widescreen(players,std::uint64_t(width)*3>std::uint64_t(height)*4,split);
  auto viewport=hud_viewport(players,view,widescreen,split);
  const float scale=std::min(width/640.f,height/480.f),left=(width-640*scale)/2,top=(height-480*scale)/2;
  const auto old=viewport.outer;
  viewport.outer={(old.top*height/480.f-top)/scale,(old.left*width/640.f-left)/scale,
      (old.bottom*height/480.f-top)/scale,(old.right*width/640.f-left)/scale};
  viewport.safe={viewport.outer.top+viewport.safe.top-old.top,viewport.outer.left+viewport.safe.left-old.left,
      viewport.outer.bottom+viewport.safe.bottom-old.bottom,viewport.outer.right+viewport.safe.right-old.right};
  const auto short_word=[](int value){return std::bit_cast<std::int16_t>(std::uint16_t(value));};
  const auto index=players>1?1u:0u;const auto& font=*status_fonts_[index];
  const auto line_height=font.ascent+font.descent+font.leading;
  const auto row_top=short_word(int(viewport.safe.bottom)-int(viewport.outer.top)-22);
  FullScoreboardLayout layout;layout.status=FullScoreboardStatus::Ready;
  auto& title=layout.title;title.valid=true;title.right_justified=true;title.text=text;
  title.text_rectangle={row_top,short_word(int(viewport.safe.left)-int(viewport.outer.left)),
      short_word(int(row_top)+line_height*5),short_word(int(viewport.safe.right)-int(viewport.outer.left))};
  const auto rgb=source_hud_message_color(elite_hud(content_,state.players[id]));
  title.foreground={alpha,rgb[0],rgb[1],rgb[2]};
  FullScoreboardPaintFrame frame;
  frame.clip={0,0,short_word(int(viewport.outer.bottom)-int(viewport.outer.top)),
      short_word(int(viewport.outer.right)-int(viewport.outer.left))};
  frame.clock={30,tick_seconds,std::int32_t(state.tick)};
  frame.font_horizontal_offset=status_font_offsets_[index];frame.shadow_enabled=true;frame.x87_control_word=0x27f;
  const FullScoreboardPainter painter(font,status_materials_[index],status_materials_[index]);
  auto result=painter.draw(layout,constants,frame);
  if(result.status!=FullScoreboardPaintStatus::Ready)
    throw FormatError("Original player status requires unavailable text controls or glyph state");
  for(auto& vertex:result.draw.vertices) {
    vertex.position.x=((vertex.position.x+viewport.outer.left)*scale+left)*640/width;
    vertex.position.y=((vertex.position.y+viewport.outer.top)*scale+top)*480/height;
  }
  return std::move(result.draw.vertices);
}
std::vector<Vertex> MultiplayerHud::draw_primary_message(
    const SourceHudPrimaryMessageTu15& message,const MultiplayerState& state,
    unsigned id,unsigned view,unsigned players,unsigned width,unsigned height,
    const FullScoreboardPaintConstants& constants,std::optional<TwoPlayerSplit> split,float alpha) {
  Call call(*this,false);
  if(id>=state.players.size() || view>=materials_.size() || !width || !height ||
      !std::isfinite(alpha) || alpha<0 || alpha>1)
    throw FormatError("Invalid primary message view");
  if(hand_ || !state.players[id].connected || !message.active ||
      message.text.empty() || message.duration_seconds<=0 || alpha==0)return {};
  //1494F0 converts the authored integer seconds to the30Hz simulation clock.
  //25B49B..25B4B3 has a strict upper bound, including for duration zero.
  const auto duration=std::int64_t(message.duration_seconds)*30;
  if(duration>std::numeric_limits<std::int32_t>::max())return {};
  const auto now=std::bit_cast<std::int32_t>(state.tick);
  const auto created=std::bit_cast<std::int32_t>(message.created_tick);
  const auto end=std::bit_cast<std::int32_t>(message.created_tick+std::uint32_t(duration));
  if(now<created || now>=end)return {};
  const auto age=std::int64_t(now)-created;
  //25B4CC..25B502 uses the source double exponent containing float0.4.
  alpha*=float(std::pow(1-double(age)/double(duration),double(.4f)));
  const bool widescreen=hud_layout_widescreen(players,
      std::uint64_t(width)*3>std::uint64_t(height)*4,split);
  const auto presentation=hud_presentation_layout(hud_viewport(players,view,widescreen,split),width,height);
  const auto& viewport=presentation.viewport;
  const auto short_word=[](int value){return std::bit_cast<std::int16_t>(std::uint16_t(value));};
  const auto index=players>1?1u:0u;const auto& font=*status_fonts_[index];
  const auto line_height=font.ascent+font.descent+font.leading;
  const auto row_top=short_word(int(viewport.safe.bottom)-int(viewport.outer.top)-40);
  FullScoreboardLayout layout;layout.status=FullScoreboardStatus::Ready;
  auto& title=layout.title;title.valid=true;title.right_justified=true;
  title.text=message.text.substr(0,62); //25AB76's63-word terminated buffer.
  title.text_rectangle={row_top,short_word(int(viewport.safe.left)-int(viewport.outer.left)),
      short_word(int(row_top)+line_height*5),short_word(int(viewport.safe.right)-int(viewport.outer.left))};
  const auto rgb=source_hud_message_color(elite_hud(content_,state.players[id]));
  title.foreground={alpha,rgb[0],rgb[1],rgb[2]};
  FullScoreboardPaintFrame frame;
  frame.clip={0,0,short_word(int(viewport.outer.bottom)-int(viewport.outer.top)),
      short_word(int(viewport.outer.right)-int(viewport.outer.left))};
  frame.clock={30,tick_seconds,std::int32_t(state.tick)};
  frame.font_horizontal_offset=status_font_offsets_[index];frame.shadow_enabled=true;frame.x87_control_word=0x27f;
  const FullScoreboardPainter painter(font,status_materials_[index],status_materials_[index]);
  auto result=painter.draw(layout,constants,frame);
  if(result.status!=FullScoreboardPaintStatus::Ready)
    throw FormatError("Original primary message requires unavailable text controls or glyph state");
  for(auto& vertex:result.draw.vertices) {
    vertex.position.x=((vertex.position.x+viewport.outer.left)*presentation.scale+
        presentation.offset.x)*640/width;
    vertex.position.y=((vertex.position.y+viewport.outer.top)*presentation.scale+
        presentation.offset.y)*480/height;
  }
  return std::move(result.draw.vertices);
}
std::vector<Vertex> MultiplayerHud::draw(const MultiplayerState &s, unsigned id,
                                         unsigned view, unsigned players,
                                         unsigned width, unsigned height,
                                         int limit, unsigned time_ticks,
                                         std::optional<Vec2> player_aim_anchor_pixels,
                                         std::optional<TwoPlayerSplit> split,
                                         std::optional<SourceHudAimCamera> target_camera) {
  return draw(s,id,view,players,width,height,limit,time_ticks,player_aim_anchor_pixels,split,target_camera,nullptr);
}
std::vector<Vertex> MultiplayerHud::draw(const MultiplayerState &s, unsigned id,
                                         unsigned view, unsigned players,
                                         unsigned width, unsigned height,
                                         int limit, unsigned time_ticks,
                                         std::optional<Vec2> player_aim_anchor_pixels,
                                         std::optional<TwoPlayerSplit> split,
                                         std::optional<SourceHudAimCamera> target_camera,
                                         const SourceHudMessages* messages) {
  Call call(*this,false);
  if (id >= multiplayer_capacity || view >= materials_.size() || !width ||
      !height)
    throw FormatError("Invalid HUD draw viewport");
  std::vector<Vertex> out;
  if (!s.players[id].connected)
    return out;
  //25AE69 admits event text independently of the current unit. Death hides
  // first-person artwork and the sensor, while the recipient's queued kill/
  // death messages retain their actual clock and local viewport.
  const bool unit_hud=!s.players[id].dead || (s.defusal && defusal_objectives_);
  const auto score=unit_hud?multiplayer_hud_score_display(s,id,limit,time_ticks,content_):SourceHudScoreDisplay{};
  // Native output aspect selects the corresponding retail display mode.
  const bool widescreen = hud_layout_widescreen(players,
      std::uint64_t(width) * 3 > std::uint64_t(height) * 4,split);
  auto viewport = hud_viewport(players, view, widescreen,split);
  const auto camera_viewport=viewport;
  const unsigned logical_width =
                     unsigned(viewport.outer.right - viewport.outer.left),
                 logical_height =
                     unsigned(viewport.outer.bottom - viewport.outer.top);
  const auto presentation = hud_presentation_layout(viewport, width, height);
  const float scale = presentation.scale,
              left = presentation.offset.x, top = presentation.offset.y;
  viewport = presentation.viewport;
  if(unit_hud)for (unsigned i = 0; i < content_.widgets.size(); ++i) {
    const auto &w = content_.widgets[i];
    if (materials_[view][i].empty() ||
        !hud_widget_visible(w, conditions(w, content_, s, id, score,!voice_text_.empty(),hand_)))
      continue;
    const unsigned mode =
        hud_screen_mode(logical_width, logical_height, w.anchor, widescreen);
    const auto input =
        widget_inputs(w, content_, s, id, score, winning_[view].amount,hand_);
    Vec2 p = w.anchor == 4 ? presentation.center : anchor(w.anchor, viewport),
         ax{1, 0}, ay{0, 1};
    if(w.anchor==5) {
      if(!target_camera)continue;
      const auto point=(w.weapon_type==1 || w.weapon_type==4)?
          multiplayer_hud_weapon_state(s.players[id],content_,hand_).target_position:Vec3{};
      const auto physical=[&](UiRect r) {return full_surface_viewport(
          {r.left,r.top,r.right-r.left,r.bottom-r.top},width,height);};
      const auto projected=source_hud_target_anchor(*target_camera,physical(camera_viewport.outer),physical(camera_viewport.safe),point);
      if(!projected)continue;
      p={(projected->x-left)/scale,(projected->y-top)/scale};
    }
    if(w.anchor==4 && player_aim_anchor_pixels)
      p={(player_aim_anchor_pixels->x-left)/scale,
         (player_aim_anchor_pixels->y-top)/scale};
    p.x += w.offset[mode].x;
    p.y += w.offset[mode].y;
    if (!w.effects.empty()) {
      const auto &effect = w.effects[0];
      const auto &tables = content_.shaders.at(w.shader).animation.tables;
      if (!tables)
        throw FormatError("Missing original HUD function tables");
      auto value = [&](unsigned k) {
        const auto &f = effect.functions[k];
        return shader_function_value(f.curve, function(f.input, input,
            elite_hud(content_,s.players[id])), 1,
                                     *tables);
      };
      if (effect.flags & 2) {
        const auto angle = value(2);
        const float sine = std::sin(angle), cosine = std::cos(angle);
        ax = {cosine, sine};
        ay = {-sine, cosine};
      }
      if (effect.flags & 4) {
        const auto x = value(3), y = value(4);
        p.x += ax.x * x + ay.x * y;
        p.y += ax.y * x + ay.y * y;
      }
      if (effect.flags & 1) {
        const auto x = value(0), y = value(1);
        ax.x *= x;
        ax.y *= x;
        ay.x *= y;
        ay.y *= y;
      }
    }
    auto quad = [&](float x, float y, float w, float h, unsigned material,
                    float u0, float v0, float u1, float v1) {
      auto vertex = [&](float dx, float dy, float u, float v) {
        return Vertex{
            {p.x + ax.x * dx + ay.x * dy, p.y + ax.y * dx + ay.y * dy, 1},
            {1, 1, 1},
            {u, v},
            material};
      };
      const auto a = vertex(x, y, u0, v0), b = vertex(x + w, y, u1, v0),
                 c = vertex(x, y + h, u0, v1), d = vertex(x + w, y + h, u1, v1);
      out.insert(out.end(), {a, b, c, b, d, c});
    };
    if (!w.is_text) {
      const auto &image = content_.images.at(w.bitmap);
      const int seq = w.sequence[mode];
      if (seq < 0)
        continue;
      UiSprite sprite;
      if (image.sequences.empty() && seq == 0)
        sprite.image = 0;
      else {
        if (unsigned(seq) >= image.sequences.size() ||
            image.sequences[seq].empty())
          throw FormatError("Original HUD sequence outside bitmap");
        const auto &sprites=image.sequences[seq];
        const auto selected=elite_hud(content_,s.players[id]) && sprites.size()>1 ? 1u:0u;
        sprite = sprites[selected];
      }
      const auto &texture = image.images.at(sprite.image);
      const auto territories=s.players[id].source_hud_objectives.territory_count;
      if(territories>8)throw FormatError("Territory HUD exceeds original eight-meter bank");
      const unsigned layers=w.special==5?5u:w.special==7?territories:1u;
      for (unsigned layer = 0; layer < layers; ++layer) {
        if (w.special == 5 &&
            !hud_shield_layer(shields_[view], s.players[id].shield, layer)
                 .visible)
          continue;
        const auto material=materials_[view][i].at(layer*image.images.size()+sprite.image);
        {
          if((w.flags&0x1c) && !geometry_)throw FormatError("Original HUD stretch/mirror constants unavailable");
          //22AFF1 rounds the near and far terms independently. Rounding a
          //size after rounding its registration can shift a meter edge away
          //from the authored outline when sprite bounds are fractional.
          const auto logical=mcc_hud_bitmap_logical_extent(image,sprite.image)
              .value_or(std::array<unsigned,2>{texture.width,texture.height});
          //22B30E..22B3C6/231D0E: center the source-width meter row, then
          //advance in screen X by width+1 for each territory. Each view and
          //territory owns a material so later meters cannot overwrite it.
          const float territory_x=w.special==7?
              (float(layer)-float(territories-1)*.5f)*float(logical[0]+1):0;
          const auto geometry=source_hud_bitmap_geometry({{p.x,p.y},{ax.x,ax.y},{ay.x,ay.y},
              float(logical[0]),float(logical[1]),{sprite.left,sprite.right,sprite.top,sprite.bottom},
              {w.registration[mode].x,w.registration[mode].y},std::uint16_t(w.flags)},
              geometry_.value_or(HudBitmapGeometryConstants{}));
          for(unsigned n=0;n<geometry.count;++n) {
            const auto &q=geometry.quads[n];const auto &uv=q.sprite_bounds;
            const std::array<std::array<float,2>,4> tex{{{uv[0],uv[2]},{uv[1],uv[2]},{uv[1],uv[3]},{uv[0],uv[3]}}};
            std::array<Vertex,4> v;
            const auto offset=sampled_offsets_.find(material);
            for(unsigned k=0;k<4;++k)v[k]={{q.positions[k][0]+territory_x,q.positions[k][1],1},{1,1,1},
                {tex[k][0]+(offset==sampled_offsets_.end()?0:offset->second[0]),
                 tex[k][1]+(offset==sampled_offsets_.end()?0:offset->second[1])},material};
            out.insert(out.end(),{v[0],v[1],v[3],v[1],v[2],v[3]});
          }
        }
      }
    } else {
      // Original 22AA16 transforms the text anchor, then draws glyphs at
      // scale one. Rotation/scale axes do not transform the font pixels.
      ax = {1, 0};
      ay = {0, 1};
      const int f = w.fonts[mode];
      if (f < 0 || f >= int(fonts_.size()))
        continue;
      const auto &font = *fonts_[f];
      const auto text = codepoints(
          text_value(w, content_, input, score,elite_hud(content_,s.players[id]),voice_text_));
      float pen = 0, ink_left = 32767, ink_right = -32768;
      unsigned previous = 0;
      for (auto c : text) {
        const auto it = font.glyphs.find(c);
        if (it == font.glyphs.end())
          continue;
        const auto &g = it->second;
        float kern = 0;
        if (const auto k = font.kerning.find({c, previous});
            k != font.kerning.end())
          kern = float(k->second);
        const float gx = pen + g.bearing_x + kern;
        ink_left = std::min(ink_left, std::trunc(gx));
        ink_right = std::max(ink_right, std::trunc(gx + g.width + 1));
        pen += g.advance + g.bearing_x + kern;
        previous = c;
      }
      const float ink_width = std::max(0.f, ink_right - ink_left);
      float x = w.justification == 1   ? -std::floor(ink_width * .5f)
                : w.justification == 2 ? -ink_width
                                       : 0;
      // Original 22AA16:22AB7F..22AC94 subtracts the current outer
      // viewport before truncating the justified text rectangle. Rounding
      // absolute coordinates shifts counters between wide player cells.
      p.x = std::trunc(p.x - viewport.outer.left + x) + viewport.outer.left;
      p.y = std::trunc(p.y - viewport.outer.top) + viewport.outer.top;
      x = 0;
      previous = 0;
      for (auto c : text) {
        const auto it = font.glyphs.find(c);
        if (it == font.glyphs.end())
          continue;
        const auto &g = it->second;
        if (const auto k = font.kerning.find({c, previous});
            k != font.kerning.end())
          x += k->second;
        auto uv=retail_font_sample_uvs(font,c,0,0,g.width,g.height);
        auto material=materials_[view][i].at(f);
        if(prompt_artwork_)if(const auto sample=prompt_artwork_->sample(f?8:6,c,prompt_devices_[view],g,0,0,g.width,g.height)) {
          uv=sample->uv;material=sample->material;
        }
        quad(x + g.bearing_x, float(font.ascent - g.bearing_y), float(g.width),
             float(g.height), material,
             uv[0],uv[1],uv[2],uv[3]);
        x += g.advance + g.bearing_x;
        previous = c;
      }
    }
  }
  const auto message=action_text_impl(s,id,view);
  if(!message.empty()) {
    //24CF66..24D005 selects font6/5 and safe-top +60/+35. 131BB0
    //uses the HUD help RGB with a separately supplied opaque text alpha.
    const auto f=players>1?1u:0u;const auto &font=*action_fonts_[f];
    const auto color=rgb(content_.action_messages.color);
    const Vec3 tint{color[0],color[1],color[2]};
    const float origin=viewport.safe.left;
    float x=origin,y=viewport.safe.top+(players>1?35.f:60.f);
    const auto line=std::max(1,font.ascent+font.descent+font.leading);
    const auto quad=[&](float l,float t,float r,float b,const std::array<float,4> &uv,
        unsigned material,Vec3 color) {
      const Vertex a{{l,t,1},color,{uv[0],uv[1]},material},b0{{r,t,1},color,{uv[2],uv[1]},material},
          c0{{l,b,1},color,{uv[0],uv[3]},material},d{{r,b,1},color,{uv[2],uv[3]},material};
      out.insert(out.end(),{a,b0,c0,b0,d,c0});
    };
    unsigned previous=0;const auto characters=codepoints(message);
    for(std::size_t ci=0;ci<characters.size();++ci) {
      const auto c=characters[ci];
      if(c=='\r')continue;
      if(c=='\n'){x=origin;y+=line;previous=0;continue;}
      if(vista_action_buttons_ && ((c>=0xe100 && c<=0xe10f) || c==0xe132)) {
        const auto &buttons=*action_button_fonts_[f];auto glyph=c;
        std::vector<unsigned> label;
        if(c==0xe132)for(const auto &name:prompt_keyboard_[view]) {
          const auto candidate=codepoints(name);
          if(candidate.size()>label.size() && ci+1+candidate.size()<=characters.size() &&
              std::equal(candidate.begin(),candidate.end(),characters.begin()+ci+1))label=candidate;
        }
        float label_width=0;unsigned previous_label=0;
        for(const auto key:label) {
          const auto &g=buttons.glyphs.at(key);
          if(const auto k=buttons.kerning.find({key,previous_label});k!=buttons.kerning.end())label_width+=k->second;
          label_width+=g.advance+g.bearing_x;previous_label=key;
        }
        // The supplied Vista font contains four actual keyboard backgrounds
        // (38/75/100/140 pixels). Select a wider source glyph for longer names.
        if(!label.empty())while(glyph<0xe135 && buttons.glyphs.at(glyph).width<label_width+8)++glyph;
        const auto &g=buttons.glyphs.at(glyph);
        // Preserve Vista's icon aspect and the original HUD X-button height.
        // In these supplied families the physical button raster is twice as tall.
        const float scale=float(font.glyphs.at(0xe102).height)/buttons.glyphs.at(0xe102).height;
        const float advance=(g.advance+g.bearing_x)*scale;
        if(x+advance>viewport.safe.right && x>origin){x=origin;y+=line;}
        if(y+font.ascent+font.descent>viewport.safe.bottom)break;
        const float l=x+g.bearing_x*scale,t=y+font.ascent-g.bearing_y*scale;
        auto button_uv=retail_font_sample_uvs(buttons,glyph,0,0,g.width,g.height);
        auto button_material=action_button_materials_[f];
        if(prompt_artwork_)if(const auto sample=prompt_artwork_->sample(f?5:6,glyph,prompt_devices_[view],g,0,0,g.width,g.height)) {
          button_uv=sample->uv;button_material=sample->material;
        }
        quad(l,t,l+g.width*scale,t+g.height*scale,button_uv,button_material,{1,1,1});
        if(!label.empty()) {
          const float label_scale=scale*std::min(1.f,float(g.width-8)/std::max(1.f,label_width));
          float lx=l+(g.width*scale-label_width*label_scale)*.5f;previous_label=0;
          for(const auto key:label) {
            const auto &letter=buttons.glyphs.at(key);
            if(const auto k=buttons.kerning.find({key,previous_label});k!=buttons.kerning.end())lx+=k->second*label_scale;
            const float kl=lx+letter.bearing_x*label_scale,kt=y+font.ascent-letter.bearing_y*label_scale;
            quad(kl,kt,kl+letter.width*label_scale,kt+letter.height*label_scale,
                retail_font_sample_uvs(buttons,key,0,0,letter.width,letter.height),action_button_materials_[f],tint);
            lx+=(letter.advance+letter.bearing_x)*label_scale;previous_label=key;
          }
          ci+=label.size();
        }
        x+=advance;previous=0;continue;
      }
      const auto at=font.glyphs.find(c);if(at==font.glyphs.end())continue;
      const auto &g=at->second;
      float kern=0;if(const auto k=font.kerning.find({c,previous});k!=font.kerning.end())kern=k->second;
      if(x+g.bearing_x+kern+g.width>viewport.safe.right && x>origin){x=origin;y+=line;previous=0;kern=0;}
      if(y+font.ascent+font.descent>viewport.safe.bottom)break;
      x+=kern;
      const float l=x+g.bearing_x,t=y+font.ascent-g.bearing_y,r=l+g.width,b=t+g.height;
      const auto uv=retail_font_sample_uvs(font,c,0,0,g.width,g.height);
      quad(l,t,r,b,uv,action_materials_[f],tint);x+=g.advance+g.bearing_x;previous=c;
    }
  }
  if(s.defusal && s.defusal_rules && defusal_objectives_ && target_camera &&
      (s.defusal->phase==BombDefusalPhase::live || s.defusal->phase==BombDefusalPhase::planted || s.defusal->phase==BombDefusalPhase::freeze)) {
    const auto f=players>1?1u:0u;const auto &font=*action_fonts_[f];
    const auto physical=[&](UiRect r){return full_surface_viewport(
        {r.left,r.top,r.right-r.left,r.bottom-r.top},width,height);};
    const auto rectangle=[&](float l,float t,float r,float b,Vec3 color) {
      if(l>=r || t>=b)return;
      const auto material=*defusal_marker_material_;
      const Vertex a{{l,t,1},color,{0,0},material},b0{{r,t,1},color,{1,0},material},
          c{{l,b,1},color,{0,1},material},d{{r,b,1},color,{1,1},material};
      out.insert(out.end(),{a,b0,c,b0,d,c});
    };
    const auto text=[&](std::string_view label,float center,float y,Vec3 color) {
      float width=0;for(const auto c:label)if(const auto at=font.glyphs.find(unsigned(c));at!=font.glyphs.end())width+=at->second.advance+at->second.bearing_x;
      center=std::clamp(center,viewport.safe.left+width*.5f,viewport.safe.right-width*.5f);
      float x=center-width*.5f;
      for(const auto character:label) {
        const auto c=unsigned(character);const auto at=font.glyphs.find(c);if(at==font.glyphs.end())continue;
        const auto &g=at->second;const float l=x+g.bearing_x,t=y+font.ascent-g.bearing_y,r=l+g.width,b=t+g.height;
        const auto uv=retail_font_sample_uvs(font,c,0,0,g.width,g.height);const auto material=action_materials_[f];
        const Vertex a{{l,t,1},color,{uv[0],uv[1]},material},b0{{r,t,1},color,{uv[2],uv[1]},material},
            c0{{l,b,1},color,{uv[0],uv[3]},material},d{{r,b,1},color,{uv[2],uv[3]},material};
        out.insert(out.end(),{a,b0,c0,b0,d,c0});x+=g.advance+g.bearing_x;
      }
    };
    for(unsigned i=0;i<s.defusal->sites.size();++i) {
      const auto visual=bomb_defusal_marker_visual(*s.defusal,*s.defusal_rules,i);
      auto point=visual.planted?s.defusal->bomb_position:s.defusal->sites[i].position;point.z+=.5f;
      const auto projected=source_hud_target_anchor(*target_camera,physical(camera_viewport.outer),physical(camera_viewport.safe),point);
      if(!projected)continue;
      const float size=players>1?22.f:28.f;
      const float x=std::clamp((projected->x-left)/scale,viewport.safe.left+size,viewport.safe.right-size);
      const float y=std::clamp((projected->y-top)/scale,viewport.safe.top+size,viewport.safe.bottom-size-22.f);
      const float l=x-size*.5f,t=y-size*.5f,r=x+size*.5f,b=y+size*.5f;
      const Vec3 color=visual.planted?Vec3{1,.18f,.08f}*visual.brightness:Vec3{.4f,.85f,1};
      rectangle(l,t,r,b,{.03f,.04f,.06f});
      rectangle(l+2,b-2-(size-4)*visual.red_fill,r-2,b-2,{visual.brightness,.025f,.015f});
      rectangle(l,t,r,t+2,color);rectangle(l,b-2,r,b,color);
      rectangle(l,t,l+2,b,color);rectangle(r-2,t,r,b,color);
      text(std::string(1,char('A'+i)),x,y-(font.ascent+font.descent)*.5f,{1,1,1});
      std::string label="BOMBSITE "+std::string(1,char('A'+i));
      if(visual.planted) {
        const auto seconds=(s.defusal->remaining+29)/30;
        label="BOMB "+std::to_string(seconds/60)+":"+(seconds%60<10?"0":"")+std::to_string(seconds%60);
      }
      text(label,x,b+3,color);
    }
  }
  if(content_.messages.enabled && !hand_) {
    const auto f=players>1?1u:0u;const auto &font=*action_fonts_[f];
    const auto color=source_hud_message_color(elite_hud(content_,s.players[id]));
    const Vec3 tint{color[0],color[1],color[2]};
    const auto line_height=std::max(1,font.ascent+font.descent+font.leading);
    float y=viewport.safe.top+(players>1?35.f:60.f);
    if(!message.empty())y+=float(line_height)*(1+std::count(message.begin(),message.end(),'\n'));
    const auto maximum=players>1?(message.empty()?3u:2u):4u;
    const auto lines=source_hud_message_lines(messages?*messages:s.players[id].source_messages,s.tick,tick_seconds,
        content_.messages.hold_seconds,content_.messages.fade_seconds,maximum);
    for(const auto &line:lines) {
      const auto row_top=y;
      float x=viewport.safe.left;unsigned previous{};unsigned text_lines=1;
      for(const auto character:line.text) {
        const auto c=unsigned(character);
        if(c=='\r')continue;
        if(c=='\n'){x=viewport.safe.left;y+=line_height;previous=0;++text_lines;continue;}
        const auto at=font.glyphs.find(c);if(at==font.glyphs.end())continue;
        const auto &g=at->second;
        float kern{};if(const auto k=font.kerning.find({c,previous});k!=font.kerning.end())kern=k->second;
        if(x+g.bearing_x+kern+g.width>viewport.safe.right && x>viewport.safe.left) {
          x=viewport.safe.left;y+=line_height;previous=0;kern=0;++text_lines;
        }
        if(y+font.ascent+font.descent>viewport.safe.bottom)break;
        x+=kern;
        const auto l=x+g.bearing_x,t=y+font.ascent-g.bearing_y,r=l+g.width,b=t+g.height;
        const auto uv=retail_font_sample_uvs(font,c,0,0,g.width,g.height);
        const auto material=action_materials_[f];
        const Vertex a{{l,t,line.alpha},tint,{uv[0],uv[1]},material},
            b0{{r,t,line.alpha},tint,{uv[2],uv[1]},material},
            c0{{l,b,line.alpha},tint,{uv[0],uv[3]},material},
            d{{r,b,line.alpha},tint,{uv[2],uv[3]},material};
        out.insert(out.end(),{a,b0,c0,b0,d,c0});
        x+=g.advance+g.bearing_x;previous=c;
      }
      //25B414 advances by the font's completed line extent times hudg+80,
      // truncating after each row, including the completed wrapped extent.
      y=float(int(float(line_height*text_lines)*content_.messages.line_spacing+row_top));
    }
  }
  //13954B draws the sensor separately after the authored nhdt widgets.
  // Its center offsets47FF40 and full/split extents come from the original
  //139614..139685/2548F0 corridor; they are not inferred from the background.
  if(unit_hud && !s.defusal && sensors_[view].material && sensors_[view].tick!=~0u &&
      (s.players[id].source_hud_zoom_mask&0x40)) {
    const auto mode=hud_screen_mode(logical_width,logical_height,2,widescreen);
    const auto base=anchor(2,viewport);
    const float half=players>1?32.f:42.f;
    const Vec2 center{base.x+(mode?31.f:41.f),base.y+(mode?-31.f:-42.f)};
    const unsigned material=*sensors_[view].material;
    const Vertex a{{center.x-half,center.y-half,1},{1,1,1},{0,0},material},
                 b{{center.x+half,center.y-half,1},{1,1,1},{1,0},material},
                 c{{center.x-half,center.y+half,1},{1,1,1},{0,1},material},
                 d{{center.x+half,center.y+half,1},{1,1,1},{1,1},material};
    out.insert(out.end(),{a,b,c,b,d,c});
  }
  // HUD groups use a full-surface viewport. Express pixel positions in the
  // UI vertex program's 640x480 range so Vulkan's clip volume also includes
  // the widescreen edges (changing only the scissor cannot do that).
  for (auto &v : out) {
    v.position.x = (v.position.x * scale + left) * 640 / width;
    v.position.y = (v.position.y * scale + top) * 480 / height;
  }
  return out;
}
} // namespace halo2
