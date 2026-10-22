#include "helpers/gameplay_fixture.hpp"
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
  std::uint32_t target=null_datum;float initial{},yaw{},pitch{};bool one_shot{};
  for(const auto &[handle,owner]:simulation.devices().owners()) {
    const auto &definition=*content.devices.at(owner.placement);
    if(owner.bsp_hidden || !definition.control || !definition.control->action_string || definition.control_nodes.empty())continue;
    const auto nodes=source_control_world_nodes(definition,owner);
    const auto sphere=source_shadow_sphere(definition.control_bounds,nodes[0],owner.root.scale);
    const auto markers=pose_model_markers(definition.control_markers,nodes,definition.control_permutations,{},1);
    const auto forward=markers.empty()?nodes[0].basis[0]:markers[0].world.basis[0];
    auto position=sphere.center+forward*.6f;
    const auto floor=simulation.collision().raycast(position,{0,0,-1},5);
    if(!floor)continue;
    position.z=floor->position.z+.01f;
    const auto ray=sphere.center-ordinary_unit_eye(position,content.player.camera_height,content.player.crouch_camera_height,0);
    yaw=std::atan2(ray.y,ray.x);pitch=std::atan2(ray.z,std::hypot(ray.x,ray.y));
    Spawn at;at.position=position;at.facing=yaw;at.bsp=0;test::place_player(simulation,at);
    // Diagnostic initial placement only. All subsequent targeting, input,
    // collision and group state come from the actual campaign simulation.
    simulation.step({0,0,yaw,pitch});
    if(simulation.control_action_prompt().target.object!=handle)continue;
    const auto &group=simulation.devices().position_group(owner);
    const auto &control=*definition.control;
    const float requested=control.type==0?(group.value>.5f?0.f:1.f):control.type==1?1.f:
        control.type==2?0.f:std::clamp(control.call_value,0.f,1.f);
    if(requested==group.value)continue;
    target=handle;initial=group.value;one_shot=group.flags&1;
    std::cout<<"Actual live campaign control "<<definition.control->datum<<" handle="<<handle
        <<" placement="<<owner.placement<<" position="<<position.x<<','<<position.y<<','<<position.z<<'\n';break;
  }
  check(target!=null_datum,"No autoplaced usable campaign control with a changing action in BSP 0");
  auto mesh=cache.load_bsp(0);const auto &owner=*simulation.devices().find(target);
  const auto &placement=content.objects.at(owner.placement);
  auto model=cache.load_model(placement.definition,true,placement.variant);
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
    const auto prompt=simulation.control_action_prompt();
    const auto frame=hud.frame(prompt,desktop.keyboard_settings,desktop.controller_settings,gamepad,1280,800);
    const auto &group=simulation.devices().position_group(*simulation.devices().find(target));
    if(!activated && group.value==initial)check(prompt.target.object==target && !frame.text.empty() && !frame.vertices.empty(),"Live campaign lost its idle prompt");
    Camera camera;camera.position=ordinary_unit_eye(simulation.player().body.position,content.player.camera_height,
        content.player.crouch_camera_height,simulation.player().crouch);camera.yaw=yaw;camera.pitch=pitch;
    auto geometry=pose_model_world(model,simulation.devices().find(target)->root);
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
    if(group.value!=initial) {
      activated=true;check(simulation.player().source_player_actions.flags&0x10u,"Actual held action was not consumed");
      if(one_shot)check(prompt.target.object!=target,"Used campaign control retained its prompt");
      if(++post_use==5)break;
    }
    SDL_Delay(34);
  }
  if(controller)SDL_GameControllerClose(controller);
  if(joystick)SDL_JoystickClose(joystick);
  if(pad_index>=0)SDL_JoystickDetachVirtual(pad_index);
  check(captured && activated,"Campaign SDL hold did not activate the displayed control");
  std::cout<<"PASS actual campaign placement, collision, original Vista prompt, SDL "<<(gamepad?"X":"E")<<" hold and device-group activation\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
