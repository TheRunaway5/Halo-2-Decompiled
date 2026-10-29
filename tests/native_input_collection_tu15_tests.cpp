#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/original_data.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool yes,const char*why){if(!yes)throw FormatError(why);}
struct Pad {
  SDL_Joystick*joystick{};SDL_JoystickID id=-1;
  Pad() {
    check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,SDL_GetError());
    SDL_VirtualJoystickDesc d{};d.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    d.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;d.naxes=SDL_CONTROLLER_AXIS_MAX;
    d.nbuttons=SDL_CONTROLLER_BUTTON_MAX;d.name="TU1.5 complete local input regression";
    const auto index=SDL_JoystickAttachVirtualEx(&d);check(index>=0,SDL_GetError());
    joystick=SDL_JoystickOpen(index);check(joystick,SDL_GetError());id=SDL_JoystickInstanceID(joystick);
    for(int a=0;a<SDL_CONTROLLER_AXIS_MAX;++a)axis(SDL_GameControllerAxis(a),a>=4?-32768:0);
  }
  void axis(SDL_GameControllerAxis a,Sint16 value){check(SDL_JoystickSetVirtualAxis(joystick,a,value)==0,SDL_GetError());}
  void button(SDL_GameControllerButton b,bool pressed){check(SDL_JoystickSetVirtualButton(joystick,b,pressed)==0,SDL_GetError());}
  ~Pad(){if(joystick)SDL_JoystickClose(joystick);
    for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_JoystickGetDeviceInstanceID(i)==id){SDL_JoystickDetachVirtual(i);break;}
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);}
};
}
int main(int argc,char**argv)try {
  check(argc==5,"Supply MAP MAPS native-mcc|XBE OUTPUT");std::cout<<std::unitbuf;
  if(!*argv[1]){std::cout<<"SKIP: supply HALO2_MCC_SWORD_LUNGE_MAP\n";return 77;}
  const bool native=std::string_view(argv[3])=="native-mcc";
  Cache cache(argv[1],argv[2],native?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(argv[3]));
  const auto original=cache.original_data();
  check(!native||(cache.is_mcc()&&original==OriginalData::native_tu15()),"Native collector requires prepared MCC data and compiled TU1.5 operands");
  auto content=cache.load_content(0,true);check(!cache.spawns().empty(),"Map lacks an authored player start");
  const auto sword_case=std::getenv("HALO2_NATIVE_INPUT_SWORD");
  const bool sword=sword_case!=nullptr,cancel=sword&&std::string_view(sword_case)=="cancel";
  check(!sword||cancel||std::string_view(sword_case)=="lunge","Unknown physical Sword input case");
  std::filesystem::create_directories(argv[4]);
  Pad pad;
  auto owner=native?std::make_unique<MultiplayerObjectRuntime>(cache,0,std::filesystem::path(argv[4])/"world"):
      std::make_unique<MultiplayerObjectRuntime>(argv[3],cache,0,std::filesystem::path(argv[4])/"world");
  auto&objects=*owner;
  auto&world=objects.tu15_world();auto variant=source_game_variant_default(0);
  if(sword)source_game_variant_set(variant,0x59,17); // Actual authored starting Sword choice.
  objects.configure_session(variant,1,30,0x1234);
  world.initialize_native_input({true,{pad.id}});
  // Explicit physical-machine identity fixture, shared by actual local profile
  // publication and actual player construction. No account/identity result is supplied.
  const SourceMachineIdentity machine{0x02,0x15,0x10,0x28,0x40,0x01};
  world.publish_native_input_profile(0,source_default_full_player_profile(),machine);
  DatumPool players(world.memory(),world.player_pool());
  auto config=source_default_player_configuration();
  constexpr auto profile=0x6005b000u;world.memory().write(profile,std::as_bytes(std::span(config)));
  auto sanitize=world.prepare_source_call();sanitize.gp[2]=profile;world.run_actual(0x14e7b0u,sanitize);
  world.memory().read(profile,std::as_writable_bytes(std::span(config)));
  const auto player=players.allocate_at(0);world.construct_player(player,0,config,machine);
  auto start=cache.spawns().front();
  CollisionWorld terrain(content.collision);
  if(sword) {
    // Reuse the accepted lunge fixture's authored-start/floor-corridor survey.
    bool found=false;
    for(const auto&candidate:cache.spawns()) {
      for(unsigned n=0;n<4&&!found;++n) {
        const auto angle=float(n)*1.5707963267948966f;const auto d=direction(angle);
        const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
        if(terrain.raycast(eye,d,3.f)||terrain.raycast(eye,d*-1.f,3.f))continue;
        bool floor=true;
        for(const auto distance:{-3.f,-1.5f,0.f,1.5f,3.f}) {
          const auto hit=terrain.raycast(candidate.position+d*distance+Vec3{0,0,.5f},{0,0,-1},1.f);
          floor&=hit&&hit->normal.z>.9f;
        }
        if(floor){start=candidate;start.facing=angle;found=true;}
      }
      if(found)break;
    }
    check(found,"Physical Sword input has no clear authored floor corridor");
  }
  const auto floor=terrain.raycast(start.position+Vec3{0,0,.5f},{0,0,-1},1.f);
  check(floor.has_value(),"Authored start has no BSP floor");
  // Declared placement uses the qualified fixture's capsule-origin convention.
  const auto position=floor->position+Vec3{0,0,content.player.radius};
  auto facing=start.facing;Vec3 target_position{};
  if(sword) {
    bool found=false;
    for(unsigned n=0;n<32&&!found;++n) {
      const auto angle=start.facing+float(n)*6.283185307179586f/32.f;
      const auto point=position+direction(angle)*3.f;
      const auto target_floor=terrain.raycast(point+Vec3{0,0,2},{0,0,-1},4);
      if(!target_floor||target_floor->normal.z<.9f||std::abs(target_floor->position.z-floor->position.z)>.08f)continue;
      const auto placed=target_floor->position+Vec3{0,0,content.player.radius};
      if(terrain.overlaps(placed,content.player.height,content.player.radius))continue;
      const auto eye=position+Vec3{0,0,content.player.camera_height};
      if(terrain.raycast(eye,direction(angle),3.f))continue;
      target_position=placed;facing=angle;found=true;
    }
    check(found,"No clear authored floor for the physical Sword target");
  }
  //151DA0 includes14FDC0 admission. A manually created/bound biped leaves
  // the player inactive and cannot stand in for original multiplayer birth.
  const auto unit=world.birth_player(player);check(unit!=null_datum,"Original local player birth rejected");
  world.set_object_position(unit,position);
  std::uint32_t target=null_datum;
  if(sword) {
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){
      return t.group=="weap"&&t.name=="objects\\weapons\\melee\\energy_blade\\energy_blade";});
    check(tag!=cache.tags().end(),"Map lacks the authored Energy Sword");
    const auto inventory=objects.player_equipment(unit);const auto selected=inventory.current[0];
    check(selected>=0&&selected<4&&inventory.definitions[unsigned(selected)]==tag->datum,
        "Original player factory did not select the declared starting Sword");
    const auto victim=players.allocate_at(1);auto victim_config=config;
    // Same FFA membership input as MultiplayerMatch::join: each source
    // slot has its own membership. Two default-zero members are allies.
    victim_config[0x7c]=1;world.construct_player(victim,1,victim_config,machine);
    target=world.birth_player(victim);check(target!=null_datum,"Original target player birth rejected");
    world.set_object_position(target,target_position);
  }
  world.set_local_player(player,0);world.set_player_input_user(player,0);
  NativeVehicleCamera camera(objects,*original);nlohmann::json frames=nlohmann::json::array();
  unsigned tick{};
  unsigned mode6_samples{},cancel_stores{};std::optional<unsigned> cancel_tick,trigger_tick;
  std::optional<NativeDeathCameraView> observed_view;
  bool locked{};
  const auto body=world.source_object_body(unit);
  const auto component=body+std::uint32_t(std::bit_cast<std::int16_t>(Bytes(world.memory().view(body+0x346u,2)).u16(0)));
  objects.observe_physics_stores([&](auto,const auto&store){
    if(cancel&&cancel_tick&&store.instruction==0xec770u&&store.address==component+0x2cu&&
       Bytes(store.before).u32(0)!=null_datum)++cancel_stores;
  });
  const auto capture=std::getenv("HALO2_NATIVE_INPUT_CAPTURE")!=nullptr;
  test::LivePhysicsEntryCaptureTu15 cold(std::filesystem::path(argv[4])/"cold");
  test::LivePhysicsEntryCaptureTu15 held(std::filesystem::path(argv[4])/"held");
  SourcePhysicsEntryObserverBinding observation(world.memory(),[&](bool entering,auto entry,const auto&abi){
    if(capture&&entry==0x1898a0u) {
      if(tick==0)cold.observe(entering,abi,world.memory(),entry);
      if(sword?trigger_tick&&tick==*trigger_tick+2:tick==4)
        held.observe(entering,abi,world.memory(),entry);
    }
  });
  for(;tick<(sword?120u:12u);++tick) {
    if(sword&&observed_view) {
      const auto look=objects.player_look(player,unit);check(look.has_value(),"Physical aiming lost original retained look");
      const auto delta=objects.object_sphere(target).center-observed_view->position;
      const auto yaw=std::atan2(delta.y,delta.x),pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
      // Keep the test's steering samples outside the real stick dead zone.
      // Small proportional samples previously stopped before reaching aim.
      const auto steer=[](float error){
        if(std::abs(error)<.005f)return Sint16{};
        return Sint16(-std::copysign(std::min(1.f,.36f+std::abs(error)*.75f),error)*32767.f);
      };
      pad.axis(SDL_CONTROLLER_AXIS_RIGHTX,steer(std::remainder(yaw-look->x,6.283185307179586f)));
      pad.axis(SDL_CONTROLLER_AXIS_RIGHTY,steer(pitch-look->y));
    }
    if(sword&&!trigger_tick&&tick>=20&&locked)trigger_tick=tick;
    if(cancel&&!cancel_tick&&mode6_samples)cancel_tick=tick;
    pad.axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,(sword?trigger_tick&&tick<*trigger_tick+6:tick>=3&&tick<6)&&!cancel_tick?32767:-32768);
    pad.button(SDL_CONTROLLER_BUTTON_X,sword?cancel_tick&&tick<*cancel_tick+2:tick>=7&&tick<=8);
    pad.button(SDL_CONTROLLER_BUTTON_Y,!sword&&tick>=9&&tick<=10);
    SDL_GameControllerUpdate();world.sample_native_input();
    const auto command=camera.collect_player_input(0,player,1.f/30.f,1.f/30.f);
    const Bytes b(command);const auto flags=b.u32(0);
    locked=sword&&b.u32(0x34)==target&&b.f32(0x50)>=1.f;
    check(source_player_command_valid(command),"Complete collector produced invalid command bytes");
    if(!sword&&tick==4)check((flags&0x10040u)==0x10040u&&b.f32(0x14)==1.f,
        "Held physical RT failed original primary intent/pressure");
    if(sword)std::cout<<nlohmann::json{{"physical_input_tick",tick},{"player",player},{"unit",unit},
        {"current_unit",Bytes(std::as_bytes(std::as_const(players).lookup(player))).u32(0x2c)},
        {"position_z",objects.biped_motion(unit).position.z},
        {"player_flags",Bytes(std::as_bytes(std::as_const(players).lookup(player))).u16(2)},
        {"shared_controller_entry",Bytes(world.memory().view(0x4f5520u,4)).u32(0)},
        {"weapon_current",objects.player_equipment(unit).current[0]},
        {"command_selected",b.u16(0x1e)},
        {"observer_mode",observed_view?unsigned(observed_view->camera_mode):255u},
        {"eye",observed_view?nlohmann::json{observed_view->position.x,observed_view->position.y,observed_view->position.z}:nlohmann::json()},
        {"target_center",nlohmann::json{objects.object_sphere(target).center.x,objects.object_sphere(target).center.y,objects.object_sphere(target).center.z}},
        {"yaw",b.f32(4)},{"pitch",b.f32(8)},
        {"flags",flags},{"target",b.u32(0x34)},{"confidence",b.f32(0x50)}}.dump()<<'\n';
    objects.deliver_command(player,unit,command);objects.step_objects();
    const auto physical=objects.biped_motion(unit).physical_state;mode6_samples+=physical==6;
    const auto row=std::as_const(players).lookup(player);
    camera.update_player(0,player,Bytes(std::as_bytes(row)).u16(2),1.f/30.f,false,&observed_view);
    frames.push_back({{"tick",tick},{"flags",flags},{"primary_pressure",b.f32(0x14)},
        {"selected",b.u16(0x1e)},{"target",b.u32(0x34)},{"confidence",b.f32(0x50)},
        {"x_sample",std::to_integer<unsigned>(world.memory().view(0x4f2aacu+0x12u,1)[0])},
        {"y_sample",std::to_integer<unsigned>(world.memory().view(0x4f2aacu+0x13u,1)[0])},
        {"physical_mode",physical},{"activity",Bytes(world.memory().view(component+0x2cu,4)).u32(0)}});
    if(sword&&!cancel&&objects.object_vitality(target).body<=0)break;
    if(cancel&&cancel_stores&&tick>=*cancel_tick+15)break;
  }
  objects.observe_physics_stores({});
  if(sword) {
    check(mode6_samples>0,"Physical Sword input never generated an actual native lunge");
    const auto vitality=objects.object_vitality(target);
    if(cancel)check(cancel_stores>0&&vitality.body>0,"Physical X failed to cancel the native Sword before damage");
    else check(vitality.body<=0,"Physical native Sword lunge failed to kill its ordinary target");
  }
  world.close_native_input();
  std::ofstream out(std::filesystem::path(argv[4])/"report.json");
  out<<nlohmann::json{{"passed",true},{"frames",frames},
    {"scope","Actual SDL virtual pad -> native startup/profile/sample/mapping -> complete1898A0 -> delivered command -> native frame -> preceding private camera"}}.dump(2)<<'\n';
  check(bool(out),"Cannot write complete collector report");
  std::cout<<nlohmann::json{{"native_input_frames",frames}}.dump()<<'\n';
  std::cout<<"PASS complete original local input collector: "<<frames.size()<<" physical input/game/camera phases\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
