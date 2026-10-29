#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/desktop_session.hpp"
#include <array>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
#if SDL_VERSION_ATLEAST(2,0,14)
struct VirtualPads {
  struct Pad {SDL_Joystick *joystick{};SDL_JoystickID instance=-1;};
  std::array<Pad,4> pads;
  VirtualPads() {
    check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,"Initialize SDL controller test owner");
  }
  void attach() {
    for(auto &pad:pads) {
      const auto index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
          SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
      check(index>=0,"Attach actual SDL virtual game controller");
      pad.joystick=SDL_JoystickOpen(index);check(pad.joystick,"Open virtual controller state");
      pad.instance=SDL_JoystickInstanceID(pad.joystick);
      char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(pad.joystick),guid,sizeof(guid));
      const auto mapping=std::string(guid)+",Scoreboard Back regression,back:b"+
          std::to_string(SDL_CONTROLLER_BUTTON_BACK)+
          ",leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
      check(SDL_GameControllerAddMapping(mapping.c_str())>=0 && SDL_IsGameController(index),
          "Map actual SDL Back input");
      for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;++axis)
        check(SDL_JoystickSetVirtualAxis(pad.joystick,axis,
            axis>=SDL_CONTROLLER_AXIS_TRIGGERLEFT?SDL_JOYSTICK_AXIS_MIN:0)==0,
            "Set neutral virtual controller axes");
    }
    SDL_GameControllerUpdate();
  }
  void back(unsigned view,bool held) {
    check(SDL_JoystickSetVirtualButton(pads.at(view).joystick,SDL_CONTROLLER_BUTTON_BACK,
        held?SDL_PRESSED:SDL_RELEASED)==0,"Deliver actual SDL Back hold/release");
    SDL_GameControllerUpdate();
  }
  ~VirtualPads() {
    for(auto &pad:pads) {
      if(pad.joystick)SDL_JoystickClose(pad.joystick);
      for(int index=0;index<SDL_NumJoysticks();++index)
        if(SDL_JoystickGetDeviceInstanceID(index)==pad.instance) {
          SDL_JoystickDetachVirtual(index);break;
        }
    }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  }
};
#endif
}
int main(int argc,char **argv)try {
#if !SDL_VERSION_ATLEAST(2,0,14)
  return 77;
#else
  check(argc==4,"Supply MAP SHARED_MAPS TU15_XBE|native-mcc");
  const bool native_mcc=std::string_view(argv[3])=="native-mcc";
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) ||
      (!native_mcc && !std::filesystem::exists(argv[3])))return 77;
  SDL_SetMainReady();
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  if(native_mcc)check(Cache(options.map,options.shared_maps).is_mcc(),"native-mcc requires a prepared imported MCC cache");
  else options.gameplay_xbe=argv[3];
  options.width=640;options.height=480;options.local_players=4;options.starting_weapon=5;options.mute=true;
  options.validation=std::getenv("HALO2_SCOREBOARD_VALIDATION");
  options.reference_tick_cadence=true;options.reference_render_fraction=1.f;
  options.frames=224;
  if(const auto *assets=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))options.ui_assets=assets;
  options.desktop=std::make_shared<DesktopSession>();
  options.desktop->window(options.width,options.height,false);
  VirtualPads pads;pads.attach();
  for(const auto &pad:pads.pads)options.input_instances.push_back(pad.instance);
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  unsigned tick{},held_mask{},restored_mask{},observations{};
  MultiplayerState current;
  const auto press_tick=[](unsigned view){return 8u+56u*view;};
  options.input_observer=[&](unsigned step,std::span<const Controls> controls,const MultiplayerState &state) {
    tick=step;current=state;
    check(controls.size()==4,"Controller scoreboard lost a local input binding");
    for(unsigned view=0;view<4;++view) {
      if(step==press_tick(view))pads.back(view,true);
      if(step==press_tick(view)+24)pads.back(view,false);
    }
  };
  options.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout &layout,
      const FullScoreboardPaintResult &paint) {
    ++observations;check(view<4,"Controller scoreboard emitted an unbound local view");
    for(unsigned target=0;target<4;++target) {
      const auto press=press_tick(target);
      if(tick>=press+13 && tick<=press+23) {
        if(view!=target) {
          check(normal && layout.rows.empty() && paint.draw.vertices.empty(),
              "Back scoreboard leaked into another local view");
          continue;
        }
        check(!normal && layout.status==FullScoreboardStatus::Ready && layout.rows.size()==4 &&
            paint.status==FullScoreboardPaintStatus::Ready && !paint.draw.vertices.empty() &&
            !paint.emblems_unavailable,"Actual Back hold did not submit the full source scoreboard");
        unsigned highlighted=0;
        for(const auto &row:layout.rows)if(row.current_player) {
          ++highlighted;
          check(row.datum==current.players[target].source_player,
              "Controller scoreboard highlighted a different local player");
        }
        check(highlighted==1,"Controller scoreboard lost its full-datum local highlight");
        held_mask|=1u<<view;
      }
    }
    if(tick>=press_tick(view)+40 && tick<=press_tick(view)+47) {
      check(normal && layout.rows.empty() && paint.draw.vertices.empty(),
          "Released controller Back did not restore the normal HUD");
      restored_mask|=1u<<view;
    }
  };
  check(run_multiplayer(options)==0 && held_mask==15 && restored_mask==15 && observations>100,
      "Actual controller Back hold/release was not observed on all four independent views");
  std::cout<<"PASS actual SDL Back hold/release on each of four independent local views, source roster/font/emblems, full-datum highlight and HUD restoration\n";
  return 0;
#endif
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
