#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_game_variant.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-player-collision-XXXXXX").string();
    const auto created=::mkdtemp(name.data());check(created,"Player collision scratch creation failed");path=created;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct Start {Vec3 position,direction;};
Start clear_start(const Cache &cache,const LevelContent &content,bool boost) {
  // Select declared initial positions from real BSP floors. This host query
  // supplies only fixture placement; every subsequent movement/contact is
  // produced by the retained original TU1.5 world.
  const CollisionWorld terrain(content.collision);
  std::vector<Vec3> anchors;
  for(const auto &spawn:cache.spawns())anchors.push_back(spawn.position);
  if(boost)for(const auto &triangle:content.collision) {
    const auto normal=cross(triangle.b-triangle.a,triangle.c-triangle.a);
    if(length(normal)<.5f || std::abs(normalized(normal).z)<.99f)continue;
    const auto center=(triangle.a+triangle.b+triangle.c)/3.f;
    if(std::any_of(cache.spawns().begin(),cache.spawns().end(),[&](const auto &spawn) {
      return length(center-spawn.position)<8.f && std::abs(center.z-spawn.position.z)<content.player.height*.5f;
    }))anchors.push_back(center);
  }
  for(const auto anchor:anchors)for(unsigned direction=0;direction<8;++direction) {
    const float angle=float(direction)*.7853981633974483f;
    const Vec3 forward{std::cos(angle),std::sin(angle),0};
    const Vec3 side{-forward.y,forward.x,0};
    const auto origin=terrain.raycast(anchor+Vec3{0,0,1},{0,0,-1},3);
    if(!origin || origin->normal.z<.99f)continue;
    bool clear=true;
    // The lower player moves to the positive side. Survey that route and
    // the full jump arc without requiring unused floor on the other side.
    for(int x=-2;x<=12 && clear;++x)for(int y=boost?-2:-3;y<=(boost?7:3) && clear;++y) {
      const auto p=origin->position+forward*(float(x)*.25f)+side*(float(y)*.25f);
      const auto floor=terrain.raycast(p+Vec3{0,0,.5f},{0,0,-1},1);
      clear=floor && floor->normal.z>.99f && std::abs(floor->position.z-origin->position.z)<.02f &&
          !terrain.overlaps(p+Vec3{0,0,.05f},content.player.height*(boost?4.f:1.f),content.player.radius);
    }
    if(clear)return {origin->position+Vec3{0,0,.05f},forward};
  }
  throw FormatError("No clear authored BSP floor for the player collision fixture");
}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  if(argc!=4)throw FormatError("Supply prepared MCC map, collision case and representation pair");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const Cache cache(argv[1]);check(cache.is_mcc(),"Player collision acceptance requires prepared MCC assets");
  const auto content=cache.load_content(0,true);
  const std::string scenario=argv[2];
  const bool boost=scenario=="boost-standing" || scenario=="boost-crouched" ||
      scenario=="boost-lan" || scenario=="boost-lan-crouched";
  const bool lan=scenario=="lan-head-on" || scenario=="boost-lan" || scenario=="boost-lan-crouched";
  const bool lower_crouched=scenario=="crouched" || scenario=="boost-crouched" || scenario=="boost-lan-crouched";
  const bool teams=scenario=="teammate" || scenario=="enemy" || boost;
  const bool unsupported=scenario=="stacked-beside";
  const bool stacked=scenario=="stacked" || unsupported;
  check(scenario=="stationary" || scenario=="head-on" || scenario=="crouched" || scenario=="beside" || lan || teams || stacked,
      "Unknown player collision case");
  const std::string representations=argv[3];
  check(representations.size()==2 && representations[0]>='2' && representations[0]<='3' &&
      representations[1]>='2' && representations[1]<='3',"Invalid multiplayer representation pair");
  const auto start=clear_start(cache,content,boost);
  Files files;MultiplayerObjectRuntime world(cache,0,files.path);
  UiVariant variant{"Slayer",0,{{89,5}}};
  if(teams)variant.values.append(91,1);
  auto rules=slayer_rules(variant,teams);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  rules.random_seed=0x15c011u;
  world.configure_session(source_game_variant(variant),1,30,rules.random_seed);
  MultiplayerMatch match(content,0,cache.spawns(),rules,{}, {},&world);
  check(match.source_engine_owned(),"Player collision test did not select the original source match engine");
  std::array<unsigned,2> slots;
  std::array<std::uint32_t,2> units;
  std::array<Controls,multiplayer_capacity> controls{};
  std::unique_ptr<LanHost> host;
  std::unique_ptr<LanClient> client;
  unsigned exact_snapshots{};
  for(unsigned i=0;i<2;++i) {
    auto configuration=source_default_player_configuration();configuration[0x44]=representations[i]-'0';
    if(teams)configuration[0x7c]=scenario=="enemy"?i:0;
    if(lan && i==1) {
      host=std::make_unique<LanHost>(match,0,0x15c011u);
      const std::array<SourcePlayerConfiguration,1> profiles{configuration};
      const std::array<std::int8_t,1> selected_teams{0};
      client=std::make_unique<LanClient>(host->join_address(),0x15c011u,1,
          teams?std::span<const std::int8_t>(selected_teams):std::span<const std::int8_t>{},profiles);
      std::array<Controls,1> remote{};
      for(unsigned tick=0;tick<30 && !client->ready();++tick) {
        client->send(remote);host->receive(controls);host->broadcast();client->receive();
      }
      check(client->ready() && client->error().empty(),"Collision LAN participant did not join");
      slots[i]=client->slots().front();
    } else slots[i]=match.join("Collision local "+std::to_string(i),teams?
        (scenario=="enemy"?std::int8_t(i):std::int8_t(0)):std::int8_t(-1),configuration);
  }
  std::vector<FirstPersonBindingDefinition> views;
  const auto local_count=lan?1u:2u;
  for(unsigned i=0;i<local_count;++i)if(i==0 || representations[i]!=representations[0])
    views.push_back(cache.load_first_person_binding(rules.weapon,representations[i]-'0'));
  match.configure_local_views(std::span(slots).first(local_count),views);
  const auto step=[&] {
    if(client) {
      // The receiver's 128-assembly/second admission bound is intentional.
      // Exercise the real 30 Hz session cadence rather than flooding it with
      // hundreds of accelerated test ticks in the same wall-clock second.
      std::this_thread::sleep_for(std::chrono::duration<float>(tick_seconds));
      const std::array<Controls,1> remote{controls[slots[1]]};
      client->send(remote);host->receive(controls);
    }
    match.step(controls);
    if(client) {
      const auto authoritative=match.state();
      host->broadcast();
      // Receive is deliberately nonblocking. Let the real UDP fragments
      // arrive without advancing the source world or replacing any state.
      const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
      do {
        client->receive();
        if(client->state().tick==authoritative.tick || !client->error().empty())break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }while(std::chrono::steady_clock::now()<deadline);
      check(client->error().empty(),"Player collision LAN snapshot was rejected");
      check(client->state().tick==authoritative.tick,"Player collision LAN snapshot is stale");
      for(const auto slot:slots)check(!std::memcmp(&client->state().players[slot].position,
          &authoritative.players[slot].position,sizeof(Vec3)),"Player contact position diverged over LAN");
      ++exact_snapshots;
    }
  };
  for(unsigned tick=0;tick<32 && (match.player(slots[0]).source_current_object==null_datum ||
      match.player(slots[1]).source_current_object==null_datum);++tick)step();
  // The native look adapter binds its first input to the source-created
  // heading. Establish that input owner before requesting the fixture turn.
  for(const auto slot:slots) {
    controls[slot].yaw=match.unit(slot).yaw;
    controls[slot].pitch=match.unit(slot).pitch;
  }
  step();
  const Vec3 side{-start.direction.y,start.direction.x,0};
  constexpr float separation=1.5f;
  const float lane=scenario=="beside"?content.player.radius*3.f:0.f;
  for(unsigned i=0;i<2;++i) {
    units[i]=match.player(slots[i]).source_current_object;
    check(units[i]!=null_datum,"Source match player biped birth failed");
    check(world.object_definition(units[i])==multiplayer_biped(content,representations[i]-'0').datum,
        "Collision participant lost its selected Spartan/Elite biped");
    const auto position=start.position+(stacked?Vec3{0,0,i?content.player.height+.05f:0.f}+side*(i && unsupported?content.player.radius*3.f:0.f):
        start.direction*(i?separation:0.f)+side*(i?lane:0.f));
    // Declared pre-test placement through the original B95D0 owner. Once
    // settling starts, only ordinary Controls and the original match move it.
    world.tu15_world().set_object_position(units[i],position);
    controls[slots[i]].yaw=std::atan2(start.direction.y,start.direction.x)+(i?3.141592653589793f:0.f);
    controls[slots[i]].crouch=i && lower_crouched;
  }
  for(unsigned tick=0;tick<30;++tick)step();
  if(lower_crouched)check(world.biped_motion(units[1]).crouch>.5f,
      "Crouched player collision case did not enter the original crouch state");
  if(teams)check((match.player(slots[0]).source_membership==match.player(slots[1]).source_membership)==
      (scenario!="enemy"),"Collision participants lost their selected team relationship");
  const auto initial=world.biped_motion(units[0]).position;
  const auto target=world.biped_motion(units[1]).position;
  const auto progress=[&](Vec3 p){return dot(p-initial,start.direction);};
  std::cout<<nlohmann::json{{"scenario",scenario},{"initial",{initial.x,initial.y,initial.z}},
      {"target",{target.x,target.y,target.z}},{"direction",{start.direction.x,start.direction.y}},
      {"radius",content.player.radius},{"source_units",units}}.dump()<<'\n';
  if(boost) {
    check(progress(target)>separation*.8f && progress(target)<separation*1.2f &&
        std::abs(target.z-initial.z)<content.player.radius,
        "Boost participants did not settle at the declared separate floor starts");
    const auto &upper=multiplayer_biped(content,representations[0]-'0');
    const auto &lower=multiplayer_biped(content,representations[1]-'0');
    const float support_height=lower_crouched?lower.crouch_height:lower.height;
    const float contact_radius=upper.radius+lower.radius;
    auto &input=controls[slots[0]];
    unsigned mounted_ticks{},launches{},landings{};
    nlohmann::json phases=nlohmann::json::array();
    const auto sample=[&](const char *phase,unsigned tick) {
      const auto a=world.biped_motion(units[0]),b=world.biped_motion(units[1]);
      const auto state=match.state();
      const auto support=Bytes(world.memory().view(world.object_body(units[0])+0x448u,4)).u32(0);
      const nlohmann::json row{{"phase",phase},{"tick",tick},{"match_tick",state.tick},
        {"upper",{a.position.x,a.position.y,a.position.z}},{"lower",{b.position.x,b.position.y,b.position.z}},
        {"velocity",{a.velocity.x,a.velocity.y,a.velocity.z}},
        {"grounded",state.players[slots[0]].native_grounded},{"source_support_owner",support}};
      std::cout<<row.dump()<<'\n';return row;
    };
    const auto on_head=[&] {
      const auto a=world.biped_motion(units[0]),b=world.biped_motion(units[1]);
      const auto d=a.position-b.position;
      const auto support=Bytes(world.memory().view(world.object_body(units[0])+0x448u,4)).u32(0);
      return support==units[1] && match.state().players[slots[0]].native_grounded && std::hypot(d.x,d.y)<contact_radius &&
          d.z>support_height*.8f && d.z<support_height*1.2f;
    };
    const auto mount=[&](unsigned repetition) {
      unsigned supported{},jump_ticks{};bool requested=false;
      input.jump=false;
      for(unsigned tick=0;tick<180;++tick) {
        const auto a=world.biped_motion(units[0]),b=world.biped_motion(units[1]);
        Vec3 delta=b.position-a.position;delta.z=0;
        const auto distance=length(delta);
        if(distance>.01f) {
          const auto direction=delta/distance;
          input.yaw=std::atan2(direction.y,direction.x);
          if(!requested) {
            // Approach at walking speed and release the stick before
            // takeoff. Air braking is much weaker than ground braking.
            input.forward=distance>contact_radius+.1f?
                std::clamp((distance-contact_radius-.1f)*2.f-dot(a.velocity,direction)*.6f,0.f,.4f):0.f;
          } else input.forward=distance>.15f?.2f:0.f;
        } else input.forward=0;
        if(!requested && distance<contact_radius+.17f &&
            std::hypot(a.velocity.x,a.velocity.y)<.08f && match.state().players[slots[0]].native_grounded) {
          requested=true;jump_ticks=3;++launches;
        }
        input.jump=jump_ticks>0;if(jump_ticks)--jump_ticks;
        step();
        check(world.object_vitality(units[0]).body>0 && world.object_vitality(units[1]).body>0,
            "Boost mounting left its living player scenario");
        if(tick%15==0)sample("mount",tick);
        if(on_head()) {
          input.forward=0;
          if(++supported>=20) {
            mounted_ticks+=supported;++landings;
            auto row=sample("mounted",repetition);row["repetition"]=repetition;phases.push_back(row);return;
          }
        } else supported=0;
      }
      sample("mount_failed",repetition);
      throw FormatError("Ordinary player jump did not mount and sustain teammate head support");
    };
    const auto leave=[&](bool jump) {
      const auto before=world.biped_motion(units[0]).position;
      input.yaw=std::atan2(start.direction.y,start.direction.x);input.forward=1;input.jump=jump;
      float rise{};bool left=false,landed=false;
      for(unsigned tick=0;tick<120;++tick) {
        step();input.jump=false;
        check(world.object_vitality(units[0]).body>0 && world.object_vitality(units[1]).body>0,
            "Player died during support release or boost landing");
        const auto a=world.biped_motion(units[0]),b=world.biped_motion(units[1]);
        rise=std::max(rise,a.position.z-before.z);
        left|=!on_head();
        if(tick>=15)input.forward=0;
        if(tick%15==0)sample(jump?"jump_off":"walk_off",tick);
        if(left && match.state().players[slots[0]].native_grounded &&
            a.position.z-b.position.z<support_height*.5f) {landed=true;break;}
      }
      input.forward=0;
      check(landed,"Player did not release head support and land on the original floor");
      if(jump)check(rise>upper.radius,"Jumping from player support produced no upward launch");
      auto row=sample(jump?"jump_off_landed":"walk_off_landed",0);row["rise"]=rise;phases.push_back(row);
    };
    mount(0);leave(true);mount(1);leave(true);mount(2);
    const auto carrier_before=world.biped_motion(units[1]).position;
    controls[slots[1]].yaw=std::atan2(side.y,side.x);controls[slots[1]].forward=1;
    for(unsigned tick=0;tick<20;++tick) {
      step();
      check(world.object_vitality(units[0]).body>0 && world.object_vitality(units[1]).body>0,
          "Player died while the supporting teammate moved");
      if(tick%5==0)sample("support_moves",tick);
    }
    controls[slots[1]].forward=0;
    check(length(world.biped_motion(units[1]).position-carrier_before)>lower.radius,
        "Support-moving case did not move the actual lower player");
    phases.push_back(sample("support_moved",0));
    leave(false);
    std::cout<<nlohmann::json{{"scenario",scenario},{"map",cache.name()},{"representations",representations},
        {"support_height",support_height},{"mount_landings",landings},{"jump_requests",launches},
        {"supported_ticks",mounted_ticks},{"exact_lan_snapshots",exact_snapshots},{"phases",phases}}.dump()<<'\n';
    std::cout<<"PASS TU1.5 player boost "<<scenario<<'\n';return 0;
  }
  if(unsupported)check(std::abs(target.z-initial.z)<content.player.height*.2f,
      "Unsupported upper player did not fall to the original floor");
  else if(stacked)check(target.z-initial.z>content.player.height*.8f,
      "Upper player fell through the lower player's body while settling");
  else check(progress(target)>separation*.8f,"Declared players did not settle in separate initial positions");
  if(!stacked)controls[slots[0]].forward=1;
  if(scenario=="head-on" || lan || teams)controls[slots[1]].forward=1;
  float minimum_distance=1000.f,maximum_progress=0.f,target_travel=0.f;
  unsigned contact_ticks=0;
  for(unsigned tick=0;tick<90;++tick) {
    step();
    const auto a=world.biped_motion(units[0]),b=world.biped_motion(units[1]);
    check(std::isfinite(a.position.x) && std::isfinite(a.position.y) && std::isfinite(a.position.z) &&
          std::isfinite(b.position.x) && std::isfinite(b.position.y) && std::isfinite(b.position.z),
          "Player contact produced a non-finite position");
    check(world.object_vitality(units[0]).body>0 && world.object_vitality(units[1]).body>0,
          "Player collision fixture left its living contact scenario");
    const auto delta=b.position-a.position;
    const auto distance=std::hypot(delta.x,delta.y);
    minimum_distance=std::min(minimum_distance,distance);
    maximum_progress=std::max(maximum_progress,progress(a.position));
    target_travel=std::max(target_travel,length(b.position-target));
    if(tick%15==0)std::cout<<nlohmann::json{{"tick",tick},{"a",{a.position.x,a.position.y,a.position.z}},
        {"b",{b.position.x,b.position.y,b.position.z}},{"distance",distance},
        {"crouch",b.crouch}}.dump()<<'\n';
    if(unsupported) {
      check(std::abs(b.position.z-a.position.z)<content.player.height*.2f,
          "Player retained head support beside the other body");
    } else if(stacked) {
      check(b.position.z-a.position.z>content.player.height*.8f,
          "Stacked player fell through the other player's body");
      check(b.position.z-a.position.z<content.player.height*1.2f,
          "Stacked collision did not settle on the actual lower player");
      ++contact_ticks;
    } else if(scenario!="beside") {
      check(dot(delta,start.direction)>0,"Player walked through the other player's body");
      check(distance>content.player.radius,"Player bodies substantially interpenetrated");
      if(distance<content.player.radius*2.5f)++contact_ticks;
    }
  }
  const nlohmann::json report{{"scenario",scenario},{"map",cache.name()},{"representations",representations},
    {"radius",content.player.radius},{"initial_separation",progress(target)},
    {"minimum_horizontal_distance",minimum_distance},{"maximum_progress",maximum_progress},
    {"target_travel",target_travel},{"contact_ticks",contact_ticks},{"exact_lan_snapshots",exact_snapshots}};
  std::cout<<report.dump()<<'\n';
  if(!stacked)check(maximum_progress>.25f,"Player input did not approach the other body");
  if(scenario=="beside")check(maximum_progress>separation+content.player.radius,
      "Player beside another body was incorrectly blocked");
  else if(!unsupported)check(contact_ticks>=10,"Player input did not sustain actual body contact");
  std::cout<<"PASS TU1.5 player collision "<<scenario<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
