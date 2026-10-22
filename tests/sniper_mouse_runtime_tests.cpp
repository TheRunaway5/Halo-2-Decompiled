#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_fov.hpp"
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *reason){if(!value)throw FormatError(reason);}
void push(SDL_Event event){check(SDL_PushEvent(&event)==1,"Could not queue sniper mouse stimulus");}
void scope(Uint32 window) {
  SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.windowID=window;
  event.button.button=SDL_BUTTON_RIGHT;event.button.state=SDL_PRESSED;push(event);
  event.type=SDL_MOUSEBUTTONUP;event.button.state=SDL_RELEASED;push(event);
}
}
int main(int argc,char **argv)try {
  check(argc==5,"Prepared MCC MAPS WEAPON FOV WIDTH required");
  const auto maps=std::filesystem::path(argv[1]);
  if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(maps/"lockout.map"))return 77;
  std::cout<<"Sniper mouse: loading "<<argv[2]<<" FOV="<<argv[3]<<std::endl;
  Cache cache(maps/"lockout.map",maps);
  check(cache.is_mcc(),"Live sniper regression requires prepared MCC content");
  const auto content=cache.load_content(0,true);
  std::cout<<"Sniper mouse: authored content loaded"<<std::endl;
  const auto name=std::string(argv[2]);
  check(name=="sniper_rifle" || name=="beam_rifle","Unknown sniper rifle");
  unsigned starting_weapon{};
  for(unsigned option=3;option<=20;++option) {
    const auto found=content.weapons.find(cache.slayer_starting_weapon(option));
    if(found!=content.weapons.end() && found->second.name.ends_with("\\"+name)) {
      check(found->second.zoom.levels==2,"Actual sniper lost its two authored zoom levels");
      starting_weapon=option;break;
    }
  }
  check(starting_weapon!=0,"Authored sniper starting equipment is unavailable");
  // Xvfb owns DISPLAY for CTest, even when the desktop also exports Wayland.
#if defined(__linux__)
  SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER,"x11",SDL_HINT_OVERRIDE);
#endif
  // Physical devices must not add unrelated host movement to this SDL replay.
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions options;
  options.map=maps/"lockout.map";options.shared_maps=maps;options.local_players=1;
  options.starting_weapon=starting_weapon;options.source_object_world=true;
  options.width=std::stoul(argv[4]);options.height=480;options.frames=65;options.mute=true;
  options.reference_tick_cadence=true;options.reference_render_fraction=1.f;options.input_instances={-1};
  options.desktop=std::make_shared<DesktopSession>();
  options.desktop->display_settings.field_of_view=std::stoul(argv[3]);
  auto *window=options.desktop->window(options.width,options.height,false);
  const auto window_id=SDL_GetWindowID(window);
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Actual sniper mouse capture failed");
  struct Expected {float yaw,pitch,delta;};
  std::optional<Expected> pending;
  Controls delivered;unsigned step{},motions{},narrow{},transitions{},responses{};
  std::array<bool,3> selectors{};std::optional<Vec3> previous_forward;
  float last_ratio=1.f;
  options.input_observer=[&](std::uint32_t tick,std::span<const Controls> input,const MultiplayerState &state) {
    check(input.size()==1,"Sniper mouse command went to another view");
    step=tick;
    if(pending) {
      // Camera inspection round-trips through float degrees; allow a few
      // ULPs in the cumulative raw angles while rejecting unscaled input.
      check(std::abs(input[0].yaw-(pending->yaw-pending->delta))<1e-6f &&
            std::abs(input[0].pitch-(pending->pitch+pending->delta))<1e-6f,
            "Live scoped SDL mouse command did not follow the submitted camera lens");
      pending.reset();++motions;
    }
    delivered=input[0];
    const auto &player=state.players[0];
    check(player.source_unit!=null_datum && !player.dead,"Actual sniper source unit is unavailable");
    if(tick==0 || tick==29 || tick==41 || tick==53 || tick==64)
      std::cout<<"Sniper mouse tick="<<tick<<" selector="<<int(player.source_zoom.current)<<std::endl;
    if(tick==38)check(player.source_zoom.current==0,"First RMB did not admit original sniper zoom level0");
    if(tick==50)check(player.source_zoom.current==1,"Second RMB did not admit original sniper zoom level1");
    if(tick==62)check(player.source_zoom.current==-1,"Third RMB did not exit the original sniper scope");
    if(tick==29 || tick==41 || tick==53)scope(window_id);
  };
  options.view_observer=[&](unsigned view,const Camera &camera,float,std::int16_t selector) {
    check(view==0,"Sniper mouse created an extra view");
    selectors.at(unsigned(selector+1))=true;
    if(previous_forward && length(camera.forward-*previous_forward)>1e-7f)++responses;
    previous_forward=camera.forward;
    if(step<10)return;
    const float wide=gameplay_field_of_view(options.desktop->display_settings.field_of_view);
    const float base=gameplay_camera_angles(wide,wide,float(options.width)/options.height).projection_angle;
    const double current=double(camera.field_of_view)*.01745329238474369;
    const float ratio=float(std::min(1.0,std::tan(current*.5)/std::tan(double(base)*.5)));
    narrow+=ratio<.9f;transitions+=std::abs(ratio-last_ratio)>1e-5f;last_ratio=ratio;
    pending=Expected{delivered.yaw,delivered.pitch,.0025f*ratio};
    SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.windowID=window_id;
    event.motion.xrel=1;event.motion.yrel=-1;push(event);
  };
  std::cout<<"Sniper mouse: entering runtime"<<std::endl;
  check(run_multiplayer(options)==0,"Live sniper runtime failed");
  check(motions>=50 && narrow>=20 && transitions>=6 && responses>=50 &&
        selectors[0] && selectors[1] && selectors[2],
        "Incomplete live sniper fine-movement, scope-transition or source-camera response coverage");
  std::cout<<"PASS "<<name<<" FOV="<<argv[3]<<" width="<<argv[4]<<": "<<motions
      <<" actual SDL fine movements, "<<narrow<<" magnified frames, "<<transitions
      <<" lens changes, "<<responses<<" camera responses; original RMB0/1/out cycle\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
