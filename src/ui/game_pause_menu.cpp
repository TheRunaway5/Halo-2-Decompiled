#include "halo2/game_pause_menu.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include "halo2/audio.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <fstream>

namespace halo2 {
UiStrings load_campaign_pause_objectives(const Cache &cache,const std::filesystem::path &map) {
  // Original23223D reads primary scnr+3BC. Locale offsets belong to this map's
  // matg English table, not to the donor pause cache or the main menu.
  const auto tag=[&](const char *group) -> const Tag & {
    if(const auto* t=cache.query_tag({.group=TagGroup::from_string(group), .require_address=true}))return *t;
    throw FormatError("Campaign objective source owner missing");
  };
  const auto &scenario=tag("scnr");
  const auto source=cache.load_source_tag_metadata(scenario.datum);
  const auto datum=Bytes(source.bytes_at(scenario.address,scenario.size)).u32(0x3bc);
  if(datum==null_datum)return {};
  const auto owner=cache.load_source_tag_metadata(datum);
  const auto strings=Bytes(owner.bytes_at(owner.source_address(),owner.size()));
  const auto &globals=tag("matg");
  const auto g=cache.load_source_tag_metadata(globals.datum);
  const auto b=Bytes(g.bytes_at(globals.address,globals.size));
  const unsigned first=strings.u16(16),count=strings.u16(18),total=b.u32(0x190),size=b.u32(0x194);
  if(total>65536 || size>16*1024*1024 || first>total || count>total-first)
    throw FormatError("Campaign objective locale exceeds its source table");
  if(!count)return {};
  std::vector<std::byte> indices,text;
  if(cache.is_mcc()) {
    // Version13 locale offsets address decoded resources. Preserve the
    // encoded cache selector through the ordinary resource owner instead of
    // seeking these logical offsets in the compressed physical map file.
    const auto all=cache.load_source_runtime_resource(b.u32(0x198),total*8);
    const auto selected=std::span<const std::byte>(all).subspan(first*8,count*8);
    indices.assign(selected.begin(),selected.end());
    text=cache.load_source_runtime_resource(b.u32(0x19c),size);
  } else {
    std::ifstream file(map,std::ios::binary|std::ios::ate);const auto length=file.tellg();
    const auto read=[&](std::uint32_t at,std::size_t n) {
      if(length<0 || at>std::uint64_t(length) || n>std::uint64_t(length)-at)
        throw FormatError("Campaign objective locale is outside the selected cache");
      std::vector<std::byte> data(n);file.seekg(at);
      if(!file.read(reinterpret_cast<char *>(data.data()),n))throw FormatError("Incomplete campaign objective locale");
      return data;
    };
    indices=read(b.u32(0x198)+first*8,count*8);text=read(b.u32(0x19c),size);
  }
  UiStrings result;const Bytes rows(indices),locale(text);
  for(unsigned i=0;i<count;++i) {
    const auto at=rows.u32(i*8+4);if(at>=size)throw FormatError("Invalid campaign objective locale offset");
    result.emplace(rows.u32(i*8),locale.string(at,size-at));
  }
  return result;
}
GamePauseMenu::GamePauseMenu(const std::filesystem::path &maps,
    const std::filesystem::path &hud_assets,Mesh &mesh,bool campaign,
    const std::filesystem::path &preferences,const std::filesystem::path &menu_assets):campaign_(campaign) {
  const auto assets=hud_menu_asset_directory(hud_assets);
  content_=Cache(maps/"mainmenu.map").load_frontend();
  content_.gameplay_overlay=true;
  // This supplied solo cache retains the shared gameplay pause/settings bank
  // and source file selectors for shared art. Mainmenu has no pause widgets.
  auto pause_content=load_vista_pause(assets/"maps/01a_tutorial.map");
  // This owner remains the original Vista gameplay widget bank. Only its
  // bitmap samples and font rasters come from the separately selected pack.
  pause_content->gameplay_overlay=true;
  content_.vista=std::move(pause_content);
  state_=std::make_unique<MenuState>(content_,preferences);
  state_->set_game_pause_action([this](GamePauseAction action) {
    if(action==GamePauseAction::Quit)quit_=true;
    else if(action!=GamePauseAction::Resume) {
      if(!command_)return std::string("The current game cannot perform this action.");
      if(auto error=command_(action);!error.empty())return error;
    }
    visible_=false;
    await_release_=true;
    if(previous_cursor_)SDL_SetCursor(previous_cursor_);
    // Quit leaves gameplay for the menu. Re-arming relative mode here (while
    // the activating button is still held) re-hides and re-confines the
    // pointer just before the runtime releases it, which can leave Xwayland's
    // hidden-cursor pointer lock behind. Only actions that resume play recapture.
    if(window_ && action!=GamePauseAction::Quit)SDL_SetRelativeMouseMode(SDL_TRUE);
    return std::string{};
  });
  painter_=std::make_unique<FrontendPainter>(content_,maps/"fonts",mesh,
      std::filesystem::path{},load_frontend_ui_assets(content_,menu_assets),
      menu_assets.empty()?std::filesystem::path{}:menu_assets/"fonts");
}
bool GamePauseMenu::captures_game_input() {
  if(visible_)return true;
  if(!await_release_)return false;
  bool held=SDL_GetMouseState(nullptr,nullptr)!=0;
  int count{};const auto *keys=SDL_GetKeyboardState(&count);
  for(int i=0;i<count;++i)held|=keys[i]!=0;
  // The A/Start press that dismissed the GUI must be released before pad
  // polling can turn it into Jump or a gameplay trigger in the same frame.
  for(int i=0;i<SDL_NumJoysticks();++i) {
    if(auto *pad=SDL_GameControllerFromInstanceID(SDL_JoystickGetDeviceInstanceID(i))) {
      for(int button=0;button<SDL_CONTROLLER_BUTTON_MAX;++button)
        held|=SDL_GameControllerGetButton(pad,SDL_GameControllerButton(button))!=0;
      held|=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERLEFT)>6553 ||
          SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>6553;
    }
  }
  if(!held)await_release_=false;
  return await_release_;
}
GamePauseMenu::~GamePauseMenu() {
  if(previous_cursor_)SDL_SetCursor(previous_cursor_);
  if(hand_cursor_)SDL_FreeCursor(hand_cursor_);
}
void GamePauseMenu::audio(AudioEngine *value) {audio_layout_=value?std::function<std::string(unsigned)>([value](unsigned layout){return value->speaker_layout(layout);}):std::function<std::string(unsigned)>{};}
void GamePauseMenu::bind(SDL_Window *window,DesktopSession &desktop,Renderer &renderer) {
  window_=window;desktop_=&desktop;renderer_=&renderer;
  if(!hand_cursor_) {previous_cursor_=SDL_GetCursor();hand_cursor_=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);}
  int w{},h{};SDL_GetWindowSize(window_,&w,&h);
  state_->adopt_controller_settings(desktop.controller_settings);
  auto initial_display=desktop.display_settings;
  if(!desktop.upscaling_preferences_initialized) {
    initial_display.upscaler=state_->pc_display_settings().upscaler;
    initial_display.upscaling_quality=state_->pc_display_settings().upscaling_quality;
  }
  desktop.upscaling_preferences_initialized=true;
  state_->adopt_pc_settings(desktop.keyboard_settings,initial_display,
      {unsigned(w),unsigned(h),unsigned(desktop_->fullscreen_mode())});
  state_->set_pc_display_adapter([this](const DisplaySettings &settings) {
    const auto error=renderer_->validate_display_settings(settings);
    if(error.empty()) {
      if(audio_layout_)if(auto failure=audio_layout_(settings.speakers);!failure.empty())return failure;
      desktop_->display_settings=settings;
    }
    return error;
  });
  state_->set_pc_video_adapter([this](unsigned w,unsigned h,FullscreenMode fullscreen) {
    auto candidate=desktop_->display_settings;
    if(!candidate.internal_width){candidate.internal_width=w;candidate.internal_height=h;}
    if(const auto error=renderer_->validate_display_settings(candidate);!error.empty())return error;
    const auto error=desktop_->apply_video(w,h,fullscreen);
    if(error.empty())renderer_->resized();
    return error;
  });
  state_->set_pc_video_query([this] {
    int w{},h{};SDL_GetWindowSize(window_,&w,&h);
    return std::array<unsigned,3>{unsigned(w),unsigned(h),unsigned(desktop_->fullscreen_mode())};
  });
  std::vector<std::pair<unsigned,unsigned>> modes;
  const auto display=SDL_GetWindowDisplayIndex(window_);
  for(int i=0;i<SDL_GetNumDisplayModes(display);++i) {
    SDL_DisplayMode mode{};
    if(!SDL_GetDisplayMode(display,i,&mode) && mode.w>0 && mode.h>0)modes.emplace_back(mode.w,mode.h);
  }
  state_->set_pc_video_modes(std::move(modes),w,h,desktop_->fullscreen_mode());
}
void GamePauseMenu::show() {
  if(desktop_) {
    state_->adopt_controller_settings(desktop_->controller_settings);
    int width{},height{};SDL_GetWindowSize(window_,&width,&height);
  state_->adopt_pc_settings(desktop_->keyboard_settings,desktop_->display_settings,
      {unsigned(width),unsigned(height),unsigned(desktop_->fullscreen_mode())});
  }
  state_->ui_clock(SDL_GetTicks());state_->open_game_pause(campaign_);
  visible_=true;entered_=SDL_GetTicks();screen_=state_->frame().screen;
  if(window_)SDL_SetRelativeMouseMode(SDL_FALSE);
  refresh();
}
void GamePauseMenu::refresh() {
  if(!visible_ || !window_)return;
  int w{},h{};SDL_Vulkan_GetDrawableSize(window_,&w,&h);
  draw(std::max(1,w),std::max(1,h));
}
std::optional<FrontendPointerTarget> GamePauseMenu::pointer(int x,int y) const {
  if(!window_)return {};
  int w{},h{};SDL_GetWindowSize(window_,&w,&h);
  const float scale=std::min(float(w)/640,float(h)/480);
  if(scale<=0)return {};
  return painter_->pointer_hit((x-(w-640*scale)*.5f)/scale,(y-(h-480*scale)*.5f)/scale);
}
bool GamePauseMenu::event(const SDL_Event &e) {
  if(window_) {
    const auto id=SDL_GetWindowID(window_);
    if((e.type==SDL_KEYDOWN || e.type==SDL_KEYUP) && e.key.windowID!=id)return false;
    if((e.type==SDL_MOUSEBUTTONDOWN || e.type==SDL_MOUSEBUTTONUP) && e.button.windowID!=id)return false;
    if(e.type==SDL_MOUSEMOTION && e.motion.windowID!=id)return false;
    if(e.type==SDL_MOUSEWHEEL && e.wheel.windowID!=id)return false;
  }
  const bool game_input=e.type==SDL_KEYDOWN || e.type==SDL_KEYUP ||
      e.type==SDL_MOUSEBUTTONDOWN || e.type==SDL_MOUSEBUTTONUP ||
      e.type==SDL_MOUSEMOTION || e.type==SDL_MOUSEWHEEL ||
      e.type==SDL_CONTROLLERBUTTONDOWN || e.type==SDL_CONTROLLERBUTTONUP ||
      e.type==SDL_CONTROLLERAXISMOTION;
  if(!visible_) {
    if(e.type==SDL_KEYDOWN || e.type==SDL_MOUSEBUTTONDOWN)painter_->prompt_device(PromptDevice::KeyboardMouse);
    else if(e.type==SDL_CONTROLLERBUTTONDOWN)painter_->prompt_device(controller_prompt_device(SDL_GameControllerFromInstanceID(e.cbutton.which)));
  }
  // Both gameplay loops drain events before polling captures_game_input().
  // Keep the rest of a dismissal's event batch here, including releases and
  // relative motion. Only the subsequent released-device poll lifts this gate.
  if(!visible_ && await_release_)return game_input;
  if(e.type==SDL_CONTROLLERDEVICEREMOVED && e.cdevice.which==input_instance_) {
    input_instance_=-1;painter_->prompt_device(PromptDevice::KeyboardMouse);
  }
  if(visible_) {
    if((e.type==SDL_CONTROLLERBUTTONDOWN || e.type==SDL_CONTROLLERBUTTONUP || e.type==SDL_CONTROLLERAXISMOTION) &&
        (e.type==SDL_CONTROLLERAXISMOTION?e.caxis.which:e.cbutton.which)!=input_instance_)return game_input;
    if(input_instance_>=0 && (e.type==SDL_KEYDOWN || e.type==SDL_KEYUP || e.type==SDL_MOUSEBUTTONDOWN ||
        e.type==SDL_MOUSEBUTTONUP || e.type==SDL_MOUSEMOTION || e.type==SDL_MOUSEWHEEL))return game_input;
  }
  state_->ui_clock(SDL_GetTicks());
  if(party_notice_ && party_notice_action_ && drawn_notice_) {
    const auto current=party_notice_();
    if(current && current->id==drawn_notice_->id) {
      const bool dismiss=(e.type==SDL_KEYDOWN && !e.key.repeat && e.key.keysym.sym==SDLK_F10) ||
        (visible_ && e.type==SDL_CONTROLLERBUTTONDOWN && e.cbutton.button==SDL_CONTROLLER_BUTTON_LEFTSTICK);
      if(dismiss && party_notice_action_(false)){drawn_notice_.reset();return true;}
      if(visible_ && e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT)
        if(const auto target=pointer(e.button.x,e.button.y);target && target->action==FrontendPointerAction::InviteDismiss && party_notice_action_(false)) {
          drawn_notice_.reset();return true;
        }
    }
  }
  if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE) {
    if(!visible_) {input_instance_=-1;painter_->prompt_device(PromptDevice::KeyboardMouse);}
    if(!e.key.repeat) {if(visible_)state_->back();else show();}
    refresh();return true;
  }
  if(e.type==SDL_CONTROLLERBUTTONDOWN && e.cbutton.button==SDL_CONTROLLER_BUTTON_START) {
    if(!visible_) {
      input_instance_=e.cbutton.which;
      painter_->prompt_device(controller_prompt_device(SDL_GameControllerFromInstanceID(input_instance_)));
    }
    if(visible_) {
      // Unwind through each owner so an unconfirmed video preview is reverted.
      for(unsigned i=0;visible_ && i<16;++i)state_->back();
    } else show();
    refresh();return true;
  }
  if(!visible_)return false;
  refresh();
  if(e.type==SDL_KEYDOWN && !e.key.repeat) {
    if(state_->capturing_binding())state_->bind_pc_input(e.key.keysym.scancode);
    else switch(e.key.keysym.sym) {
    case SDLK_UP:state_->move(-1);break;
    case SDLK_DOWN:state_->move(1);break;
    case SDLK_LEFT:state_->navigate(-1,0);break;
    case SDLK_RIGHT:state_->navigate(1,0);break;
    case SDLK_RETURN:case SDLK_KP_ENTER:state_->accept();break;
    case SDLK_x:state_->auxiliary();break;
    case SDLK_BACKSPACE:
      state_->activate_pointer({FrontendPointerAction::ClearBinding,0,state_->revision});break;
    default:break;
    }
  } else if(e.type==SDL_CONTROLLERBUTTONDOWN) {
    switch(e.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:state_->accept();break;
    case SDL_CONTROLLER_BUTTON_B:state_->back();break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:state_->move(-1);break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:state_->move(1);break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:state_->navigate(-1,0);break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:state_->navigate(1,0);break;
    case SDL_CONTROLLER_BUTTON_X:state_->auxiliary();break;
    default:break;
    }
  } else if(e.type==SDL_MOUSEBUTTONDOWN) {
    if(state_->capturing_binding())state_->bind_pc_input(pc_mouse_button(e.button.button));
    else if(e.button.button==SDL_BUTTON_LEFT) {
      if(auto target=pointer(e.button.x,e.button.y))state_->activate_pointer(*target);
    } else if(e.button.button==SDL_BUTTON_RIGHT)state_->back();
  } else if(e.type==SDL_MOUSEMOTION) {
    const auto target=pointer(e.motion.x,e.motion.y);
    if(hand_cursor_)SDL_SetCursor(target?hand_cursor_:previous_cursor_);
    if(target && target->action==FrontendPointerAction::Entry)state_->select(target->index);
  } else if(e.type==SDL_MOUSEWHEEL) {
    if(state_->capturing_binding() && e.wheel.y!=0)state_->bind_pc_input(pc_wheel_up);
    else state_->mouse_wheel(e.wheel.y);
  } else if(e.type==SDL_CONTROLLERAXISMOTION && !state_->capturing_binding()) {
    const auto now=SDL_GetTicks();
    if(std::abs(int(e.caxis.value))<12000)axis_repeat_=0;
    else if(!axis_repeat_ || now-axis_repeat_>=180) {
      const int direction=e.caxis.value<0?-1:1;
      if(e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY)state_->move(direction);
      else if(e.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX)state_->navigate(direction,0);
      axis_repeat_=now?now:1;
    }
  }
  if(desktop_) {desktop_->keyboard_settings=state_->pc_keyboard_settings();desktop_->controller_settings=state_->controller_settings();}
  refresh();
  // All gameplay input stays isolated, including key/button releases and
  // motion. Window resize/close/hotplug events still reach the application.
  return game_input;
}
UiDraw GamePauseMenu::draw(unsigned width,unsigned height) {
  const auto now=SDL_GetTicks();state_->ui_clock(now);
  if(screen_!=state_->frame().screen) {screen_=state_->frame().screen;entered_=now;}
  auto result=visible_?painter_->draw(*state_,float(now-entered_)/1000,-1,now,{width,height}):UiDraw{};
  drawn_notice_=party_notice_?party_notice_():std::nullopt;
  if(drawn_notice_)result=painter_->draw_invite_notice(*drawn_notice_,{width,height},visible_);
  return result;
}
} // namespace halo2
