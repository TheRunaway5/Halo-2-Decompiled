#include "halo2/action_prompt_bindings.hpp"
#include "halo2/hud.hpp"
#include "halo2/slayer.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/full_scoreboard_providers.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>

namespace halo2 {
SourceWeaponHudState multiplayer_hud_weapon_state(const MultiplayerState::Player &p,const HudContent &content,unsigned hand) {
  if(hand>1)throw FormatError("Invalid weapon HUD hand");
  const auto &shader=hand?p.secondary_weapon.shader:p.weapon_shader;
  const int loaded=hand?p.secondary_weapon.loaded:p.loaded;
  const int reserve=hand?p.secondary_weapon.reserve:p.reserve;
  const int available=hand?p.secondary_weapon.ammunition_available:p.ammunition_available;
  const auto datum=hand?p.secondary_weapon.datum:p.source_weapon;
  WeaponHudProjectionInput input;input.datum=datum;input.heat=shader.heat;input.age=shader.power_fraction;
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
  // Target acquisition and marker queries retain their separate live owners.
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
std::array<std::uint16_t, 4> conditions(const HudWidget &w,const HudContent &content,
                                        const MultiplayerState &s, unsigned id,
                                        const SourceHudScoreDisplay &score,bool talking,unsigned hand) {
  const auto &p = s.players.at(id);
  // Original13A163..13A18F sets shield availability and Elite appearance
  // independently. Both multiplayer bipeds use shield-enabled nhdt rows.
  std::uint16_t unit = 1 | (p.secondary_weapon.datum==null_datum?0x10:0x20) | p.source_hud_zoom_mask | 0x800 | 0x1000;
  if (multiplayer_representation(p.source_configuration)==3) unit |= 0x2000;
  unit |= p.grenade_current == 0 ? 4 : p.grenade_current == 1 ? 8 : 2;
  const auto projected=multiplayer_hud_weapon_state(p,content,hand);
  const auto *item=w.weapon_type==1?&projected:nullptr;
  const auto weapon=weapon_hud_cutoffs(item,w.cutoffs,weapon_hud_mask(item,w.weapon_type==1?(hand?2u:1u):w.weapon_type));
  return {unit, 0, weapon, std::uint16_t(source_hud_score_mask(score) | (talking?0x800:0))};
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
    case 16:
      out[i] = p.shield;
      break;
    case 17:
      out[i] = p.health;
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
      out[i]=weapon_hud_input(w.weapon_type==1?&projected:nullptr,w.inputs[i],float(s.tick)*tick_seconds);
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
  return w.special != 1 && w.special != 2 && w.special != 6 && w.special != 7;
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
MultiplayerHud::MultiplayerHud(HudContent content,
                               const std::filesystem::path &directory,
                               Mesh &mesh, unsigned local_players,
                               std::string variant,std::optional<HudBitmapGeometryConstants> geometry,
                               const std::filesystem::path &ui_assets,unsigned hand)
    : content_(std::move(content)),hand_(hand), mesh_(mesh), materials_(local_players),
      variant_(std::move(variant)),geometry_(geometry) {
  if(hand_>1)throw FormatError("Invalid HUD weapon hand");
  if(hand_)std::erase_if(content_.widgets,[](const auto &widget){return widget.weapon_type!=1;});
  hud_viewport(local_players, 0);
  std::ifstream table(directory / "font_table.txt");
  if (!table)
    throw FormatError("Missing original HUD font table");
  std::vector<std::string> names;
  std::string name;
  while (std::getline(table, name)) {
    if (!name.empty() && name.back() == '\r')
      name.pop_back();
    if (name.find_first_of("/\\") != std::string::npos || name == "..")
      throw FormatError("Invalid HUD font name");
    names.push_back(name);
  }
  if (names.size() != 11)
    throw FormatError("Incomplete original HUD font table");
  fonts_[0] = load_retail_font(directory / names[6]);
  fonts_[1] = load_retail_font(directory / names[8]);
  if(content_.action_messages.enabled && !hand_)for(unsigned i=0;i<2;++i){
    const auto selector=i?5:6;
    action_fonts_[i]=load_retail_font(directory/names[selector]);
    const auto sampled_font=hud_sampled_font_path(ui_assets,selector,names[selector]);
    if(!ui_assets.empty() && std::filesystem::is_regular_file(sampled_font)) {
      const auto sampled=load_retail_font(sampled_font);
      // User selected high-resolution redraws for pickup/interact controls.
      // Keep Vista logical metrics; the optional overlay supplies sample pixels.
      const auto button_source=hud_menu_asset_directory(ui_assets);
      action_button_fonts_[i]=load_retail_font(hud_sampled_font_path(button_source,selector,names[selector]));
      auto &buttons=action_button_fonts_[i];
      // Use the selected redraw pack for action controls, while retaining
      // the original logical glyph metrics and live binding substitution.
      apply_vista_prompt_sampling(buttons,ui_assets,selector);
      // Vista names E132 keyboard_button_background0. The actual decoded
      // control/keycap glyphs retain their native RGB without HUD tinting.
      for(const auto c:{0xe100u,0xe101u,0xe102u,0xe103u,0xe104u,0xe105u,0xe106u,0xe107u,
          0xe108u,0xe109u,0xe10au,0xe10bu,0xe10cu,0xe10du,0xe10eu,0xe10fu,
          0xe132u,0xe133u,0xe134u,0xe135u}) {
        if(buttons.glyph_record_indices.size()!=65536 ||
            std::count(buttons.glyph_record_indices.begin(),buttons.glyph_record_indices.end(),
                buttons.glyph_record_indices.at(c))!=1 || !buttons.glyphs.at(c).width)
          throw FormatError("Selected UI fonts lack authored Vista action button artwork");
      }
      if(is_mcc_classic_hud_pack(ui_assets))apply_mcc_classic_font_sampling(action_fonts_[i],sampled);
      else apply_retail_font_sampling(action_fonts_[i],sampled);
      apply_vista_prompt_sampling(action_fonts_[i],ui_assets,selector);
      action_button_materials_[i]=unsigned(mesh_.materials.size());
      mesh_.materials.push_back({"action-message/vista-buttons/font"+std::to_string(selector),buttons.atlas});
      auto &material=mesh_.materials.back();material.ui_full_surface=true;
      material.texture_states[0].address_u=material.texture_states[0].address_v=3;
      if(i==1)vista_action_buttons_=true;
    }
    action_materials_[i]=unsigned(mesh_.materials.size());
    mesh_.materials.push_back({"action-message/font"+std::to_string(selector),action_fonts_[i].atlas});
    auto &material=mesh_.materials.back();material.ui_full_surface=true;
    material.texture_states[0].address_u=material.texture_states[0].address_v=3;
  }
  const auto sampled=load_ui_asset_library(content_.images,ui_assets,"hud.tsv");
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
  if(!ui_assets.empty()) for(unsigned i=0;i<fonts_.size();++i) {
    const auto path=hud_sampled_font_path(ui_assets,i==0?6:8,names[i==0?6:8]);
    if(std::filesystem::is_regular_file(path)) {
      const auto donor=ui_assets/"fonts-source"/names[i==0?6:8];
      if(mcc)apply_mcc_classic_font_sampling(fonts_[i],load_retail_font(path));
      else if(std::filesystem::exists(ui_assets/"hud-reconstruction.tsv"))
        apply_retail_font_sampling(fonts_[i],load_retail_font(path),load_retail_font(donor));
      else apply_retail_font_sampling(fonts_[i],load_retail_font(path));
    }
    apply_vista_prompt_sampling(fonts_[i],ui_assets,i==0?6:8);
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
      if (w.is_text)
        for (const auto &font : fonts_)
          add(font.atlas);
      else if (w.bitmap != null_datum)
        for (unsigned layer = 0; layer < (w.special == 5 ? 5u : 1u); ++layer)
          {
            const auto &image=content_.images.at(w.bitmap);
            const auto replacement=sampled.find(image.name);
            unsigned frame=0;
            for(const auto &t:replacement==sampled.end()?image.images:replacement->second) {
              // Relative fill/outline placement belongs to the donor artwork.
              // Apply it only to shield fill sampling, retaining source quads.
              if(w.special==5 && replacement!=sampled.end())
                if(const auto offset=sampled_offsets.find({image.name,frame});offset!=sampled_offsets.end())
                  sampled_offsets_.emplace(unsigned(mesh_.materials.size()),offset->second);
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
      auto &p=m.forward_program.words;
      p[0]=0x18110000;p[8]=8;p[9]=0x1800;p[10]=0xffffffff;
      p[26]=0x80;p[34]=0x08010000;p[43]=0xff00ff00;p[45]=0x80;p[53]=1;p[54]=1;
      m.texture_states[0].address_u=m.texture_states[0].address_v=3;
      m.texture_states[0].mip_filter=0;
      sensors_[view].material=unsigned(mesh_.materials.size());
      mesh_.materials.push_back(std::move(m));
    }
  }
}
std::optional<unsigned> MultiplayerHud::motion_sensor_material(unsigned view)const {
  return sensors_.at(view).material;
}
void MultiplayerHud::update_motion_sensor(const MultiplayerState &s,unsigned id,unsigned view) {
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
    const std::uint8_t type=n==id?1 : q.source_membership==p.source_membership?2:3;
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
                             multiplayer_representation(s.players[id].source_configuration)==3);
        resolved_inputs = shield.inputs;
      }
      auto &m = mesh_.materials.at(material);
      const auto &pass = m.passes.at(m.forward_pass_index);
      m.forward_program = pass.program;
      animate_forward_material(m, seconds, [&](auto name) {
        return function(name, resolved_inputs,multiplayer_representation(s.players[id].source_configuration)==3);
      });
      std::array<float, 256> scalars{};
      scalars[40] = 1;
      std::array<ShaderVector, 256> colors{};
      if (w.special == 5) {
        for (unsigned c = 0; c < 3; ++c)
          colors[36 + c] = shield.colors[c];
      }
      else if (w.special == 3 || w.special == 4) {
        const auto &bits=w.special==3 ? score.color : score.rival_color;
        colors[36]=colors[37]={std::bit_cast<float>(bits[0]),
            std::bit_cast<float>(bits[1]),std::bit_cast<float>(bits[2]),1};
      }
      remap_pixel_externs(m.forward_program, scalars, colors,
                          pass.pixel_externs, pass.fragments,
                          m.shader_switches);
    }
  }
}
void MultiplayerHud::prompt_boarding_buttons(unsigned view,const PcKeyboardSettings &keys,const ControllerSettings &pad) {
  const auto k=native_boarding_prompt_keyboard_buttons(keys);const auto p=native_boarding_prompt_controller_buttons(pad);
  for(unsigned i=0;i<2;++i){prompt_keyboard_.at(view)[i+2]=k[i];prompt_buttons_.at(view)[i+2]=p[i];}
}
std::string MultiplayerHud::action_text(const MultiplayerState &s,unsigned id,unsigned view)const {
  if(id>=s.players.size() || view>=prompt_gamepad_.size())throw FormatError("Invalid action prompt view");
  const auto &p=s.players[id];
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
std::vector<Vertex> MultiplayerHud::draw(const MultiplayerState &s, unsigned id,
                                         unsigned view, unsigned players,
                                         unsigned width, unsigned height,
                                         int limit, unsigned time_ticks,
                                         std::optional<Vec2> player_aim_anchor_pixels,
                                         std::optional<TwoPlayerSplit> split) {
  if (id >= multiplayer_capacity || view >= materials_.size() || !width ||
      !height)
    throw FormatError("Invalid HUD draw viewport");
  std::vector<Vertex> out;
  if (!s.players[id].connected || s.players[id].dead)
    return out;
  const auto score=multiplayer_hud_score_display(s,id,limit,time_ticks,content_);
  // Native output aspect selects the corresponding retail display mode.
  const bool widescreen = hud_layout_widescreen(players,
      std::uint64_t(width) * 3 > std::uint64_t(height) * 4,split);
  auto viewport = hud_viewport(players, view, widescreen,split);
  const unsigned logical_width =
                     unsigned(viewport.outer.right - viewport.outer.left),
                 logical_height =
                     unsigned(viewport.outer.bottom - viewport.outer.top);
  // Preserve the authored pixel shape on wider output surfaces. Anchor
  // margins remain in original HUD pixels while each world viewport expands.
  const float scale = std::min(width / 640.f, height / 480.f),
              left = (width - 640 * scale) / 2,
              top = (height - 480 * scale) / 2;
  const auto old = viewport.outer;
  viewport.outer = {(old.top * height / 480.f - top) / scale,
                    (old.left * width / 640.f - left) / scale,
                    (old.bottom * height / 480.f - top) / scale,
                    (old.right * width / 640.f - left) / scale};
  viewport.safe = {viewport.outer.top + viewport.safe.top - old.top,
                   viewport.outer.left + viewport.safe.left - old.left,
                   viewport.outer.bottom + viewport.safe.bottom - old.bottom,
                   viewport.outer.right + viewport.safe.right - old.right};
  for (unsigned i = 0; i < content_.widgets.size(); ++i) {
    const auto &w = content_.widgets[i];
    if (materials_[view][i].empty() ||
        !hud_widget_visible(w, conditions(w, content_, s, id, score,!voice_text_.empty(),hand_)))
      continue;
    const unsigned mode =
        hud_screen_mode(logical_width, logical_height, w.anchor, widescreen);
    const auto input =
        widget_inputs(w, content_, s, id, score, winning_[view].amount,hand_);
    Vec2 p = anchor(w.anchor, viewport), ax{1, 0}, ay{0, 1};
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
            multiplayer_representation(s.players[id].source_configuration)==3), 1,
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
        const auto selected=multiplayer_representation(s.players[id].source_configuration)==3 && sprites.size()>1 ? 1u:0u;
        sprite = sprites[selected];
      }
      const auto &texture = image.images.at(sprite.image);
      for (unsigned layer = 0; layer < (w.special == 5 ? 5u : 1u); ++layer) {
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
          const auto geometry=source_hud_bitmap_geometry({{p.x,p.y},{ax.x,ax.y},{ay.x,ay.y},
              float(texture.width),float(texture.height),{sprite.left,sprite.right,sprite.top,sprite.bottom},
              {w.registration[mode].x,w.registration[mode].y},std::uint16_t(w.flags)},
              geometry_.value_or(HudBitmapGeometryConstants{}));
          for(unsigned n=0;n<geometry.count;++n) {
            const auto &q=geometry.quads[n];const auto &uv=q.sprite_bounds;
            const std::array<std::array<float,2>,4> tex{{{uv[0],uv[2]},{uv[1],uv[2]},{uv[1],uv[3]},{uv[0],uv[3]}}};
            std::array<Vertex,4> v;
            const auto offset=sampled_offsets_.find(material);
            for(unsigned k=0;k<4;++k)v[k]={{q.positions[k][0],q.positions[k][1],1},{1,1,1},
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
      const auto &font = fonts_[f];
      const auto text = codepoints(
          text_value(w, content_, input, score,multiplayer_representation(s.players[id].source_configuration)==3,voice_text_));
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
        const auto uv=retail_font_sample_uvs(font,c,0,0,g.width,g.height);
        quad(x + g.bearing_x, float(font.ascent - g.bearing_y), float(g.width),
             float(g.height), materials_[view][i].at(f),
             uv[0],uv[1],uv[2],uv[3]);
        x += g.advance + g.bearing_x;
        previous = c;
      }
    }
  }
  const auto message=action_text(s,id,view);
  if(!message.empty()) {
    //24CF66..24D005 selects font6/5 and safe-top +60/+35. 131BB0
    //uses the HUD help RGB with a separately supplied opaque text alpha.
    const auto f=players>1?1u:0u;const auto &font=action_fonts_[f];
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
        const auto &buttons=action_button_fonts_[f];auto glyph=c;
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
        quad(l,t,l+g.width*scale,t+g.height*scale,
            retail_font_sample_uvs(buttons,glyph,0,0,g.width,g.height),action_button_materials_[f],{1,1,1});
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
  //13954B draws the sensor separately after the authored nhdt widgets.
  // Its center offsets47FF40 and full/split extents come from the original
  //139614..139685/2548F0 corridor; they are not inferred from the background.
  if(sensors_[view].material && sensors_[view].tick!=~0u &&
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
