#include "halo2/bomb_defusal.hpp"
#include "halo2/frontend.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
bool near(Vec3 a,Vec3 b,float radius) {
  const auto d=a-b;
  // Floors above/below a site must not permit planting or defusing through it.
  return std::abs(d.z)<=.4f && d.x*d.x+d.y*d.y<=radius*radius;
}
bool participant(const BombDefusalState &s,const BombDefusalPlayer &p) {
  return p.connected && p.alive && p.team>=0 && p.team<2 && p.datum!=none &&
      std::ranges::find(s.roster,p.datum)!=s.roster.end();
}
int site_at(const BombDefusalState &s,Vec3 point) {
  for(unsigned i=0;i<s.sites.size();++i)
    if(near(point,s.sites[i].position,s.sites[i].radius))return int(i);
  return -1;
}
void reset_progress(BombDefusalState &s){s.actor=none;s.progress=0;}
void win(BombDefusalState &s,const BombDefusalRules &r,int team,BombDefusalOutcome why) {
  s.winner=team;s.outcome=why;++s.scores.at(unsigned(team));reset_progress(s);
  s.carrier=none;
  if(s.scores[unsigned(team)]>=r.wins_needed || s.round>=r.half_rounds*2) {
    s.phase=BombDefusalPhase::complete;
    s.match_winner=s.scores[0]==s.scores[1]?-1:(s.scores[0]>s.scores[1]?0:1);
    s.remaining=0;
  }else {s.phase=BombDefusalPhase::ended;s.remaining=r.intermission_ticks;}
}
void begin(BombDefusalState &s,const BombDefusalRules &r,std::span<const BombDefusalPlayer> players) {
  ++s.round;s.attacking_team=s.round>r.half_rounds?1:0;
  s.phase=BombDefusalPhase::freeze;s.outcome=BombDefusalOutcome::none;
  s.remaining=r.freeze_ticks;s.winner=-1;s.site=-1;s.carrier=none;
  s.roster.fill(none);reset_progress(s);
  unsigned n{};
  for(const auto &p:players)if(p.connected && p.team>=0 && p.team<2) {
    if(n==s.roster.size())throw FormatError("Too many Bomb Defusal participants");
    s.roster[n++]=p.datum;
  }
}
}
bool bomb_defusal_parameter(unsigned id) {
  return (id>=native_defusal_parameter && id<=native_defusal_disarm_parameter) ||
      (id>=native_defusal_site_parameter && id<native_defusal_site_parameter+16);
}
bool bomb_defusal_variant(const UiVariant &v) {
  std::optional<int> kind;
  for(const auto &[id,value]:v.values)if(id==native_defusal_parameter)kind=value;
  if(!kind)return false;
  if(v.engine!=5 || *kind!=1)throw FormatError("Bomb Defusal requires its Assault ruleset");
  return true;
}
BombDefusalRules bomb_defusal_settings(const UiVariant &v) {
  if(!bomb_defusal_variant(v))throw FormatError("Missing Bomb Defusal ruleset");
  BombDefusalRules r;
  const auto source=source_game_variant(v);
  r.round_ticks=unsigned(source_game_variant_get(source,10))*30;
  for(const auto &[id,value]:v.values)if(id>=native_defusal_fuse_parameter) {
    if(id>=native_defusal_site_parameter && id<native_defusal_site_parameter+16) {
      if(value<0 || value>32767)throw FormatError("Invalid Bomb Defusal site marker index");
      continue;
    }
    if(id>native_defusal_disarm_parameter || value<1 || value>180*30)
      throw FormatError("Invalid Bomb Defusal timer (30 Hz ticks)");
    if(id==native_defusal_fuse_parameter)r.fuse_ticks=unsigned(value);
    if(id==native_defusal_plant_parameter)r.plant_ticks=unsigned(value);
    if(id==native_defusal_disarm_parameter)r.defuse_ticks=unsigned(value);
  }
  if(r.round_ticks<30 || r.round_ticks>600*30 || r.fuse_ticks<=r.defuse_ticks)
    throw FormatError("Bomb Defusal requires a finite round and a fuse longer than defusing");
  return r;
}
std::vector<BombSite> bomb_defusal_sites(std::span<const NetgameFlag> markers,const UiVariant *variant) {
  std::vector<BombSite> sites;
  std::array<std::optional<unsigned>,16> overrides;
  if(variant)for(const auto &[id,value]:variant->values)
    if(id>=native_defusal_site_parameter && id<native_defusal_site_parameter+16) {
      if(value<0)throw FormatError("Invalid Bomb Defusal site marker index");
      overrides[id-native_defusal_site_parameter]=unsigned(value);
    }
  const bool explicit_sites=std::ranges::any_of(overrides,[](auto i){return i.has_value();});
  std::vector<const NetgameFlag*> selected;
  if(explicit_sites)for(const auto index:overrides)if(index) {
    const auto found=std::ranges::find(markers,*index,&NetgameFlag::source_index);
    if(found==markers.end())throw FormatError("Bomb Defusal site references an absent map marker");
    selected.push_back(&*found);
  }
  if(!explicit_sites)for(const auto &m:markers)if(m.type==3 && m.selector12==1)selected.push_back(&m);
  for(const auto *marker:selected) {
    const auto &m=*marker;
    if(!finite(m.position))throw FormatError("Invalid Bomb Defusal site position");
    // Duplicate Assault variant markers at the same location are one site.
    if(std::ranges::any_of(sites,[&](const auto &s){return length(s.position-m.position)<.1f;}))continue;
    sites.push_back({m.source_index,m.position});
  }
  if(sites.empty())throw FormatError("Bomb Defusal needs at least one defending-side (team 1) Assault goal marker");
  if(sites.size()>16)throw FormatError("Bomb Defusal supports at most 16 distinct bomb sites");
  return sites;
}
bool bomb_defusal_interacting(const BombDefusalState &s,const BombDefusalPlayer &p) {
  if(!participant(s,p) || !p.use || p.moving || !p.grounded || !p.unobstructed)return false;
  return (s.phase==BombDefusalPhase::live && s.carrier==p.datum && site_at(s,p.position)>=0) ||
      (s.phase==BombDefusalPhase::planted && p.team!=s.attacking_team && near(p.position,s.bomb_position,.75f));
}
bool bomb_defusal_tick(BombDefusalState &s,const BombDefusalRules &r,std::span<const BombDefusalPlayer> players) {
  if(s.phase==BombDefusalPhase::complete)return false;
  if(s.phase==BombDefusalPhase::waiting) {
    std::array<bool,2> present{};
    for(const auto &p:players)if(p.connected && p.team>=0 && p.team<2)present[unsigned(p.team)]=true;
    if(!present[0] || !present[1])return false;
    begin(s,r,players);return true;
  }
  if(s.phase==BombDefusalPhase::ended) {
    if(s.remaining && --s.remaining)return false;
    begin(s,r,players);return true;
  }
  if(s.phase==BombDefusalPhase::freeze) {
    if(s.remaining && --s.remaining)return false;
    s.phase=BombDefusalPhase::live;s.remaining=r.round_ticks;
    // Rotate the initial carrier, avoiding a permanent first-slot advantage.
    std::vector<const BombDefusalPlayer*> attackers;
    for(const auto &p:players)if(participant(s,p) && p.team==s.attacking_team)attackers.push_back(&p);
    if(!attackers.empty()) {
      const auto &p=*attackers[(s.round-1)%attackers.size()];
      s.carrier=p.datum;s.bomb_position=p.position;
    }
    return false;
  }
  std::array<unsigned,2> alive{};
  for(const auto &p:players)if(participant(s,p))++alive[unsigned(p.team)];
  // A planted bomb remains live after the entire attacking team dies.
  if(!alive[unsigned(1-s.attacking_team)] && (alive[unsigned(s.attacking_team)] || s.phase==BombDefusalPhase::planted)) {
    win(s,r,s.attacking_team,BombDefusalOutcome::eliminated);return false;
  }
  if(s.phase==BombDefusalPhase::live && !alive[unsigned(s.attacking_team)]) {
    win(s,r,1-s.attacking_team,BombDefusalOutcome::eliminated);return false;
  }
  if(s.phase==BombDefusalPhase::live) {
    const BombDefusalPlayer *carrier=nullptr;
    for(const auto &p:players)if(p.datum==s.carrier) {
      if(p.grounded && p.alive)s.bomb_position=p.position;
      if(participant(s,p) && p.team==s.attacking_team)carrier=&p;
    }
    if(!carrier) {
      s.carrier=none;reset_progress(s);
      // Attackers alone can recover the dropped bomb, by walking over it.
      for(const auto &p:players)if(participant(s,p) && p.team==s.attacking_team && p.unobstructed && near(p.position,s.bomb_position,.65f)) {
        s.carrier=p.datum;carrier=&p;break;
      }
    }
    // Deadline wins over an incomplete or last-tick plant. A completed plant
    // starts a fresh fuse and does not consume any fuse time on this tick.
    if(!s.remaining || --s.remaining==0){win(s,r,1-s.attacking_team,BombDefusalOutcome::time);return false;}
    if(carrier && bomb_defusal_interacting(s,*carrier)) {
      const auto site=site_at(s,carrier->position);
      if(s.actor!=carrier->datum || s.site!=site){reset_progress(s);s.actor=carrier->datum;s.site=site;}
      if(++s.progress>=r.plant_ticks) {
        s.bomb_position=carrier->position;s.carrier=none;s.phase=BombDefusalPhase::planted;
        s.remaining=r.fuse_ticks;reset_progress(s);
      }
    }else {reset_progress(s);s.site=-1;}
  }else if(s.phase==BombDefusalPhase::planted) {
    // Fuse expiry has priority over a defuse finishing on the same tick.
    if(!s.remaining || --s.remaining==0){win(s,r,s.attacking_team,BombDefusalOutcome::exploded);return false;}
    const BombDefusalPlayer *defuser=nullptr;
    for(const auto &p:players)if(bomb_defusal_interacting(s,p) && p.datum==s.actor)defuser=&p;
    if(!defuser)for(const auto &p:players)if(bomb_defusal_interacting(s,p)){defuser=&p;break;}
    if(defuser) {
      if(s.actor!=defuser->datum){reset_progress(s);s.actor=defuser->datum;}
      if(++s.progress>=r.defuse_ticks)win(s,r,1-s.attacking_team,BombDefusalOutcome::defused);
    }else reset_progress(s);
  }
  return false;
}
void validate_bomb_defusal_state(const BombDefusalState &s) {
  if(unsigned(s.phase)>unsigned(BombDefusalPhase::complete) || unsigned(s.outcome)>unsigned(BombDefusalOutcome::time) ||
      s.attacking_team<0 || s.attacking_team>1 || s.winner< -1 || s.winner>1 || s.match_winner< -1 || s.match_winner>1 ||
      s.round>24 || s.remaining>600*30 || s.progress>180*30 || s.sites.empty() || s.sites.size()>16 ||
      s.site< -1 || s.site>=int(s.sites.size()) || !finite(s.bomb_position) || s.scores[0]>13 || s.scores[1]>13 ||
      (s.phase==BombDefusalPhase::planted && (s.site<0 || s.carrier!=none)))throw FormatError("Invalid Bomb Defusal state");
  std::vector<std::uint32_t> members;
  for(const auto h:s.roster)if(h!=none) {
    if(!(h>>16) || (h&65535u)>=16 || std::ranges::find(members,h)!=members.end())
      throw FormatError("Invalid Bomb Defusal round roster");
    members.push_back(h);
  }
  for(const auto h:{s.carrier,s.actor})if(h!=none && std::ranges::find(members,h)==members.end())
    throw FormatError("Bomb Defusal interaction lost its round participant");
  for(const auto &site:s.sites)if(!finite(site.position) || !std::isfinite(site.radius) || site.radius<=0 || site.radius>8)
    throw FormatError("Invalid Bomb Defusal site");
}
std::string bomb_defusal_status(const BombDefusalState &s,const BombDefusalRules &r,const BombDefusalPlayer &p) {
  const auto seconds=(s.remaining+29)/30;
  std::string text="Bomb Defusal  "+std::to_string(s.scores[0])+" - "+std::to_string(s.scores[1])+"  Round "+std::to_string(s.round)+"\n";
  if(s.phase==BombDefusalPhase::waiting)return text+"Waiting for Red and Blue teams";
  if(s.phase==BombDefusalPhase::complete)return text+(s.match_winner<0?"Match drawn":(s.match_winner==p.team?"Match won":"Match lost"));
  if(s.phase==BombDefusalPhase::ended) {
    const char *reason=s.outcome==BombDefusalOutcome::exploded?"Bomb detonated":s.outcome==BombDefusalOutcome::defused?"Bomb defused":s.outcome==BombDefusalOutcome::time?"Time expired":"Team eliminated";
    return text+reason+(s.winner==p.team?" - Round won":" - Round lost");
  }
  text+=p.team==s.attacking_team?"ATTACK":"DEFEND";
  text+="  "+std::to_string(seconds/60)+":"+(seconds%60<10?"0":"")+std::to_string(seconds%60);
  if(s.phase==BombDefusalPhase::freeze)return text+"  Get ready";
  if(s.phase==BombDefusalPhase::planted)text+="  Bomb planted at "+std::string(1,char('A'+s.site));
  if(!p.alive)return text+"\nEliminated - next round to respawn";
  if(s.carrier==p.datum)text+="\nYou have the bomb - hold USE at a site to plant";
  else if(s.phase==BombDefusalPhase::live && p.team==s.attacking_team && s.carrier==none)text+="\nBomb dropped - walk over it to recover";
  if(s.phase==BombDefusalPhase::planted && p.team!=s.attacking_team)text+="\nHold USE beside the bomb to defuse";
  if(s.actor==p.datum)text+="  "+std::to_string(100*s.progress/(s.phase==BombDefusalPhase::planted?r.defuse_ticks:r.plant_ticks))+"%";
  for(unsigned i=0;i<s.sites.size();++i)text+="\nSite "+std::string(1,char('A'+i))+"  "+std::to_string(int(3.048f*length(p.position-s.sites[i].position)))+"m";
  return text;
}
BombDefusalMarkerVisual bomb_defusal_marker_visual(const BombDefusalState &s,const BombDefusalRules &r,unsigned site) {
  if(site>=s.sites.size() || !r.fuse_ticks)throw FormatError("Invalid Bomb Defusal marker presentation");
  BombDefusalMarkerVisual visual;
  visual.planted=s.phase==BombDefusalPhase::planted && s.site==int(site);
  if(visual.planted) {
    visual.red_fill=1.f-float(std::min(s.remaining,r.fuse_ticks))/float(r.fuse_ticks);
    // Half a pulse/second initially; one below20s and two below10s.
    // Keep the letter/outline visible throughout the pulse.
    const unsigned period=s.remaining<=10*30?15:s.remaining<=20*30?30:60;
    const float phase=float(s.remaining%period)/float(period);
    visual.brightness=.6f+.4f*(1.f-std::abs(2.f*phase-1.f));
  }
  return visual;
}
} // namespace halo2
