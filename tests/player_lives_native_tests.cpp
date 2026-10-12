#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char* text){if(!ok)throw FormatError(text);}
void fatal(MultiplayerMatch& match,unsigned slot,int owner=-2) {
  auto& unit=const_cast<Unit&>(match.unit(slot));
  unit.vitality.dead=true;unit.vitality.body=0;unit.state=Unit::Dead;unit.last_damager=owner;
  match.step({});
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the repository root");const std::filesystem::path root(argv[1]),maps=root/"assets/disc/maps";
  Cache front(maps/"mainmenu.map");const auto variant=front.load_frontend().variants.at(0);
  Cache cache(maps/"lockout.map",{},OriginalData::load_xbe(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"));
  const auto content=cache.load_content(0,true);auto rules=slayer_rules(variant,true);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);rules.score_limit=rules.time_ticks=0;
  auto finite=variant;finite.values.set(16,2);
  rules=slayer_rules(finite,true);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);
  match.join("Depleted");match.join("Survivor");match.join("Other survivor");
  check(match.state().life_limit==2 && match.player(0).source_lives_remaining==2,"Native join lost the original life initialization");
  fatal(match,0);check(match.player(0).source_lives_remaining==1,"First death did not consume exactly one life");
  for(unsigned i=0;i<200 && match.player(0).source_current_object==null_datum;++i)match.step({});
  check(match.player(0).source_current_object!=null_datum && match.player(0).source_lives_remaining==1,
      "Respawn reset lives or failed with one life remaining");
  fatal(match,0);const auto countdown=match.player(0).respawn.countdown;
  for(unsigned i=0;i<300;++i)match.step({});
  check(match.player(0).source_lives_remaining==0 && match.player(0).source_current_object==null_datum &&
      match.player(0).respawn.countdown==countdown,"Exhausted player respawned or mutated a gated countdown");
  LanHost host(match,0,0x160570);LanClient client("127.0.0.1:"+std::to_string(host.port()),0x160570,2);
  std::array<Controls,16> controls{};
  for(unsigned i=0;i<100 && !client.ready();++i){client.send(std::array<Controls,2>{});host.receive(controls);host.broadcast();client.receive();}
  check(client.ready() && client.state().life_limit==2 && client.state().players[0].source_lives_remaining==0 &&
      client.state().players[1].source_lives_remaining==2 && client.state().players[client.slots()[0]].source_lives_remaining==2,
      "Actual native UDP snapshot lost original life fields or new-participant initialization");
  Mesh mesh;MultiplayerScoreboard scoreboard(cache,maps/"fonts",mesh,variant);
  check(scoreboard.available(),"Original scoreboard providers unavailable");
  for(unsigned i=0;i<40;++i)scoreboard.step(0,true);
  auto state=client.state();state.players[0].score=100; // Declared rank stimulus, not a live score producer.
  const std::array<unsigned,1> local{0};const auto drawn=scoreboard.draw(state,local,0,640,480);
  check(drawn.layout.status==FullScoreboardStatus::Ready && drawn.layout.rows.front().datum!=match.player(0).source_datum,
      "Actual scoreboard ranked exhausted high-score player above living participants");
  const auto row=std::find_if(drawn.layout.rows.begin(),drawn.layout.rows.end(),[&](const auto& r){return r.datum==match.player(0).source_datum;});
  check(row!=drawn.layout.rows.end() && row->icon_kind==1 &&
      row->widgets[2].foreground[1]<drawn.layout.rows.front().widgets[2].foreground[1],
      "Actual scoreboard did not consume exhausted-player icon/dimming fields");
  auto edited=finite;edited.values.set(0,5);bool rejected=false;
  try{slayer_rules(edited,true);}catch(const FormatError&){rejected=true;}
  check(rejected,"Multi-round life settings admitted before complete round/reset owners");
  auto unlimited=rules;unlimited.life_limit=0;MultiplayerMatch normal(content,0,cache.spawns(),unlimited);
  normal.join("Unlimited");fatal(normal,0);
  check(normal.player(0).source_lives_remaining==-1,"Ordinary Slayer lost its original NONE life sentinel");
  // A real production SMG impact must consume the winning victim's active
  // life before scoring makes the engine inactive. Only arena collision,
  // positions and vitality are declared test stimuli, as in game_end_native.
  for(bool team:{false,true}) {
    auto selected=variant;selected.values.set(91,team?1:0);
    auto ending_rules=slayer_rules(selected,true);
    ending_rules.weapon=rules.weapon;ending_rules.score_limit=1;ending_rules.time_ticks=0;ending_rules.life_limit=1;
    auto arena=content;arena.collision.clear();arena.kill_triggers.clear();
    MultiplayerMatch ending(arena,0,cache.spawns(),ending_rules);
    ending.join("Shooter",team?0:-1);ending.join("Victim",team?7:-1);
    auto& shooter=const_cast<Unit&>(ending.unit(0));auto& victim=const_cast<Unit&>(ending.unit(1));
    shooter.body.position={0,0,0};shooter.body.velocity={};shooter.yaw=shooter.pitch=0;
    victim.body.position={20,0,0};victim.body.velocity={};victim.body.radius=1;
    victim.vitality.body=.001f;victim.vitality.shield=0;
    victim.vitality.body_stun=victim.vitality.shield_stun=32767;
    controls={};controls[0].fire=true;
    for(unsigned i=0;i<40 && !ending.finished();++i){ending.step(controls);controls[0].fire=false;}
    check(ending.finished() && ending.simulation().shots()==1 && ending.simulation().hits()==1 &&
        ending.player(1).source_lives_remaining==0 && ending.player(0).source_lives_remaining==1,
        "Winning production SMG death did not consume exactly one active life before game over");
    fatal(ending,0);for(unsigned i=0;i<300;++i)ending.step({});
    check(ending.player(0).source_lives_remaining==1 && ending.player(0).source_current_object==null_datum &&
        ending.player(1).source_lives_remaining==0 && ending.player(1).source_current_object==null_datum,
        "Postgame death consumed an inactive life or postgame players respawned");
    // Also retain ordering at the explicitly bounded native fatal-input
    // adapter: owner-1 names player0; it is not original damage attribution.
    MultiplayerMatch fallback(arena,0,cache.spawns(),ending_rules);
    fallback.join("Credited",team?0:-1);fallback.join("Fatal input",team?7:-1);fatal(fallback,1,-1);
    check(fallback.finished() && fallback.player(1).source_lives_remaining==0,
        "Score-ending fatal-input adapter consumed life after making the engine inactive");
  }
  std::cout<<"Native authored one-round life initialization/death/respawn gates, winning SMG/postgame ordering, UDP and font-backed scoreboard pass; multi-round settings remain gated\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
