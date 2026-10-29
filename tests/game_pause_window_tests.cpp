#include "halo2/game_pause_menu.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string_view>
using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
struct ValidationLog {
  std::ostringstream messages;
  std::streambuf *previous=std::cerr.rdbuf(messages.rdbuf());
  ~ValidationLog(){std::cerr.rdbuf(previous);std::cerr<<messages.str();}
};
int native_mcc_pause(const std::filesystem::path &maps,const std::filesystem::path &map,bool online=false) {
  check(Cache(map,maps).is_mcc(),"Native MCC pause requires its actual gameplay cache");
  DesktopSession desktop;auto *window=desktop.window(960,720,true);Mesh mesh;
  bool rejected{};
  try{GamePauseMenu missing(maps,{},mesh,false);}catch(const FormatError&){rejected=true;}
  check(rejected,"Native MCC pause guessed a gameplay widget owner");
  GamePauseMenu menu(maps,{},mesh,false,{},{},map);mesh.vertices.resize(3);
  menu.online_match(online);
  const bool validation=std::getenv("H2_TEST_VK_VALIDATION")&&std::string_view(std::getenv("H2_TEST_VK_VALIDATION"))=="1";
  Renderer renderer(window,mesh,validation);menu.bind(window,desktop,renderer);
  const auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=SDL_GetWindowID(window);e.key.keysym.sym=code;return menu.event(e);};
  const auto choose=[&](unsigned row) {
    check(row<menu.state().frame().entries.size(),"Native MCC settings row is missing");
    for(unsigned remaining=unsigned(menu.state().frame().entries.size());remaining&&menu.state().frame().selected!=row;--remaining)
      check(key(menu.state().frame().selected<row?SDLK_DOWN:SDLK_UP),"Native MCC menu lost keyboard navigation");
    check(menu.state().frame().selected==row,"Native MCC keyboard could not select its settings row");
    check(key(SDLK_RETURN),"Native MCC menu lost keyboard activation");
  };
  unsigned submitted{};
  const auto draw=[&]{const auto ui=menu.draw(960,720);check(!ui.vertices.empty(),"Native MCC pause emitted no authored widgets");
    const auto before=submitted;
    for(unsigned i=0;i<4;++i)submitted+=renderer.draw(Camera{}, {},{},false,{},{},ui.vertices,{},{},ui.backdrops);
    check(submitted>before,"Native MCC settings did not submit to the actual Vulkan window");
    check(menu.visible()&&menu.captures_game_input()&&!menu.pauses_game_time(),
        "Native MCC settings released gameplay input or paused multiplayer time");};
  SDL_SetRelativeMouseMode(SDL_TRUE);
  check(key(SDLK_ESCAPE)&&menu.visible()&&!menu.pauses_game_time(),"Native MCC Escape did not open the multiplayer overlay");draw();
  check(!SDL_GetRelativeMouseMode(),"Native MCC pause retained relative mouse capture");
  check(menu.state().frame().entries.at(2).label==(online?"Leave Match":"Quit"),
      "Native MCC pause did not select the correct session exit action");
  choose(1);draw();
  check(menu.state().frame().screen==vista_menu(259)&&menu.state().frame().entries.size()==4,
      "Native MCC Escape omitted Controller, Keyboard, Sound or Video settings");
  for(const auto &entry:menu.state().frame().entries)
    check(!entry.label.empty(),"Native MCC settings category has no localized label");
  choose(0);draw();
  check(menu.state().frame().screen==vista_menu(200)&&menu.state().frame().entries.size()==6,
      "Native MCC Controller settings are unreachable");
  choose(2);draw();choose(8);
  check(menu.state().frame().screen==vista_menu(200)&&desktop.controller_settings.sensitivity==9,
      "Native MCC Controller sensitivity did not reach the gameplay settings owner");
  key(SDLK_ESCAPE);
  choose(1);draw();
  check(menu.state().frame().screen==vista_menu(260)&&menu.state().frame().entries.size()==6,
      "Native MCC Keyboard settings are unreachable");
  key(SDLK_ESCAPE);
  choose(2);draw();
  check(menu.state().frame().screen==vista_menu(262)&&menu.state().frame().entries.size()==4,
      "Native MCC Sound settings are unreachable");
  choose(0);draw();choose(7);
  check(menu.state().frame().screen==vista_menu(262)&&desktop.display_settings.volume==7&&
      menu.state().pc_display_settings().volume==7,
      "Native MCC Sound volume did not reach the gameplay settings owner");
  key(SDLK_ESCAPE);
  choose(3);draw();
  check(menu.state().frame().screen==vista_menu(261)&&menu.state().frame().entries.size()>=14,
      "Native MCC Video settings are unreachable");
  choose(9);draw();
  check(menu.state().frame().context==video_fov_context,"Native MCC Video lost the on-foot FOV settings owner");
  choose(20);
  check(menu.state().frame().screen==vista_menu(261)&&desktop.display_settings.field_of_view==90&&
      menu.state().pc_display_settings().field_of_view==90&&desktop.display_settings.volume==7,
      "Native MCC Video FOV did not apply independently to the gameplay settings owner");
  key(SDLK_ESCAPE);key(SDLK_ESCAPE);
  check(menu.state().frame().entries.size()==3,"Native MCC Settings did not return to its multiplayer pause screen");
  choose(2);draw();
  check(menu.state().frame().selected==1&&!menu.quit_requested(),"Native MCC Quit confirmation did not default to Cancel");
  if(online)check(menu.state().frame().title=="Leave Match" &&
      menu.state().frame().entries.at(0).label=="Leave Match" &&
      menu.state().frame().description=="Leave the current online match?",
      "Native MCC Leave Match confirmation described a generic game quit");
  menu.state().back();check(key(SDLK_ESCAPE)&&!menu.visible(),"Native MCC pause did not resume");
  check(SDL_GetRelativeMouseMode(),"Native MCC Resume did not restore mouse capture");
  // The gameplay loop polls this after draining the dismissal event batch.
  // These direct synthetic events do not hold an SDL physical key down.
  check(!menu.captures_game_input(),"Released Native MCC dismissal retained the input barrier");
  const auto pad=[&](Uint8 button) {
    SDL_Event e{};e.type=SDL_CONTROLLERBUTTONDOWN;e.cbutton.which=37;e.cbutton.button=button;
    return menu.event(e);
  };
  check(!pad(SDL_CONTROLLER_BUTTON_BACK)&&!menu.visible(),"Native MCC Select/Back opened pause instead of remaining available to the scoreboard");
  check(pad(SDL_CONTROLLER_BUTTON_START)&&menu.visible(),"Native MCC controller Pause/Start did not open pause");
  SDL_Event escape{};escape.type=SDL_KEYDOWN;escape.key.windowID=SDL_GetWindowID(window);
  escape.key.keysym.sym=SDLK_ESCAPE;escape.key.repeat=1;
  check(menu.event(escape)&&menu.visible(),"Native MCC held Escape dismissed a controller-opened pause");
  escape.key.repeat=0;++escape.key.windowID;
  check(!menu.event(escape)&&menu.visible(),"Native MCC another window's Escape took over pause");
  pad(SDL_CONTROLLER_BUTTON_DPAD_DOWN);pad(SDL_CONTROLLER_BUTTON_A);
  check(menu.state().frame().screen==vista_menu(259),"Native MCC controller could not open Settings");
  check(key(SDLK_ESCAPE)&&menu.state().frame().screen==vista_menu(18)&&menu.visible(),
      "Native MCC Escape could not take over a controller-opened settings menu");
  key(SDLK_DOWN);
  check(menu.state().frame().selected==2,"Native MCC keyboard navigation retained the prior controller owner");
  check(key(SDLK_ESCAPE)&&!menu.visible(),"Native MCC Escape could not resume a controller-opened pause");
  check(!menu.captures_game_input(),"Native MCC keyboard takeover retained the dismissal barrier");
  check(key(SDLK_ESCAPE)&&menu.visible(),"Native MCC pause did not reopen");
  check(menu.state().frame().entries.at(2).label==(online?"Leave Match":"Quit"),
      "Native MCC reopening pause lost its session exit action");
  choose(2);choose(0);
  check(menu.quit_requested()&&!menu.visible()&&!SDL_GetRelativeMouseMode(),"Native MCC confirmed Quit did not release gameplay and the pointer");
  check(submitted>=4,"Native MCC pause did not submit real Vulkan frames");
  std::cout<<"PASS actual cache-owned MCC Escape -> Controller/Keyboard/Sound/Video settings, live preferences, controller-to-keyboard takeover, Resume and confirmed "<<(online?"Leave Match":"Quit")<<'\n';return 0;
}
}
int main(int argc,char **argv)try {
  if((argc==4||argc==5)&&std::string_view(argv[2])=="--native-mcc") {
    check(argc==4||std::string_view(argv[4])=="--online","Unknown native pause test mode");
    return native_mcc_pause(argv[1],argv[3],argc==5);
  }
  if(argc!=3)return 77;
  ValidationLog validation;
  {
  DesktopSession desktop;
  auto *window=desktop.window(960,720,false);
  Mesh mesh;
  GamePauseMenu solo(argv[1],argv[2],mesh,true),multiplayer(argv[1],argv[2],mesh,false);
  mesh.vertices.resize(3);
  Renderer renderer(window,mesh,true);
  solo.bind(window,desktop,renderer);multiplayer.bind(window,desktop,renderer);
  SDL_SetRelativeMouseMode(SDL_TRUE);
  const auto key=[&](GamePauseMenu &menu,SDL_Keycode key) {
    SDL_Event event{};event.type=SDL_KEYDOWN;event.key.windowID=SDL_GetWindowID(window);
    event.key.keysym.sym=key;return menu.event(event);
  };
  const auto draw=[&](GamePauseMenu &menu) {
    const auto ui=menu.draw(960,720);
    check(!ui.vertices.empty(),"Visible pause emitted no authored GUI");
    bool submitted=false;
    for(unsigned n=0;n<4 && !submitted;++n)
      submitted=renderer.draw(Camera{}, {},{},false,{},{},ui.vertices,{},{},ui.backdrops);
    check(submitted,"Pause GUI was not submitted to the actual Vulkan window");
  };
  check(key(solo,SDLK_ESCAPE) && solo.visible() && solo.pauses_game_time(),"Window Escape did not pause campaign");
  check(!SDL_GetRelativeMouseMode(),"Pause did not release the relative mouse");
  draw(solo);
  solo.state().select(3);solo.state().accept();draw(solo);
  check(solo.pauses_game_time(),"Settings unpaused campaign");
  solo.state().back();solo.state().select(4);solo.state().accept();draw(solo);
  check(solo.state().frame().selected==1 && solo.pauses_game_time(),"Quit confirmation lost Cancel or pause");
  solo.state().back();
  check(key(solo,SDLK_ESCAPE) && !solo.visible() && !solo.pauses_game_time(),"Window Escape did not resume campaign");
  check(SDL_GetRelativeMouseMode(),"Resume did not restore the relative mouse");
  SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=SDL_GetWindowID(window);motion.motion.xrel=100;
  check(solo.event(motion),"Window Resume leaked queued mouse motion");
  SDL_Event other=motion;other.motion.windowID+=1;
  check(!solo.event(other),"Pause swallowed another window's input");

  SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
  desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=SDL_CONTROLLER_AXIS_MAX;
  desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;desc.name="Pause release verification";
  const auto device=SDL_JoystickAttachVirtualEx(&desc);check(device>=0,SDL_GetError());
  auto *pad=SDL_GameControllerOpen(device);check(pad,SDL_GetError());
  auto *stick=SDL_GameControllerGetJoystick(pad);
  SDL_JoystickSetVirtualAxis(stick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768);
  SDL_JoystickSetVirtualAxis(stick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-32768);
  SDL_JoystickSetVirtualButton(stick,SDL_CONTROLLER_BUTTON_A,1);SDL_GameControllerUpdate();
  check(solo.captures_game_input(),"Held controller dismissal input escaped the release barrier");
  SDL_JoystickSetVirtualButton(stick,SDL_CONTROLLER_BUTTON_A,0);SDL_GameControllerUpdate();
  check(!solo.captures_game_input(),"Released controller input never returned to gameplay");
  check(!solo.event(motion),"Released Resume barrier still captured gameplay motion");

  SDL_Event start{};start.type=SDL_CONTROLLERBUTTONDOWN;
  start.cbutton.which=SDL_JoystickInstanceID(stick);start.cbutton.button=SDL_CONTROLLER_BUTTON_START;
  check(multiplayer.event(start) && multiplayer.visible() && !multiplayer.pauses_game_time(),
      "Controller Start did not open the TU1.5 multiplayer overlay policy");
  draw(multiplayer);
  check(multiplayer.event(start) && !multiplayer.visible(),"Controller Start did not resume multiplayer");
  SDL_GameControllerClose(pad);SDL_JoystickDetachVirtual(device);
  }
  check(validation.messages.str().find("VUID-")==std::string::npos &&
      validation.messages.str().find("Validation Error")==std::string::npos,
      "Pause window produced Vulkan validation errors");
  std::cout<<"PASS actual SDL/Vulkan campaign/MP pause, Settings, Cancel, Resume and held-controller input release\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
