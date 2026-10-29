#include "helpers/gameplay_fixture.hpp"
#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void hold(Uint32 window,bool down,bool dual=false){
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.keysym.scancode=dual?SDL_SCANCODE_C:SDL_SCANCODE_E;e.key.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue actual interact key");
}
}
int main(int argc,char **argv){try{
  check(argc==5 || argc==6,"Supply MAP SHARED XBE WEAPON_NAME|control|keypad [CAPTURE]");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
  SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.gameplay_xbe=argv[3];options.width=1280;options.height=800;options.local_players=1;
  options.starting_weapon=5;options.mute=true;options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(std::getenv("HALO2_TEST_PROMPT_4K")){options.width=3840;options.height=2160;}
  if(const auto *artwork=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))options.ui_assets=artwork;
  const std::string scenario=argv[4];
  const bool keypad=scenario=="keypad" || scenario=="control";
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;const auto slot=world.local_slots.at(0);
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(options.width,options.height,false);
  const bool virtual_pad=std::getenv("HALO2_TEST_PROMPT_GAMEPAD");
  const bool dual=std::getenv("HALO2_TEST_PROMPT_DUAL");
  check(!dual || !keypad,"Controls have no secondary pickup action");
  SDL_JoystickID pad_instance=-1;int pad_index=-1;
  SDL_Joystick *virtual_joystick=nullptr;
  if(virtual_pad){
    pad_index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    check(pad_index>=0 && SDL_IsGameController(pad_index),"Create mapped SDL virtual controller");
    pad_instance=SDL_JoystickGetDeviceInstanceID(pad_index);
    options.input_instances={pad_instance};
    virtual_joystick=SDL_JoystickOpen(pad_index);check(virtual_joystick,"Open actual virtual controller state");
  }
  const auto press=[&](bool down) {
    if(virtual_joystick)check(SDL_JoystickSetVirtualButton(virtual_joystick,
        dual?SDL_CONTROLLER_BUTTON_Y:SDL_CONTROLLER_BUTTON_X,down)==0,"Set actual virtual controller button state");
    else hold(SDL_GetWindowID(window),down,dual);
  };
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);const auto window_id=SDL_GetWindowID(window);
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  std::optional<unsigned> placed,seen;std::uint32_t target=null_datum;bool used=false;
  float control_initial{};bool control_one_shot{};
  std::uint32_t initial_primary=null_datum,last_prompt=null_datum;
  options.frame_observer=[&](const MultiplayerFrameTiming &){
    if(!placed && SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
  };
  options.input_observer=[&](unsigned tick,std::span<const Controls>,const MultiplayerState &state){
    const auto &p=state.players[slot];last_prompt=p.action_prompt.target.object;
    if(!placed && tick>=30 && SDL_GetKeyboardFocus()==window){
      Vec3 position{};float yaw=0,pitch=0;
      if(keypad){
        for(const auto &[handle,owner]:world.match.simulation().devices().owners()){
          const auto &d=*world.content.devices[owner.placement];if(!d.control || !d.control->action_string || d.control_nodes.empty())continue;
          const auto nodes=source_control_world_nodes(d,owner);const auto &node=nodes.at(0);
          const auto center=source_shadow_sphere(d.control_bounds,node,owner.root.scale).center;
          const auto markers=pose_model_markers(d.control_markers,nodes,d.control_permutations,{},1);
          const auto forward=markers.empty()?node.basis[0]:markers[0].world.basis[0];
          position=center+forward*.6f;
          // Search below the actual target height. Starting two units above
          // Relic's switch incorrectly selects a surface above the target.
          const auto hit=world.match.simulation().collision().raycast(position,{0,0,-1},5);
          check(bool(hit),"No actual floor in front of authored control");position.z=hit->position.z+.01f;
          const auto ray=center-ordinary_unit_eye(position,world.content.player.camera_height,world.content.player.crouch_camera_height,0);
          yaw=std::atan2(ray.y,ray.x);pitch=std::atan2(ray.z,std::hypot(ray.x,ray.y));target=handle;break;
        }
      }else{
        for(const auto &item:state.world_weapons)if(world.content.weapons.at(item.definition).name.ends_with("\\"+scenario)){
          position=item.position;target=item.handle;break;
        }
      }
      check(target!=null_datum,"Actual action prompt target missing");
      if(keypad){
        const auto &owner=*world.match.simulation().devices().find(target);
        const auto &group=world.match.simulation().devices().position_group(owner);
        control_initial=group.value;control_one_shot=group.flags&1;
      }
      Spawn at;at.position=position;at.bsp=0;at.facing=yaw;
      test::place_player(world.match,slot,at);
      SDL_SetRelativeMouseMode(SDL_TRUE);SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
      const auto &settings=options.desktop->keyboard_settings;const float gain=.0025f*settings.sensitivity/3.f;
      look.motion.xrel=int(std::lround(std::remainder(p.yaw-yaw,6.283185307f)/gain));
      look.motion.yrel=int(std::lround((p.pitch-pitch)/gain*(settings.invert?-1.f:1.f)));
      check(SDL_PushEvent(&look)==1,"Queue actual keypad look");placed=tick;
      std::cout<<"Placed at "<<position.x<<','<<position.y<<','<<position.z<<" yaw="<<yaw<<" pitch="<<pitch<<'\n';
    }
    if(placed && p.action_prompt.target.object==target && !seen){seen=tick;initial_primary=p.source_weapon;std::cout<<"Idle prompt target="<<target<<" tick="<<tick<<'\n';}
    if(virtual_pad && seen && tick==*seen+3){
      SDL_Event button{};button.type=SDL_CONTROLLERBUTTONDOWN;button.cbutton.which=pad_instance;
      button.cbutton.button=SDL_CONTROLLER_BUTTON_Y;button.cbutton.state=SDL_PRESSED;
      check(SDL_PushEvent(&button)==1,"Select controller action labels after keyboard/mouse");
    }
    if(argc==5 && seen && tick==*seen+5)press(true);
    const auto control=keypad && seen?world.match.simulation().devices().find(target):nullptr;
    const bool activated=control && world.match.simulation().devices().position_group(*control).value!=control_initial;
    if(argc==5 && seen && (keypad?activated:dual?p.secondary_weapon.datum==target:p.source_weapon==target)){
      used=true;press(false);
      if(dual)check(p.source_weapon==initial_primary,"Displayed dual-wield button switched the primary weapon");
      if(!keypad || control_one_shot)check(p.action_prompt.target.object!=target,"Used target retained prompt");
      SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Finish action prompt runtime probe");
    }
    check(tick<150,"Live action prompt/interaction timed out");
    if(argc==6 && tick+1==options.capture_tick)check(seen && target!=null_datum &&
        p.action_prompt.target.object==target,"Idle capture lost target prompt");
  };
  if(argc==6){options.screenshot=argv[5];options.capture_tick=60;}
  check(run_multiplayer(options)==0 && seen.has_value(),"Runtime did not display an idle action context");
  if(argc==6)check(last_prompt==target,"Captured frame did not retain the idle target prompt");
  if(virtual_joystick)SDL_JoystickClose(virtual_joystick);
  if(pad_index>=0)SDL_JoystickDetachVirtual(pad_index);
  if(argc==5)check(used,"Actual SDL hold did not perform the displayed action");
  std::cout<<"PASS live "<<scenario<<(argc==6?" idle prompt capture":" SDL hold, action and prompt suppression")<<'\n';
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
