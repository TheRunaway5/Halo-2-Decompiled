#include "helpers/gameplay_fixture.hpp"
#define SDL_MAIN_HANDLED
#include "halo2/native_unit_interaction.hpp"
#include "halo2/native_action_prompt_hud.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/renderer.hpp"
#include "halo2/appearance.hpp"
#include "halo2/math.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {void check(bool v,const char *why){if(!v)throw FormatError(why);}}
int main(int argc,char **argv)try {
 check(argc==5,"Supply MAP HUD_ASSETS trade|vehicle|turret|boarding|flip CAPTURE");
 if(!std::getenv("DISPLAY"))return 77;SDL_SetMainReady();
 Cache cache(argv[1]);const std::string mode=argv[3];const bool trade=mode=="trade",flip=mode=="flip",board=mode=="boarding";
 auto content=cache.load_content(0,!trade);const auto spawns=cache.spawns();check(!spawns.empty(),"No actual player start");
 std::unique_ptr<Simulation> campaign;std::unique_ptr<MultiplayerMatch> match;
 if(trade)campaign=std::make_unique<Simulation>(content,0,spawns.front());
 else {SlayerRules rules;rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
  match=std::make_unique<MultiplayerMatch>(content,0,spawns,rules);match->configure_weapon_pickups();match->join("Prompt probe");if(board)match->join("Occupied seat");}
 auto &sim=trade?*campaign:const_cast<Simulation&>(match->simulation());
 for(unsigned i=0;i<30&&!trade;++i){std::array<Controls,multiplayer_capacity> c{};match->step(c);}
 std::uint32_t target=null_datum;float yaw{},pitch{};Vec3 position;NativeActionPrompt selected;
 std::uint32_t original_player=null_datum,original_ally=null_datum;std::int16_t expected_seat=-1;
 if(trade) {
  for(const auto &s:content.squads) {
   if(s.team!=1&&s.team!=2)continue;if(!sim.place_squad(s.name))continue;
   if(sim.player().weapon.definition==null_datum)for(const auto &item:sim.weapon_pickups())if(item.weapon){sim.equip(item.definition);break;}
   for(const auto &ally:sim.actors()) {
    if(!sim.allied(sim.player().team,ally.team)||ally.weapon.definition==null_datum||ally.weapon.definition==sim.player().weapon.definition)continue;
    for(float distance:{.4f,.6f,.8f}) {
     Spawn at;at.position=ally.body.position+direction(ally.yaw)*distance;at.facing=ally.yaw+3.141592654f;test::place_player(sim,at);
     const auto p=sim.action_prompt();if(p.target.action!=2||p.target.object!=ally.source_datum)continue;
     target=ally.source_datum;selected=p;position=at.position;yaw=at.facing;original_player=sim.player().weapon.source_datum;original_ally=ally.weapon.source_datum;break;
    }if(target!=null_datum)break;
   }if(target!=null_datum)break;
  }
 } else {
  std::vector<std::uint32_t> vehicles;for(const auto &o:sim.scene().objects())if(o.handle!=null_datum&&o.visible&&content.objects[o.placement].type==1)vehicles.push_back(o.handle);
  for(const auto handle:vehicles) {
   auto &o=const_cast<SceneObjects&>(sim.scene()).get(handle);const auto &d=content.objects[o.placement];const auto name=cache.query_tag({.handle=TagHandle{d.definition}})->name;
   if((board||flip)&&!name.ends_with("\\ghost"))continue;if(mode=="turret"&&name.find("turrets")==std::string::npos)continue;
   if(board) {auto &enemy=const_cast<Unit&>(match->unit(1));enemy.source_parent=handle;enemy.source_seat=0;enemy.seat_action.phase=Unit::SeatAction::Seated;}
   if(flip){const auto b=authored_basis(d.rotation);o.solved_transform=Transform{o.scale,{b.forward,b.left*-1.f,b.up*-1.f},o.position};}
   for(float radius:{.2f,.5f,.8f,1.1f,1.5f,2.f,2.5f}) {
    for(unsigned angle=0;angle<48;++angle) {
     const float a=float(angle)*6.2831853f/48;Spawn at;at.position=o.position+direction(a)*radius;at.facing=a+3.141592654f;
     const auto floor=sim.collision().raycast(at.position+Vec3{0,0,2},{0,0,-1},4);if(!floor)continue;at.position.z=floor->position.z+.01f;
     test::place_player(sim,at);const auto p=match->action_prompt(0);
     if(p.target.object!=handle||p.target.action!=(flip?8:board?6:5))continue;
     selected=p;target=handle;position=at.position;yaw=at.facing;expected_seat=p.target.index;break;
    }if(target!=null_datum)break;
   }if(target!=null_datum)break;
  }
 }
 check(target!=null_datum,"No actual admitted target for requested prompt route");
 DesktopSession desktop;auto *window=desktop.window(1280,800,false);SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
 MultiplayerKbm keyboard(SDL_GetWindowID(window),desktop.keyboard_settings);const bool gamepad=std::getenv("HALO2_TEST_PROMPT_GAMEPAD");
 int pad=-1;SDL_Joystick *joystick=nullptr;SDL_GameController *controller=nullptr;
 if(gamepad){pad=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);check(pad>=0&&SDL_IsGameController(pad),"Mapped SDL controller unavailable");joystick=SDL_JoystickOpen(pad);controller=SDL_GameControllerOpen(pad);check(joystick&&controller,"Open mapped SDL controller");}
 auto mesh=cache.load_bsp(0);struct Visual{RenderModel model;unsigned material;std::uint32_t handle;};std::vector<Visual> visuals;
 const auto add=[&](std::uint32_t id,std::uint32_t variant,std::uint32_t handle){auto model=cache.load_model(id,true,variant);prepare_forward_materials(cache,model.mesh);const auto base=unsigned(mesh.materials.size());mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());visuals.push_back({std::move(model),base,handle});};
 if(trade){const auto ally=std::find_if(sim.actors().begin(),sim.actors().end(),[&](const auto &u){return u.source_datum==target;});add(ally->definition.datum,0,target);}
 else for(const auto &o:sim.scene().objects())if(o.handle!=null_datum&&o.visible&&content.objects[o.placement].type==1)add(content.objects[o.placement].definition,content.objects[o.placement].variant,o.handle);
 NativeActionPromptHud hud(cache,content.player.datum,cache.shared_directory()/"fonts",mesh,argv[2]);Renderer renderer(window,mesh,false);
 NativeInteractionHold hold;bool captured{},used{};unsigned after{};
 for(unsigned tick=0;tick<180;++tick) {
  SDL_Event event{};while(SDL_PollEvent(&event))keyboard.handle_event(event);SDL_GameControllerUpdate();const auto now=SDL_GetTicks();auto input=keyboard.consume(yaw,pitch,now).controls;
  if(controller){const auto pad_input=controller_input(sample_controller(controller),desktop.controller_settings);hold.set(pad_input.pickup,now);const auto h=hold.sample(now);input.pickup=h.first;input.pickup_milliseconds=h.second;hold.consume();}
  if(match){std::array<Controls,multiplayer_capacity> c{};c[0]=input;match->step(c);}else sim.step(input);
  const auto prompt=match?match->action_prompt(0):sim.action_prompt();const auto frame=hud.frame(prompt,desktop.keyboard_settings,desktop.controller_settings,gamepad,1280,800);
  if(tick<15)check(prompt.target.object==target&&!frame.text.empty()&&!frame.vertices.empty(),"Idle live prompt disappeared");
  Camera camera;camera.position=ordinary_unit_eye(sim.player().body.position,content.player.camera_height,content.player.crouch_camera_height,sim.player().crouch);camera.yaw=yaw;camera.pitch=pitch;
  std::vector<Vertex> geometry;for(const auto &v:visuals){Transform transform;
   if(trade){const auto a=std::find_if(sim.actors().begin(),sim.actors().end(),[&](const auto &u){return u.source_datum==v.handle;});const auto b=authored_basis({a->yaw,0,0});transform={1,{b.forward,b.left,b.up},a->body.position};}
   else {const auto &o=sim.scene().get(v.handle);const auto b=authored_basis(o.rotation);transform=o.solved_transform.value_or(Transform{o.scale,{b.forward,b.left,b.up},o.position});}
   auto vertices=pose_model_world(v.model,transform);for(auto &vertex:vertices)vertex.material+=v.material;geometry.insert(geometry.end(),vertices.begin(),vertices.end());}
  if(renderer.draw(camera,tick==12?argv[4]:std::filesystem::path{},geometry,true,{},sim.presentation().effects(),frame.vertices)&&tick==12){captured=true;std::cout<<"Captured live "<<mode<<": "<<frame.text<<" GPU="<<renderer.device_name()<<'\n';}
  if(tick==20){if(joystick)check(SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_X,1)==0,"Hold mapped SDL X");else {event={};event.type=SDL_KEYDOWN;event.key.windowID=SDL_GetWindowID(window);event.key.keysym.scancode=SDL_SCANCODE_E;event.key.state=SDL_PRESSED;check(SDL_PushEvent(&event)==1,"Hold SDL E");}}
  if(trade){const auto a=std::find_if(sim.actors().begin(),sim.actors().end(),[&](const auto &u){return u.source_datum==target;});used=sim.player().weapon.source_datum==original_ally&&a->weapon.source_datum==original_player;}
  else if(flip){const auto &o=sim.scene().get(target);used=o.solved_transform&&o.solved_transform->basis[2].z>.9f;}
  else used=sim.player().source_parent==target&&(board?sim.player().source_seat==0&&match->unit(1).source_parent==null_datum:sim.player().source_seat==expected_seat&&sim.player().seat_action.phase==Unit::SeatAction::Seated);
  if(used&&++after>3)break;SDL_Delay(34);
 }
 if(controller)SDL_GameControllerClose(controller);if(joystick)SDL_JoystickClose(joystick);if(pad>=0)SDL_JoystickDetachVirtual(pad);
 check(captured&&used,"Displayed prompt failed its real SDL hold/action route");std::cout<<"PASS live "<<mode<<" new button artwork, SDL "<<(gamepad?"X":"E")<<" and actual object action\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
