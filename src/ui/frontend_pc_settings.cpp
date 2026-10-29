#include "halo2/frontend.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include <SDL.h>
#include <algorithm>
#include <atomic>

namespace halo2 {
namespace {
#include "ui/vista/vista_keyboard_presets.inc"
// PC 634CB6..634E0B/641333: original advanced editor action label order.
constexpr std::uint32_t editor_labels[]{
    0x2100082e, 0x1e000832, 0x1b000833, 0x1d000830, 0x1e000831, 0x16000834,
    0x18000835, 0x19000857, 0x1b000858, 0x1b000859, 0x1c00085a, 0x1f00082f,
    0x16000836, 0x2200085c, 0x1f000837, 0x2000083b, 0x1d00085e, 0x1f00083d,
    0x2100085f, 0x1f00083c, 0x1e000838, 0x1d00083e, 0x18000956, 0x20000855,
    0x1f000861, 0x1e000862, 0x17000864, 0x21000865, 0x1e000866, 0x21000867,
    0x2300097f, 0x1e000868, 0x1d000869, 0x20000856, 0x1e00086b, 0x1c00086c,
    0x1600086d, 0x19000962, 0x1a000963, 0x1d00086e, 0x1b00086f, 0x28000978,
    0x1e00097e};
std::string low_string(const FrontendContent &content, const UiScreen &screen, unsigned low) {
  return detail::frontend_pc_compiled_low_localized(content, screen, low);
}
std::atomic<std::uint64_t> pc_instance{1ull << 63};
std::string upscaling_description(Upscaler method) {
  return method==Upscaler::Off?
      "Upscaling is off. Choose a method to upscale the world. Menus and HUD stay at native resolution.":
      "Upscale the 3D world. Menus and HUD stay at native resolution.";
}
} // namespace
PcKeyboardSettings MenuState::pc_keyboard_settings() const {
  PcKeyboardSettings value;
  value.sensitivity = std::clamp(profile_option(0x380, 3), 1, 10);
  value.invert = profile_option(0x381, 0) != 0;
  value.auto_center=profile_option(0x382,0)!=0;value.dual_wield_invert=profile_option(0x383,0)!=0;
  for (unsigned i = 0; i < pc_action_count; ++i) {
    const auto a = profile_option(0x400 + i, value.bindings[i]);
    const auto b = profile_option(0x440 + i, value.secondary[i]);
    if (valid_pc_binding(a))
      value.bindings[i] = a;
    if (valid_pc_binding(b))
      value.secondary[i] = b;
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
    break;
  case 259:
    add(0x2ef,vista_menu(200));add(0x7c2,vista_menu(260));
    add(0x81f,vista_menu(262));add(0x820,vista_menu(261));
    break;
  case 200:
    add(0x3dd,vista_menu(41),1);add(0x3de,vista_menu(40),1);add(0x3df,vista_menu(42),1);
    add(0x3e0,vista_menu(43),1);add(0x3e1,vista_menu(44),1);add(0x3e2,vista_menu(183),1);
    break;
  case 40:
    add(1,-2,0);add(0x3e4,-2,1);add(0x3e5,-2,2);add(0x3e6,-2,3);
    f.selected=controller_settings().buttons;
    break;
  case 41:
    add(1,-2,0);add(0x2f5,-2,1);add(0x2f6,-2,2);add(0x2f7,-2,3);
    f.selected=controller_settings().sticks;
    break;
  case 43:
    add(0x308,-2,1);add(0x309,-2,0);
    f.selected=controller_settings().invert?0:1;
    break;
  case 183:
    add(0x182,-2,1);add(0x183,-2,0);
    f.selected=controller_settings().vibration?0:1;
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
  case 19:
    add(0x197, 193);
    add(0x198, 60, 1);
    add(0x820, vista_menu(256));
    add(0x81f, vista_menu(257));
    add(0x976, vista_menu(296));
    add(0x821, vista_menu(297));
    break;
  case 257:
    add(0x876, vista_menu(263));
    add(0x87a, vista_menu(267));
    add(0x2f1, vista_menu(242));
    add(0x88a, vista_menu(268));
    break;
  case 263:
    add(0x957, -2, 0);
    for (unsigned i=0;i<10;++i) add(0x880+i,-2,int(i+1));
    f.selected=pc_display_.volume;
    break;
  case 267:
    for (unsigned i=0;i<5;++i) add(0x87b+i,-2,int(i));
    f.selected=pc_display_.speakers;
    break;
  case 268:
    add(0x895,-2,0); add(0x896,-2,1); f.selected=1;
    break;
  case 242:
    add(1,-2,0); add(0x182,-2,1); add(0x183,-2,2);
    f.selected=pc_display_.subtitles;
    break;
  case 296:
    add(0x977,-2,0);
    f.entries.front().label="System Default";
    f.description="Native LAN uses the operating system's network routing.";
    break;
  case 297:
    f.title="ABOUT HALO 2 NATIVE";
    break;
  case 258:
    // PC 65975E table and 6598F5 dispatch (all six source rows).
    add(0x7c2, vista_menu(content_.gameplay_overlay?244:243));
    add(0x3df, vista_menu(42));
    add(0x3e0, vista_menu(294));
    add(0x3e1, vista_menu(44));
    add(0x7c7, vista_menu(245));
    add(0x848, vista_menu(295));
    break;
  case 243:
    // PC 641051/6410A5: four live rows; custom has source enum4.
    add(0x83f, -2, 0);
    add(0x840, -2, 1);
    add(0x841, -2, 2);
    add(0x843, vista_menu(content_.gameplay_overlay?254:253), 4);
    f.selected=unsigned(std::clamp(profile_option(0x384,4),0,4));if(f.selected==4)f.selected=3;
    break;
  case 253:
    f.description = "Enter: primary binding   X: secondary binding   "
                    "Backspace: clear   Esc: cancel";
    for (auto sid : editor_labels) {
      const auto at =
          std::find(pc_action_labels.begin(), pc_action_labels.end(), sid);
      f.entries.push_back({detail::frontend_pc_compiled_localized(*content_.vista, s, sid), -2,
                           sid == 0x2100082e || sid == 0x1f00082f ||
                                   sid == 0x20000855 || sid == 0x20000856
                               ? -2
                           : at == pc_action_labels.end()
                               ? -1
                               : int(at - pc_action_labels.begin())});
    }
    f.selected = 1;
    break;
  case 42:
    for (unsigned i = 0; i < 10; ++i)
      add(0x3cf + i, -2, i + 1);
    f.selected = unsigned((context==1?controller_settings().sensitivity:pc_keyboard_settings().sensitivity) - 1);
    break;
  case 294:
  case 44:
  case 245:
    add(0x309, -2, 0);
    add(0x308, -2, 1);
    if(local==44)f.selected=context==1?controller_settings().auto_center:pc_keyboard_settings().auto_center;
    if(local==245)f.selected=pc_keyboard_settings().dual_wield_invert;
    if (local == 294)
      f.selected = pc_keyboard_settings().invert;
    break;
  case 295:
    add(0x846, -2, 0);
    add(0x847, -2, 1);
    f.selected = 1;
    break;
  case 256:
    f.entries.push_back({"Graphics Mode",-2,2});
    // PC 6487D3 jump table: eight source rows, including LOD.
    f.entries.push_back({"Fullscreen Mode",int(vista_menu(271)),0});
    add(0x897, vista_menu(269));
    f.entries.push_back({"Internal Resolution",int(vista_menu(269)),1});
    add(0x89a, vista_menu(272));
    add(0x89c, vista_menu(273));
    f.entries.push_back({"Upscaling",int(vista_menu(274)),0});
    add(0x95e, vista_menu(290));
    add(0x907, vista_menu(285));
    f.entries.push_back({"Field of View",int(vista_menu(272)),int(video_fov_context)});
    f.entries.push_back({"Vehicle Field of View",int(vista_menu(272)),int(video_vehicle_fov_context)});
    f.entries.push_back({"CRT Filter",-2,0});
    f.entries.push_back({"2-Player Split Screen",-2,1});
    add(0x89f, vista_menu(276));
    if(pc_display_.graphics_mode==pstencil::GraphicsMode::PathTracing)
      f.entries.push_back({"Path Tracing Quality",int(vista_menu(274)),int(video_path_tracing_context)});
    break;
  case 269:
    if (context == 1) {
      f.title="INTERNAL RESOLUTION";
      f.entries.push_back({"Match Screen Resolution",-2,-1});
    }
    for (unsigned i = 0; i < pc_video_modes_.size(); ++i) {
      auto [w, h] = pc_video_modes_[i];
      f.entries.push_back(
          {std::to_string(w) + " x " + std::to_string(h), -2, int(i)});
      if (w == (context == 1 ? pc_display_.internal_width : pc_width_) &&
          h == (context == 1 ? pc_display_.internal_height : pc_height_))
        f.selected = i + (context == 1 ? 1 : 0);
    }
    break;
  case 271:
    f.title="FULLSCREEN MODE";
    f.entries.push_back({"Windowed",-2,int(FullscreenMode::Windowed)});
    f.entries.push_back({"Fullscreen",-2,int(FullscreenMode::Fullscreen)});
    f.entries.push_back({"Borderless Fullscreen",-2,int(FullscreenMode::BorderlessFullscreen)});
    f.selected=unsigned(pc_fullscreen_mode_);
    break;
  case 272:
    if(context==video_fov_context || context==video_vehicle_fov_context) {
      const bool vehicle=context==video_vehicle_fov_context;
      f.title=vehicle?"VEHICLE FIELD OF VIEW":"FIELD OF VIEW";
      f.description=vehicle?"70 degrees preserves the Xbox TU1.5 vehicle camera. Higher values widen its view.":"Widen the view while on foot.";
      for(unsigned degrees=minimum_field_of_view;degrees<=maximum_field_of_view;++degrees)
        f.entries.push_back({std::to_string(degrees)+" degrees",-2,int(degrees)});
      f.selected=(vehicle?pc_display_.vehicle_field_of_view:pc_display_.field_of_view)-minimum_field_of_view;
      break;
    }
    for (unsigned i = 0; i < 10; ++i)
      add(0x8ac + i, -2, i);
    f.selected = pc_display_.brightness;
    break;
  case 273:
    for (unsigned i = 0; i < 11; ++i)
      add(0x8b6 + i, -2, i);
    f.selected = pc_display_.gamma;
    break;
  case 274:
    if(context>=video_path_tracing_context) {
      f.title="PATH TRACING QUALITY";
      f.description="More ray samples and bounces increase GPU cost. Both default to 1.";
      if(context==video_path_tracing_context) {
        f.entries.push_back({"Ray Samples Per Pixel: "+std::to_string(pc_display_.path_tracing_samples),int(vista_menu(274)),int(video_path_tracing_samples_context)});
        f.entries.push_back({"Bounce Depth: "+std::to_string(pc_display_.path_tracing_bounces),int(vista_menu(274)),int(video_path_tracing_bounces_context)});
      } else {
        const bool samples=context==video_path_tracing_samples_context;
        f.title=samples?"RAY SAMPLES PER PIXEL":"BOUNCE DEPTH";
        f.entries.push_back({"1",-2,1});f.entries.push_back({"2",-2,2});
        f.selected=(samples?pc_display_.path_tracing_samples:pc_display_.path_tracing_bounces)-1;
      }
      break;
    }
    f.title="UPSCALING";
    if(context==0) {
      f.entries.push_back({std::string("Method: ")+upscaler_name(pc_display_.upscaler),int(vista_menu(274)),1});
      f.entries.push_back({std::string("Quality: ")+upscaling_quality_name(pc_display_.upscaling_quality,pc_display_.upscaler),int(vista_menu(274)),2});
      f.description=upscaling_description(pc_display_.upscaler);
    } else if(context==1) {
      f.title="UPSCALING METHOD";
      for(unsigned i=0;i<3;++i)f.entries.push_back({upscaler_name(Upscaler(i)),-2,int(i)});
      f.selected=unsigned(pc_display_.upscaler);
      f.description="FSR supports a wide range of graphics cards. DLSS requires an NVIDIA RTX graphics card.";
    } else {
      f.title="QUALITY MODE";
      for(unsigned i=0;i<6;++i)f.entries.push_back({upscaling_quality_name(UpscalingQuality(i),pc_display_.upscaler),-2,int(i)});
      f.selected=unsigned(pc_display_.upscaling_quality);
      f.description=pc_display_.upscaler==Upscaler::Off?
          "Choose a quality mode, then select an upscaling method to enable it.":
          "DLAA and Native AA render the world at full resolution. Other modes trade detail for performance.";
    }
    break;
  case 290:
    add(0x95f, -2, 0);
    add(0x960, -2, 1);
    add(0x961, -2, 2);
    f.selected=pc_display_.detail;
    break;
  case 285:
    add(0x183, -2, 0);
    add(0x182, -2, 1);
    f.selected=pc_display_.safe_area;
    break;
  case 293:
    f.entries.push_back({detail::frontend_pc_compiled_localized(*content_.vista, s, 0x02000f02), -2, 0});
    break;
  case 276:
    add(0x8a0, -2, 0);
    add(0x8a1, -2, 1);
    f.selected = 1;
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
  const auto k = pc_keyboard_settings();
  if (is_vista_settings(f.screen, 253)) {
    if (column == 3)
      return {};
    const auto a = f.entries[index].value;
    if (a == -2)
      return {};
    if (a < 0)
      return column == 1 ? "Unavailable" : std::string{};
    if (binding_action_ == unsigned(a) &&
        ((column == 2) == bool(binding_slot_)))
      return "Press a key or mouse button";
    return pc_binding_name(column == 2 ? k.secondary[a] : k.bindings[a]);
  }
  if(is_vista_settings(f.screen,200)) {
    const auto c=controller_settings();const auto &screen=content_.screen(f.screen);
    if(index==0)return low_string(*content_.vista, content_.screen(vista_menu(41)),c.sticks==0?1:0x2f4+c.sticks);
    if(index==1)return low_string(*content_.vista, content_.screen(vista_menu(40)),c.buttons==0?1:0x3e3+c.buttons);
    if(index==2)return std::to_string(c.sensitivity);
    if(index==3||index==4)return low_string(*content_.vista, screen,(index==3?c.invert:c.auto_center)?0x308:0x309);
    if(index==5)return low_string(*content_.vista, content_.screen(vista_menu(183)),c.vibration?0x182:0x183);
  }
  if (is_vista_settings(f.screen, 258)) {
    if (index == 0) {
      const PcKeyboardSettings defaults;
      if(k.bindings==defaults.bindings && k.secondary==defaults.secondary)return "MCC Default";
      return low_string(*content_.vista, content_.screen(content_.gameplay_overlay?vista_menu(244):f.screen), 0x844);
    }
    if (index == 1)
      return std::to_string(k.sensitivity);
    if (index == 2)
      return low_string(*content_.vista, content_.screen(f.screen), k.invert ? 0x308 : 0x309);
    if(index==3||index==4)return low_string(*content_.vista, content_.screen(f.screen),(index==3?k.auto_center:k.dual_wield_invert)?0x308:0x309);
  }
  if (is_vista_settings(f.screen, 256)) {
    if (index == 0)
      return pstencil::graphics_mode_name(pc_display_.graphics_mode);
    --index;
    if (index == 0)
      return fullscreen_mode_name(pc_fullscreen_mode_);
    if (index == 1)
      return std::to_string(pc_width_) + " x " + std::to_string(pc_height_);
    const auto &s=content_.screen(f.screen);
    if(index==2 && pc_display_.upscaler!=Upscaler::Off)return "Automatic";
    if(index==2)return pc_display_.internal_width ? std::to_string(pc_display_.internal_width)+" x "+std::to_string(pc_display_.internal_height) : "Match Screen";
    if(index==3)return low_string(*content_.vista, s,0x8ac+pc_display_.brightness);
    if(index==4)return low_string(*content_.vista, s,0x8b6+pc_display_.gamma);
    if(index==5)return upscaler_name(pc_display_.upscaler);
    if(index==6)return low_string(*content_.vista, s,0x95f+pc_display_.detail);
    if(index==7)return low_string(*content_.vista, content_.screen(vista_menu(285)),pc_display_.safe_area?0x182:0x183);
    if(index==8)return std::to_string(pc_display_.field_of_view);
    if(index==9)return std::to_string(pc_display_.vehicle_field_of_view);
    if(index==10)return pc_display_.crt_filter?"On":"Off";
    if(index==11)return pc_display_.two_player_split==TwoPlayerSplit::Horizontal?"Horizontal":"Vertical";
    if(index==13)return std::to_string(pc_display_.path_tracing_samples)+
        (pc_display_.path_tracing_samples==1?" sample, ":" samples, ")+
        std::to_string(pc_display_.path_tracing_bounces)+(pc_display_.path_tracing_bounces==1?" bounce":" bounces");
  }
  if(is_vista_settings(f.screen,274) && f.context==video_path_tracing_context) {
    if(index==0)return std::to_string(pc_display_.path_tracing_samples);
    if(index==1)return std::to_string(pc_display_.path_tracing_bounces);
  }
  if(is_vista_settings(f.screen,274) && f.context==0) {
    if(index==0)return upscaler_name(pc_display_.upscaler);
    if(index==1)return upscaling_quality_name(pc_display_.upscaling_quality,pc_display_.upscaler);
  }
  if(is_vista_settings(f.screen, 257)) {
    if(index==0)return pc_display_.volume ? std::to_string(pc_display_.volume) : low_string(*content_.vista, content_.screen(f.screen),0x957);
    if(index==1)return low_string(*content_.vista, content_.screen(f.screen),0x87b+pc_display_.speakers);
    if(index==2)return low_string(*content_.vista, content_.screen(f.screen),pc_display_.subtitles==0?1:pc_display_.subtitles==1?0x182:0x183);
  }
  return {};
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
  const auto update_path_tracing_entry=[&] {
    if(stack_.empty() || !is_vista_settings(stack_.back().screen,256))return;
    auto& frame=stack_.back();auto& entries=frame.entries;
    const auto entry=std::find_if(entries.begin(),entries.end(),[](const auto& row) {
      return row.destination==int(vista_menu(274)) && row.value==int(video_path_tracing_context);
    });
    if(pc_display_.graphics_mode==pstencil::GraphicsMode::PathTracing) {
      if(entry==entries.end())entries.push_back({"Path Tracing Quality",int(vista_menu(274)),int(video_path_tracing_context)});
    } else if(entry!=entries.end())entries.erase(entry);
    if(!entries.empty())frame.selected=std::min(frame.selected,unsigned(entries.size()-1));
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
    else if(e.value==2)value.graphics_mode=pstencil::GraphicsMode(
        (unsigned(value.graphics_mode)+1)%(unsigned(pstencil::GraphicsMode::PathTracing)+1));
    if(!pc_display_adapter_) { unavailable("A live native renderer is required to apply this setting."); return; }
    if(const auto error=pc_display_adapter_(value);!error.empty()) {
      // An unavailable optional mode must not trap this cyclic control on
      // pStencil. Original is the next supported choice; report the failed
      // Path Tracing request after applying that fallback transactionally.
      if(e.value==2 && value.graphics_mode==pstencil::GraphicsMode::PathTracing) {
        value.graphics_mode=pstencil::GraphicsMode::Original;
        if(pc_display_adapter_(value).empty()){pc_display_=value;save();++revision;}
      }
      unavailable(error);return;
    }
    pc_display_=value;
    update_path_tracing_entry();
    save();++revision;return;
  }
  if (id==272 || id==273 || id==274 || id==290 || id==285 ||
      (id==269 && f.context==1) || id==263 || id==242 || id==267 ||
      ((id==276 || id==268) && e.value==0)) {
    auto v=pc_display_;
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
    pc_display_=v;save();back();update_path_tracing_entry();
    if(id==274 && is_vista_settings(frame().screen,274) && frame().context==0) {
      stack_.back().entries.at(0).label=std::string("Method: ")+upscaler_name(pc_display_.upscaler);
      stack_.back().entries.at(1).label=std::string("Quality: ")+upscaling_quality_name(pc_display_.upscaling_quality,pc_display_.upscaler);
      stack_.back().description=upscaling_description(pc_display_.upscaler);
    } else if(id==274 && is_vista_settings(frame().screen,274) && frame().context==video_path_tracing_context) {
      stack_.back().entries.at(0).label="Ray Samples Per Pixel: "+std::to_string(pc_display_.path_tracing_samples);
      stack_.back().entries.at(1).label="Bounce Depth: "+std::to_string(pc_display_.path_tracing_bounces);
    }
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
