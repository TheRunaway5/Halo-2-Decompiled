#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/source_game_variant.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string &message) {
  if(!value)throw std::runtime_error(message);
}
std::uint32_t word(const Json &j) {return std::stoul(j.get<std::string>(),nullptr,16);}
std::u16string utf16(const Json &j) {
  const auto raw=j.get<std::string>();std::u16string out;
  for(unsigned i=0;i<raw.size();i+=4) {
    const auto c=std::stoul(raw.substr(i,2),nullptr,16)|(std::stoul(raw.substr(i+2,2),nullptr,16)<<8);
    if(!c)break;out.push_back(char16_t(c));
  }
  return out;
}
FullScoreboardConstants constants(const Json &fixture) {
  const auto get=[&](const char *address) {
    const auto s=fixture.at("source_data_words").at(address).at("bytes_hex").get<std::string>();
    std::uint64_t value=0;
    for(unsigned i=0;i<s.size()/2;++i)value|=std::uint64_t(std::stoul(s.substr(i*2,2),nullptr,16))<<(i*8);
    return value;
  };
  FullScoreboardConstants c;
  c.counter_seconds=get("0046a130");c.fade_scale=get("0046a120");c.stagger_fraction=get("0046a0cc");
  c.stagger_scale=get("0046a11c");c.one=get("0046a0d0");c.header_rgb=get("0046a1d8");
  c.background_alpha=get("0046a130");c.text_scale=get("00456918");c.text_bias=get("0046a22c");
  c.dim_scale=get("0046a240");c.scroll_axis_scale=get("00445b20");c.scroll_pixels=get("0046a81c");
  c.fade_exponent=get("0046a378");return c;
}
FullScoreboardRect rect(const Json &j) {return {j[0],j[1],j[2],j[3]};}
void widget(const FullScoreboardWidget &actual,const Json &expected,const std::string &name) {
  check(actual.valid==bool(expected.at("valid").get<unsigned>()),name+": score/wins admission differs");
  if(actual.valid)check(actual.text==utf16(expected.at("text_utf16_hex")),name+": score/wins text differs");
}
void original_case(const Json &test,const Json &fixture,const FullScoreboardConstants &c) {
  const auto &x=test.at("input"),&expected=test.at("output");const auto name=test.at("name").get<std::string>();
  MultiplayerState state;state.source_engine=x.at("display_mode");state.score_limit=x.at("score_limit");
  state.life_limit=x.at("life_limit");state.round_mode=x.at("round_limit");state.team_mode=x.at("team_mode");
  state.results_finalized=x.at("game_over");
  // Match the preexisting constructor's static round/comparison inputs. The
  // real draw adapter below must select the engine published by this match.
  FullScoreboardOptions base;base.engine_present=true;base.display_mode=2;
  base.round_limit=state.round_mode;base.extended_round_comparison=x.at("extended");
  const auto options=multiplayer_scoreboard_options(base,state);
  std::array<std::u16string,16> names;std::vector<FullScoreboardPlayer> players;
  for(const auto &p:x.at("players")) {
    const unsigned slot=p.at("slot");FullScoreboardPlayer row;
    row.datum=word(p.at("datum"));row.flags=p.at("flags");row.membership=p.at("membership");
    row.current_unit=p.at("current_unit");row.lives_remaining=p.at("lives");
    row.local_view=slot==0?x.at("view").get<std::int16_t>():-1;
    const auto s=p.at("stats");row.score=s[0];row.total_score=s[1];row.stat8=s[2];row.stat_a=s[3];row.stat10=s[4];row.wins=s[5];
    const auto text=p.at("name").get<std::string>();names[slot]={text.begin(),text.end()};row.display_name=names[slot];
    for(unsigned j=0;j<3;++j)row.color[j]=std::bit_cast<float>(word(p.at("color_bits")[j]));
    players.push_back(row);
  }
  std::array<FullScoreboardTeam,8> teams;
  for(unsigned i=0;i<8;++i) {
    auto &row=teams[i];row.retained=x.at("retained").get<unsigned>()&(1u<<i);row.playing=x.at("playing").get<unsigned>()&(1u<<i);
    const auto s=x.at("teams")[i];row.score=s[0];row.total_score=s[1];row.stat8=s[2];row.stat_a=s[3];row.stat10=s[4];row.wins=s[5];
    for(unsigned j=0;j<3;++j)row.color[j]=std::bit_cast<float>(word(fixture.at("team_palette_bits")[i][j]));
  }
  const auto layout=full_scoreboard_layout(players,0x80000000,options,rect(x.at("outer")),rect(x.at("inner")),
      x.at("scroll"),x.at("alpha"),{},c,teams);
  check(layout.status==FullScoreboardStatus::Ready,name+": layout rejected");
  for(unsigned i=0;i<3;++i)check(layout.columns[i]==bool(expected.at("columns")[i].get<unsigned>()),name+": columns differ");
  check(layout.rows.size()==expected.at("player_count"),name+": player count differs");
  check(layout.team_rows.size()==expected.at("team_count"),name+": team count differs");
  for(unsigned i=0;i<layout.rows.size();++i) {
    const auto &row=layout.rows[i];const auto &e=expected.at("players")[i];
    check(row.datum==word(e.at("full_datum")),name+": player order differs");
    widget(row.widgets[3],e.at("widgets").at("wins"),name);
    widget(row.widgets[4],e.at("widgets").at("score"),name);
  }
  for(unsigned i=0;i<layout.team_rows.size();++i) {
    const auto &row=layout.team_rows[i];const auto &e=expected.at("teams")[i];
    check(row.team==e.at("team"),name+": team order differs");
    widget(row.widgets[2],e.at("widgets").at("wins"),name);
    widget(row.widgets[3],e.at("widgets").at("score"),name);
  }
}
void lifecycle_cases(const FullScoreboardConstants &c) {
  FullScoreboardOptions base;base.engine_present=true;
  FullScoreboardPlayer player;player.datum=0x80000000;player.current_unit=0xffffffffu;
  player.flags=1;player.membership=0;player.local_view=0;player.score=125;player.total_score=185;
  for(bool available:{false,true})for(std::int16_t phase:{0,1,2,3})
      for(bool finished:{false,true})for(bool finalized:{false,true}) {
    MultiplayerState state;state.source_engine=3;state.score_limit=120;
    state.source_round_lifecycle_available=available;state.round_phase=phase;
    state.finished=finished;state.results_finalized=finalized;
    const auto options=multiplayer_scoreboard_options(base,state);
    const bool active=available?phase==1:!finished;
    const bool game_over=available?finished:finalized;
    check(options.engine_active==active,"Native scoreboard ignored published source round phase");
    check(options.game_over==game_over,"Native scoreboard selected final statistics from the wrong lifecycle owner");
    const auto layout=full_scoreboard_layout(std::span(&player,1),player.datum,options,
        {0,0,480,640},{36,48,444,592},0,1,{},c);
    check(layout.status==FullScoreboardStatus::Ready&&layout.rows.size()==1,"Lifecycle layout rejected");
    check(layout.rows[0].widgets[4].text==(game_over?u"3:05":u"2:05"),
        "Actual scoreboard score switched before the original game-over record");
    check(layout.rows[0].icon_kind==(active?1:0),"Death icon remained active outside original phase1");
  }
}
}
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original Team/FFA scoreboard fixture");
  std::ifstream file(argv[1]);check(bool(file),"Original scoreboard fixture missing");Json fixture;file>>fixture;
  check(fixture.at("source_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 scoreboard authority");
  check(fixture.at("instruction_patches").empty()&&fixture.at("supplied_game_or_graphics_returns").empty(),"Modified source scoreboard evidence");
  const auto c=constants(fixture);
  std::ifstream scroll_file(std::filesystem::path(argv[1]).parent_path()/"scoreboard_scroll_tu15.json");
  check(bool(scroll_file),"Original scoreboard scroll fixture missing");Json scroll;scroll_file>>scroll;
  check(scroll.at("source_xbe_sha256")==fixture.at("source_xbe_sha256")&&
      scroll.at("exhaustive_cases")==65536&&scroll.at("instruction_patches").empty(),"Invalid source scroll reference");
  for(const auto &probe:scroll.at("probes"))
    check(multiplayer_scoreboard_scroll_axis(probe.at("native_right_y"))==probe.at("processed_scroll_axis"),
        "Native right-stick Y differs from TU1.5 processed scoreboard scroll axis");
  for(const auto &test:fixture.at("cases"))original_case(test,fixture,c);
  lifecycle_cases(c);
  // Original19D220/1A0A90 variant constructors and15BB50 formatting gate.
  for(unsigned engine=0;engine<7;++engine)for(bool teams:{false,true}) {
    const auto source=source_game_variant_default(engine);MultiplayerState state;
    state.source_engine=source_game_variant_type(source);state.round_mode=3;state.score_limit=25;state.team_mode=teams;
    FullScoreboardOptions base;base.engine_present=true;base.display_mode=2;
    const auto options=multiplayer_scoreboard_options(base,state);
    check(options.round_limit==3,"Published round mode did not reach scoreboard");
    FullScoreboardPlayer player;player.datum=0x80000000;player.membership=0;player.current_unit=1;player.score=125;player.local_view=0;
    std::array<FullScoreboardTeam,8> team;team[0].retained=team[0].playing=true;team[0].score=125;
    const auto layout=full_scoreboard_layout(std::span(&player,1),player.datum,options,{0,0,480,640},{36,48,444,592},0,1,{},c,team);
    const auto expected=engine==1||engine==3||engine==6?u"2:05":u"125";
    check(layout.rows.size()==1&&layout.rows[0].widgets[4].text==expected,"Native engine score formatting differs");
    if(teams)check(layout.team_rows.size()==1&&layout.team_rows[0].widgets[3].text==expected,"Native team engine score formatting differs");
  }
  std::cout<<"PASS "<<fixture.at("cases").size()<<" unchanged TU1.5 score/wins layouts through native draw options, all seven engine selectors in FFA/team modes and "
      <<scroll.at("probes").size()<<" original right-stick scroll probes; source round phases/game-over and legacy lifecycle fallback\n";
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
