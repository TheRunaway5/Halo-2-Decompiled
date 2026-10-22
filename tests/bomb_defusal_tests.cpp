#include "halo2/bomb_defusal.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/frontend.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char *message){++checks;if(!value)throw FormatError(message);}
template<class F>void rejects(F action,const char *message){bool rejected=false;try{action();}catch(const std::exception &){rejected=true;}check(rejected,message);}
struct Match {
  BombDefusalRules rules;
  BombDefusalState state;
  std::array<BombDefusalPlayer,4> players{{
      {0x10000,0,true,true,false,false,true,{-10,0,0}},
      {0x10001,1,true,true,false,false,true,{10,0,0}},
      {0x10002,0,true,true,false,false,true,{-12,0,0}},
      {0x10003,1,true,true,false,false,true,{12,0,0}}}};
  Match(){state.sites={{3,{0,0,0}},{5,{0,10,0}},{7,{0,20,0}}};state.roster.fill(0xffffffffu);}
  bool tick(){return bomb_defusal_tick(state,rules,players);}
  void ticks(unsigned n){while(n--)tick();}
  void start(){check(tick(),"Round must request new bodies");ticks(rules.freeze_ticks);check(state.phase==BombDefusalPhase::live,"Freeze did not start live round");}
  unsigned carrier(){for(unsigned i=0;i<players.size();++i)if(players[i].datum==state.carrier)return i;throw FormatError("No carrier");}
  void plant(unsigned site=0){auto &p=players[carrier()];p.position=state.sites[site].position;p.use=true;ticks(rules.plant_ticks);p.use=false;check(state.phase==BombDefusalPhase::planted,"Stationary carrier failed to plant");check(state.site==int(site),"Plant selected wrong site");check(state.remaining==rules.fuse_ticks,"Plant consumed fresh fuse time");}
};
void variants(const std::filesystem::path &root) {
  const auto variants=load_custom_slayer_variants(root/"data/tags/variants");
  check(variants.size()==3,"Custom variant catalog lost a mode");
  const auto found=std::ranges::find_if(variants,bomb_defusal_variant);check(found!=variants.end(),"Defusal absent from Assault catalog");
  check(found->engine==5 && custom_slayer(*found)==CustomSlayer::none,"Defusal misclassified as Slayer");
  const auto rules=bomb_defusal_settings(*found);
  check(rules.round_ticks==4500 && rules.fuse_ticks==1350 && rules.plant_ticks==120 && rules.defuse_ticks==180,"Halo balance defaults differ");
  check(source_game_variant_valid(source_game_variant(*found)) && source_game_variant_type(source_game_variant(*found))==9,"Defusal corrupted original Assault settings");
  auto edited=*found;edited.values.set(native_defusal_fuse_parameter,1500);
  check(custom_slayer_fingerprint(7,*found)!=custom_slayer_fingerprint(7,edited),"Timer edit not fenced from mismatched peers");
  edited.values.set(native_defusal_fuse_parameter,180);rejects([&]{bomb_defusal_settings(edited);},"Fuse may not expire before a full defuse");
  edited=*found;edited.engine=0;rejects([&]{bomb_defusal_variant(edited);},"Defusal accepted Slayer engine");
  edited=*found;edited.values.set(0x80000005u,1);rejects([&]{custom_slayer(edited);},"Unknown native extension admitted");
  edited=*found;edited.values.set(10,0);rejects([&]{bomb_defusal_settings(edited);},"Infinite plant window admitted");
}
void sites() {
  std::vector<NetgameFlag> markers;
  for(unsigned n=0;n<12;++n){NetgameFlag m;m.source_index=n;m.position={float(n*4),0,0};m.type=3;m.selector12=1;m.channel=std::int16_t(n);markers.push_back(m);}
  const auto sites=bomb_defusal_sites(markers);check(sites.size()==12 && sites.back().marker==11,"Multiple sites truncated by vanilla query/channel limits");
  auto duplicate=markers.back();duplicate.source_index=12;markers.push_back(duplicate);
  check(bomb_defusal_sites(markers).size()==12,"Co-located variants created duplicate sites");
  UiVariant configured{"Sites",5,{{native_defusal_parameter,1},{native_defusal_site_parameter,5},{native_defusal_site_parameter+1,2}}};
  const auto selected=bomb_defusal_sites(markers,&configured);check(selected.size()==2 && selected[0].marker==5 && selected[1].marker==2,"Explicit map marker site order differs");
  configured.values.set(native_defusal_site_parameter,100);rejects([&]{bomb_defusal_sites(markers,&configured);},"Missing marker override accepted");
  rejects([]{bomb_defusal_sites({});},"Map without sites silently admitted");
  for(auto &m:markers)m.selector12=0;rejects([&]{bomb_defusal_sites(markers);},"Attacking home was made a default bomb site");
}
void layouts(const std::filesystem::path &root) {
  const auto file=root/"data/tags/bomb-defusal-sites-v1.json";
  nlohmann::json catalog;std::ifstream(file)>>catalog;
  check(catalog.at("maps").size()==11,"Recommended map pool lost an A/B layout");
  for(const auto &row:catalog.at("maps")) {
    std::vector<NetgameFlag> markers;
    const auto add=[&](const nlohmann::json &pin) {
      NetgameFlag m;m.source_index=pin.at("index");m.type=pin.at("type");m.selector12=pin.at("team");
      const auto &p=pin.at("position");m.position={p[0],p[1],p[2]};
      if(std::ranges::find(markers,m.source_index,&NetgameFlag::source_index)==markers.end())markers.push_back(m);
    };
    for(const auto &site:row.at("sites"))add(site.at("marker"));
    for(const auto &spawn:row.at("spawns"))add(spawn);
    const auto scenario=row.at("scenario").get<std::string>();
    const auto layout=load_bomb_defusal_layout(file,scenario,markers);
    check(layout && layout->sites[0].marker==row.at("sites")[0].at("marker").at("index") &&
        layout->sites[1].marker==row.at("sites")[1].at("marker").at("index"),"Authored A/B order changed");
    check(length(layout->sites[0].position-layout->sites[1].position)>=6,"A/B sites overlap");
    check(!load_bomb_defusal_layout(file,"mods\\"+scenario,markers),"Mod scenario borrowed stock layout");
    markers.front().position.x+=1;
    rejects([&]{load_bomb_defusal_layout(file,scenario,markers);},"Changed map markers borrowed stock layout");
  }
}
void marker_visuals() {
  Match m;m.start();m.plant(1);
  auto &s=m.state;const auto &r=m.rules;
  s.remaining=r.fuse_ticks;
  check(bomb_defusal_marker_visual(s,r,1).red_fill==0,"Fresh bomb marker already filled");
  float previous=-1;
  for(unsigned remaining=r.fuse_ticks;;--remaining) {
    s.remaining=remaining;const auto visual=bomb_defusal_marker_visual(s,r,1);
    check(visual.planted && visual.red_fill>=previous && visual.red_fill<=1 &&
        visual.brightness>=.6f && visual.brightness<=1,"Fuse marker fill or readable pulse is invalid");
    previous=visual.red_fill;if(!remaining)break;
  }
  check(previous==1,"Expired fuse marker did not fill completely red");
  check(!bomb_defusal_marker_visual(s,r,0).planted && !bomb_defusal_marker_visual(s,r,0).red_fill,
      "Unplanted site inherited the bomb's urgency");
  s.phase=BombDefusalPhase::ended;
  check(!bomb_defusal_marker_visual(s,r,1).planted && !bomb_defusal_marker_visual(s,r,1).red_fill,
      "Ended round retained active bomb presentation");
}
void interaction() {
  for(unsigned site=0;site<3;++site){Match m;m.start();m.plant(site);}
  Match m;m.start();auto &p=m.players[m.carrier()];p.position=m.state.sites[0].position;p.use=true;
  m.ticks(60);check(m.state.progress==60,"Plant hold did not accumulate");p.use=false;m.tick();check(m.state.progress==0,"Released plant retained progress");
  p.use=true;p.moving=true;m.ticks(120);check(m.state.phase==BombDefusalPhase::live && !m.state.progress,"Moving carrier planted");
  p.moving=false;p.grounded=false;m.ticks(120);check(!m.state.progress,"Airborne carrier planted");
  p.grounded=true;p.position.z=.5f;m.ticks(120);check(!m.state.progress,"Carrier planted through a floor");
  p.position={0,0,0};m.ticks(60);p.position={0,10,0};m.tick();check(m.state.progress==1,"Plant progress carried between sites");
  m.ticks(119);check(m.state.phase==BombDefusalPhase::planted,"Reset plant never completed at second site");
  auto &d=m.players[1];d.position=m.state.bomb_position;d.use=true;m.ticks(90);check(m.state.progress==90,"Defuse hold did not accumulate");
  d.use=false;m.tick();check(!m.state.progress,"Released defuse retained progress");
  d.use=true;d.unobstructed=false;m.ticks(180);check(!m.state.progress && m.state.phase==BombDefusalPhase::planted,"Defused through obstruction");
  d.unobstructed=true;m.ticks(90);d.use=false;auto &other=m.players[3];other.position=d.position;other.use=true;m.tick();
  check(m.state.progress==1 && m.state.actor==other.datum,"Defuse progress transferred to a teammate");
  m.ticks(179);check(m.state.outcome==BombDefusalOutcome::defused && m.state.winner==1 && m.state.scores[1]==1,"Completed defuse did not award exactly one defending round");
}
void drop_and_roster() {
  Match m;m.start();const auto carrier=m.carrier();auto &p=m.players[carrier];p.position={-3,1,0};m.tick();
  p.alive=false;m.tick();check(m.state.carrier==0xffffffffu && m.state.bomb_position.x==-3,"Dead carrier failed to drop at last grounded position");
  m.players[1].position=m.state.bomb_position;m.tick();check(m.state.carrier==0xffffffffu,"Defender picked up bomb");
  m.players[2].position=m.state.bomb_position;m.players[2].unobstructed=false;m.tick();check(m.state.carrier==0xffffffffu,"Attacker recovered bomb through wall");
  m.players[2].unobstructed=true;m.tick();check(m.state.carrier==m.players[2].datum,"Teammate failed to recover dropped bomb");
  m.players[2].datum=0x20002;m.tick();check(m.state.carrier==0xffffffffu,"Reused player slot inherited bomb/round eligibility");
  Match departed;departed.start();auto &c=departed.players[departed.carrier()];c.position={-3,2,0};departed.tick();c.connected=false;departed.tick();
  check(departed.state.carrier==0xffffffffu && departed.state.bomb_position.x==-3,"Departed carrier retained possession");
}
void endings() {
  Match before;before.start();before.players[0].alive=before.players[2].alive=false;before.tick();
  check(before.state.winner==1 && before.state.outcome==BombDefusalOutcome::eliminated,"Attackers eliminated before plant did not lose");
  Match after;after.start();after.plant();after.players[0].alive=after.players[2].alive=false;after.tick();
  check(after.state.phase==BombDefusalPhase::planted,"Attack elimination ended planted round");
  after.ticks(after.state.remaining);check(after.state.winner==0 && after.state.outcome==BombDefusalOutcome::exploded,"Dead attackers lost bomb explosion win");
  Match defenders;defenders.start();defenders.plant();defenders.players[1].alive=defenders.players[3].alive=false;defenders.tick();
  check(defenders.state.winner==0,"Eliminating defenders did not win round");
  Match timeout;timeout.start();timeout.state.remaining=1;timeout.tick();check(timeout.state.winner==1 && timeout.state.outcome==BombDefusalOutcome::time,"Preplant timeout did not defend");
  Match last;last.start();auto &carrier=last.players[last.carrier()];carrier.position={0,0,0};carrier.use=true;last.ticks(119);last.state.remaining=1;last.tick();
  check(last.state.outcome==BombDefusalOutcome::time,"Last-tick plant bypassed expired round clock");
  Match fuse;fuse.start();fuse.plant();fuse.players[1].position=fuse.state.bomb_position;fuse.players[1].use=true;
  fuse.ticks(179);fuse.state.remaining=1;fuse.tick();check(fuse.state.outcome==BombDefusalOutcome::exploded,"Defuse won a fuse-expiry tie");
  Match stalled;stalled.players[1].connected=stalled.players[3].connected=false;stalled.ticks(6000);
  check(stalled.state.phase==BombDefusalPhase::waiting && !stalled.state.round,"Missing team consumed round clock");
}
void match_format() {
  Match m;m.rules.freeze_ticks=1;m.rules.intermission_ticks=1;m.start();
  for(unsigned round=1;round<=24;++round) {
    check(m.state.round==round && m.state.attacking_team==(round<=12?0:1),"Halftime changed wrong round or team identity");
    m.state.remaining=1;m.tick(); // Defenders win: 12 rounds to each side.
    check(m.state.scores[0]+m.state.scores[1]==round,"Round award repeated or skipped");
    if(round<24){check(m.tick(),"Next round did not request fresh bodies");m.tick();}
  }
  check(m.state.phase==BombDefusalPhase::complete && m.state.match_winner==-1 && m.state.scores[0]==12 && m.state.scores[1]==12,"Regulation draw differs");
  m.ticks(100);check(m.state.scores[0]==12 && m.state.scores[1]==12,"Completed match kept scoring");
  Match win;win.rules.freeze_ticks=1;win.rules.intermission_ticks=1;win.start();
  for(unsigned round=1;round<=13;++round) {
    win.players[0].alive=win.players[2].alive=false;
    win.tick();
    if(round<13){win.tick();for(auto &p:win.players)p.alive=true;win.tick();}
  }
  check(win.state.phase==BombDefusalPhase::complete && win.state.match_winner==1 && win.state.scores[1]==13,"First to 13 did not end match");
  validate_bomb_defusal_state(win.state);
  auto bad_roster=win.state;bad_roster.roster[1]=bad_roster.roster[0];rejects([&]{validate_bomb_defusal_state(bad_roster);},"Duplicate wire round participant admitted");
  auto bad_score=win.state;bad_score.scores[0]=65535;rejects([&]{validate_bomb_defusal_state(bad_score);},"Invalid replicated round score admitted");
  win.state.sites[0].radius=std::numeric_limits<float>::quiet_NaN();rejects([&]{validate_bomb_defusal_state(win.state);},"Invalid wire site radius admitted");
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply repository root");variants(argv[1]);sites();layouts(argv[1]);marker_visuals();interaction();drop_and_roster();endings();match_format();
  std::cout<<"PASS "<<checks<<" Bomb Defusal checks: catalog, sites, interrupts, roster, timers and complete MR12 matches\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
