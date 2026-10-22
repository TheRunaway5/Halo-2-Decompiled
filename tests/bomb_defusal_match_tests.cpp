#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char *message){if(!v)throw FormatError(message);}
LevelContent arena() {
  LevelContent c;c.player.datum=1;c.player.height=1;c.player.crouch_height=.6f;c.player.radius=.2f;
  c.player.camera_height=.8f;c.player.crouch_camera_height=.5f;c.player.crouch_velocity=2;
  c.player.jump_velocity=3;c.player.vitality.health=100;c.player.vitality.shield=100;
  c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.movement={2,2,2,10,1,1,1,5,2,1};
  WeaponDefinition w;w.datum=3;w.instantaneous=true;w.capacity=20;w.initial_rounds=120;w.reserve=100;
  w.reload_rounds=20;w.reload_time=1;w.rounds_per_shot=1;w.minimum_rounds=1;w.rounds_per_second=3;w.burst=1;
  w.range=50;w.damage_min=10;w.projectiles_per_shot=1;c.weapons.emplace(3,w);
  w.datum=4;c.weapons.emplace(4,w);w.datum=5;w.pickup_priority=1;c.weapons.emplace(5,w);
  for(unsigned i=0;i<2;++i){NetgameFlag f;f.source_index=i;f.type=3;f.selector12=1;f.position={0,float(i*10),0};c.netgame_flags.push_back(f);}
  return c;
}
struct Fixture {
  LevelContent content=arena();
  std::array<Spawn,multiplayer_capacity> starts;
  SlayerRules rules;
  std::unique_ptr<MultiplayerMatch> match;
  std::array<Controls,multiplayer_capacity> controls{};
  Fixture(bool shared_starts=false) {
    for(unsigned i=0;i<starts.size();++i){starts[i].position={i%2?10.f:-10.f,float(i/2)*3,0};starts[i].team=int(i%2);}
    if(shared_starts) {
      for(auto &start:starts)start.team=8;
      for(unsigned team=0;team<2;++team){NetgameFlag home;home.type=2;home.selector12=std::int16_t(team);home.position={team?10.f:-10.f,0,0};content.netgame_flags.push_back(home);}
    }
    rules=slayer_rules({"Bomb Defusal",5,{{2,13},{10,150},{16,1},{89,3},{94,4},{native_defusal_parameter,1}}},true);
    rules.weapon=3;rules.weapon_choices.fill(null_datum);rules.weapon_choices[3]=3;rules.weapon_choices[0]=4;
    rules.defusal->freeze_ticks=1;rules.defusal->intermission_ticks=1;
    match=std::make_unique<MultiplayerMatch>(content,0,starts,rules);match->configure_weapon_pickups();
    for(unsigned i=0;i<4;++i)match->join("Player "+std::to_string(i),std::int8_t(i%2));
  }
  void ticks(unsigned n){while(n--)match->step(controls);}
  void start(){ticks(2);check(match->state().defusal->phase==BombDefusalPhase::live,"Actual Match did not start live round");}
  void place(unsigned i,Vec3 p){auto &u=const_cast<Unit&>(match->unit(i));u.body.position=p;u.body.velocity={};u.body.grounded=true;}
};
void flow() {
  Fixture f;f.start();auto state=f.match->state();const auto carrier=state.defusal->carrier&65535u;
  check(state.source_engine==9 && state.defusal->sites.size()==2,"Match did not publish Assault extension and both sites");
  const auto &inventory=f.match->unit(carrier).source_inventory;
  check(state.players[carrier].weapon==3 && inventory.inactive_weapons[1] && inventory.inactive_weapons[1]->definition==4,
      "Equal rifle/pistol loadout was not constructed");
  check(state.players[carrier].grenades==std::array<std::uint8_t,2>{1,0},"Defusal spawn lacks its single frag grenade");
  f.place(carrier,{0,10,0});f.controls[carrier].pickup=true;
  f.ticks(f.rules.defusal->plant_ticks);state=f.match->state();
  check(state.defusal->phase==BombDefusalPhase::planted && state.defusal->site==1,"Normal held USE failed to plant at site B");
  check(std::ranges::any_of(state.world_weapons,[](const auto &w){return w.definition==5;}),"Planted bomb absent from actual world presentation");
  f.controls[carrier].pickup=false;
  f.place(1,{0,10,0});f.controls[1].pickup=true;f.ticks(f.rules.defusal->defuse_ticks);
  state=f.match->state();check(state.defusal->outcome==BombDefusalOutcome::defused && state.team_scores[1]==1,"Normal held USE failed to award defending round");
  const auto old=f.match->player(1).source_current_object;f.ticks(1);
  check(f.match->player(1).source_current_object!=old && !f.match->state().players[1].dead,"New round failed to construct fresh player generation");
  check(f.match->state().players[1].deaths==0 && f.match->state().team_scores[1]==1,"Round restart changed death statistics or erased team score");
}
void life_and_join() {
  Fixture f;f.start();const auto dead=0u;auto &u=const_cast<Unit&>(f.match->unit(dead));u.vitality.dead=true;u.vitality.body=0;
  f.ticks(60);check(f.match->state().players[dead].dead && f.match->player(dead).source_current_object==null_datum,"Eliminated player respawned during live round");
  const auto joined=f.match->join("Late",0);f.ticks(5);check(f.match->state().players[joined].dead,"Late join bypassed one-life round roster");
  check(f.match->state().team_scores[0]==0 && f.match->state().team_scores[1]==0,"A kill awarded an objective point");
}
void transport() {
  Fixture f(true);
  for(unsigned i=0;i<6;++i)f.match->join("Big Team Red "+std::to_string(i),0);
  const auto available=f.match->available_player_slots();bool rejected{};
  try{f.match->join("Ninth Red",0);}catch(const FormatError &){rejected=true;}
  check(rejected && f.match->available_player_slots()==available,"Ninth attacker was admitted or consumed a player slot");
  for(unsigned i=0;i<5;++i)f.match->join("Big Team Blue "+std::to_string(i),1);
  LanHost host(*f.match,0,0x44454655);
  std::array<std::int8_t,1> teams{1};LanClient client("127.0.0.1:"+std::to_string(host.port()),0x44454655,1,teams);
  std::array<Controls,1> local{};
  for(unsigned n=0;n<100 && !client.ready();++n){client.send(local);host.receive(f.controls);host.broadcast();client.receive();}
  check(client.ready() && client.error().empty(),"Defusal client failed actual native UDP admission");
  check(f.match->available_player_slots()==0,"Big Team UDP join did not fill the sixteenth player slot");
  rejected=false;try{f.match->join("Ninth Blue",1);}catch(const FormatError &){rejected=true;}
  check(rejected,"Ninth defender was admitted");
  f.start();
  for(unsigned n=0;n<30;++n){host.broadcast();client.receive();if(client.state().tick==f.match->state().tick)break;}
  check(client.state().defusal && client.state().defusal_rules && client.state().defusal->sites.size()==2 &&
      client.state().defusal->carrier==f.match->state().defusal->carrier && client.state().defusal_rules->defuse_ticks==180,
      "Defusal sites, carrier or timing rules lost in snapshot transport");
  const auto &state=client.state();std::array<unsigned,2> teams_alive{};
  for(const auto &p:state.players) {
    check(p.connected && !p.dead && p.source_membership>=0 && p.source_membership<2,
        "Big Team snapshot omitted or failed to spawn a participant");
    ++teams_alive[unsigned(p.source_membership)];
  }
  check(teams_alive==std::array<unsigned,2>{8,8},"Big Team snapshot did not retain an 8v8 roster");
  const auto host_state=f.match->state();
  for(unsigned i=0;i<multiplayer_capacity;++i) {
    check(host_state.defusal->roster[i]!=null_datum,"Big Team round roster omitted a full player identity");
    for(unsigned j=0;j<i;++j)check(length(host_state.players[i].position-host_state.players[j].position)>.5f,
        "Big Team players shared an opening spawn");
  }
}
void complete_match() {
  Fixture f;f.start();
  for(unsigned round=1;round<=13;++round) {
    for(unsigned slot:{0u,2u}){auto &u=const_cast<Unit&>(f.match->unit(slot));u.vitality.dead=true;u.vitality.body=0;}
    f.ticks(1);const auto state=f.match->state();
    check(state.team_scores[1]==int(round),"Native round score repeated or disappeared");
    if(round<13){f.ticks(2);check(f.match->state().defusal->phase==BombDefusalPhase::live,"Native next round failed to become live");}
    if(round==12)check(f.match->state().players[1].position.x<0 && f.match->state().defusal->attacking_team==1,
        "Native halftime failed to exchange physical spawn sides");
  }
  const auto state=f.match->state();
  check(state.finished && state.results_finalized && state.winner_team==1 && state.team_scores[1]==13,
      "Native first-to-13 did not finalize the winning team and results");
}
void shared_map_starts() {
  Fixture f(true);f.start();const auto state=f.match->state();
  check(state.players[0].position.x<0 && state.players[1].position.x>0 &&
      length(state.players[0].position-state.players[2].position)>.5f && length(state.players[1].position-state.players[3].position)>.5f,
      "Shared map starts failed to form distinct Assault-home spawn groups");
}
void depleted_pickup_reset() {
  Fixture f;f.start();auto &dead=const_cast<Unit&>(f.match->unit(0));dead.vitality.dead=true;dead.vitality.body=0;f.ticks(1);
  const auto pickups=f.match->weapon_pickups();check(!pickups.empty(),"Fatal boundary did not drop inventory");
  // Completed ammunition depletion retires the object but retains its empty
  // pickup row. Round cleanup must not release that full generation again.
  auto &depleted=const_cast<NativeWeaponPickup&>(pickups.front());depleted.weapon.reset();
  const_cast<NativeObjectLifetimes&>(f.match->simulation().object_lifetimes()).release(depleted.handle);
  auto &last=const_cast<Unit&>(f.match->unit(2));last.vitality.dead=true;last.vitality.body=0;f.ticks(3);
  check(f.match->state().defusal->phase==BombDefusalPhase::live,"Retired pickup row broke next round cleanup");
}
}
int main() {
  unsigned failures{};
  for(const auto &[name,test]:std::array<std::pair<const char*,void(*)()>,6>{{
      {"plant/defuse/reset",flow},{"one life/late joins",life_and_join},{"8v8 admission/spawns/UDP",transport},{"full match/halftime",complete_match},{"shared map starts",shared_map_starts},{"depleted pickup reset",depleted_pickup_reset}}})
    try{test();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception &e){++failures;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}
  return failures?1:0;
}
