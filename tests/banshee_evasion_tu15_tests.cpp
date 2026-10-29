#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/controller_settings.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/math.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_match_authority.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *why) {if(!value)throw FormatError(why);}
template<class Address>
std::uint32_t word(const SourceRuntimeMemory &memory,Address address,unsigned size=4) {
  const auto bytes=memory.view(address,size);std::uint32_t result{};
  for(unsigned i=0;i<size;++i)result|=std::to_integer<unsigned>(bytes[i])<<(8*i);
  return result;
}
Json xyz(Vec3 v) {return {v.x,v.y,v.z};}
Spawn entrance(const Cache &cache,const LevelContent &content) {
  check(bool(content.unit_interactions),"Banshee test has no authored vehicle seats");
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,index]:assets.netgame_placements) {
    const auto &placed=content.objects.at(index);
    const auto tag=cache.query_tag({.handle=TagHandle{placed.definition}});
    if(!tag || !tag->name.ends_with("\\banshee") || placed.model_attachment)continue;
    const auto basis=authored_basis(placed.rotation);
    const auto root=source_object_transform(placed.position,basis.forward,basis.up,placed.scale,false);
    const auto &definition=assets.units.at(placed.definition);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(const auto &seat:definition.seats) {
      if(!(seat.flags&4u))continue;
      const auto marker=definition.markers.find(seat.entry_marker);
      if(marker==definition.markers.end())continue;
      const auto points=pose_model_markers(marker->second,nodes,definition.permutations,{},1);
      if(points.empty() || seat.radius<=0)continue;
      auto position=points.front().world.position+points.front().world.basis[0]*(seat.radius*.5f);
      const CollisionWorld terrain(content.collision);
      const auto floor=terrain.raycast(position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(!floor)continue;
      position.z=floor->position.z+content.player.radius;
      const auto look=points.front().world.position-(position+Vec3{0,0,content.player.camera_height});
      return {position,std::atan2(look.y,look.x)};
    }
  }
  throw FormatError("No authored Coagulation Banshee driver approach");
}
struct Keyboard {
  MultiplayerKbm input;
  void key(SDL_Scancode scan,bool down) {
    SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;
    event.key.state=down?SDL_PRESSED:SDL_RELEASED;event.key.keysym.scancode=scan;
    check(SDL_PushEvent(&event)==1,"SDL rejected Banshee keyboard input");
    while(SDL_PollEvent(&event))input.handle_event(event);
  }
  void boost(bool down) {
    SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
    event.button.state=down?SDL_PRESSED:SDL_RELEASED;event.button.button=SDL_BUTTON_RIGHT;
    check(SDL_PushEvent(&event)==1,"SDL rejected Banshee boost input");
    while(SDL_PollEvent(&event))input.handle_event(event);
  }
  Controls controls(float yaw,float pitch) {return input.consume(yaw,pitch).gameplay_controls(false,true,false);}
};
Controls controller(float forward,float right,bool jump,float yaw,float pitch,bool boost=false) {
  ControllerSample sample;
  sample.hardware[16]=std::max(0.f,forward);sample.hardware[17]=std::max(0.f,-forward);
  sample.hardware[18]=std::max(0.f,-right);sample.hardware[19]=std::max(0.f,right);
  sample.hardware[12]=jump?1.f:0.f;
  sample.hardware[0]=boost?1.f:0.f;
  const auto mapped=controller_input(sample,ControllerSettings{});
  Controls result;result.forward=mapped.forward;result.right=mapped.right;
  result.jump=mapped.jump;result.yaw=yaw;result.pitch=pitch;
  result.fire_secondary=mapped.secondary;result.crouch=mapped.secondary;return result;
}
}
int main(int argc,char **argv) {
  Json report{{"passed",false},{"cases",Json::array()},
      {"inputs","SDL Space/WASD and controller A/movement mapping"},
      {"physics_or_maneuver_results_supplied",false}};
  std::filesystem::path output;
  try {
    check(argc==4,"Supply prepared MCC_MAP MCC_MAPS REPORT");output=argv[3];
    if(!std::filesystem::exists(argv[1]))return 77;
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_EVENTS)==0,"SDL event initialization failed");
    struct Quit {~Quit(){SDL_Quit();}} quit;
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=1;options.source_object_world=true;options.starting_weapon=5;options.mute=true;
    options.variant.values.set(79,1);options.session_random_seed=0x22a0d10u;
    Spawn setup;
    {
      Cache cache(options.map,options.shared_maps);
      check(cache.is_mcc(),"Banshee evasion runtime requires prepared MCC assets");
      setup=entrance(cache,cache.load_content(0,true,source_game_variant(options.variant)));
    }
    MultiplayerPreparedWorld world(options,std::nullopt);world.bind_source_local_views(world.local_slots);
    auto &source=*world.source_objects;
    const auto slot=world.local_slots.front();std::array<Controls,multiplayer_capacity> commands{};
    const auto born=world.match.state().players[slot];
    commands[slot].yaw=born.yaw;commands[slot].pitch=born.pitch;world.match.step(commands);
    // Declared offline fixture setup places only the already-born player near
    // an authored entrance. Original admission still chooses and enters a seat.
    check(born.source_unit!=null_datum,"Banshee fixture has no genuinely born player");
    test::place_player(world.match,slot,setup);
    source.tu15_world().set_object_position(born.source_unit,setup.position);
    commands[slot].yaw=setup.facing;commands[slot].pitch=0;
    for(unsigned tick=0;tick<60;++tick)world.match.step(commands);
    const auto threshold=source_pickup_hold_threshold(world.content.pickup_hold_seconds);
    std::uint32_t vehicle=null_datum;
    for(unsigned tick=0;tick<150;++tick) {
      commands[slot].pickup=true;commands[slot].pickup_milliseconds=std::uint16_t(threshold+100u);
      commands[slot].vehicle_interaction_target=world.match.state().players[slot].action_prompt.target;
      world.match.step(commands);
      const auto player=world.match.state().players[slot];
      if(player.source_parent!=null_datum){vehicle=player.source_parent;break;}
    }
    check(vehicle!=null_datum,"Original Banshee driver entry did not complete");
    const auto tag=world.cache->query_tag({.handle=TagHandle{source.object_definition(vehicle)}});
    check(tag && tag->name.ends_with("\\banshee"),"Driver entered a different vehicle");
    report["vehicle"]=vehicle;
    commands[slot]={};
    const auto initial=world.match.state().players[slot];commands[slot].yaw=initial.yaw;commands[slot].pitch=initial.pitch;
    // Normal forward/pitch input owns takeoff. No transform, velocity, force,
    // maneuver code or readiness result is written into the source world.
    commands[slot].forward=1;commands[slot].pitch+=.3f;
    const auto grounded=source.object_transform(vehicle);
    for(unsigned tick=0;tick<120;++tick)world.match.step(commands);
    const auto airborne=source.object_transform(vehicle);
    report["takeoff_start"]=xyz(grounded.position);report["takeoff_end"]=xyz(airborne.position);
    check(airborne.position.z-grounded.position.z>2.f,"Banshee did not take off under ordinary movement input");
    commands[slot].forward=0;
    const auto body=[&]{return source.object_body(vehicle);};
    Keyboard keyboard;
    struct Direction {const char *name;SDL_Scancode key;float forward,right;unsigned expected;};
    const std::array directions{
      Direction{"forward",SDL_SCANCODE_W,1,0,0},Direction{"left",SDL_SCANCODE_A,0,-1,1},
      Direction{"right",SDL_SCANCODE_D,0,1,2},Direction{"back",SDL_SCANCODE_S,-1,0,3}};
    const auto sample=[&] {
      const auto pose=source.object_transform(vehicle);
      return Json{{"maneuver",word(source.memory(),body()+0x350u,1)},{"age",word(source.memory(),body()+0x351u,1)},
          {"flags",word(source.memory(),body()+0x348u,2)},{"unit_flags",word(source.memory(),body()+0x148u)},
          {"position",xyz(pose.position)},{"forward",xyz(pose.basis[0])},{"up",xyz(pose.basis[2])}};
    };
    // Each input cohort starts from the exact same genuinely flown world.
    // The coupled owner restores its complete Match, native bank and sidecars;
    // this prevents later cases flying into distant scenery. No scalar pose,
    // maneuver, physics or admission result is fabricated for the baseline.
    SourceMatchAuthority owner(world.match,source);
    SourceMatchAuthority::AuthorityDigest binding{};binding.front()=std::byte{0x42};
    const auto admissions=owner.player_admissions(world.local_slots);
    const auto saved=owner.checkpoint(binding);
    const auto yaw=commands[slot].yaw,pitch=commands[slot].pitch;
    const auto restore=[&] {
      auto staged=owner.prepare(saved,binding,admissions);
      check(owner.commit(std::move(staged)),"Banshee fixture could not restore its genuinely flown world");
      keyboard.input.release();commands[slot]=controller(0,0,false,yaw,pitch);
      check(world.match.state().players[slot].source_parent==vehicle,"Restored Banshee lost its actual driver");
      check(word(source.memory(),body()+0x351u,1)>45,"Genuinely flown Banshee baseline is still cooling down");
    };
    restore();
    keyboard.key(SDL_SCANCODE_SPACE,true);
    for(unsigned tick=0;tick<4;++tick) {
      commands[slot]=keyboard.controls(yaw,pitch);world.match.step(commands);
      check(word(source.memory(),body()+0x351u,1)>45,"Neutral Space incorrectly started a Banshee maneuver");
    }
    keyboard.key(SDL_SCANCODE_SPACE,false);report["neutral_modifier_blocked"]=true;
    for(bool pad:{false,true})for(const auto &direction:directions) {
      restore();
      Json row{{"device",pad?"controller":"keyboard"},{"direction",direction.name},
          {"expected_maneuver",direction.expected},{"samples",Json::array()}};
      const auto before=sample();row["before"]=before;
      report["active_case"]=row;
      if(!pad){keyboard.key(direction.key,true);keyboard.key(SDL_SCANCODE_SPACE,true);keyboard.boost(true);}
      for(unsigned tick=0;tick<4;++tick) {
        commands[slot]=pad?controller(direction.forward,direction.right,true,yaw,pitch,true):keyboard.controls(yaw,pitch);
        check(commands[slot].fire_secondary,"Banshee boost was lost in the input adapter");
        world.match.step(commands);
        report["active_case"]["boost_sample"]=sample();
        check(word(source.memory(),body()+0x351u,1)>45,"Banshee evasion bypassed its original boost exclusion");
      }
      row["boost_blocked"]=true;
      if(!pad)keyboard.boost(false);
      bool triggered=false;unsigned trigger_tick=0;
      for(unsigned tick=0;tick<8;++tick) {
        commands[slot]=pad?controller(direction.forward,direction.right,true,yaw,pitch):keyboard.controls(yaw,pitch);
        check(commands[slot].jump && commands[slot].forward==direction.forward && commands[slot].right==direction.right,
            "Banshee modifier/direction was lost in the input adapter");
        world.match.step(commands);row["samples"].push_back(sample());
        report["active_case"]=row;
        const auto age=word(source.memory(),body()+0x351u,1);
        if(direction.expected && age<=2 && word(source.memory(),body()+0x350u,1)==direction.expected){triggered=true;trigger_tick=tick;break;}
        if(!direction.expected)check(age>45,"Forward input incorrectly started a Banshee evasion maneuver");
      }
      row["triggered"]=triggered;row["trigger_tick"]=trigger_tick;
      report["cases"].push_back(row);
      if(!direction.expected) {
        if(!pad){keyboard.key(SDL_SCANCODE_SPACE,false);keyboard.key(direction.key,false);}
        continue;
      }
      check(triggered,"Banshee jump/direction did not start its original evasion maneuver");
      float orientation_change=0;
      const auto start=source.object_transform(vehicle);
      unsigned previous_age=word(source.memory(),body()+0x351u,1);bool repeated=false;
      for(unsigned tick=0;tick<52;++tick) {
        commands[slot]=pad?controller(direction.forward,direction.right,true,yaw,pitch):keyboard.controls(yaw,pitch);
        world.match.step(commands);
        const auto age=word(source.memory(),body()+0x351u,1);
        const auto pose=source.object_transform(vehicle);
        orientation_change=std::max(orientation_change,length(pose.basis[2]-start.basis[2]));
        if(age<previous_age) {
          check(previous_age>45,"Held evasion retriggered before the original cooldown boundary");
          check(word(source.memory(),body()+0x350u,1)==direction.expected,"Held evasion changed its original direction");
          report["cases"].back()["repeat_previous_age"]=previous_age;
          report["cases"].back()["repeat_after_ticks"]=tick+1;repeated=true;break;
        }
        previous_age=age;
      }
      check(repeated,"Held Banshee evasion did not repeat after its original cooldown");
      if(!pad){keyboard.key(SDL_SCANCODE_SPACE,false);keyboard.key(direction.key,false);}
      commands[slot]=controller(0,0,false,yaw,pitch);
      previous_age=word(source.memory(),body()+0x351u,1);
      for(unsigned tick=0;tick<52;++tick) {
        world.match.step(commands);
        const auto age=word(source.memory(),body()+0x351u,1);
        check(age>=previous_age,"Released evasion input restarted the native maneuver");previous_age=age;
        const auto pose=source.object_transform(vehicle);
        orientation_change=std::max(orientation_change,length(pose.basis[2]-start.basis[2]));
      }
      report["cases"].back()["orientation_change"]=orientation_change;
      check(orientation_change>.1f,"Banshee maneuver code did not produce vehicle rotation");
      check(world.match.state().players[slot].source_parent==vehicle,"Banshee test lost its real driver");
    }
    // A complete Space+direction chord between simulation ticks still owns one
    // native evasion request. Its following neutral tick cannot replay it.
    restore();keyboard.key(SDL_SCANCODE_A,true);keyboard.key(SDL_SCANCODE_SPACE,true);
    keyboard.key(SDL_SCANCODE_SPACE,false);keyboard.key(SDL_SCANCODE_A,false);
    commands[slot]=keyboard.controls(yaw,pitch);
    check(commands[slot].jump && commands[slot].right==-1,"Short Banshee evasion chord lost its direction before delivery");
    world.match.step(commands);
    check(word(source.memory(),body()+0x350u,1)==1 && word(source.memory(),body()+0x351u,1)<=2,
        "Short keyboard chord did not start the original Banshee roll");
    commands[slot]=keyboard.controls(yaw,pitch);
    check(!commands[slot].jump && !commands[slot].right,"Consumed short Banshee chord replayed");
    report["short_chord_delivered_once"]=true;
    report.erase("active_case");report["passed"]=true;
  }catch(const std::exception &error){report["error"]=error.what();std::cerr<<error.what()<<'\n';}
  if(!output.empty()){std::filesystem::create_directories(output.parent_path());std::ofstream(output)<<report.dump(2)<<'\n';}
  std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
