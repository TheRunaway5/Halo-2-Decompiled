#include "halo2/full_scoreboard.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace halo2;
using Json = nlohmann::json;
namespace {
unsigned checks = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) throw std::runtime_error(message);
}
std::vector<std::uint8_t> bytes(const std::string& hex) {
  check(hex.size()%2 == 0,"invalid original hex input");
  std::vector<std::uint8_t> out;
  for (std::size_t i=0;i<hex.size();i+=2)
    out.push_back(std::uint8_t(std::stoul(hex.substr(i,2),nullptr,16)));
  return out;
}
std::uint32_t u(const std::vector<std::uint8_t>& b,std::size_t at,unsigned n=4) {
  check(at <= b.size() && n <= b.size()-at,"truncated original record");
  std::uint32_t x=0;
  for (unsigned i=0;i<n;++i) x|=std::uint32_t(b[at+i])<<(8*i);
  return x;
}
std::int16_t h(const std::vector<std::uint8_t>& b,std::size_t at) {
  return std::bit_cast<std::int16_t>(std::uint16_t(u(b,at,2)));
}
std::u16string text(const std::vector<std::uint8_t>& b,std::size_t at,std::size_t count) {
  std::u16string out;
  for (std::size_t i=0;i<count;++i) {
    const auto ch=u(b,at+i*2,2);
    if (!ch) break;
    out.push_back(char16_t(ch));
  }
  return out;
}
float f(const std::vector<std::uint8_t>& b,std::size_t at) { return std::bit_cast<float>(u(b,at)); }
std::uint32_t hexword(const Json& j) { return std::uint32_t(std::stoul(j.get<std::string>(),nullptr,16)); }
FullScoreboardConstants constants(const Json& report) {
  const auto& words=report.at("source_data_words");
  auto get=[&](const char* address){return u(bytes(words.at(address).at("bytes_hex")),0);};
  FullScoreboardConstants c;
  c.counter_seconds=get("0046a130");c.fade_scale=get("0046a120");
  c.stagger_fraction=get("0046a0cc");c.stagger_scale=get("0046a11c");
  c.one=get("0046a0d0");c.header_rgb=get("0046a1d8");
  c.background_alpha=get("0046a130");c.text_scale=get("00456918");
  c.text_bias=get("0046a22c");c.dim_scale=get("0046a240");
  c.scroll_axis_scale=get("00445b20");c.scroll_pixels=get("0046a81c");
  const auto exponent=bytes(words.at("0046a378").at("bytes_hex"));
  c.fade_exponent=std::uint64_t(u(exponent,0))|(std::uint64_t(u(exponent,4))<<32);
  return c;
}
void rectangle(FullScoreboardRect actual,const Json& expected) {
  check(actual.top==expected[0] && actual.left==expected[1] &&
        actual.bottom==expected[2] && actual.right==expected[3],"rectangle differs from original");
}
void widget(const FullScoreboardWidget& actual,const Json& expected) {
  check(actual.valid==bool(expected.at("valid").get<unsigned>()),"widget validity differs from original");
  if (!actual.valid) return; // source deliberately leaves discarded fields undefined
  rectangle(actual.rectangle,expected.at("rectangle"));
  rectangle(actual.text_rectangle,expected.at("text_rectangle"));
  check(actual.right_justified==bool(expected.at("right_justified").get<unsigned>()),"justification differs from original");
  const auto raw=bytes(expected.at("text_utf16_hex"));
  check(actual.text==text(raw,0,raw.size()/2),"widget UTF16 differs from original");
  for (unsigned i=0;i<4;++i) {
    check(std::bit_cast<std::uint32_t>(actual.background[i])==hexword(expected.at("background_bits")[i]),"background color differs from original");
    check(std::bit_cast<std::uint32_t>(actual.foreground[i])==hexword(expected.at("text_color_bits")[i]),"foreground color differs from original");
  }
}
void compare_layout(const FullScoreboardLayout& actual,const Json& expected) {
  check(actual.status==FullScoreboardStatus::Ready,"ordinary original layout rejected");
  for (unsigned i=0;i<3;++i) check(actual.columns[i]==bool(expected.at("columns")[i].get<unsigned>()),"column admission differs from original");
  check(actual.rows.size()==expected.at("player_count").get<unsigned>(),"row count differs from original");
  widget(actual.title,expected.at("title"));
  const std::array<const char*,4> headers={"place","name","wins","score"};
  for (unsigned i=0;i<4;++i) widget(actual.headers[i],expected.at("headers").at(headers[i]));
  const std::array<const char*,6> fields={"place","icon","name","wins","score","connection"};
  for (unsigned i=0;i<actual.rows.size();++i) {
    const auto& a=actual.rows[i];const auto& e=expected.at("players")[i];
    check(a.datum==hexword(e.at("full_datum")),"full datum/order differs from original");
    check(a.current_player==bool(e.at("current_player").get<unsigned>()),"current row differs from original");
    for (unsigned j=0;j<6;++j) widget(a.widgets[j],e.at("widgets").at(fields[j]));
  }
}
FullScoreboardClock clock(const Json& records) {
  const auto b=bytes(records.at("clock"));
  return {h(b,2),f(b,4),std::bit_cast<std::int32_t>(u(b,8))};
}
FullScoreboardHeldState held_state(const Json& records) {
  const auto b=bytes(records.at("scoreboard_globals"));
  FullScoreboardHeldState s;
  for (unsigned i=0;i<4;++i) {s.counters[i]=b[i];s.scroll[i]=f(b,8+i*4);}
  s.inactive_counter=b[4];return s;
}
void equal_held(const FullScoreboardHeldState& a,const FullScoreboardHeldState& b) {
  check(a.counters==b.counters && a.inactive_counter==b.inactive_counter,"held counters differ from original");
  for (unsigned i=0;i<4;++i) check(std::bit_cast<std::uint32_t>(a.scroll[i])==std::bit_cast<std::uint32_t>(b.scroll[i]),"scroll words differ from original");
}
} // namespace
int main(int argc,char** argv) {
  try {
    if (argc!=2) throw std::runtime_error("Supply the frozen full-scoreboard report.json");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream input(argv[1]);Json report;input>>report;
    check(report.at("source_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","wrong original image");
    check(report.at("oracle_sha256")=="fd65899fb9ced274fd7d2442ab151223ab066f85d87cb6ea960abd18fbbddfb7","wrong frozen oracle");
    check(report.at("instruction_patches").empty() && report.at("supplied_graphic_world_or_gameplay_returns").empty(),"fixture includes patched/supplied game results");
    const auto c=constants(report);
    const auto& records=report.at("explicit_layout_inputs").at("raw_records");
    const auto player_bytes=bytes(records.at("players")),engine_bytes=bytes(records.at("engine")),game=bytes(records.at("game"));
    std::array<FullScoreboardPlayer,4> players;
    std::array<std::u16string,4> names;
    for (unsigned i=0;i<4;++i) {
      const std::size_t p=i*0x21c,s=0x304+i*0x1c;
      auto& a=players[i];a.datum=(u(player_bytes,p,2)<<16)|i;
      a.current_unit=u(player_bytes,p+0x2c);a.flags=std::uint16_t(u(player_bytes,p+2,2));
      a.membership=std::bit_cast<std::int8_t>(std::uint8_t(u(player_bytes,p+0xc0,1)));
      a.local_view=h(player_bytes,p+0x28);a.lives_remaining=h(player_bytes,p+0x1ac);
      a.score=h(engine_bytes,s+4);a.total_score=h(engine_bytes,s+6);
      a.stat8=h(engine_bytes,s+8);a.stat_a=h(engine_bytes,s+0xa);
      a.stat10=h(engine_bytes,s+0x10);a.wins=h(engine_bytes,s+0x12);
      names[i]=text(player_bytes,p+0x44,20);a.display_name=names[i];
      // Palette resolution is a preceding provider: the original row's raw
      // background RGB is exactly the resolved830C0 input, before row math.
      const auto& row=report.at("full_layout").at("output").at("players");
      auto r=std::find_if(row.begin(),row.end(),[&](const Json& j){return hexword(j.at("full_datum"))==a.datum;});
      check(r!=row.end(),"missing source palette delivery");
      for (unsigned j=0;j<3;++j) a.color[j]=std::bit_cast<float>(hexword(r->at("widgets").at("name").at("background_bits")[j+1]));
    }
    FullScoreboardOptions o;
    o.engine_present=true;o.engine_active=true;o.team_mode=(u(game,0x184)&1)!=0;
    o.game_over=u(game,0x1128,1)!=0;o.extended_round_comparison=(u(game,0x184)&16)!=0;
    o.display_mode=std::bit_cast<std::int32_t>(u(game,0x180));o.score_limit=std::bit_cast<std::int32_t>(u(game,0x18c));
    o.round_limit=std::bit_cast<std::int32_t>(u(game,0x188));o.life_limit=std::bit_cast<std::int32_t>(u(game,0x1b8));
    const FullScoreboardRect outer{0,0,480,640},inner{24,32,456,608};
    auto layout=full_scoreboard_layout(players,players[0].datum,o,outer,inner,0,1,{},c);
    compare_layout(layout,report.at("full_layout").at("output"));
    check(layout.rows[0].encoded_rank==1 && layout.rows[1].encoded_rank==1 &&
          layout.rows[2].encoded_rank==4 && layout.rows[3].encoded_rank==6,"encoded ties differ from original rank kernel");
    FullScoreboardHeldState state=held_state(report.at("held_counter_sequence")[0].at("before_records"));
    for (const auto& step:report.at("held_counter_sequence")) {
      const auto& before=step.at("before_records");equal_held(state,held_state(before));
      const auto p=bytes(before.at("players")),e=bytes(before.at("engine")),g=bytes(before.at("game")),raw=bytes(before.at("processed_control"));
      FullScoreboardHeldInput request;
      request.local_view=h(p,0x28);request.input_user=std::bit_cast<std::int32_t>(u(p,0x24));
      request.processed_record_present=true;request.held=raw[0x1d]!=0;request.scroll_axis=h(raw,0x46);
      request.engine_present=true;request.lifecycle=h(e,0x6c);request.options_state=std::uint8_t(u(g,0xc,1));request.engine_status=std::bit_cast<std::int32_t>(u(e,0xc04));
      request.current_unit=u(p,0x2c);request.death_countdown=std::bit_cast<std::int32_t>(u(p,0x170));request.player_flags=std::uint16_t(u(p,2,2));
      request.exhausted=o.life_limit>0 && request.current_unit==0xffffffffu && h(p,0x1ac)==0;
      const auto time=clock(before);
      check(full_scoreboard_update_held(state,request,time,FullScoreboardRounding::NearestEven,c),"ordinary source held state rejected");
      equal_held(state,held_state(step.at("after_records")));
      check(full_scoreboard_normal_hud(true,0,state.counters[0],time,c)==bool(step.at("HUD_predicate_call").at("defined_al").get<unsigned>()),"normal HUD predicate differs from original");
    }
    const auto time=clock(records);
    const auto alpha=full_scoreboard_display_alpha(8,time,c,full_scoreboard_host_power);
    const auto expected_alpha=bytes(report.at("first_graphics_stop").at("renderer_alpha_bits"));
    check(std::bit_cast<std::uint32_t>(alpha)==u(expected_alpha,0),"full-fade power/caller differs from original");
    layout=full_scoreboard_layout(players,players[0].datum,o,outer,inner,0,alpha,{},c);
    compare_layout(layout,report.at("first_graphics_stop").at("renderer_roster_output"));
    // Host/provider bounds, not new original-behavior variants.
    auto missing_teams=o;missing_teams.team_mode=true;
    check(full_scoreboard_layout(players,players[0].datum,missing_teams,outer,inner,0,1,{},c).status==FullScoreboardStatus::InvalidInput,"missing team stat provider admitted");
    auto duplicate=players;duplicate[1].datum=duplicate[0].datum;
    check(full_scoreboard_layout(duplicate,players[0].datum,o,outer,inner,0,1,{},c).status==FullScoreboardStatus::InvalidInput,"duplicate slot admitted");
    check(full_scoreboard_layout(players,0xffffffffu,o,outer,inner,0,1,{},c).status==FullScoreboardStatus::InvalidInput,"missing current identity admitted");
    FullScoreboardHeldInput bad;bad.local_view=4;bad.input_user=0;
    const auto saved=state;
    check(!full_scoreboard_update_held(state,bad,time,FullScoreboardRounding::NearestEven,c),"invalid host view admitted");equal_held(state,saved);
    FullScoreboardLabels labels;
    std::array<std::u16string,3> literal_labels;
    for (const auto& lookup:report.at("literal_locale_lookup")) {
      const auto s=lookup.at("returned_text").get<std::string>();
      const unsigned i=lookup.at("string_id")=="07000105"?0:lookup.at("string_id")=="070000fd"?1:2;
      literal_labels[i].assign(s.begin(),s.end());
    }
    labels.name=literal_labels[0];labels.wins=literal_labels[1];labels.score=literal_labels[2];
    layout=full_scoreboard_layout(players,players[0].datum,o,outer,inner,0,1,labels,c);
    check(layout.headers[1].text==labels.name && layout.headers[3].text==labels.score,"literal label provider ignored");
    std::cout<<checks<<" checks: original full roster, held sequence, fade/HUD and host bounds passed\n";
    return 0;
  } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
