#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
void choose(MenuState& menu,unsigned row,unsigned screen) {
  menu.select(row);menu.accept();check(menu.frame().screen==screen,"Finite-life actual menu route changed");
}
unsigned entry(const MenuState& menu,int value) {
  const auto& rows=menu.frame().entries;
  const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.value==value;});
  check(found!=rows.end(),"Finite-life authored menu entry absent");return unsigned(found-rows.begin());
}
FrontendLaunch normal_launch(const FrontendContent& content,bool team,unsigned protocol,int lives) {
  MenuState menu(content);menu.accept();choose(menu,0,167);menu.text("Life Settings");menu.commit_text();
  check(menu.frame().screen==38,"Finite-life profile creation missed customization");menu.back();
  check(menu.frame().screen==6 && !menu.signed_in_profile().empty(),"Finite-life profile failed actual sign-in");
  choose(menu,protocol?3:2,30);choose(menu,0,14);choose(menu,1,23);choose(menu,1,207);
  choose(menu,entry(menu,0),208);choose(menu,entry(menu,team?1:0),14);
  choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,0),208);
  menu.select(entry(menu,team?1:0));menu.auxiliary();
  check(menu.frame().screen==178,"Finite-life edit did not open original categories");
  for(const auto [parameter,value]:{std::pair{16,lives},std::pair{2,0},std::pair{10,0}}) {
    const auto p=std::find_if(content.parameters.begin(),content.parameters.end(),
        [&](const auto& p){return p.second.parameter==parameter;});
    check(p!=content.parameters.end(),"Finite-life authored setting absent");
    bool edited=false;const auto categories=unsigned(menu.frame().entries.size());
    for(unsigned category=0;category<categories && !edited;++category) {
      choose(menu,category,221);
      const auto& rows=menu.frame().entries;
      const auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.value==int(p->first);});
      if(row!=rows.end()) {
        choose(menu,unsigned(row-rows.begin()),223);choose(menu,entry(menu,value),221);edited=true;
      }
      menu.back();check(menu.frame().screen==178,"Finite-life category did not return to editor");
    }
    check(edited,"Finite-life setting absent from actual edit categories");
  }
  for(const unsigned screen:{208u,207u,14u}){menu.back();check(menu.frame().screen==screen,"Finite-life editor return route changed");}
  menu.select(0);menu.accept();check(bool(menu.launch),"Authored one-round life menu settings still reject Start Game");
  check(menu.launch->multiplayer && menu.launch->map=="lockout.map" &&
      menu.launch->host_port==(protocol?2002u:0u) && menu.launch->selected_profiles.size()==1,
      "Finite-life Start lost its selected map, protocol or signed-in profile");
  const auto source=source_game_variant(menu.launch->variant);
  check(source_game_variant_get(source,16)==lives && source_game_variant_get(source,0)==0 &&
      source_game_variant_get(source,2)==0 && source_game_variant_get(source,10)==0 &&
      source_game_variant_get(source,91)==int(team),"Normal Start lost actual authored life/round/score/time/team fields");
  return *menu.launch;
}
void fatal(MultiplayerMatch& match,unsigned slot) {
  auto& unit=const_cast<Unit&>(match.unit(slot));unit.vitality.dead=true;
  unit.vitality.body=0;unit.state=Unit::Dead;unit.last_damager=-2;match.step({});
}
void shot(MultiplayerMatch& match,unsigned target) {
  // Declared arena/position/vitality stimuli. The actual native projectile,
  // damage, attribution, life store and ownership release all execute.
  const auto previous_lives=match.player(target).source_lives_remaining;
  check(previous_lives>0,"Production shot requires a participant with remaining lives");
  for(unsigned i=0;i<multiplayer_capacity;++i)if(match.player(i).connected) {
    auto& unit=const_cast<Unit&>(match.unit(i));unit.body.position={0,float(i*20+50),0};unit.body.velocity={};
  }
  auto& shooter=const_cast<Unit&>(match.unit(0));auto& victim=const_cast<Unit&>(match.unit(target));
  shooter.body.position={0,0,0};shooter.yaw=shooter.pitch=0;
  victim.body.position={20,0,0};victim.body.radius=1;victim.vitality.body=.001f;victim.vitality.shield=0;
  victim.vitality.body_stun=victim.vitality.shield_stun=32767;
  std::array<Controls,16> controls{};controls[0].fire=true;
  for(unsigned n=0;n<40 && !match.unit(target).vitality.dead;++n){match.step(controls);controls[0].fire=false;}
  check(match.unit(target).vitality.dead && match.player(target).source_lives_remaining==previous_lives-1 &&
      match.player(target).source_current_object==null_datum,"Production shot did not consume exactly one life/release its victim");
}
void exhaust(MultiplayerMatch& match,unsigned target,int lives) {
  for(int death=0;death<lives;++death) {
    shot(match,target);
    if(death+1<lives) {
      for(unsigned n=0;n<1000 && match.player(target).source_current_object==null_datum;++n)match.step({});
      check(!match.finished() && match.player(target).source_current_object!=null_datum &&
          match.player(target).source_lives_remaining==lives-death-1,
          "Normal finite-life match did not respawn with its surviving life count");
    }
  }
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the repository root");const std::filesystem::path root(argv[1]),maps=root/"assets/disc/maps";
  Cache frontend(maps/"mainmenu.map");const auto front=frontend.load_frontend();const auto variant=front.variants.at(0);
  Cache cache(maps/"lockout.map",{},OriginalData::load_xbe(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"));
  const auto content=cache.load_content(0,true);auto arena=content;arena.collision.clear();arena.kill_triggers.clear();
  for(bool team:{false,true})for(int lives:{1,2}) {
    const auto local_launch=normal_launch(front,team,0,lives),lan_launch=normal_launch(front,team,1,lives);
    check(source_game_variant(local_launch.variant).bytes==source_game_variant(lan_launch.variant).bytes,
        "Normal Split Screen/System Link life settings differ");
    const auto selected=local_launch.variant;auto rules=slayer_rules(selected,true);
    rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
    check(rules.life_limit==lives && rules.score_limit==0 && rules.time_ticks==0,"Native rules lost actual menu life values");
    MultiplayerMatch match(arena,0,cache.spawns(),rules);
    match.join("Life Settings",team?0:-1,local_launch.selected_profiles.at(0));match.join("Other",team?7:-1);
    check(match.player(0).source_profile_request==local_launch.selected_profiles.at(0) &&
        match.player(0).source_configuration && (*match.player(0).source_configuration)[0x44]==2 &&
        (*match.player(0).source_configuration)[0x7c]==0 &&
        match.player(0).source_lives_remaining==lives,"Native life match lost the actual menu-selected profile");
    LanHost host(match,0,0x15e440);const std::array<std::int8_t,1> membership{7};
    LanClient peer(host.join_address(),0x15e440,1,
        team?std::span<const std::int8_t>(membership):std::span<const std::int8_t>{});
    std::array<Controls,16> controls{};
    for(unsigned n=0;n<100 && !peer.ready();++n){peer.send(std::array<Controls,1>{});host.receive(controls);host.broadcast();peer.receive();}
    check(peer.ready() && peer.slots()[0]==2,"Actual round-end UDP participant did not join");
    exhaust(match,2,lives);
    for(unsigned n=0;n<10;++n)match.step({});
    check(!match.finished(),"Exhausting one of three players/two teammates ended the round too early");
    host.broadcast();peer.receive();
    Mesh exhausted_mesh;MultiplayerScoreboard exhausted(cache,maps/"fonts",exhausted_mesh,selected);
    for(unsigned n=0;n<150;++n){match.step({});host.broadcast();peer.receive();exhausted.step(0,false,peer.state(),peer.slots()[0]);}
    const std::array<unsigned,1> exhausted_view{peer.slots()[0]};
    const auto automatic=exhausted.draw(peer.state(),exhausted_view,0,640,480);
    // Original15E6BC excludes exhausted players from its ordinary dead-timer
    // automatic branch. Preserve that distinction; manual input still works.
    check(!match.finished() && peer.state().tick>90 && peer.state().players[2].source_lives_remaining==0 &&
        peer.state().players[2].source_unit==null_datum && automatic.normal_hud && automatic.paint.draw.vertices.empty(),
        "Exhausted participant incorrectly entered the ordinary dead-timer scoreboard branch");
    for(unsigned n=0;n<45;++n)exhausted.step(0,true,peer.state(),peer.slots()[0]);
    const auto requested=exhausted.draw(peer.state(),exhausted_view,0,640,480);
    check(!requested.normal_hud && requested.layout.status==FullScoreboardStatus::Ready &&
        !requested.paint.draw.vertices.empty(),"Exhausted participant lost manual original scoreboard input");
    exhaust(match,1,lives);
    check(!match.finished(),"Life exhaustion ended inside world damage instead of the next engine pass");
    match.step({});host.broadcast();peer.receive();const auto final=match.state();
    check(final.finished && final.results_finalized && final.winner==(team?-1:0) && final.winner_team==(team?0:-1) &&
        final.players[0].wins==1 && final.players[1].wins==0 && final.players[2].wins==0 &&
        final.players[0].total_score==2*lives && final.players[0].kills==2*lives && final.players[1].deaths==lives &&
        final.players[2].deaths==lives && match.simulation().shots()==2*lives && match.simulation().hits()==2*lives,
        "Participant/life ending did not award and finalize the actual SMG match");
    check(match.recorded_statistics() && match.recorded_statistics()->players[0][6]==1,
        "Life-based ending did not reach the original retained win producer");
    check(peer.state().finished && peer.state().results_finalized && peer.state().winner==final.winner &&
        peer.state().winner_team==final.winner_team && peer.state().players[0].wins==1 &&
        peer.state().players[0].total_score==2*lives && peer.state().players[2].source_lives_remaining==0,
        "Actual UDP peer lost finalized life-based results");
    check(peer.state().players[0].source_configuration==final.players[0].source_configuration,
        "Finite-life UDP results lost the complete configured participant record");
    Mesh mesh;MultiplayerScoreboard board(cache,maps/"fonts",mesh,selected);
    for(unsigned n=0;n<40;++n)board.step(0,false,peer.state(),peer.slots()[0]);
    const std::array<unsigned,1> local{peer.slots()[0]};const auto drawn=board.draw(peer.state(),local,0,640,480);
    check(!drawn.normal_hud && drawn.layout.status==FullScoreboardStatus::Ready && drawn.layout.rows.size()==3 &&
        !drawn.paint.draw.vertices.empty(),
        "Life-based results did not reach the actual font-backed scoreboard");
    for(unsigned n=0;n<300;++n)match.step({});host.broadcast();peer.receive();
    check(match.state().game_over_complete() && match.player(1).source_current_object==null_datum &&
        match.player(2).source_current_object==null_datum && peer.state().players[0].wins==1,
        "Life-based postgame did not complete its countdown or exhausted players respawned");
    auto single_rules=rules;MultiplayerMatch single(arena,0,cache.spawns(),single_rules);single.join("Nobody left",team?0:-1);
    for(int death=0;death<lives;++death) {
      fatal(single,0);check(!single.finished(),"Fatal-input ending ran before the next source engine pass");
      if(death+1<lives) {
        for(unsigned n=0;n<1000 && single.player(0).source_current_object==null_datum;++n)single.step({});
        check(single.player(0).source_current_object!=null_datum,"Single finite-life participant did not respawn");
      }
    }
    single.step({});
    check(single.finished() && single.state().winner==-1 && single.state().winner_team==-1 && single.state().players[0].wins==0,
        "Exhausting every participant manufactured a winner");
    if(team) {
      auto unlimited=rules;unlimited.life_limit=0;MultiplayerMatch departure(arena,0,cache.spawns(),unlimited);
      departure.join("Retained",0);const auto leaving=departure.join("Departed",7);departure.leave(leaving);departure.step({});
      check(departure.finished() && departure.state().winner_team==0 && departure.state().players[0].wins==1,
          "Original retained-empty-team end check was omitted from ordinary Team Slayer");
    }
    std::cout<<(team?"Team":"FFA")<<" lives="<<lives<<" menu_starts=2 shots="<<2*lives
        <<" wins=1 exhausted_respawns=0 finalized_udp=1 automatic_results=1\n";
  }
  auto edited=variant;edited.values.set(16,1);edited.values.set(0,5);bool blocked=false;
  try{slayer_rules(edited,true);}catch(const FormatError&){blocked=true;}
  check(blocked,"Multi-round settings admitted before complete round/reset owners");
  std::cout<<"Normal authored life menu setup in Split Screen/System Link, real FFA/team SMG exhaustion/results, UDP, source-gated exhausted/manual and automatic result scoreboards, no-winner and retained-team departure pass; multi-round flows remain gated\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
