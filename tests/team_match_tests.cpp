#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const std::string &message) {if(!value)throw FormatError(message);}
template<class F> void rejects(F action,const char *message) {
  bool rejected=false;try {action();}catch(const FormatError &){rejected=true;}
  check(rejected,message);
}
Unit &unit(MultiplayerMatch &match,unsigned id) {return const_cast<Unit &>(match.unit(id));}
SlayerRules rules(Cache &cache,int scoring=0) {
  auto result=slayer_rules({"Team Slayer",0,{{2,50},{91,1},{89,5},{23,scoring}}},true);
  result.weapon=cache.slayer_starting_weapon(5);result.score_limit=result.time_ticks=0;
  return result;
}
void join_four(MultiplayerMatch &match) {
  for(unsigned id=0;id<4;++id) {
    check(match.join("Source team",std::int8_t(id/2))==id,"Native team slot allocation differs");
    check(match.player(id).source_membership==int(id/2) &&
        match.unit(id).source_membership==int(id/2) && match.player(id).source_ffa_identity==id,
        "Source team membership was conflated with FFA identity");
  }
}
// Explicit non-SMG death boundary used by the older integration suite too.
// The separate projectile cases below enter through production SMG damage.
void death(MultiplayerMatch &match,unsigned victim,int credited) {
  unit(match,victim).vitality.dead=true;
  unit(match,victim).last_damager=credited<0 ? -2 : credited-1;
}
void compare(const MultiplayerState &expected,const MultiplayerState &actual) {
  check(actual.tick==expected.tick && actual.finished==expected.finished &&
      actual.winner==expected.winner && actual.team_mode==expected.team_mode &&
      actual.retained_team_mask==expected.retained_team_mask &&
      actual.winner_team==expected.winner_team && actual.team_scores==expected.team_scores &&
      actual.results_finalized==expected.results_finalized && actual.team_total_scores==expected.team_total_scores &&
      actual.team_wins==expected.team_wins,
      "Actual UDP team scores, retained mask or completion differ");
  for(unsigned id=0;id<16;++id) {
    const auto &a=actual.players[id], &e=expected.players[id];
    check(a.connected==e.connected,"Actual UDP team roster differs");
    if(e.connected)check(a.source_membership==e.source_membership &&
        a.source_ffa_identity==e.source_ffa_identity && a.score==e.score && a.kills==e.kills &&
        a.deaths==e.deaths && a.dead==e.dead && a.total_score==e.total_score && a.wins==e.wins && a.source_player==e.source_player &&
        a.source_unit==e.source_unit && a.source_weapon==e.source_weapon,
        "Actual UDP source team/player/object state differs");
  }
}
void projectile(const LevelContent &original,Cache &cache,bool teammate,bool lan) {
  auto content=original;content.collision.clear();content.kill_triggers.clear();
  auto selected=rules(cache);selected.random_seed=0x12345678;
  MultiplayerMatch match(content,0,cache.spawns(),selected);
  LanHost host(match,0,0x158ee0);
  const std::array<std::int8_t,1> chosen{0};
  std::unique_ptr<LanClient> client;
  std::array<Controls,16> controls{};
  if(lan) {
    client=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host.port()),0x158ee0,1,chosen);
    for(unsigned n=0;n<64 && !client->ready();++n) {
      client->send(std::array<Controls,1>{});host.receive(controls);host.broadcast();client->receive();
    }
    check(client->ready() && client->slots()[0]==0,"Team projectile shooter did not join actual UDP");
  } else match.join("Shooter",0);
  match.join("Victim",teammate ? 0 : 1);match.join("Other teammate",0);match.join("Other enemy",1);
  while(match.simulation().ticks()<200)match.step(controls);
  for(unsigned id=0;id<4;++id) {
    auto &u=unit(match,id);u.body.position={0,10.f+5.f*id,0};u.body.velocity={};u.yaw=u.pitch=0;
  }
  auto &shooter=unit(match,0), &target=unit(match,1);
  shooter.body.position={0,0,0};target.body.position={20,0,0};target.body.radius=1;
  target.vitality.body=.001f;target.vitality.shield=0;
  target.vitality.body_stun=target.vitality.shield_stun=32767;
  const auto tick=[&](bool fire) {
    controls={};controls[0].fire=fire;
    if(client) {client->send(std::array<Controls,1>{controls[0]});host.receive(controls);}
    match.step(controls);
    if(client) {host.broadcast();client->receive();compare(match.state(),client->state());}
  };
  tick(true);
  check(match.simulation().shots()==1 && match.simulation().hits()==0,"Source SMG deferred projectile not reached");
  for(unsigned n=0;n<16 && !match.simulation().hits();++n)tick(false);
  check(match.simulation().hits()==1 && match.unit(1).vitality.dead && match.player(1).deaths==1,
      "Team fatal production SMG projectile not reached");
  check(match.player(0).score==(teammate ? -1 : 1) && match.player(0).kills==(teammate ? 0 : 1) &&
      match.player(0).nonenemy_credit==(teammate ? 1 : 0) &&
      match.state().team_scores[0]==(teammate ? -1 : 1),"Source team lethal attribution/scoring differs");
  check(match.player(0).respawn.penalty==(teammate ? 300 : 0) && match.player(1).respawn.countdown==150 &&
      match.player(1).respawn.death_elapsed==int(match.simulation().ticks()-1),
      "Betrayal penalty owner or synchronous source death timer differs");
  if(teammate) {
    death(match,0,-1);tick(false);
    check(match.player(0).respawn.countdown==450 && !match.player(0).respawn.penalty,
        "Attacker's later death did not consume its betrayal penalty once");
    const auto expected=std::uint32_t(match.player(0).respawn.death_elapsed)+450;
    const auto spawn_clock=expected+((0u-expected)&31u);
    while(match.simulation().ticks()<spawn_clock) {
      tick(false);check(!match.player(0).source_current_unit,"Betrayer respawn preceded original slot gate");
    }
    tick(false);check(match.player(0).source_current_unit,"Betrayer did not respawn at original slot gate");
  }
}
void lan_roster(const LevelContent &content,Cache &cache) {
  auto selected=rules(cache);selected.score_limit=2;
  MultiplayerMatch match(content,0,cache.spawns(),selected);
  match.join("Host red",0);match.join("Host blue",1);
  LanHost host(match,0,0x1597e0);const auto address="127.0.0.1:"+std::to_string(host.port());
  const std::array<std::int8_t,2> choices{0,1};
  LanClient one(address,0x1597e0,2,choices),two(address,0x1597e0,2,choices),missing(address,0x1597e0,2);
  std::array<Controls,16> controls{};
  for(unsigned n=0;n<64 && (!one.ready() || !two.ready() || missing.error().empty());++n) {
    for(auto *client:{&one,&two,&missing})client->send(std::array<Controls,2>{});
    host.receive(controls);host.broadcast();for(auto *client:{&one,&two,&missing})client->receive();
  }
  check(one.ready() && two.ready() && !missing.ready() && !missing.error().empty() && host.peers()==2,
      "Actual UDP explicit team admission/rejection differs");
  check(one.slots().size()==2 && two.slots().size()==2 && !match.player(6).connected,
      "Rejected team hello leaked native players");
  for(unsigned n=0;n<90;++n) {
    one.send(std::array<Controls,2>{});two.send(std::array<Controls,2>{});
    host.receive(controls);match.step(controls);host.broadcast();one.receive();two.receive();
    compare(match.state(),one.state());compare(match.state(),two.state());
  }
  for(unsigned id=6;id<15;++id)check(match.join("Full team roster",0)==id,"Full team roster slot differs");
  const std::array<std::int8_t,2> other_team{7,7};
  LanClient overflow(address,0x1597e0,2,other_team);
  overflow.send(std::array<Controls,2>{});host.receive(controls);overflow.receive();
  check(!overflow.ready() && !overflow.error().empty() && !match.player(15).connected &&
      match.state().retained_team_mask==3 && host.peers()==2,
      "Rejected two-view hello consumed a slot or inserted a never-joined team into rankings");
  check(match.join("Final team slot",1)==15,"Rejected team hello changed slot allocation");
  host.broadcast();one.receive();two.receive();compare(match.state(),one.state());compare(match.state(),two.state());
  // Different peers on the same logical team contribute to one team total.
  death(match,1,int(one.slots()[0]));death(match,one.slots()[1],int(two.slots()[0]));match.step(controls);
  check(match.finished() && match.state().winner==-1 && match.state().winner_team==0 &&
      match.state().team_scores[0]==2 && match.player(one.slots()[0]).score==1 &&
      match.player(two.slots()[0]).score==1,"Cross-peer teammate score limit did not finish immediately");
  host.broadcast();one.receive();two.receive();compare(match.state(),one.state());compare(match.state(),two.state());
}
void original_timers(const LevelContent &content,Cache &cache,const char *path) {
  std::ifstream file(path);check(bool(file),"Original team timer fixtures missing");unsigned count=0;
  for(std::string line;std::getline(file,line);) {
    std::istringstream row(line);char kind;row>>kind;if(kind!='T')continue;
    int modifier,attacker;row>>modifier>>attacker;
    std::array<int,3> before{},after{},penalties{};
    for(auto &value:before)row>>value;
    for(auto &value:after)row>>value;
    for(auto &value:penalties)row>>value;
    int elapsed;row>>elapsed;check(bool(row),"Malformed original team timer fixture");
    auto selected=rules(cache);selected.respawn_modifier=modifier;
    MultiplayerMatch match(content,0,cache.spawns(),selected);
    match.join("Original slot 0",0);match.join("Original slot 3",0);match.join("Original slot 15",1);
    while(match.simulation().ticks()<200)match.step({});
    for(unsigned id=0;id<3;++id)const_cast<MultiplayerPlayer &>(match.player(id)).respawn.countdown=before[id];
    death(match,1,attacker==-1 ? -1 : attacker==0 ? 0 : attacker==3 ? 1 : 2);match.step({});
    for(unsigned id=0;id<3;++id)
      check(match.player(id).respawn.countdown==after[id] && match.player(id).respawn.penalty==penalties[id],
          "Native death boundary differs from original team respawn countdown/penalty");
    check(match.player(1).respawn.death_elapsed==elapsed,"Native team death timer used advanced clock");++count;
  }
  check(count==48,"Incomplete native original team timer comparison");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply project root and original team-session fixtures");
    Cache cache(std::filesystem::path(argv[1])/"assets/disc/maps/lockout.map");
    const auto original=cache.load_content(0,true);
    const Cache menu_cache(std::filesystem::path(argv[1])/"assets/disc/maps/mainmenu.map");
    const auto frontend=menu_cache.load_frontend();
    const auto authored=std::find_if(frontend.variants.begin(),frontend.variants.end(),[](const auto &variant) {
      return variant.engine==0 && variant.name.starts_with("Team Slayer ");
    });
    check(authored!=frontend.variants.end(),"Actual mainmenu Team Slayer variant missing");
    auto selected=slayer_rules(*authored,true);
    selected.weapon=cache.slayer_starting_weapon(selected.starting_weapon);
    check(selected.team_mode && selected.score_limit==50,"Authored Team Slayer settings differ");
    rejects([]{slayer_rules({"Team Slayer",0,{{91,1}}});},"Unfinished frontend team flow was enabled");
    rejects([]{slayer_rules({"Team Slayer",0,{{91,1},{25,1},{17,0}}},true);},"Zero cycling interval was admitted");
    MultiplayerMatch ordinary(original,0,cache.spawns(),selected);
    rejects([&]{ordinary.join("Missing team");},"Missing source team was admitted");
    rejects([&]{ordinary.join("Invalid team",8);},"Invalid source team was admitted");
    check(!ordinary.player(0).connected,"Rejected source team mutated native roster");
    join_four(ordinary);for(unsigned n=0;n<90;++n)ordinary.step({});
    const auto ordinary_state=ordinary.state();
    check(ordinary_state.tick==90 && ordinary_state.retained_team_mask==3 &&
        std::count_if(ordinary_state.players.begin(),ordinary_state.players.end(),
            [](const auto &player){return player.connected && !player.dead;})==4,
        "Actual Lockout Team Slayer 4-player timeline failed");
    auto controlled=original;controlled.collision.clear();controlled.kill_triggers.clear();
    for(int scoring:{0,1,2}) {
      auto r=rules(cache,scoring);MultiplayerMatch match(controlled,0,cache.spawns(),r);join_four(match);
      death(match,2,0);match.step({});
      check(match.state().team_scores[0]==(scoring==1 ? 0 : 1),"First teammate aggregate score differs");
      death(match,3,1);match.step({});
      check(match.state().team_scores[0]==(scoring==0 ? 2 : 1),"Second teammate aggregate score differs");
      death(match,1,1);match.step({});
      check(match.state().team_scores[0]==1,"Original aggregate high water or sum suicide differs");
    }
    for(bool tie:{false,true}) {
      auto r=rules(cache);r.time_ticks=3;MultiplayerMatch match(controlled,0,cache.spawns(),r);join_four(match);
      if(!tie) {
        death(match,2,0);match.step({});
        // Expiry runs before the participant check in original15EBA0.
        // Depart at the actual deadline; an earlier departure now correctly
        // ends for the remaining team on its next engine pass.
        while(match.simulation().ticks()<r.time_ticks)match.step({});
        match.leave(0);match.leave(1);
      }
      while(!match.finished())match.step({});
      check(match.state().tick==4 && match.state().winner_team==(tie ? -1 : 0) &&
          match.state().retained_team_mask==3,"Time expiry ignored retained departed teams or team tie");
    }
    {
      auto r=rules(cache);r.time_ticks=3;MultiplayerMatch match(controlled,0,cache.spawns(),r);join_four(match);
      death(match,2,0);match.step({});match.leave(0);match.leave(1);match.step({});
      check(match.finished() && match.state().tick==2 && match.state().winner_team==1 &&
          match.state().team_scores[0]==1 && match.state().team_wins[1]==1,
          "Original participant check failed to end before expiry for the remaining team");
    }
    for(bool teammate:{false,true})for(bool lan:{false,true})projectile(original,cache,teammate,lan);
    original_timers(controlled,cache,argv[2]);
    lan_roster(original,cache);
    std::cout<<"Actual Lockout: 4 local team players/90 ticks; six participants/two UDP peers/90 snapshots; "
        "four lethal SMG timelines; 48 original native death timers; betrayal consumption/respawn, aggregate scores, early departure/deadline/tie endings and cross-peer completion passed\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
