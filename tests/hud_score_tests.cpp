#include "halo2/hud.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool pass,const std::string &message) {if(!pass)throw FormatError(message);}
std::vector<std::uint8_t> bytes(const std::string &hex) {
  check(hex.size()==0xfc*2,"original score display length changed");
  std::vector<std::uint8_t> out;
  for(unsigned n=0;n<hex.size();n+=2)out.push_back(std::uint8_t(std::stoul(hex.substr(n,2),nullptr,16)));
  return out;
}
std::uint32_t word(const std::vector<std::uint8_t> &b,unsigned at) {
  return std::uint32_t(b.at(at)) | std::uint32_t(b.at(at+1))<<8 |
      std::uint32_t(b.at(at+2))<<16 | std::uint32_t(b.at(at+3))<<24;
}
std::u16string string(const std::vector<std::uint8_t> &b,unsigned at,unsigned count) {
  std::u16string out;
  for(unsigned n=0;n<count;++n) {
    const auto ch=char16_t(unsigned(b.at(at+n*2)) | unsigned(b.at(at+n*2+1))<<8);
    if(!ch)break;out+=ch;
  }
  return out;
}
std::array<std::vector<SourceHudScoreColor>,2> oracle(const char *path) {
  std::ifstream file(path);check(bool(file),"missing unchanged original HUD score fixture");
  std::array<std::vector<SourceHudScoreColor>,2> palettes;
  unsigned count=0;
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='A' || kind=='T') {
      unsigned n;row>>n;auto &p=palettes[kind=='T'];p.resize(n);
      for(auto &rgb:p)for(auto &value:rgb)row>>value;
      check(bool(row),"truncated original color block");continue;
    }
    check(kind=='S',"unexpected original score fixture kind");
    SourceHudScoreInput input;
    int engine,teams,local,scenario,clock;unsigned connected,playing;
    row>>input.source_type>>engine>>teams>>input.limit>>input.round_mode>>local>>connected>>playing>>scenario>>input.time_limit>>clock;
    input.engine_present=engine;input.teams=teams;input.clock_seconds=std::int16_t(clock);
    input.local_player=local<0 ? null_datum : (std::uint32_t(0x7000+local)<<16)|unsigned(local);
    input.connected_players=std::uint16_t(connected);input.playing_teams=std::uint16_t(playing);
    input.variant_name=u"Source score";
    for(unsigned n=0;n<3;++n) {
      unsigned slot;int membership,flags,color,score,wins;
      row>>slot>>membership>>flags>>color>>score>>wins;
      input.players.at(slot)={(std::uint32_t(0x7000+slot)<<16)|slot,std::int8_t(membership),
          std::int8_t(color),std::uint16_t(flags),std::int16_t(score),std::int16_t(wins)};
    }
    for(unsigned n=0;n<8;++n) {int score,wins;row>>score>>wins;input.team_scores[n]=std::int16_t(score);input.team_wins[n]=std::int16_t(wins);}
    int ready;std::string hex;unsigned mask,score_bits,rival_bits;
    row>>ready>>hex>>mask>>score_bits>>rival_bits;check(bool(row),"truncated original score row");
    const auto actual=source_hud_score_display(input,{bool(scenario),palettes[0],palettes[1]});
    const auto expected=bytes(hex);
    const auto equal=[&](bool value,const char *field){check(value,"original HUD score case"+std::to_string(count)+" differs: "+field);};
    equal((actual.status==SourceHudScoreStatus::Ready)==bool(ready),"admission");
    equal(std::uint32_t(actual.type)==word(expected,0),"engine display type");
    equal(actual.teams==bool(expected[4]),"team mode");
    equal(std::uint32_t(actual.limit)==word(expected,8),"effective score limit");
    equal(actual.name==string(expected,0xc,32),"variant name");
    equal(actual.timer==bool(expected[0x4c]) && actual.timer_text==string(expected,0x4e,8),"timer");
    for(unsigned n=0;n<3;++n)equal(actual.color[n]==word(expected,0x60+n*4) &&
        actual.rival_color[n]==word(expected,0x90+n*4),"tag color");
    equal(actual.leading==bool(expected[0x6c]),"strict lead");
    equal(actual.player==word(expected,0x70) && std::uint32_t(actual.score)==word(expected,0x74) &&
        actual.score_text==string(expected,0x78,8),"owner score/identity/text");
    equal(actual.rival==bool(expected[0x88]) && actual.rival_player==word(expected,0x8c) &&
        std::uint32_t(actual.rival_score)==word(expected,0x9c) && actual.rival_text==string(expected,0xa0,8),"rival score/identity/text");
    equal(source_hud_score_mask(actual)==mask,"owning engine mask");
    equal(std::bit_cast<std::uint32_t>(actual.score_fraction)==score_bits &&
        std::bit_cast<std::uint32_t>(actual.rival_fraction)==rival_bits,"owning float divisions");
    ++count;
  }
  check(count==1330,"original HUD score fixture coverage changed");
  std::cout<<"PASS "<<count<<" retail/TU score displays, tag colors, signed score/win fields, masks and float divisions\n";
  return palettes;
}
void missing_wins() {
  SourceHudScoreInput in;in.limit=1;in.local_player=0x70000000;
  in.players[0]={in.local_player,0,0,1,123,{}};
  check(source_hud_score_display(in,{}).status==SourceHudScoreStatus::UnavailableRoundStatistics,
      "missing original wins were synthesized from individual scores");
  check(slayer_rules({"Limit one",0,{{2,1}}}).score_limit==1,
      "Recovered native win-stat owner did not admit source limit one");
}
void native(const std::filesystem::path &maps,
    const std::array<std::vector<SourceHudScoreColor>,2> &palettes) {
  Cache cache(maps/"lockout.map");auto content=cache.load_content(0,true);
  auto rules=slayer_rules({"Team Slayer",0,{{2,50},{91,1},{89,5}}},true);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  MultiplayerMatch match(content,0,cache.spawns(),rules);
  for(unsigned n=0;n<4;++n)match.join("HUD source",std::int8_t(n/2));
  for(unsigned n=0;n<90;++n)match.step({});
  const auto hud_content=cache.load_hud(content.player.datum,rules.weapon);
  check(hud_content.score_scenario_present && hud_content.score_appearance_colors==palettes[0] &&
      hud_content.score_team_colors==palettes[1],"production HUD skipped original color tag blocks");
  auto state=match.state();
  for(unsigned n=0;n<4;++n) {
    const auto display=multiplayer_hud_score_display(state,n,50,14400,hud_content);
    check(display.status==SourceHudScoreStatus::Ready && display.teams && display.rival &&
        !display.leading && display.score==0 && display.rival_score==0 &&
        display.color==palettes[1][n/2] && display.rival_color==palettes[1][1-n/2],
        "alive production Team Slayer roster did not reach the original HUD query");
  }
  // Declared presentation input, independent of score-producing gameplay:
  // teammates differ individually, while the shared source team stat is123.
  state.team_scores[0]=123;state.team_scores[1]=7;
  state.players[0].score=1;state.players[1].score=29;
  const auto display=multiplayer_hud_score_display(state,0,0,0,hud_content);
  check(display.score==123 && display.rival_score==7 && display.limit==123 &&
      display.score_fraction==1 && display.score_text==u"123" && display.leading,
      "production HUD substituted individual scores or lost an unlimited meter denominator");
  auto text=hud_content;
  std::erase_if(text.widgets,[](const auto &w){return !w.is_text || w.text!=0x1600126c;});
  check(text.widgets.size()==1,"original HUD player score text missing");
  Mesh text_mesh;MultiplayerHud text_hud(text,maps/"fonts",text_mesh,2);
  const auto glyphs=text_hud.draw(state,0,0,1,640,480,50,14400);
  check(glyphs.size()==18,"actual HUD glyph consumer did not draw the three-digit team score");
  state.team_mode=false;
  state.players[0].score=123;state.players[1].score=123;state.players[2].score=29999;
  state.players[2].source_flags|=2;
  const auto ffa=multiplayer_hud_score_display(state,0,0,0,hud_content);
  check(ffa.score==123 && ffa.rival_score==123 && !ffa.leading && ffa.limit==123 &&
      ffa.rival_player==state.players[1].source_player && ffa.color==palettes[0][10],
      "production FFA HUD lost suppression, strict ties or original default appearance");
  match.leave(2);match.leave(3);
  const auto departed=multiplayer_hud_score_display(match.state(),0,50,14400,hud_content);
  check(!departed.rival && departed.leading,"retained departed team was used as a live HUD rival");
  // Declared snapshot presentation inputs exercise the real live adapter;
  // scoring, round admission and objective geometry have separate tests.
  state.source_engine=4;state.round_mode=0;
  state.players[0].score=123;state.players[1].score=7;
  const auto king=multiplayer_hud_score_display(state,0,120,0,hud_content);
  check(king.score_text==u"2:03" && king.rival_text==u" :07" && king.score==123 && king.rival_score==7,
      "Live King HUD substituted Slayer numeric text for source time formatting");
  state.source_engine=2;
  check(multiplayer_hud_score_display(state,0,120,0,hud_content).score_text==u"123",
      "Live Slayer HUD acquired objective time formatting");
  state.source_engine=4;state.players[0].wins=2;state.players[1].wins=1;
  constexpr std::array round_limits{1,2,4,6,2,3,4};
  for(unsigned round=0;round<round_limits.size();++round) {
    state.round_mode=int(round);
    const auto wins=multiplayer_hud_score_display(state,0,1,0,hud_content);
    check(wins.limit==round_limits[round] && wins.score==2 && wins.rival_score==1 &&
        wins.score_text==u" :02" && wins.rival_text==u" :01",
        "Live HUD lost the original round selector or retained win statistics");
  }
  std::cout<<"PASS production four-player90-tick Team Slayer HUD, original tag palettes, actual glyphs, FFA ties and departed-team presence\n";
  std::cout<<"PASS actual snapshot engine/round settings reach King time scores and all seven limit-one HUD denominators; declared presentation inputs do not admit rounds\n";
}
} // namespace
int main(int argc,char **argv) {
  try {check(argc>=2,"HUD score test needs original fixture");const auto palettes=oracle(argv[1]);missing_wins();
    if(argc>=3)native(argv[2],palettes);return 0;
  }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
