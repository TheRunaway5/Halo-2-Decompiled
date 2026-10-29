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
  const bool campaign=std::getenv("HALO2_NATIVE_INPUT_CAMPAIGN")!=nullptr;
  Cache cache(argv[1],argv[2],native?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(argv[3]));
  const auto original=cache.original_data();
  check(!native||(cache.is_mcc()&&original==OriginalData::native_tu15()),"Native collector requires prepared MCC data and compiled TU1.5 operands");
  check(!campaign||cache.load_source_geometry_scenario_type()==0,"Campaign collector requires authored Campaign geometry");
  auto content=cache.load_content(0,!campaign);check(!cache.spawns().empty(),"Map lacks an authored player start");
  const auto sword_case=std::getenv("HALO2_NATIVE_INPUT_SWORD");
  const bool sword=sword_case!=nullptr,jump_cancel=sword&&std::string_view(sword_case)=="cancel-jump";
  const bool cancel=jump_cancel||(sword&&std::string_view(sword_case)=="cancel");
  check(!sword||cancel||std::string_view(sword_case)=="lunge","Unknown physical Sword input case");
  std::filesystem::create_directories(argv[4]);
  Pad pad;
  auto owner=native?std::make_unique<MultiplayerObjectRuntime>(cache,0,std::filesystem::path(argv[4])/"world"):
      std::make_unique<MultiplayerObjectRuntime>(argv[3],cache,0,std::filesystem::path(argv[4])/"world");
  auto&objects=*owner;
  auto&world=objects.tu15_world();auto variant=source_game_variant_default(0);
  if(sword)source_game_variant_set(variant,0x59,17); // Actual authored starting Sword choice.
  if(campaign){world.initialize_map();world.configure_campaign_session(1,1,30,0x1234);}
  else objects.configure_session(variant,1,30,0x1234);
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
        for(const auto distance:{-2.5f,-1.25f,0.f,1.25f,2.5f}) {
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
  // Preserve the actual authored attacker start, as the canonical lunge
  // fixture does; its target alone uses the nearby floor-placement input.
  const auto position=start.position;
  auto facing=start.facing;Vec3 target_position{};
  if(sword) {
    const auto point=position+direction(facing)*2.5f;
    const auto target_floor=terrain.raycast(point+Vec3{0,0,.5f},{0,0,-1},1.f);
    check(target_floor&&target_floor->normal.z>.9f,"Physical Sword target has no actual floor");
    target_position=target_floor->position+Vec3{0,0,.002f};
  }
  //151DA0 includes14FDC0 admission. A manually created/bound biped leaves
  // the player inactive and cannot stand in for original multiplayer birth.
  const auto unit=campaign?objects.create_biped(content.player.datum,position,direction(facing),player):world.birth_player(player);
  check(unit!=null_datum,"Original local player birth rejected");
  if(campaign) {
    objects.bind_player(player,unit);objects.initialize_player_equipment(player,unit);
    // The Campaign constructor owns its biped independently of151DA0.
    // Run the same original global player-admission phase before collection.
    world.call_actual(0x14fdc0u);
  }
  world.set_object_position(unit,position);
  std::uint32_t target=null_datum;
  if(sword) {
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){
      return t.group=="weap"&&t.name=="objects\\weapons\\melee\\energy_blade\\energy_blade";});
    check(tag!=cache.tags().end(),"Map lacks the authored Energy Sword");
    if(campaign)check(world.equip_weapon(unit,tag->datum)!=null_datum,"Campaign Sword birth rejected");
    const auto inventory=objects.player_equipment(unit);const auto selected=inventory.current[0];
    check(selected>=0&&selected<4&&inventory.definitions[unsigned(selected)]==tag->datum,
        "Original player factory did not select the declared starting Sword");
    if(campaign) {
      const auto player_row=std::as_const(players).lookup(player);
      const auto team=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(std::as_bytes(player_row)[0xc0]));
      for(const auto&[id,character]:content.characters) {
        if(character.biped.datum==null_datum)continue;
        const auto definition=cache.load_source_tag_metadata(character.biped.datum);
        const auto candidate=std::bit_cast<std::int16_t>(Bytes(definition.bytes_at(definition.source_address(),definition.size())).u16(0xc0));
        if(candidate<0)continue;
        auto allegiance=world.prepare_source_call();allegiance.gp[1]=std::uint16_t(team);
        allegiance.gp[6]=std::uint16_t(candidate);world.run_actual(0x1e7810u,allegiance);
        if(!(allegiance.gp[0]&255u))continue;
        target=objects.create_biped(character.biped.datum,target_position,direction(facing+3.14159265f));
        break;
      }
      check(target!=null_datum,"Campaign lacks a resident hostile biped constructor input");
    } else {
      const auto victim=players.allocate_at(1);auto victim_config=config;
      // Same FFA membership input as MultiplayerMatch::join: each source
      // slot has its own membership. Two default-zero members are allies.
      victim_config[0x7c]=1;world.construct_player(victim,1,victim_config,machine);
      target=world.birth_player(victim);check(target!=null_datum,"Original target player birth rejected");
      world.set_object_position(target,target_position);
    }
  }
  world.set_local_player(player,0);world.set_player_input_user(player,0);
  NativeVehicleCamera camera(objects,*original);nlohmann::json frames=nlohmann::json::array();
  struct FailureReport {
    std::filesystem::path path;const nlohmann::json&frames;
    ~FailureReport(){if(std::uncaught_exceptions())try{
      std::ofstream out(path);out<<nlohmann::json{{"passed",false},{"frames",frames}}.dump(2)<<'\n';
    }catch(...){}}
  } failure_report{std::filesystem::path(argv[4])/"report.json",frames};
  unsigned tick{};
  unsigned mode6_samples{},cancel_stores{},reload_cancels{},old_impacts{};
  std::uint32_t reload_return{};std::optional<unsigned> cancel_tick,trigger_tick;
  std::optional<std::uint32_t> cancel_source_tick;
  unsigned continuation_samples{},jump_commands{};
  std::optional<MultiplayerSourceVitality> attack_vitality;
  std::optional<NativeDeathCameraView> observed_view;
  bool locked{};
  const auto body=world.source_object_body(unit);
  const auto component=body+std::uint32_t(std::bit_cast<std::int16_t>(Bytes(world.memory().view(body+0x346u,2)).u16(0)));
  objects.observe_physics_stores([&](auto,const auto&store){
    if(cancel&&cancel_tick&&store.instruction==0xec770u&&store.address==component+0x2cu&&
       Bytes(store.before).u32(0)!=null_datum)++cancel_stores;
    if(cancel&&cancel_tick) {
      const auto word=[&](auto address){return Bytes(world.memory().view(address,4)).u32(0);};
      if(store.instruction==0xe9741u)reload_return=store.address;
      if(store.instruction==0xe8f41u&&word(store.address+4u)==unit&&
         word(store.address+8u)==27u&&store.address+16u==reload_return&&
         word(reload_return)==0xe9746u)++reload_cancels;
      if(store.instruction==0xec684u&&word(store.address+4u)==unit)++old_impacts;
    }
  });
  const auto capture=std::getenv("HALO2_NATIVE_INPUT_CAPTURE")!=nullptr;
  test::LivePhysicsEntryCaptureTu15 cold(std::filesystem::path(argv[4])/"cold");
  test::LivePhysicsEntryCaptureTu15 held(std::filesystem::path(argv[4])/"held");
  test::LivePhysicsEntryCaptureTu15 interrupted(std::filesystem::path(argv[4])/"cancel");
  SourcePhysicsEntryObserverBinding observation(world.memory(),[&](bool entering,auto entry,const auto&abi){
    if(capture&&entry==0x1898a0u) {
      if(tick==0)cold.observe(entering,abi,world.memory(),entry);
      if(sword?trigger_tick&&tick==*trigger_tick+2:tick==4)
        held.observe(entering,abi,world.memory(),entry);
      if(cancel&&cancel_tick&&tick==*cancel_tick+1)
        interrupted.observe(entering,abi,world.memory(),entry);
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
    if(sword&&!trigger_tick&&tick>=20&&locked) {
      trigger_tick=tick;attack_vitality=objects.object_vitality(target);
    }
    if(cancel&&!cancel_tick&&mode6_samples) {
      cancel_tick=tick;
      const auto clock=Bytes(world.memory().view(0x51df54u,4)).u32(0);
      cancel_source_tick=Bytes(world.memory().view(clock+8u,4)).u32(0);
    }
    pad.axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,(sword?trigger_tick&&tick<*trigger_tick+6:tick>=3&&tick<6)&&!cancel_tick?32767:-32768);
    pad.button(SDL_CONTROLLER_BUTTON_X,sword?cancel_tick&&tick<*cancel_tick+2:tick>=7&&tick<=8);
    pad.button(SDL_CONTROLLER_BUTTON_A,jump_cancel&&cancel_tick&&tick==*cancel_tick+1);
    pad.button(SDL_CONTROLLER_BUTTON_Y,!sword&&tick>=9&&tick<=10);
    SDL_GameControllerUpdate();world.sample_native_input();
    const auto command=camera.collect_player_input(0,player,1.f/30.f,1.f/30.f);
    const Bytes b(command);const auto flags=b.u32(0);
    jump_commands+=(flags&2u)!=0;
    locked=sword&&b.u32(0x34)==target&&b.f32(0x50)>=1.f;
    check(source_player_command_valid(command),"Complete collector produced invalid command bytes");
    if(!sword&&tick==4)check((flags&0x10040u)==0x10040u&&b.f32(0x14)==1.f,
        "Held physical RT failed original primary intent/pressure");
    objects.deliver_command(player,unit,command);objects.step_objects();
    const auto motion=objects.biped_motion(unit);const auto physical=motion.physical_state;mode6_samples+=physical==6;
    continuation_samples+=cancel_stores&&physical==6;
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
  const auto write_report=[&](bool passed) {
    std::ofstream out(std::filesystem::path(argv[4])/"report.json");
    out<<nlohmann::json{{"passed",passed},{"frames",frames},
      {"campaign",campaign},
      {"mode6_samples",mode6_samples},{"cancel_stores",cancel_stores},
      {"reload_cancels",reload_cancels},{"old_impacts",old_impacts},
      {"continuation_samples",continuation_samples},{"jump_commands",jump_commands},
      {"cancel_source_tick",cancel_source_tick?nlohmann::json(*cancel_source_tick):nlohmann::json()},
      {"scope","Actual SDL virtual pad -> native startup/profile/sample/mapping -> complete1898A0 -> delivered command -> native frame -> preceding private camera"}}.dump(2)<<'\n';
    check(bool(out),"Cannot write complete collector report");
  };
  write_report(false);
  if(sword) {
    check(mode6_samples>0,"Physical Sword input never generated an actual native lunge");
    const auto vitality=objects.object_vitality(target);
    if(cancel)check(cancel_stores>0&&reload_cancels>0&&old_impacts==0&&continuation_samples>0&&attack_vitality&&
        vitality.body>=attack_vitality->body&&vitality.shield>=attack_vitality->shield,
        "Physical X failed the original reload-cancel caller/no-impact/no-damage checks");
    else check(vitality.body<=0,"Physical native Sword lunge failed to kill its ordinary target");
  }
  if(jump_cancel)check(jump_commands>0,"Physical jump after cancellation produced no original jump command");
  world.close_native_input();
  write_report(true);
  std::cout<<nlohmann::json{{"native_input_frames",frames}}.dump()<<'\n';
  std::cout<<"PASS complete original local input collector: "<<frames.size()<<" physical input/game/camera phases\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
