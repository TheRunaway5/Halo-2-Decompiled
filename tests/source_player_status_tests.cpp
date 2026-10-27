#include "halo2/source_player_status.hpp"
#include "halo2/original_data.hpp"
#include "halo2/vista_lobby_metadata.hpp"
#include "halo2/hud.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/gameplay_font_source.hpp"
#include <algorithm>
#include <bit>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {void check(bool v,const char* message){if(!v)throw FormatError(message);}}
int main(int argc,char**argv)try {
  check(argc==4,"Supply original status fixture, map and TU1.5 image");
  for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Status fixture has another executable authority");
  check(fixture.at("instruction_patches").empty(),"Status fixture patched original instructions");
  for(const auto& row:fixture.at("cases")) {
    const auto& j=row.at("input");SourcePlayerStatusInput in;
#define READ(field) in.field=j.at(#field).get<decltype(in.field)>()
    READ(engine_status);READ(engine_override);READ(special_mode);READ(leading);READ(tied);
    READ(current_unit);READ(membership);READ(alive_ticks);READ(player_flags);READ(exhausted);
    READ(respawn_ticks);READ(life_limit);READ(score_limit);READ(tick);READ(round_start_tick);
    READ(lives_remaining);READ(rate);READ(round_countdown);
#undef READ
    check(source_player_status(in)==row.at("result"),"Player status differs from unchanged1618B0");
  }
  const auto original=OriginalData::load_xbe(argv[3]);Cache cache(argv[2],{},original);
  const auto assets=cache.load_full_scoreboard_assets();
  check(assets.player_status.size()==fixture.at("statuses").size(),"Authored player status bank missing");
  for(unsigned i=0;i<assets.player_status.size();++i) {
    const auto& actual=assets.player_status[i];const auto& expected=fixture.at("statuses").at(i);
    check(actual.status==expected.at("status") && actual.string_id==expected.at("string_id") &&
        actual.team_string_id==expected.at("team_string_id") && actual.text==expected.at("text").get<std::string>() &&
        actual.team_text==expected.at("team_text").get<std::string>(),"Authored player status text import differs");
  }
  check(assets.game_over_label==fixture.at("lifecycle_labels").at(0).get<std::string>() &&
      assets.round_over_label==fixture.at("lifecycle_labels").at(1).get<std::string>() &&
      assets.waiting_for_next_round_label==fixture.at("lifecycle_labels").at(2).get<std::string>(),
      "Authored lifecycle labels differ");
  SourcePlayerStatusTextInput text;text.status=2;text.respawn_seconds=3;
  check(source_player_status_text(assets,text)==u"Respawn in 3","Respawn status lost original170 seconds token");
  text.respawn_seconds=0;
  check(source_player_status_text(assets,text)==u"Respawn in 0","Zero respawn seconds changed original formatter");
  text.status=4;check(source_player_status_text(assets,text)==u"You are out of lives","Finite-life status omitted");
  text.status=3;check(source_player_status_text(assets,text)==u"You are sitting out","Sitting-out status omitted");
  text.status=1;check(source_player_status_text(assets,text)==u"Observing","Observer status omitted");
  text.status=5;text.player_score=u"7";text.team_score=u"9";text.score_to_win=u"25";
  check(source_player_status_text(assets,text)==u"Winning with 7 of 25","FFA status score token differs");
  text.team_mode=true;
  check(source_player_status_text(assets,text)==u"Winning with 9 of 25","Team status score token differs");
  text.suppress_playing=true;check(!source_player_status_text(assets,text),"HUD displayed source-suppressed playing status");
  text.lifecycle=2;
  check(source_player_status_text(assets,text)==vista_lobby_metadata_utf16(assets.round_over_label),"Round lifecycle label differs");
  text.special_mode=true;
  check(source_player_status_text(assets,text)==vista_lobby_metadata_utf16(assets.game_over_label),"Special-mode game-over precedence differs");
  text.engine_present=false;check(!source_player_status_text(assets,text),"Status rendered without source engine");
  auto bank=std::make_shared<GameplayFontBank>(cache.shared_directory()/"fonts");
  unsigned draws{};
  for(const auto& row:fixture.at("draw_rectangles")) {
    const auto selector=row.at("font").get<unsigned>();const auto font=bank->font(selector);
    if(font->ascent+font->descent+font->leading!=row.at("line_height"))continue;
    HudContent content;content.unit_representation=row.at("species").get<unsigned>();
    Mesh mesh;const auto players=row.at("players").get<unsigned>(),view=row.at("view").get<unsigned>();
    MultiplayerHud hud(content,cache.shared_directory()/"fonts",mesh,players,"Status",{}, {},0,bank);
    MultiplayerState state;state.players[0].connected=true;
    const auto actual=hud.draw_player_status(u"Respawn in 3",state,0,view,players,640,480,
        *original->full_scoreboard_paint_constants,players==2?std::optional{TwoPlayerSplit::Horizontal}:std::nullopt,1);
    const auto material=std::find_if(mesh.materials.begin(),mesh.materials.end(),[&](const auto& m){
      return m.name=="player-status/font"+std::to_string(selector);});
    check(material!=mesh.materials.end(),"Player status did not select the original font bank");
    const unsigned id=unsigned(material-mesh.materials.begin());
    FullScoreboardLayout layout;layout.status=FullScoreboardStatus::Ready;
    layout.title.valid=true;layout.title.right_justified=true;layout.title.text=u"Respawn in 3";
    const auto rectangle=row.at("rectangle").get<std::array<std::int16_t,4>>();
    layout.title.text_rectangle={rectangle[0],rectangle[1],rectangle[2],rectangle[3]};
    const auto encoded=row.at("foreground_hex").get<std::string>();
    for(unsigned i=0;i<4;++i) {
      std::uint32_t bits{};
      for(unsigned b=0;b<4;++b)bits|=std::stoul(encoded.substr((i*4+b)*2,2),nullptr,16)<<(b*8);
      layout.title.foreground[i]=std::bit_cast<float>(bits);
    }
    const auto outer=row.at("outer").get<std::array<std::int16_t,4>>();
    FullScoreboardPaintFrame frame;frame.clip={0,0,std::int16_t(outer[2]-outer[0]),std::int16_t(outer[3]-outer[1])};
    frame.clock={30,tick_seconds,0};frame.shadow_enabled=true;frame.x87_control_word=0x27f;
    frame.font_horizontal_offset=std::int16_t(gameplay_font_horizontal_offset(bank->source(),selector));
    auto expected=FullScoreboardPainter(*font,id,id).draw(layout,*original->full_scoreboard_paint_constants,frame);
    check(expected.status==FullScoreboardPaintStatus::Ready && !actual.empty() &&
        actual.size()==expected.draw.vertices.size(),"Player status did not produce original glyphs/shadows");
    for(unsigned n=0;n<actual.size();++n) {
      const auto& a=actual[n];auto e=expected.draw.vertices[n];
      e.position.x+=outer[1];e.position.y+=outer[0];
      check(a.position.x==e.position.x && a.position.y==e.position.y && a.position.z==e.position.z &&
          a.color.x==e.color.x && a.color.y==e.color.y && a.color.z==e.color.z &&
          a.uv.x==e.uv.x && a.uv.y==e.uv.y && a.material==e.material,"Player status differs from original draw geometry/color");
    }
    ++draws;
  }
  check(draws==8,"Original solo/split/species draw coverage changed");
  check(fixture.at("cases").size()==896,"Original selector coverage changed");
  std::cout<<"PASS 896 unchanged TU1.5 selector cases,23 actual status rows, lifecycle labels and original death/status tokens\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
