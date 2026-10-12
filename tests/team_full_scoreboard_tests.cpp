#include "halo2/full_scoreboard.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool value,const std::string &message) {
  ++checks;if(!value)throw std::runtime_error(message);
}
std::uint32_t word(const Json &j) {return std::uint32_t(std::stoul(j.get<std::string>(),nullptr,16));}
std::u16string ascii(const std::string &s) {return {s.begin(),s.end()};}
std::u16string utf16(const Json &j) {
  const auto raw=j.get<std::string>();check(raw.size()%4==0,"Invalid source UTF16 export");
  std::u16string out;
  for(unsigned i=0;i<raw.size();i+=4) {
    const auto c=std::stoul(raw.substr(i,2),nullptr,16)|(std::stoul(raw.substr(i+2,2),nullptr,16)<<8);
    if(!c)break;out.push_back(char16_t(c));
  }
  return out;
}
FullScoreboardConstants constants(const Json &fixture) {
  auto get=[&](const char *address) {
    const auto s=fixture.at("source_data_words").at(address).at("bytes_hex").get<std::string>();
    std::uint64_t value=0;for(unsigned i=0;i<s.size()/2;++i)value|=std::uint64_t(std::stoul(s.substr(i*2,2),nullptr,16))<<(i*8);
    return value;
  };
  FullScoreboardConstants c;
  c.counter_seconds=get("0046a130");c.fade_scale=get("0046a120");c.stagger_fraction=get("0046a0cc");
  c.stagger_scale=get("0046a11c");c.one=get("0046a0d0");c.header_rgb=get("0046a1d8");
  c.background_alpha=get("0046a130");c.text_scale=get("00456918");c.text_bias=get("0046a22c");
  c.dim_scale=get("0046a240");c.scroll_axis_scale=get("00445b20");c.scroll_pixels=get("0046a81c");
  c.fade_exponent=get("0046a378");return c;
}
FullScoreboardRect rectangle(const Json &j) {return {j[0],j[1],j[2],j[3]};}
void compare_rectangle(FullScoreboardRect r,const Json &j) {
  check(r.top==j[0] && r.left==j[1] && r.bottom==j[2] && r.right==j[3],"Rectangle differs from original");
}
void compare_widget(const FullScoreboardWidget &w,const Json &j) {
  check(w.valid==bool(j.at("valid").get<unsigned>()),"Widget admission differs from original");
  if(!w.valid)return; // remaining source bytes are deliberately undefined
  compare_rectangle(w.rectangle,j.at("rectangle"));compare_rectangle(w.text_rectangle,j.at("text_rectangle"));
  check(w.right_justified==bool(j.at("right_justified").get<unsigned>()),"Text alignment differs from original");
  if(j.contains("text_utf16_hex"))check(w.text==utf16(j.at("text_utf16_hex")),"UTF16 text differs from original");
  for(unsigned i=0;i<4;++i) {
    check(std::bit_cast<std::uint32_t>(w.background[i])==word(j.at("background_bits")[i]),"Background bits differ from original");
    check(std::bit_cast<std::uint32_t>(w.foreground[i])==word(j.at("text_color_bits")[i]),"Foreground bits differ from original");
  }
}
FullScoreboardLayout build(const Json &x,const FullScoreboardConstants &c,const Json &colors,
    std::span<const std::u16string> labels={}) {
  std::array<std::u16string,16> names;
  std::vector<FullScoreboardPlayer> players;
  for(const auto &p:x.at("players")) {
    FullScoreboardPlayer row;const unsigned slot=p.at("slot");
    row.datum=word(p.at("datum"));row.membership=p.at("membership");row.flags=p.at("flags");
    row.current_unit=p.at("current_unit");row.lives_remaining=p.at("lives");
    row.local_view=slot==0 ? x.at("view").get<std::int16_t>() : -1;
    const auto &s=p.at("stats");row.score=s[0];row.total_score=s[1];row.stat8=s[2];row.stat_a=s[3];row.stat10=s[4];row.wins=s[5];
    names[slot]=ascii(p.at("name"));row.display_name=names[slot];
    for(unsigned i=0;i<3;++i)row.color[i]=std::bit_cast<float>(word(p.at("color_bits")[i]));
    players.push_back(row);
  }
  std::array<FullScoreboardTeam,8> teams;
  for(unsigned i=0;i<8;++i) {
    auto &row=teams[i];row.retained=(x.at("retained").get<unsigned>()&(1u<<i))!=0;
    row.playing=(x.at("playing").get<unsigned>()&(1u<<i))!=0;
    const auto &s=x.at("teams")[i];row.score=s[0];row.total_score=s[1];row.stat8=s[2];row.stat_a=s[3];row.stat10=s[4];row.wins=s[5];
    for(unsigned j=0;j<3;++j)row.color[j]=std::bit_cast<float>(word(colors[i][j]));
    if(!labels.empty())row.display_name=labels[i];
  }
  FullScoreboardOptions o;o.engine_present=o.engine_active=true;o.team_mode=x.at("team_mode");
  o.game_over=x.at("game_over");o.extended_round_comparison=x.at("extended");o.display_mode=x.at("display_mode");
  o.score_limit=x.at("score_limit");o.round_limit=x.at("round_limit");o.life_limit=x.at("life_limit");
  // Reverse caller delivery to independently check source bitmap enumeration.
  std::reverse(players.begin(),players.end());
  return full_scoreboard_layout(players,0x80000000,o,rectangle(x.at("outer")),rectangle(x.at("inner")),
      x.at("scroll"),x.at("alpha"),{},c,teams);
}
void compare_teams(const FullScoreboardLayout &a,const Json &expected) {
  check(a.team_rows.size()==expected.size(),"Retained team row count differs");
  const std::array<const char *,4> fields={"place","name","wins","score"};
  for(unsigned i=0;i<a.team_rows.size();++i) {
    const auto &row=a.team_rows[i];const auto &e=expected[i];
    check(row.team==e.at("team"),"Sorted team designator differs");
    if(e.contains("encoded_rank"))check(row.encoded_rank==e.at("encoded_rank"),"Encoded team tie/rank differs");
    for(unsigned j=0;j<4;++j)compare_widget(row.widgets[j],e.at("widgets").at(fields[j]));
  }
}
void compare(const FullScoreboardLayout &a,const Json &j) {
  check(a.status==FullScoreboardStatus::Ready,"Source layout rejected");
  for(unsigned i=0;i<3;++i)check(a.columns[i]==bool(j.at("columns")[i].get<unsigned>()),"Column admission differs");
  check(std::bit_cast<std::uint32_t>(a.scroll)==word(j.at("scroll_bits")),"Clamped scroll bits differ");
  compare_widget(a.title,j.at("title"));
  const std::array<const char *,4> headers={"place","name","wins","score"};
  for(unsigned i=0;i<4;++i)compare_widget(a.headers[i],j.at("headers").at(headers[i]));
  compare_teams(a,j.at("teams"));check(a.rows.size()==j.at("player_count"),"Player row count differs");
  const std::array<const char *,6> fields={"place","icon","name","wins","score","connection"};
  for(unsigned i=0;i<a.rows.size();++i) {
    const auto &row=a.rows[i];const auto &e=j.at("players")[i];
    check(row.datum==word(e.at("full_datum")),"Sorted full player datum differs");
    check(row.encoded_rank==e.at("encoded_rank"),"Encoded player tie/rank differs");
    check(row.current_player==bool(e.at("current_player").get<unsigned>()),"Current-player highlight differs");
    if(!e.at("icon_kind").is_null())check(row.icon_kind==e.at("icon_kind"),"Defined icon kind differs");
    for(unsigned k=0;k<6;++k)compare_widget(row.widgets[k],e.at("widgets").at(fields[k]));
  }
}
void native_adapter(const std::filesystem::path &map,const std::filesystem::path &xbe,
    const Json &report,const Json &fixture) {
  const auto original=OriginalData::load_xbe(xbe);Cache cache(map,{},original);
  const auto assets=cache.load_full_scoreboard_assets();
  check(assets.team_palette.size()==9,"Original team palette import truncated");
  for(unsigned i=0;i<9;++i)for(unsigned j=0;j<3;++j)
    check(assets.team_palette[i][j]==word(fixture.at("team_palette_bits")[i][j]),"Actual team palette differs from source query");
  std::array<std::u16string,8> labels;
  for(const auto &lookup:report.at("original_team_label_lookups")) {
    const unsigned team=lookup.at("team");check(assets.team_names[team]==lookup.at("text").get<std::string>(),"Team unic/ordinal/index import differs");
    if(team<8)labels[team]=ascii(assets.team_names[team]);
  }
  const auto &named=fixture.at("literal_named_rows");
  compare_teams(build(named.at("input"),constants(fixture),fixture.at("team_palette_bits"),labels),named.at("rows"));
  Cache menu(map.parent_path()/"mainmenu.map",{},original);const auto frontend=menu.load_frontend();
  const auto variant=std::find_if(frontend.variants.begin(),frontend.variants.end(),[](const auto &v) {
    return v.engine==0 && source_game_variant_get(source_game_variant(v),91)==1;
  });
  check(variant!=frontend.variants.end(),"Actual authored team Slayer variant missing");
  auto rules=slayer_rules(*variant,true);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  const auto content=cache.load_content(0,true);MultiplayerMatch match(content,0,cache.spawns(),rules);
  for(unsigned i=0;i<4;++i)check(match.join("Player"+std::to_string(i),std::int8_t(i/2))==i,"Actual explicit team join failed");
  for(unsigned i=0;i<90;++i)match.step({});
  Mesh mesh;MultiplayerScoreboard scoreboard(cache,map.parent_path()/"fonts",mesh,*variant);
  check(scoreboard.available(),"Actual scoreboard font/import provider unavailable");
  for(unsigned i=0;i<8;++i)scoreboard.step(0,true);
  const std::array<unsigned,4> ids{0,1,2,3};
  const auto verify=[&](unsigned count) {
    const auto state=match.state();const auto draw=scoreboard.draw(state,ids,0,1280,720);
    check(!draw.normal_hud && draw.layout.status==FullScoreboardStatus::Ready &&
        draw.layout.rows.size()==count && draw.layout.team_rows.size()==2 && !draw.paint.draw.vertices.empty(),
        "Actual active team state did not supply native roster/text/background vertices");
    check(draw.paint.status==FullScoreboardPaintStatus::Partial && draw.paint.emblems_unavailable,
        "Unprovided source emblem boundary hidden");
    for(const auto &row:draw.layout.team_rows) {
      check(row.widgets[1].text==labels[row.team],"Actual team tag label not delivered");
      check(row.widgets[3].text==ascii(std::to_string(state.team_scores[row.team])),"Actual published team score not delivered");
    }
    for(const auto &row:draw.layout.rows) {
      const auto &player=state.players[row.datum&65535];check(player.source_player==row.datum,"Actual salted identity lost");
      const auto team=unsigned(player.source_membership);
      const auto team_row=std::find_if(draw.layout.team_rows.begin(),draw.layout.team_rows.end(),[&](const auto &r){return r.team==team;});
      check(team_row!=draw.layout.team_rows.end() && row.widgets[0].text==team_row->widgets[0].text,"Player's team place not delivered");
      for(unsigned j=0;j<3;++j)check(std::bit_cast<std::uint32_t>(row.widgets[2].background[j+1])==assets.team_palette[team][j],"Actual team color not delivered");
    }
    return draw;
  };
  verify(4);
  auto observer_state=match.state();observer_state.players[1].source_membership=-1;
  const auto observer=scoreboard.draw(observer_state,ids,0,1280,720);
  const auto observer_row=std::find_if(observer.layout.rows.begin(),observer.layout.rows.end(),[&](const auto &r) {
    return r.datum==observer_state.players[1].source_player;
  });
  check(observer_row!=observer.layout.rows.end() && !observer_row->widgets[0].valid &&
      !observer_row->widgets[1].valid,"Observer source widget admission differs");
  for(unsigned j=0;j<3;++j)check(std::bit_cast<std::uint32_t>(observer_row->widgets[2].background[j+1])==
      (*original->full_scoreboard_palette_fallback)[j],"Observer fallback RGB provider differs");
  // Explicit existing death API boundary, followed by the real source score
  // writer. This tests published-score delivery, not a source contact claim.
  auto &victim=const_cast<Unit &>(match.unit(2));victim.vitality.dead=true;victim.last_damager=-1;
  match.step({});check(match.state().team_scores[0]==1,"Source team score producer was not observed");verify(4);
  match.leave(2);match.leave(3);const auto departed=verify(2);
  check(departed.layout.team_rows[0].team==0,"Playing team did not precede retained departed team");
  scoreboard.step(1,false);
  check(scoreboard.draw(match.state(),ids,1,1280,720).paint.draw.vertices.empty(),"Held input leaked into independent local view");
  for(unsigned i=0;i<8;++i)scoreboard.step(0,false);
  const auto released=scoreboard.draw(match.state(),ids,0,1280,720);
  check(released.normal_hud && released.paint.draw.vertices.empty(),"Released manual scoreboard did not restore HUD");
}
float original_pulse_alpha{};
std::optional<float> original_pulse(FullScoreboardClock,float,float,float,float,std::uint16_t) {
  return original_pulse_alpha; // explicit executed x87 leaf delivery in this comparison
}
std::uint32_t raw_word(const std::string &s,unsigned at,unsigned width=4) {
  std::uint32_t out=0;for(unsigned i=0;i<width;++i)out|=std::uint32_t(std::stoul(s.substr((at+i)*2,2),nullptr,16))<<(i*8);
  return out;
}
void painter_fragments(const Json &fixture,const Json &paint,const std::filesystem::path &map,
    const std::filesystem::path &xbe) {
  check(paint.at("instruction_patches").empty() && paint.at("supplied_game_or_gpu_results").empty(),"Original painter supplied GPU/game results");
  const auto original=OriginalData::load_xbe(xbe);const auto font=load_retail_font(map.parent_path()/"fonts/conduit-13");
  const FullScoreboardPainter painter(font,5,6);const auto c=constants(fixture);
  for(const auto &p:paint.at("cases")) {
    const auto input=std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[&](const auto &row){return row.at("name")==p.at("name");});
    check(input!=fixture.at("cases").end(),"Original painter input missing");
    const auto layout=build(input->at("input"),c,fixture.at("team_palette_bits"));
    const auto clock=p.at("clock12_hex").get<std::string>();
    FullScoreboardPaintFrame frame;frame.clip={0,0,480,640};frame.shadow_enabled=true;frame.x87_control_word=0x27f;
    frame.clock={std::bit_cast<std::int16_t>(std::uint16_t(raw_word(clock,2,2))),
        std::bit_cast<float>(raw_word(clock,4)),std::bit_cast<std::int32_t>(raw_word(clock,8))};
    original_pulse_alpha=std::bit_cast<float>(raw_word(p.at("pulse_color16_hex"),0));
    const auto out=painter.draw(layout,*original->full_scoreboard_paint_constants,frame,original_pulse);
    const auto &requests=p.at("rectangles");
    check(out.status==FullScoreboardPaintStatus::Partial && out.emblems_unavailable &&
        out.draw.vertices.size()>=requests.size()*6,"Actual Team background painter unavailable");
    for(unsigned i=0;i<requests.size();++i) {
      const auto r=requests[i].at("rectangle8_hex").get<std::string>(),color=requests[i].at("color16_hex").get<std::string>();
      const auto signed_word=[&](unsigned offset){return std::bit_cast<std::int16_t>(std::uint16_t(raw_word(r,offset,2)));};
      const auto &a=out.draw.vertices[i*6],&b=out.draw.vertices[i*6+4];
      check(a.material==6 && a.position.x==signed_word(2) && a.position.y==signed_word(0) &&
          b.position.x==signed_word(6) && b.position.y==signed_word(4),"Original ordered Team/header/player rectangle submission differs");
      check(std::bit_cast<std::uint32_t>(a.position.z)==raw_word(color,0) &&
          std::bit_cast<std::uint32_t>(a.color.x)==raw_word(color,4) &&
          std::bit_cast<std::uint32_t>(a.color.y)==raw_word(color,8) &&
          std::bit_cast<std::uint32_t>(a.color.z)==raw_word(color,12),"Original Team background/pulse ARGB bits differ");
    }
  }
  check(paint.at("cases").size()==4,"Original Team painter coverage truncated");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=6)throw std::runtime_error("Supply original fixture, report, MAP, UPDATE_XBE and painter report");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    Json fixture,report;std::ifstream(argv[1])>>fixture;std::ifstream(argv[2])>>report;
    check(fixture.at("source_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong source image");
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_game_or_graphics_returns").empty(),"Original fixture supplied game results");
    const auto c=constants(fixture);unsigned count=0;
    for(const auto &case_:fixture.at("cases")) {
      try {compare(build(case_.at("input"),c,fixture.at("team_palette_bits")),case_.at("output"));}
      catch(const std::exception &e) {throw std::runtime_error(case_.at("name").get<std::string>()+": "+e.what());}
      check(case_.at("completion")=="complete_original_return","Incomplete original builder");++count;
    }
    check(count==333,"Original Team/FFA coverage truncated");native_adapter(argv[3],argv[4],report,fixture);
    Json paint;std::ifstream(argv[5])>>paint;painter_fragments(fixture,paint,argv[3],argv[4]);
    std::cout<<"PASS "<<count<<" complete original Team/FFA layouts, signed statistics, ties, masks, observer/death/life branches, split scrolling, all nine actual team labels/colors, four original ordered painter fragments and four-player native Team Slayer scoreboard delivery; "<<checks<<" checks\n";
    std::cout<<"LIMIT declared original API fixtures and active native match adapter; normal Team lobby handoff, complete matches, original lifecycle/automatic display, emblems, quality and Xbox raster parity remain separate\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
