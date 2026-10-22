#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include <cstring>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F fn,const char* message) {
  bool rejected=false;try{fn();}catch(const FormatError&){rejected=true;}check(rejected,message);
}
LevelContent arena() {
  LevelContent c;c.player.datum=1;c.player.height=1;c.player.crouch_height=.6f;
  c.player.radius=.2f;c.player.camera_height=.8f;c.player.crouch_camera_height=.5f;
  c.player.crouch_velocity=2;c.player.jump_velocity=3;c.player.vitality.health=100;
  c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.movement={2,2,2,10,1,1,1,5,2,1};
  WeaponDefinition w;w.datum=3;w.instantaneous=true;w.capacity=20;w.initial_rounds=120;
  w.reserve=100;w.reload_rounds=20;w.reload_time=1;w.rounds_per_shot=1;w.minimum_rounds=1;
  // Deliberately lethal even through the source spawn-time .1 multiplier.
  // This synthetic request is not an authored weapon-balance fixture.
  w.rounds_per_second=30;w.burst=1;w.range=50;w.damage_min=4000;w.projectiles_per_shot=1;c.weapons.emplace(3,w);
  for(int hill=0;hill<2;++hill) {
    for(const auto xy:std::array<std::array<float,2>,4>{{{-8,-8},{-8,8},{8,8},{8,-8}}}) {
      SourceKingMarker marker;marker.position={xy[0]+float(hill)*30,xy[1],0};marker.type=std::int16_t(11+hill);
      NetgameFlag row;row.source_index=unsigned(c.netgame_flags.size());
      std::memcpy(row.source_bytes.data(),&marker,sizeof(marker));c.netgame_flags.push_back(row);
    }
    for(const float z:{-2.f,4.f}) {
      SourceKingMarker marker;marker.position={float(hill)*30,0,z};marker.type=std::int16_t(11+hill);marker.channel=1;
      NetgameFlag row;row.source_index=unsigned(c.netgame_flags.size());
      std::memcpy(row.source_bytes.data(),&marker,sizeof(marker));c.netgame_flags.push_back(row);
    }
  }
  return c;
}
struct Events {
  std::vector<SourceKingNotice> notices;
  std::vector<std::array<std::uint32_t,4>> records,changed;
  MultiplayerKingServices services() {
    MultiplayerKingServices services;
    // These observation sinks validate the engine/native consumer boundary.
    // They are deliberately not described as live HUD/audio or enabled recording.
    services.record=[&](const MultiplayerMatch& match,std::uint32_t player,std::int32_t kind,std::uint32_t other) {
      records.push_back({std::uint32_t(match.simulation().ticks()),player,std::uint32_t(kind),other});
    };
    services.score_changed=[&](const MultiplayerMatch& match,std::uint32_t player,std::int32_t score) {
      check(match.player_from_datum(player)!=nullptr,"Score publication has stale identity");
      changed.push_back({std::uint32_t(match.simulation().ticks()),player,std::uint32_t(score),match.finished()});
    };
    services.notification=[&](const MultiplayerMatch&,const SourceKingNotice& notice){notices.push_back(notice);};
    return services;
  }
};
SlayerRules rules(GameVariantSettings values={}) {
  auto r=king_rules({"Authored rules",3,values},true);r.weapon=3;return r;
}
void teleport(MultiplayerMatch& match,unsigned player,Vec3 position) {
  Spawn spawn;spawn.position=position;test::place_player(match,player,spawn);
}
void ticks(MultiplayerMatch& match,unsigned count) {
  std::array<Controls,multiplayer_capacity> controls{};
  while(count--)match.step(controls);
}
void free_for_all() {
  auto c=arena();auto r=rules({{2,2}});Events events;std::array<Spawn,1> spawn{};
  MultiplayerMatch match(c,0,spawn,r,events.services());
  check(match.state().source_engine==4 && match.state().king.has_value(),"King snapshot has no source objective owner");
  check(match.source_engine()==4 && match.source_king_state()->marker_count==4,"King initialization missing at native lifecycle");
  const auto id=match.join("Player");const auto& state=*match.source_king_state();
  // Original-only oracle_king_runtime_boundary.py verifies this interior;
  // the exact center sits on a discrete spline ray crossing and is rejected.
  teleport(match,id,{state.center[0],state.center[1]+.25f,0});
  const auto contained=[&] {
    const auto& player=match.player(id);const auto p=match.unit(id).body.position;
    return source_king_contains(state,{player.source_datum,player.source_current_object,player.source_membership,0},
        [&](std::uint32_t){return std::array<float,3>{p.x,p.y,p.z};});
  };
  check(contained(),"Synthetic hill did not contain its original-verified interior point");
  ticks(match,29);
  if(match.player(id).score!=0 || match.player(id).source_king_inside_ticks!=29) {
    const auto p=match.unit(id).body.position;
    std::cerr<<"King tick29 score="<<match.player(id).score<<" inside="<<match.player(id).source_king_inside_ticks
        <<" position="<<p.x<<','<<p.y<<','<<p.z<<" hill="<<state.center[0]<<','<<state.center[1]<<','<<state.minimum_z<<','<<state.maximum_z
        <<" contains="<<contained()<<" finished="<<match.finished()<<" phase="<<match.source_round_lifecycle().phase<<" status="<<match.source_round_lifecycle().status<<'\n';
  }
  check(match.player(id).score==0 && match.player(id).source_king_inside_ticks==29,"King did not sample before world movement");
  ticks(match,1);check(match.player(id).score==1 && !match.finished(),"King one-second award did not use shared score writer");
  check(events.records.size()==1 && events.changed.size()==1 && events.records[0][0]==29 && events.records[0][2]==4,
      "King recorder/score publication did not use actual native clock");
  ticks(match,30);const auto snapshot=match.state();
  check(match.finished() && snapshot.results_finalized && snapshot.winner==int(id) && snapshot.players[id].score==2 && snapshot.players[id].wins==1,
      "King limit did not run actual end/accumulate/results owners");
  check(events.changed.back()[3]==1,"Score-ending notification did not observe final status");
  ticks(match,30);check(match.player(id).score==2 && match.player(id).source_king_inside_ticks==0,"Game-over King continued scoring/timer growth");
}
void contested_and_moving() {
  auto c=arena();std::array<Spawn,1> spawn{};Events events;
  auto r=rules({{38,1},{40,1},{2,100}});MultiplayerMatch match(c,0,spawn,r,events.services());
  const auto a=match.join("A"),b=match.join("B");const auto center=match.source_king_state()->center;
  teleport(match,a,{center[0],center[1]+.25f,0});teleport(match,b,{center[0],center[1]+.25f,0});
  ticks(match,30);
  check(match.player(a).score==0 && match.player(b).score==0 && events.records.empty(),"Uncontested option was not consumed by King");
  check(match.source_king_state()->hill==1 && match.source_king_state()->move_ticks==30,"Moving hill did not use shared RNG/native world dispatch");
  check(!events.notices.empty() && events.notices.back().type==4,"Relocation notice not delivered");
  ticks(match,1);check(match.player(a).source_king_inside_ticks==0 && match.source_king_state()->occupants==0,"Relocation retained old occupancy");
}
void team_scoring() {
  for(const int multiplier:{0,1}) {
    auto c=arena();std::array<Spawn,1> spawn{};Events events;
    MultiplayerMatch match(c,0,spawn,rules({{91,1},{39,multiplier},{2,100}}),events.services());
    const auto a=match.join("A",0),b=match.join("B",0);const auto center=match.source_king_state()->center;
    teleport(match,a,{center[0],center[1]+.25f,0});teleport(match,b,{center[0],center[1]+.25f,0});ticks(match,30);
    check(match.state().team_scores[0]==1+multiplier,"King shared team score aggregation differs");
    check(match.player(b).score==1 && match.player(a).score==multiplier && match.player(b).source_king_inside_ticks==1,
        "King source tie break/multiplier/timer reset missing from native players");
    check(events.records.empty(),"Team scoring called FFA recording event");
  }
}
void authored_equipment() {
  auto c=arena();std::array<Spawn,1> spawn{};Events events;
  c.multiplayer_starting_equipment_flags=1; // Legacy Slayer-only view.
  c.multiplayer_starting_equipment={ {1,{2,0,0,0}}, {2,{4,0,0,0}}, {0,{12,0,0,0}} };
  MultiplayerMatch match(c,0,spawn,rules(),events.services());const auto id=match.join("Equipment");
  check(match.unit(id).grenades==std::array<std::uint8_t,2>{0,2},"King selected Slayer starting-equipment flags");
  // Declared synthetic resource references satisfy the native weapon gate;
  // this selector test does not request animation/model resource playback.
  c.weapons.at(3).first_person_models.push_back({4,5});
  for(const int mode:{2,4}) {
    NetgameEquipment row;row.modes={std::int16_t(mode),0,0,0};row.position={float(mode)*10,0,0};
    row.respawn_seconds=1;row.items.push_back({1,0x77656170u,3,0});c.netgame_equipment.push_back(row);
  }
  Events pickup_events;MultiplayerMatch pickups(c,0,spawn,rules(),pickup_events.services());
  pickups.configure_weapon_pickups();pickups.join("Pickups");ticks(pickups,2);
  check(pickups.weapon_pickups().size()==1 && pickups.weapon_pickups()[0].position.x==40,
      "King world pickup owner selected source engine2 rows");
}
void combat(const SourceResultStatDescriptors& descriptors) {
  for(const bool team:{false,true}) {
    auto c=arena();auto original=std::make_shared<OriginalData>();
    original->result_stat_descriptors=descriptors;c.original_data=original;c.multiplayer_spawn=SpawnSettings{};
    std::array<Spawn,1> spawn{};Events events;auto r=rules({{2,100},{91,int(team)}});r.respawn_ticks=30;r.time_ticks=0;
    MultiplayerMatch match(c,0,spawn,r,events.services());
    const auto a=match.join("Attacker",team ? 0 : -1),b=match.join("Victim",team ? 1 : -1);
    const auto center=match.source_king_state()->center;
    teleport(match,a,{center[0]-1,center[1]+.25f,0});teleport(match,b,{center[0]+1,center[1]+.25f,0});
    const auto old_unit=match.player(b).source_current_object;
    std::array<Controls,multiplayer_capacity> controls{};controls[a].yaw=0;controls[b].yaw=3.14159265f;controls[a].fire=true;
    match.step(controls);
    check(match.unit(b).vitality.dead && match.player(a).kills==1 && match.player(b).deaths==1,
        "King did not run actual combat/attribution/death ownership");
    check(match.player(a).score==0 && match.player(b).score==0,"King kill incorrectly awarded Slayer points");
    const auto* bank=match.recorded_statistics();check(bank && bank->players[a][27]==1 && bank->players[a][28]==1,
        "King hill kills did not publish actual retained descriptors");
    if(team)check(bank->teams[0][27]==1 && bank->teams[0][28]==1,"King hill stats lost shared team publication");
    controls[a].fire=false;
    for(unsigned n=0;n<100 && match.player(b).source_current_object==null_datum;++n)match.step(controls);
    check(match.player(b).source_current_object!=null_datum && match.player(b).source_current_object!=old_unit,
        "King did not respawn through actual salted native ownership");
  }
}
void damage_traits() {
  struct Case {GameVariantSettings settings;bool inside=true,same_team=false;std::uint8_t strength{};float expected=0.96f;};
  const std::array<Case,9> cases{{
    {{{41,1}},true,false,0,.94f},
    {{{42,1}},true,false,0,.98f},
    {{{41,1},{42,1}},true,false,0,.97f},
    {{{104,1},{105,1}},false,false,0,.97f},
    {{{104,1},{105,1}},true,false,0,.96f},
    {{},true,false,1,.97f},
    {{},true,false,7,.99f},
    {{{91,1},{24,0}},true,true,0,1.f},
    {{{91,1},{24,0},{41,1}},true,false,0,.94f}
  }};
  for(const auto& input:cases) {
    auto c=arena();c.weapons.at(3).damage_min=40;c.player_representation_count=4;
    std::array<Spawn,1> spawn{};Events events;auto r=rules(input.settings);r.time_ticks=0;
    MultiplayerMatch match(c,0,spawn,r,events.services());
    SourcePlayerConfiguration profile{};profile[0x7d]=input.strength;
    if(r.team_mode)profile[0x7c]=0;
    const auto a=match.join("Attacker",r.team_mode ? 0 : -1,profile);
    const auto b=match.join("Victim",r.team_mode ? (input.same_team ? 0 : 1) : -1);
    const auto center=match.source_king_state()->center;
    const float y=center[1]+(input.inside ? .25f : 20.f);
    teleport(match,a,{center[0]-1,y,0});teleport(match,b,{center[0]+1,y,0});
    check(match.player(b).source_field_194==30,"Successful spawn lost the original damage field reset");
    check(match.player(a).source_strength==std::min(input.strength,std::uint8_t(3)),"Combat did not consume sanitized player strength");
    std::array<Controls,multiplayer_capacity> controls{};controls[a].fire=true;controls[b].yaw=3.14159265f;
    match.step(controls);
    check(match.player(b).source_field_194==29,"Source pre-world damage countdown did not precede combat");
    if(std::fabs(match.unit(b).vitality.body-input.expected)>0.000001f) {
      std::cerr<<"Native trait body="<<match.unit(b).vitality.body<<" expected="<<input.expected
          <<" hill="<<r.king_flags<<" global="<<r.player_flags<<'\n';
      throw std::runtime_error("Actual native shot lost shared damage gates or King trait override");
    }
    controls[a].fire=false;
    for(unsigned n=0;n<29;++n)match.step(controls);
    check(match.player(b).source_field_194==0,"Source damage countdown did not expire after30 pre-world calls");
  }
  // Public authored-area route supplies a pre-interpolated event request.
  // Scaling still runs exactly once before vitality, and self identity bypasses
  // both the strength and spawn-field multipliers in the original owner.
  auto c=arena();c.weapons.at(3).damage_min=40;std::array<Spawn,1> spawn{};Events events;
  MultiplayerMatch match(c,0,spawn,rules({{41,1}}),events.services());const auto a=match.join("Area owner");
  PropAreaDamageDefinition area;area.damage=c.weapons.at(3);area.damage.damage_amount=DamageAmountDefinition{40,40,40};
  area.radius_minimum=area.radius_maximum=10;
  const auto& unit=match.unit(a);
  const_cast<Simulation&>(match.simulation()).area_damage(area,unit.body.position,
      {match.player(a).source_datum,unit.source_datum,match.player(a).source_membership},-1);
  check(std::fabs(match.unit(a).vitality.body-.6f)<0.000001f,"Area self damage incorrectly consumed player traits or spawn field");
}
void captured_damage_owner() {
  auto c=arena();c.player_representation_count=4;c.weapons.at(3).damage_min=40;
  std::array<Spawn,1> spawn{};Events events;auto r=rules({{41,1}});r.time_ticks=0;
  MultiplayerMatch match(c,0,spawn,r,events.services());
  SourcePlayerConfiguration profile{};profile[0x7d]=2;
  const auto a=match.join("Captured owner",-1,profile),b=match.join("Current victim");
  const auto center=match.source_king_state()->center;
  teleport(match,a,{center[0]-1,center[1]+.25f,0});teleport(match,b,{center[0]+1,center[1]+.25f,0});
  ticks(match,1);
  const DamageOwner captured{match.player(a).source_datum,match.unit(a).source_datum,match.player(a).source_membership};
  const auto old_inside=match.player(a).source_king_inside_ticks;
  const auto old_countdown=match.player(a).source_field_194;
  match.leave(a);
  check(!match.player_from_datum(captured.player) && !match.player(a).connected &&
      match.player(a).source_strength==2 && match.player(a).source_membership==captured.designator &&
      match.player(a).source_king_inside_ticks==old_inside && match.player(a).source_field_194==old_countdown,
      "Source player release discarded raw damage fields before slot reuse");
  PropAreaDamageDefinition area;area.damage=c.weapons.at(3);area.damage.damage_amount=DamageAmountDefinition{40,40,40};
  area.radius_minimum=area.radius_maximum=10;
  auto& simulation=const_cast<Simulation&>(match.simulation());
  simulation.area_damage(area,match.unit(b).body.position,captured,-1);
  check(std::fabs(match.unit(b).vitality.body-.97f)<0.000001f,
      "Captured damage did not consume departed slot strength and retained King timer");
  test::restore_vitality(match,b);
  profile[0x7d]=3;const auto replacement=match.join("Reused slot",-1,profile);
  check(replacement==a && match.player(a).source_datum!=captured.player &&
      !match.player_from_datum(captured.player) && match.player(a).source_strength==3 &&
      match.player(a).source_king_inside_ticks==0,"Source slot reallocation did not reset raw damage inputs");
  // The old captured full player is not self-equal to the new generation.
  // The original damage consumer nevertheless reads this new slot's fields.
  simulation.area_damage(area,match.unit(b).body.position,captured,-1);
  check(std::fabs(match.unit(b).vitality.body-.99f)<0.000001f,
      "Captured old-salt damage did not consume the reallocated slot's strength");
  area.damage.damage_amount=DamageAmountDefinition{4000,4000,4000};
  simulation.area_damage(area,match.unit(b).body.position,captured,-1);
  check(match.unit(b).vitality.dead && match.player(b).deaths==1 && match.player(a).kills==0,
      "Captured stale attacker passed the separate checked fatal-attribution consumer");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the original executable for retained descriptors");
  rejects([]{slayer_rules({"King",3,{}});},"Incomplete normal King menu admission opened");
  rejects([]{king_rules({"Slayer",0,{}});},"King accepted wrong source variant type");
  rejects([]{king_rules({"Unbound camouflage",3,{{101,1}}});},"Unbound camouflage admitted");
  check(king_rules({"Damage trait alias",3,{{30,1}}}).king_flags==4,"Original shared field alias lost King damage capability");
  check(king_rules({"Field alias",3,{{29,1}}}).king_flags==2,"King lost original overlapping parameter bits");
  auto c=arena();std::array<Spawn,1> spawn{};
  rejects([&]{MultiplayerMatch match(c,0,spawn,rules());},"Native King invented missing consumers");
  free_for_all();contested_and_moving();team_scoring();authored_equipment();damage_traits();captured_damage_owner();
  const auto original=OriginalData::load_xbe(argv[1]);check(original->result_stat_descriptors.has_value(),"Original retained descriptors unavailable");
  combat(*original->result_stat_descriptors);
  std::cout<<"Native King lifecycle/occupancy/FFA/team/movement/end/results passed; normal menu/presentation remain gated\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
