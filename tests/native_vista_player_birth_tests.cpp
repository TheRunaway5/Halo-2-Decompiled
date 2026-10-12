#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/vista_player_birth.hpp"
#include "halo2/vista_slayer_publication.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}
  check(rejected,"Invalid native birth/activation policy was silently accepted");}
LevelContent arena(){LevelContent c;c.player.datum=1;c.player.height=1;c.player.crouch_height=.6f;
  c.player.radius=.2f;c.player.camera_height=.8f;c.player.crouch_camera_height=.5f;
  c.player.crouch_velocity=2;c.player.jump_velocity=3;c.player.vitality.health=100;
  c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.movement={2,2,2,10,1,1,1,5,2,1};WeaponDefinition w;w.datum=3;w.instantaneous=true;
  w.capacity=20;w.initial_rounds=120;w.reserve=100;w.reload_rounds=20;w.reload_time=1;
  w.rounds_per_shot=1;w.minimum_rounds=1;w.rounds_per_second=30;w.burst=1;w.range=50;
  w.projectiles_per_shot=1;c.weapons.emplace(3,w);return c;}
struct Observed {unsigned births{},updates{},activations{};std::uint32_t datum{};};
void install(MultiplayerMatch& match,Observed& seen){
  match.set_source_player_activation_owner(true,[&](auto& player,unsigned slot){
    check(player.connected && player.source_datum!=null_datum && (player.source_datum&0xffffu)==slot &&
        player.source_current_object==null_datum && !player.source_current_unit && player.source_initial_spawn,
        "Birth callback did not own a genuinely allocated absent-unit player");
    seen.datum=player.source_datum;player.source_flags=native_player_birth_flags(1,0);++seen.births;
  },[&](auto& current){
    ++seen.updates;const auto* player=current.player_from_datum(seen.datum);
    check(player,"Pre-player callback lost the actual allocated player");
    if(!seen.activations)check(player->source_flags==8 && player->source_current_object==null_datum &&
        !player->source_current_unit,"Initial activation ran after spawn");
    const auto update=vista_player_activation_after_query(player->source_flags,1);
    current.apply_source_player_activation(seen.datum,player->source_flags,update.flags);
    for(const auto call:update.call_intents())if(call==VistaPlayerActivationService::engine_activation){
      check(current.player_from_datum(seen.datum)->source_flags==9 &&
          current.player_from_datum(seen.datum)->source_current_object==null_datum,
          "Actual active bit was not written before initial engine publication");++seen.activations;
    }
  });
}
}
int main(int argc,char** argv){try{
  auto content=arena();Spawn spawn;spawn.position={0,0,0};
  auto rules=slayer_rules({"Slayer",0,{}});rules.weapon=3;
  MultiplayerMatch ordinary(content,0,{&spawn,1},rules);const auto ordinary_slot=ordinary.join("Ordinary");
  check(ordinary.player(ordinary_slot).source_current_object!=null_datum &&
      ordinary.player(ordinary_slot).source_flags==1,"Unbound ordinary join changed");
  MultiplayerMatch match(content,0,{&spawn,1},rules);Observed seen;install(match,seen);
  const auto slot=match.join("Actual allocated birth");
  check(seen.births==1 && seen.activations==0 && match.player(slot).source_flags==8 &&
      match.player(slot).source_current_object==null_datum,"Deferred join fabricated activation or initial unit");
  rejects([&]{match.apply_source_player_activation(seen.datum^0x10000u,8,9);});
  rejects([&]{match.apply_source_player_activation(seen.datum,1,9);});
  rejects([&]{match.apply_source_player_activation(seen.datum,8,13);});
  rejects([&]{install(match,seen);});
  std::array<Controls,multiplayer_capacity> controls{};match.step(controls);
  check(seen.updates==1 && seen.activations==1 && match.player(slot).source_flags==1 &&
      match.player(slot).source_current_object!=null_datum && !match.player(slot).source_initial_spawn,
      "First native step did not activate before successful spawn continuation");
  match.step(controls);check(seen.updates==2 && seen.activations==1,"Steady active player repeated initial activation");
  MultiplayerMatch failed(content,0,{&spawn,1},rules);
  failed.set_source_player_activation_owner(true,[](auto&,unsigned){throw FormatError("deliberate birth failure");},[](auto&){});
  const auto before=failed.available_player_slots();rejects([&]{failed.join("Rejected birth");});
  check(failed.available_player_slots()==before,"Rejected birth retained a connected/native allocated player");
  if(argc==4 && std::filesystem::exists(argv[1]) && std::filesystem::exists(argv[3])) {
    MultiplayerOptions options;options.map=std::filesystem::path(argv[1])/"ascension.map";
    options.shared_maps=argv[1];options.gameplay_xbe=argv[3];options.local_players=1;
    options.selected_profiles.push_back(native_selected_player_configuration("Actual",std::array<std::uint8_t,9>{},-1));
    options.defer_initial_player_spawn=true;Observed world_seen;
    options.player_birth_owner=[&](auto& p,unsigned s){check(p.source_profile_request==options.selected_profiles[0],
        "Prepared world substituted native profile creation input");world_seen.datum=p.source_datum;
      check((p.source_datum&0xffffu)==s && p.source_current_object==null_datum,"Prepared world joined after spawn");
      p.source_flags=8;++world_seen.births;};
    options.pre_player_update_owner=[&](auto& current){const auto* p=current.player_from_datum(world_seen.datum);
      if(!(p->source_flags&1u)){current.apply_source_player_activation(p->source_datum,p->source_flags,p->source_flags|1u);++world_seen.activations;}};
    MultiplayerPreparedWorld world(options);check(world_seen.births==1 && world.local_slots.size()==1 &&
        world.match.player(world.local_slots[0]).source_current_object==null_datum,"Actual Cache world eagerly spawned");
    auto runtime=options;runtime.defer_initial_player_spawn=false;runtime.player_birth_owner={};runtime.pre_player_update_owner={};
    check(world.matches(runtime),"Runtime replacement replayed constructor-only policy");
    world.match.step(controls);check(world_seen.activations==1 &&
        world.match.player(world.local_slots[0]).source_flags==1 &&
        world.match.player(world.local_slots[0]).source_current_object!=null_datum,"Actual Cache world failed first native spawn");
  }
  std::cout<<"Native allocated birth8 -> pre-player activation9 -> successful spawn1 passed; ordinary joins and stale-handle guards passed\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
