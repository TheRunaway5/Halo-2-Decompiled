#define SDL_MAIN_HANDLED
#include "halo2/native_action_prompt_hud.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
}
int main(int argc,char **argv)try {
  check(argc==4,"Supply CAMPAIGN_MAP HUD_ASSETS CAPTURE");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
  SDL_SetMainReady();Cache cache(argv[1]);auto content=cache.load_content(0);
  const auto spawns=cache.spawns();check(!spawns.empty(),"Campaign has no actual player start");
  Simulation simulation(content,0,spawns.front());
  DesktopSession desktop;auto *window=desktop.window(1280,800,false);
  const auto window_id=SDL_GetWindowID(window);
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  MultiplayerKbm keyboard(window_id,desktop.keyboard_settings);
  const bool gamepad=std::getenv("HALO2_TEST_PROMPT_GAMEPAD");
  int pad_index=-1;SDL_Joystick *joystick=nullptr;SDL_GameController *controller=nullptr;
  if(gamepad) {
    pad_index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    check(pad_index>=0 && SDL_IsGameController(pad_index),"Create actual mapped SDL controller");
    joystick=SDL_JoystickOpen(pad_index);controller=SDL_GameControllerOpen(pad_index);
    check(joystick && controller,"Open actual SDL controller state");
  }
  std::uint32_t target=null_datum;float yaw{},pitch{};NativeWeaponPickup selected;
  const auto items=simulation.weapon_pickups();
  for(const auto &item:items) {
    if(!item.visible || !item.weapon || item.definition==simulation.player().weapon.definition)continue;
    auto position=item.position-Vec3{.25f,0,0};
    const auto floor=simulation.collision().raycast(position+Vec3{0,0,.1f},{0,0,-1},5);
    if(!floor)continue;position.z=floor->position.z+.01f;
    const auto ray=item.position-ordinary_unit_eye(position,content.player.camera_height,content.player.crouch_camera_height,0);
    yaw=std::atan2(ray.y,ray.x);pitch=std::atan2(ray.z,std::hypot(ray.x,ray.y));
    Spawn at;at.position=position;at.facing=yaw;at.bsp=0;simulation.debug_teleport(0,at);
    simulation.step({0,0,yaw,pitch});
    const auto prompt=simulation.action_prompt();
    if(prompt.target.object!=item.handle || !(prompt.target.index&3))continue;
    target=item.handle;selected=item;
    std::cout<<"Actual live campaign weapon "<<content.weapons.at(item.definition).name<<" handle="<<target
        <<" position="<<position.x<<','<<position.y<<','<<position.z<<'\n';break;
  }
  check(target!=null_datum,"No autoplaced usable campaign weapon with actual collision in BSP0");
  auto mesh=cache.load_bsp(0);auto model=cache.load_model(selected.definition);
  prepare_forward_materials(cache,model.mesh);
  const auto model_material_base=mesh.materials.size();
  for(auto &material:model.mesh.materials)mesh.materials.push_back(std::move(material));
  NativeActionPromptHud hud(cache,content.player.datum,cache.shared_directory()/"fonts",mesh,argv[2]);
  Renderer renderer(window,mesh,false);std::cout<<"Campaign GPU: "<<renderer.device_name()<<'\n';
  NativeInteractionHold pad_hold;bool captured{},activated{};unsigned post_use{};
  for(unsigned tick=0;tick<150;++tick) {
    SDL_Event event{};while(SDL_PollEvent(&event))keyboard.handle_event(event);
    SDL_GameControllerUpdate();const auto now=SDL_GetTicks();
    auto input=keyboard.consume(yaw,pitch,now).controls;
    if(controller) {
      const auto pad=controller_input(sample_controller(controller),desktop.controller_settings);
      pad_hold.set(pad.pickup,now);const auto held=pad_hold.sample(now);
      input.pickup=held.first;input.pickup_milliseconds=held.second;pad_hold.consume();
    }
    simulation.step(input);
    const auto prompt=simulation.action_prompt();
    const auto frame=hud.frame(prompt,desktop.keyboard_settings,desktop.controller_settings,gamepad,1280,800);
    const bool taken=simulation.player().weapon.source_datum==target;
    if(!activated && !taken)check(prompt.target.object==target && !frame.text.empty() && !frame.vertices.empty(),"Live campaign lost its idle pickup prompt");
    Camera camera;camera.position=ordinary_unit_eye(simulation.player().body.position,content.player.camera_height,
        content.player.crouch_camera_height,simulation.player().crouch);camera.yaw=yaw;camera.pitch=pitch;
    Transform root;root.position=selected.position;root.basis={selected.forward,cross(selected.up,selected.forward),selected.up};
    auto geometry=taken?std::vector<Vertex>{}:pose_model_world(model,root);
    for(auto &vertex:geometry)vertex.material+=model_material_base;
    const bool capture=tick==15;
    if(renderer.draw(camera,capture?argv[3]:std::filesystem::path{},geometry,true,{},simulation.presentation().effects(),frame.vertices) && capture) {
      captured=true;std::cout<<"Captured authored campaign prompt: "<<frame.text<<'\n';
    }
    if(tick==20) {
      if(joystick)check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,1)==0,"Hold actual controller X");
      else {event={};event.type=SDL_KEYDOWN;event.key.windowID=window_id;event.key.keysym.scancode=SDL_SCANCODE_E;event.key.state=SDL_PRESSED;
        check(SDL_PushEvent(&event)==1,"Hold actual SDL E");}
    }
    if(taken) {
      activated=true;check(simulation.player().source_player_actions.flags&0x10u,"Actual held action was not consumed");
      check(prompt.target.object!=target,"Carried campaign weapon retained a world pickup prompt");
      if(++post_use==5)break;
    }
    SDL_Delay(34);
  }
  if(controller)SDL_GameControllerClose(controller);
  if(joystick)SDL_JoystickClose(joystick);
  if(pad_index>=0)SDL_JoystickDetachVirtual(pad_index);
  check(captured && activated,"Campaign SDL hold did not pick up the displayed actual weapon");
  std::cout<<"PASS actual campaign placement, collision, selected redraw prompt, SDL "<<(gamepad?"X":"E")<<" hold and actual item transfer\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
