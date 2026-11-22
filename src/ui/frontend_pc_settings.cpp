#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include <SDL.h>
#include <algorithm>
#include <atomic>

namespace halo2 {
namespace {
#include "ui/vista/vista_keyboard_presets.inc"
std::string low_string(const FrontendContent &content, const UiScreen &screen, unsigned low) {
  return detail::frontend_pc_compiled_low_localized(content, screen, low);
}
std::atomic<std::uint64_t> pc_instance{1ull << 63};
namespace bridge = online_protocol_bridge;

bridge::VideoPanelFacts video_facts(const DisplaySettings &display, unsigned width,
                                  unsigned height, FullscreenMode fullscreen) {
  return {
      .window = {width, height},
      .internal = {display.internal_width, display.internal_height},
      .graphics_mode = unsigned(display.graphics_mode),
      .path_tracing = display.graphics_mode == pstencil::GraphicsMode::PathTracing,
      .fullscreen = unsigned(fullscreen),
      .upscaler = unsigned(display.upscaler),
      .upscaling_quality = unsigned(display.upscaling_quality),
      .upscaling_enabled = display.upscaler != Upscaler::Off,
      .brightness = display.brightness,
      .gamma = display.gamma,
      .detail = display.detail,
      .safe_area = display.safe_area,
      .field_of_view = display.field_of_view,
      .vehicle_field_of_view = display.vehicle_field_of_view,
      .crt_filter = display.crt_filter,
      .vertical_split = display.two_player_split != TwoPlayerSplit::Horizontal,
      .ray_samples = display.path_tracing_samples,
      .bounce_depth = display.path_tracing_bounces,
  };
}

void read_keyboard_options(const MenuState &state, PcKeyboardSettings &keys) {
  keys.sensitivity = std::clamp(state.profile_option(0x380, 3), 1, 10);
  keys.invert = state.profile_option(0x381) != 0;
  keys.auto_center = state.profile_option(0x382) != 0;
  keys.dual_wield_invert = state.profile_option(0x383) != 0;
}

int saved_binding(const MenuState &state, unsigned action, bool secondary, int fallback) {
  const int binding = state.profile_option((secondary ? 0x440 : 0x400) + action, fallback);
  return valid_pc_binding(binding) ? binding : fallback;
}

bridge::PcSettingsFacts settings_facts(const MenuState &state, unsigned screen, unsigned context,
                                       std::optional<unsigned> row = {}, int binding_action = -1) {
  bridge::PcSettingsFacts facts{};
  facts.gameplay_overlay = state.game_pause();
  const auto &display = state.pc_display_settings();
  facts.volume = display.volume;
  facts.speakers = display.speakers;
  facts.subtitles = display.subtitles;
  if (screen == 243)
    facts.keyboard_preset = state.profile_option(0x384, 4);
  const bool shared_choice = screen == 42 || screen == 44;
  if ((screen == 200 && row) || screen == 40 || screen == 41 || screen == 43 ||
      screen == 183 || (shared_choice && context == 1)) {
    const auto controller = state.controller_settings();
    facts.sticks = controller.sticks;
    facts.buttons = controller.buttons;
    facts.controller_sensitivity = controller.sensitivity;
    facts.controller_invert = controller.invert;
    facts.controller_auto_center = controller.auto_center;
    facts.vibration = controller.vibration;
  }
  if ((screen == 258 && row && *row > 0) || screen == 245 || screen == 294 ||
      (shared_choice && context != 1)) {
    PcKeyboardSettings keys;
    read_keyboard_options(state, keys);
    facts.keyboard_sensitivity = unsigned(keys.sensitivity);
    facts.keyboard_invert = keys.invert;
    facts.keyboard_auto_center = keys.auto_center;
    facts.dual_wield_invert = keys.dual_wield_invert;
  }
  if (screen == 258 && row == 0u) {
    const PcKeyboardSettings defaults;
    const auto keyboard = state.pc_keyboard_settings();
    facts.default_bindings = keyboard.bindings == defaults.bindings && keyboard.secondary == defaults.secondary;
  }
  if (screen == 253 && binding_action >= 0 && unsigned(binding_action) < pc_action_count) {
    const PcKeyboardSettings defaults;
    facts.primary_binding = unsigned(saved_binding(state, binding_action, false, defaults.bindings[binding_action]));
    facts.secondary_binding = unsigned(saved_binding(state, binding_action, true, defaults.secondary[binding_action]));
  }
  return facts;
}

std::string settings_text(const bridge::SettingsText &text, const FrontendContent &content,
                       unsigned screen, Upscaler upscaler) {
  const auto source_screen = [&]() -> const UiScreen & {
    return content.screen(text.screen ? vista_menu(text.screen) : screen);
  };
  std::string result(text.text);
  using Source = bridge::SettingsTextSource;
  switch (text.source) {
  case Source::Literal: break;
  case Source::PcBinding: result += pc_binding_name(int(text.key)); break;
  case Source::LocalizedLowId: result += low_string(*content.vista, source_screen(), text.key); break;
  case Source::LocalizedId:
    result += detail::frontend_pc_compiled_localized(*content.vista, source_screen(), text.key);
    break;
  case Source::GraphicsMode: result += pstencil::graphics_mode_name(pstencil::GraphicsMode(text.key)); break;
  case Source::FullscreenMode: result += fullscreen_mode_name(FullscreenMode(text.key)); break;
  case Source::Upscaler: result += upscaler_name(Upscaler(text.key)); break;
  case Source::UpscalingQuality: result += upscaling_quality_name(UpscalingQuality(text.key), upscaler); break;
  }
  result.append(text.suffix.data(), text.suffix.size());
  return result;
}

unsigned present_settings_panel(MenuFrame &frame, const FrontendContent &content,
                                const bridge::SettingsPanel &panel, Upscaler upscaler) {
  frame.title = std::string(panel.title);
  frame.description = settings_text(panel.description, content, frame.screen, upscaler);
  frame.entries.clear();
  frame.entries.reserve(panel.rows.size());
  for (const auto &row : panel.rows) {
    const auto destination = row.vista_route ? int(vista_menu(unsigned(row.destination))) : row.destination;
    frame.entries.push_back({settings_text(row.label, content, frame.screen, upscaler), destination, row.value});
  }
  return panel.selected;
}

unsigned build_video_presentation(MenuFrame &frame, const FrontendContent &content,
                                  const bridge::VideoPanelFacts &facts,
                                  std::span<const std::pair<unsigned, unsigned>> modes = {}) {
  const auto screen = vista_settings_kind(frame.screen);
  std::vector<bridge::VideoModeSize> sizes;
  if (screen == 269) {
    sizes.reserve(modes.size());
    for (const auto &[width, height] : modes)
      sizes.push_back({width, height});
  }
  const auto panel = bridge::frontend_video_panel(
      screen, frame.context, facts, {sizes.data(), sizes.size()},
      minimum_field_of_view, maximum_field_of_view);
  const auto upscaler = Upscaler(facts.upscaler);
  return present_settings_panel(frame, content, panel, upscaler);
}
} // namespace
PcKeyboardSettings MenuState::pc_keyboard_settings() const {
  PcKeyboardSettings value;
  read_keyboard_options(*this, value);
  for (unsigned i = 0; i < pc_action_count; ++i) {
    value.bindings[i] = saved_binding(*this, i, false, value.bindings[i]);
    value.secondary[i] = saved_binding(*this, i, true, value.secondary[i]);
  }
  return value;
}
ControllerSettings MenuState::controller_settings() const {
  ControllerSettings v;v.sticks=std::clamp(profile_option(0x390,0),0,3);v.buttons=std::clamp(profile_option(0x391,0),0,3);
  v.sensitivity=std::clamp(profile_option(0x392,3),1,10);v.invert=profile_option(0x393,0);v.auto_center=profile_option(0x394,0);v.vibration=profile_option(0x395,1);return v;
}
void MenuState::adopt_controller_settings(const ControllerSettings &v) {
  auto &p=profile_values_[profile_name()];p[0x390]=v.sticks;p[0x391]=v.buttons;p[0x392]=v.sensitivity;p[0x393]=v.invert;p[0x394]=v.auto_center;p[0x395]=v.vibration;
}
void MenuState::adopt_pc_settings(const PcKeyboardSettings &keyboard,
    const DisplaySettings &display,std::array<unsigned,3> video) {
  const auto previous=pc_keyboard_settings();
  const bool bindings_changed=previous.bindings!=keyboard.bindings || previous.secondary!=keyboard.secondary;
  auto &values=profile_values_[profile_name()];
  values[0x380]=keyboard.sensitivity;values[0x381]=keyboard.invert;values[0x382]=keyboard.auto_center;values[0x383]=keyboard.dual_wield_invert;
  for(unsigned i=0;i<pc_action_count;++i) {
    values[0x400+i]=keyboard.bindings[i];values[0x440+i]=keyboard.secondary[i];
  }
  if(bindings_changed) {
    values[0x384]=4;
    for(unsigned preset=0;preset<3;++preset) {
      bool matches=true;
      for(unsigned i=0;i<pc_action_count;++i)
        matches &= keyboard.bindings[i]==vista_keyboard_presets[preset][i][0] &&
                   keyboard.secondary[i]==vista_keyboard_presets[preset][i][1];
      if(matches){values[0x384]=preset;break;}
    }
  }
  pc_display_=display;saved_pc_video_=video;
  pc_width_=video[0];pc_height_=video[1];pc_fullscreen_mode_=FullscreenMode(video[2]);
}
void MenuState::set_pc_display_adapter(
    std::function<std::string(const DisplaySettings &)> adapter) {
  pc_display_adapter_ = std::move(adapter);
  const auto error = pc_display_adapter_(pc_display_);
  if (!error.empty()) unavailable(error);
}
void MenuState::set_pc_video_modes(
    std::vector<std::pair<unsigned, unsigned>> modes, unsigned width,
    unsigned height, FullscreenMode fullscreen) {
  std::erase_if(modes, [](auto v) {
    return !v.first || !v.second || v.first > 16384 || v.second > 16384;
  });
  if (width && height)
    modes.emplace_back(width, height);
  std::sort(modes.begin(), modes.end());
  modes.erase(std::unique(modes.begin(), modes.end()), modes.end());
  pc_video_modes_ = std::move(modes);
  pc_width_ = width;
  pc_height_ = height;
  pc_fullscreen_mode_ = fullscreen;
  if (saved_pc_video_ && pc_video_adapter_) {
    const auto v = *saved_pc_video_;
    if (pc_video_adapter_(v[0], v[1], FullscreenMode(v[2])).empty()) {
      pc_width_ = v[0];
      pc_height_ = v[1];
      pc_fullscreen_mode_ = FullscreenMode(v[2]);
      if(pc_video_query_) {
        const auto actual=pc_video_query_();
        pc_width_=actual[0];pc_height_=actual[1];pc_fullscreen_mode_=FullscreenMode(actual[2]);
      }
    }
  }
}
MenuFrame MenuState::build_pc(unsigned id, unsigned context) const {
  if (!content_.vista)
    throw FormatError("PC settings require their loaded mainmenu content");
  const auto &s = content_.screen(id);
  const auto local = vista_settings_kind(id);
  MenuFrame f;
  f.screen = id;
  f.context = context;
  f.instance = pc_instance.fetch_add(1);
  f.pane_created_clock = ui_clock_;
  auto add = [&](unsigned low, int target = -2, int value = 0) {
    f.entries.push_back({low_string(*content_.vista, s, low), target, value});
  };
  switch (local) {
  case 7: case 8: break;
  case 18:
    if(context==team_game_pause_context) {
      f.title="CHANGE TEAM";
      if(game_pause_teams_)for(unsigned team=0;team<8;++team)if(game_pause_teams_->available&(1u<<team)) {
        if(team==game_pause_teams_->current)f.selected=f.entries.size();
        f.entries.push_back({game_pause_teams_->names[team],-115,int(team)});
      }
      break;
    }
    add(0x144,-100);
    if(context==campaign_game_pause_context) {add(0x2ea,-101);add(0x2eb,-102);}
    add(0x2ef,vista_menu(259));
    if(context!=campaign_game_pause_context && game_pause_teams_)f.entries.push_back({"Change Team",-114,0});
    // The native runtime has no durable campaign-save owner.
    add(0x2ed,-103);
    if(context==online_game_pause_context)f.entries.back().label="Leave Match";
    if(context==splitscreen_game_pause_context) {
      f.entries.back()={"End Match",-116,0};
      f.entries.push_back({"Exit To Desktop",-117,0});
    }
    break;
  case 6:
    //Actual40A781 row formatter, six datums from40B5A8/60D1FD. The
    //otherwise-present SPLIT SCREEN string is not a Vista main-menu row.
    add(0x10b,-2,0);add(0x284,-2,1);add(0x285,-2,2);
    add(0x286,vista_menu(19),3);add(0x986,-2,4);add(0x2ed,-2,5);
    break;
  case 182:
    //63941D installs a task callback; there is no selectable/cancel row.
    break;
  case 19: case 259: case 200: case 40: case 41: case 42: case 43: case 44: case 183:
  case 257: case 263: case 267: case 268: case 242: case 296: case 297:
  case 258: case 243: case 253: case 294: case 245: case 295: {
    const auto panel = bridge::frontend_pc_settings_panel(local, context, settings_facts(*this, local, context),
        {pc_action_labels.data(), pc_action_labels.size()});
    f.selected = present_settings_panel(f, content_, panel, pc_display_.upscaler);
    break;
  }
  case 256: case 269: case 271: case 272: case 273:
  case 274: case 276: case 285: case 290: case 293:
    f.selected = build_video_presentation(f, content_,
        video_facts(pc_display_, pc_width_, pc_height_, pc_fullscreen_mode_), pc_video_modes_);
    break;
  default:
    throw FormatError("unimplemented source Vista settings route");
  }
  return f;
}
std::string MenuState::pc_value_label(const MenuFrame &f, unsigned index,
                                      unsigned column) const {
  if (index >= f.entries.size())
    return {};
  if (is_vista_settings(f.screen, 256) || is_vista_settings(f.screen, 274)) {
    const auto facts = video_facts(pc_display_, pc_width_, pc_height_, pc_fullscreen_mode_);
    const auto text = bridge::frontend_video_value(vista_settings_kind(f.screen), f.context, index, facts);
    return settings_text(text, content_, f.screen, pc_display_.upscaler);
  }
  const auto screen = vista_settings_kind(f.screen);
  if (!is_vista_menu(f.screen) || (screen != 200 && screen != 253 && screen != 257 && screen != 258))
    return {};
  const int action = f.entries[index].value;
  const int capturing_slot = binding_action_ == unsigned(action) ? int(binding_slot_ != 0) : -1;
  const auto text = bridge::frontend_pc_settings_value(screen, index, column, action, capturing_slot,
      settings_facts(*this, screen, f.context, index, action));
  return settings_text(text, content_, f.screen, pc_display_.upscaler);
}
void MenuState::bind_pc_input(int binding) {
  if (!binding_action_ || !valid_pc_binding(binding))
    return;
  const auto action = PcAction(*binding_action_);
  if (binding == pc_wheel_up &&
      (action == PcAction::Left || action == PcAction::Right ||
       action == PcAction::Forward || action == PcAction::Back ||
       action == PcAction::Crouch || action == PcAction::Scoreboard)) {
    binding_action_.reset();
    unavailable("This action requires a key or mouse button that can be held.");
    return;
  }
  // Host bindings remain explicit per action. Do not silently erase another
  // action when editing one binding.
  profile_values_[profile_name()]
                 [(binding_slot_ ? 0x440 : 0x400) + *binding_action_] = binding;
  if(action==PcAction::Fire)
    profile_values_[profile_name()][(binding_slot_?0x440:0x400)+unsigned(PcAction::DualPrimary)]=binding;
  profile_values_[profile_name()][0x384]=4;
  binding_action_.reset();
  binding_slot_ = 0;
  save();
  ++revision;
}
void MenuState::accept_pc() {
  if (binding_action_ || frame().entries.empty())
    return;
  const auto f = frame();
  const auto e = f.entries.at(f.selected);
  const auto id = vista_settings_kind(f.screen);
  const auto refresh_video_summary = [&] {
    if (stack_.empty())
      return;
    auto &parent = stack_.back();
    const bool overview = is_vista_settings(parent.screen, 256);
    const bool scaling = id == 274 && is_vista_settings(parent.screen, 274) &&
        (parent.context == 0 || parent.context == video_path_tracing_context);
    if (!overview && !scaling)
      return;
    build_video_presentation(parent, content_,
        video_facts(pc_display_, pc_width_, pc_height_, pc_fullscreen_mode_));
    if (overview) {
      const auto count = unsigned(parent.entries.size());
      const auto visible = content_.screen(parent.screen).panes.front().lists.front().visible;
      parent.selected = std::min(parent.selected, count - 1);
      // Restoring defaults can remove the Path Tracing Quality row.
      parent.first_visible = std::min(parent.first_visible, count - std::min(count, visible));
    }
  };
  if(f.context==1 && (id==40 || id==41 || id==42 || id==43 || id==44 || id==183)) {
    const unsigned key=id==41?0x390:id==40?0x391:id==42?0x392:id==43?0x393:id==44?0x394:0x395;
    profile_values_[profile_name()][key]=e.value;save();back();return;
  }
  if(id==6) {
    if(!vista_mainmenu_row_enabled(unsigned(e.value))) {
      unavailable(e.value==1?"Vista LIVE is not yet supported.":"Vista Network is not yet supported.");
      return;
    }
    if(e.value==2) {
      unavailable("Vista Network is not yet supported.");
      return;
    }
    if(e.value!=3) {
      unavailable(e.value==0?"Vista Campaign is not yet supported.":
          e.value==1?"Vista LIVE is not yet supported.":
          e.value==4?"The Vista Guide is not available.":
          "Vista Quit confirmation is not yet supported.");
      return;
    }
    //40B32B constructs40B071/40ADD8, whose literal source screen is19.
  }
  if (e.destination >= 0) {
    if(id==256 && e.destination==int(vista_menu(269)) && e.value==1 && pc_display_.upscaler!=Upscaler::Off) {
      // This resolution is owned by the active upscaler. Lead to the control
      // that actually changes it rather than accepting a dormant manual size.
      open(vista_menu(274),2);
      return;
    }
    open(unsigned(e.destination),unsigned(e.value));
    return;
  }
  if (id == 253) {
    if (e.value == -2)
      return;
    if (e.value < 0) {
      unavailable("This action has no native keyboard input consumer yet.");
      return;
    }
    binding_action_ = unsigned(e.value);
    ++revision;
    return;
  }
  if (id==297 || id==296) { back(); return; }
  if(id==256) {
    auto value=pc_display_;
    if(e.value==0)value.crt_filter=!value.crt_filter;
    else if(e.value==1)value.two_player_split=value.two_player_split==TwoPlayerSplit::Horizontal?
        TwoPlayerSplit::Vertical:TwoPlayerSplit::Horizontal;
    if(!pc_display_adapter_) { unavailable("A live native renderer is required to apply this setting."); return; }
    if(const auto error=pc_display_adapter_(value);!error.empty()) {
      unavailable(error);return;
    }
    pc_display_=value;
    refresh_video_summary();
    save();++revision;return;
  }
  if (id==272 || id==273 || id==274 || id==290 || id==285 ||
      (id==271 && f.context==video_graphics_mode_context) ||
      (id==269 && f.context==1) || id==263 || id==242 || id==267 ||
      ((id==276 || id==268) && e.value==0)) {
    auto v=pc_display_;
    if(id==271)v.graphics_mode=pstencil::GraphicsMode(e.value);
    if(id==272) {
      if(f.context==video_fov_context)v.field_of_view=e.value;
      else if(f.context==video_vehicle_fov_context)v.vehicle_field_of_view=e.value;
      else v.brightness=e.value;
    }
    if(id==273)v.gamma=e.value;
    if(id==274) {
      if(f.context==video_path_tracing_samples_context)v.path_tracing_samples=e.value;
      else if(f.context==video_path_tracing_bounces_context)v.path_tracing_bounces=e.value;
      else if(f.context==1)v.upscaler=Upscaler(e.value);
      else v.upscaling_quality=UpscalingQuality(e.value);
    }
    if(id==290)v.detail=e.value;
    if(id==285)v.safe_area=e.value!=0;
    if(id==263)v.volume=e.value;
    if(id==242)v.subtitles=e.value;
    if(id==267)v.speakers=e.value;
    if(id==276){const DisplaySettings defaults;v.internal_width=v.internal_height=0;v.upscaler=defaults.upscaler;v.upscaling_quality=defaults.upscaling_quality;v.brightness=defaults.brightness;v.gamma=defaults.gamma;v.detail=0;v.safe_area=false;v.field_of_view=defaults.field_of_view;v.vehicle_field_of_view=defaults.vehicle_field_of_view;v.crt_filter=defaults.crt_filter;v.two_player_split=defaults.two_player_split;v.graphics_mode=defaults.graphics_mode;v.path_tracing_samples=defaults.path_tracing_samples;v.path_tracing_bounces=defaults.path_tracing_bounces;}
    if(id==268){v.volume=10;v.subtitles=0;v.speakers=4;}
    if(id==269) {
      const auto m=e.value<0?std::pair<unsigned,unsigned>{0,0}:pc_video_modes_.at(e.value);
      v.internal_width=m.first;v.internal_height=m.second;
    }
    if(!pc_display_adapter_) { unavailable("A live native renderer is required to apply this setting."); return; }
    const auto error=pc_display_adapter_(v);
    if(!error.empty()){unavailable(error);return;}
    pc_display_=v;save();back();refresh_video_summary();
    return;
  }
  if (id == 42 || id == 294 || id==44 || id==245) {
    profile_values_[profile_name()][id == 42 ? 0x380 : id==294?0x381:id==44?0x382:0x383] = e.value;
    save();
    back();
    return;
  }
  if (id == 295) {
    if (e.value == 0) {
      auto &values = profile_values_[profile_name()];
      values.erase(0x380);
      values.erase(0x381);values.erase(0x382);values.erase(0x383);values.erase(0x384);
      for (unsigned i = 0; i < pc_action_count; ++i) {
        values.erase(0x400 + i);
        values.erase(0x440 + i);
      }
      save();
    }
    back();
    return;
  }
  if (id == 293) {
    previous_pc_video_.reset();
    saved_pc_video_ = std::array<unsigned, 3>{pc_width_, pc_height_,
                                              unsigned(pc_fullscreen_mode_)};
    save();
    stack_.pop_back();
    ++revision;
    return;
  }
  if (id == 269 || id == 271) {
    auto w = pc_width_, h = pc_height_;
    auto full = pc_fullscreen_mode_;
    if (id == 269) {
      const auto m = pc_video_modes_.at(e.value);
      w = m.first;
      h = m.second;
    } else
      full = FullscreenMode(e.value);
    if (!pc_video_adapter_) {
      unavailable("A live desktop window is required to apply video settings.");
      return;
    }
    const auto error = pc_video_adapter_(w, h, full);
    if (!error.empty()) {
      unavailable(error);
      return;
    }
    previous_pc_video_ = std::array<unsigned, 3>{pc_width_, pc_height_,
                                                 unsigned(pc_fullscreen_mode_)};
    pc_width_ = w;
    pc_height_ = h;
    pc_fullscreen_mode_ = full;
    if(pc_video_query_) {
      const auto actual=pc_video_query_();
      pc_width_=actual[0];pc_height_=actual[1];pc_fullscreen_mode_=FullscreenMode(actual[2]);
    }
    back();
    video_preview_clock_ = ui_clock_;
    open(vista_menu(293));
    ui_clock(ui_clock_);
    return;
  }
  if ((id == 276 || id==268) && e.value == 1) {
    back();
    return;
  }
  if (id == 243) {
    auto &values=profile_values_[profile_name()];
    for(unsigned i=0;i<pc_action_count;++i){values[0x400+i]=vista_keyboard_presets[e.value][i][0];values[0x440+i]=vista_keyboard_presets[e.value][i][1];}
    values[0x384]=e.value;save();back();
  }
  else
    unavailable(
        "This Vista setting is present in the reference menu, but its engine "
        "effect is not implemented by the native renderer/input adapter.");
}
void MenuState::revert_pc_video() {
  if (!previous_pc_video_)
    return;
  const auto v = *previous_pc_video_;
  const auto error = pc_video_adapter_
                         ? pc_video_adapter_(v[0], v[1], FullscreenMode(v[2]))
                         : "Missing native video adapter";
  if (!error.empty()) {
    unavailable("Could not restore the previous display mode: " + error);
    return;
  }
  pc_width_ = v[0];
  pc_height_ = v[1];
  pc_fullscreen_mode_ = FullscreenMode(v[2]);
  previous_pc_video_.reset();
  if (frame().screen == vista_menu(293))
    stack_.pop_back();
  ++revision;
}
void MenuState::ui_clock(std::uint32_t value) {
  ui_clock_ = value;
  if (!previous_pc_video_ || frame().screen != vista_menu(293))
    return;
  // PC 64B9F0: (start-now+15000)/1000, signed division toward zero, +1.
  const auto elapsed = std::uint32_t(value - video_preview_clock_);
  const auto remaining =
      elapsed >= 16000 ? 0 : (15000 - int(elapsed)) / 1000 + 1;
  if (!remaining) {
    revert_pc_video();
    return;
  }
  auto message = low_string(*content_.vista, content_.screen(vista_menu(293)), 0x974);
  const std::string marker = "\xee\x92\x92"; // source U+E492
  if (const auto at = message.find(marker); at != std::string::npos)
    message.replace(at, marker.size(), std::to_string(remaining));
  stack_.back().description = std::move(message);
}
} // namespace halo2
