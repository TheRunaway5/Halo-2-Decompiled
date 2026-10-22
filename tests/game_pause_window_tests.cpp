#include "halo2/game_pause_menu.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
struct ValidationLog {
  std::ostringstream messages;
  std::streambuf *previous=std::cerr.rdbuf(messages.rdbuf());
  ~ValidationLog(){std::cerr.rdbuf(previous);std::cerr<<messages.str();}
};
}
int main(int argc,char **argv)try {
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
