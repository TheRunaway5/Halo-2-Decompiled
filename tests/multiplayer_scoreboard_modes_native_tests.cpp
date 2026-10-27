#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/vista_lobby_metadata.hpp"
#include "halo2/gameplay_font_source.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
MultiplayerState published_state(unsigned engine,bool teams) {
  MultiplayerState state;state.tick=120;state.source_engine=source_game_variant_type(source_game_variant_default(engine));
  state.team_mode=teams;state.retained_team_mask=3;state.round_mode=3;state.score_limit=25;
  for(unsigned i=0;i<4;++i) {
    auto &p=state.players[i];p.connected=true;p.name="Player "+std::to_string(i);
    p.source_player=0x80000000u+i;p.source_unit=0x90000000u+i;p.source_flags=1;
    p.source_membership=teams?std::int16_t(i/2):std::int16_t(0);
    p.score=125;p.total_score=185;p.wins=1;p.kills=20-5*i;
  }
  state.team_scores[0]=state.team_scores[1]=125;
  state.team_total_scores[0]=state.team_total_scores[1]=185;
  state.team_wins[0]=state.team_wins[1]=1;
  return state;
}
void mode(const Cache &cache,const std::filesystem::path &fonts,unsigned engine,bool teams) {
  UiVariant variant;variant.engine=engine;variant.values.set(91,teams?1:0);variant.values.set(0,3);variant.values.set(12,1);
  Mesh mesh;MultiplayerScoreboard board(cache,fonts,mesh,variant);check(board.available(),"Native scoreboard unavailable");
  auto state=published_state(engine,teams);const std::array<unsigned,4> ids{0,1,2,3};
  for(unsigned view=0;view<4;++view)for(unsigned i=0;i<8;++i)board.step(view,true);
  for(bool final:{false,true}) {
    state.results_finalized=state.finished=final;
    for(unsigned view=0;view<4;++view) {
      const auto out=board.draw(state,ids,view,1280,720);
      check(!out.normal_hud&&out.layout.status==FullScoreboardStatus::Ready&&out.layout.rows.size()==4,
          "Native split scoreboard did not deliver four published players");
      check(!out.paint.draw.vertices.empty()&&!out.paint.emblems_unavailable,"Native scoreboard omitted fonts/player/death materials");
      const bool timed=engine==1||engine==3||engine==6;
      const std::u16string expected=timed?(final?u"3:05":u"2:05"):(final?u"185":u"125");
      for(const auto &row:out.layout.rows) {
        check(row.widgets[4].valid&&row.widgets[4].text==expected,"Actual native player score used another engine formatter");
        check(row.current_player==(row.datum==state.players[view].source_player),"Native split scoreboard highlighted another player");
        check(row.widgets[3].valid==!teams,"Native scoreboard wins column differs from round/team options");
      }
      check(out.layout.team_rows.size()==(teams?2u:0u),"Native scoreboard retained wrong team count");
      for(const auto &row:out.layout.team_rows)
        check(row.widgets[3].valid&&row.widgets[3].text==expected&&row.widgets[2].text==u"1",
            "Actual native team score/wins lost source mode");
      if(final&&!teams)for(unsigned i=0;i<4;++i)
        check(out.layout.rows[i].datum==state.players[i].source_player,"Native constructor omitted extended final comparison flag");
    }
  }
  // Original15E6BC compares displayed player+170 against1. Raw player+164
  // remains60/90 ticks for the independent original status text selector.
  for(unsigned view=0;view<4;++view)for(unsigned i=0;i<8;++i)board.step(view,false);
  state.results_finalized=state.finished=false;state.players[0].source_unit=null_datum;
  state.players[0].source_respawn_countdown=90;state.players[0].source_respawn_seconds=2;
  board.step(0,false,state,0);
  check(board.draw(state,ids,0,1280,720).layout.status==FullScoreboardStatus::InvalidInput,
      "Automatic scoreboard appeared above original displayed countdown threshold");
  state.players[0].source_respawn_countdown=60;state.players[0].source_respawn_seconds=1;
  board.step(0,false,state,0);
  const auto death=board.draw(state,ids,0,1280,720);
  check(death.layout.status==FullScoreboardStatus::Ready,"Automatic scoreboard compared raw respawn ticks instead of displayed seconds");
  const auto dead=std::find_if(death.layout.rows.begin(),death.layout.rows.end(),[&](const auto &row){return row.datum==state.players[0].source_player;});
  check(dead!=death.layout.rows.end()&&dead->icon_kind==1,"Native death row lost original dead-player icon");
}
std::u16string score_text(int score,int source_kind) {
  const auto number=[](int value){const auto s=std::to_string(value);return std::u16string(s.begin(),s.end());};
  if(source_kind!=3 && source_kind!=4 && source_kind!=8)return number(score);
  const auto minutes=score/60,seconds=score%60;
  return (minutes?number(minutes):u" ")+std::u16string(u":")+
      (seconds<10?u"0":u"")+number(seconds);
}
std::u16string authored_status(const FullScoreboardAssets &assets,const MultiplayerState &state,unsigned slot) {
  const auto &player=state.players[slot];const auto &status=*player.source_status;
  if(status.lifecycle!=1) {
    if(status.special_mode)return vista_lobby_metadata_utf16(assets.game_over_label);
    return vista_lobby_metadata_utf16(status.lifecycle==2?assets.round_over_label:assets.waiting_for_next_round_label);
  }
  const auto row=std::find_if(assets.player_status.begin(),assets.player_status.end(),
      [&](const auto &value){return value.status==status.status;});
  check(row!=assets.player_status.end(),"Original authored player status fixture missing");
  auto text=vista_lobby_metadata_utf16(state.team_mode?row->team_text:row->text);
  const auto replace=[&](std::u16string_view token,const std::u16string &value) {
    std::size_t at=0;
    while((at=text.find(token,at))!=std::u16string::npos){text.replace(at,token.size(),value);at+=value.size();}
  };
  replace(u"#local_player_score",score_text(player.score,state.source_engine));
  if(player.source_membership>=0 && player.source_membership<8)
    replace(u"#local_team_score",score_text(state.team_scores[unsigned(player.source_membership)],state.source_engine));
  replace(u"#score_to_win",score_text(state.score_limit,state.source_engine));
  const auto seconds=std::to_string(player.source_respawn_seconds);
  replace(u"#local_spawn_time",std::u16string(seconds.begin(),seconds.end()));
  check(!text.empty() && text.find(u'#')==std::u16string::npos,"Original status fixture retained an unresolved token");
  return text;
}
void authored_mode(const Cache &cache,const std::filesystem::path &fonts,const FullScoreboardAssets &assets,
    const UiVariant &variant) {
  // Declared roster/API states are separate from played engine outcomes.
  // Options retain the unchanged authored preset's serialized source values.
  const auto serialized=source_game_variant(variant);
  const auto setting=[&](int id){return source_game_variant_get(serialized,id);};
  check(source_game_variant_valid(serialized),"Authored preset failed its original variant validator");
  const bool teams=setting(91)!=0;
  auto state=published_state(variant.engine,teams);
  state.round_mode=setting(0);state.score_limit=setting(2);state.life_limit=setting(16);
  state.retained_team_mask=teams?3:0;
  state.source_round_lifecycle_available=true;state.round_phase=1;
  for(auto &player:state.players)if(player.connected) {
    player.source_lives_remaining=1;
    player.source_status.emplace();player.source_status->status=5;player.source_status->lifecycle=1;
  }
  Mesh mesh;MultiplayerScoreboard board(cache,fonts,mesh,variant);
  check(board.available(),"Authored preset scoreboard unavailable");
  const std::array<unsigned,4> ids{0,1,2,3};
  const auto draw=[&](bool dead) {
    const bool game_over=state.source_round_lifecycle_available?state.finished:state.results_finalized;
    const std::array<bool,3> columns{game_over || state.score_limit!=1,
        state.round_mode!=0,state.round_mode!=0 && !teams};
    const auto expected_score=score_text(game_over?185:125,state.source_engine);
    for(unsigned view=0;view<4;++view) {
      const auto out=board.draw(state,ids,view,1280,720);
      check(!out.normal_hud && out.layout.status==FullScoreboardStatus::Ready && out.layout.rows.size()==4 &&
          !out.paint.draw.vertices.empty() && !out.paint.emblems_unavailable,"Authored preset omitted rendered roster/font/icons");
      check(out.layout.columns==columns,"Authored preset lost original score/round/team column gates");
      const auto expected_title=authored_status(assets,state,view);
      check(out.layout.title.valid && out.layout.title.text==expected_title,
          "Authored preset title differs from original status row/token/lifecycle text");
      check(board.status_text(state,view,false,80)==expected_title,
          "Authored preset status API and full title disagree");
      for(const auto &row:out.layout.rows) {
        check(row.current_player==(row.datum==state.players[view].source_player),
            "Authored preset highlighted a different local view");
        check(row.widgets[4].valid==columns[0] && (!columns[0] || row.widgets[4].text==expected_score),
            "Authored preset score column/formatter differs");
        check(row.widgets[3].valid==columns[2],"Authored preset individual wins column differs");
        check(row.icon_kind==(dead && state.round_phase==1?1:0),"Authored preset death/final icon admission differs");
      }
      check(out.layout.team_rows.size()==(teams?2u:0u),"Authored preset retained another team roster");
      for(const auto &row:out.layout.team_rows)
        check(row.widgets[3].valid==columns[0] && (!columns[0] || row.widgets[3].text==expected_score) &&
            row.widgets[2].valid==columns[1],"Authored preset team score/wins columns differ");
    }
  };
  for(unsigned view=0;view<4;++view)for(unsigned tick=0;tick<8;++tick)board.step(view,true,state,view);
  draw(false);
  for(unsigned view=0;view<4;++view)
    check(!board.status_text(state,view,true),"HUD failed to suppress ordinary winning status");
  for(unsigned view=0;view<4;++view)for(unsigned tick=0;tick<8;++tick)board.step(view,false);
  for(auto &player:state.players)if(player.connected) {
    player.source_unit=null_datum;player.source_respawn_countdown=60;player.source_respawn_seconds=1;
    player.source_status->status=2;
  }
  for(unsigned view=0;view<4;++view)for(unsigned tick=0;tick<8;++tick)board.step(view,false,state,view);
  draw(true);
  state.finished=state.results_finalized=true;state.round_phase=2;
  for(auto &player:state.players)if(player.connected) {
    player.source_status->lifecycle=2;player.source_status->special_mode=true;
  }
  draw(true);
  //161BA0 uses distinct round/waiting labels when Game+1128 is clear.
  for(const std::int16_t lifecycle:{2,3}) {
    state.round_phase=lifecycle;state.finished=state.results_finalized=false;
    for(auto &player:state.players)if(player.connected) {
      player.source_status->lifecycle=lifecycle;player.source_status->special_mode=false;
    }
    draw(true);
  }
}
void lifecycle_gates(const Cache &cache,const std::filesystem::path &fonts) {
  const auto original=cache.original_data();const std::array<unsigned,4> ids{0,1,2,3};
  for(const std::int16_t lifecycle:{0,1,2,3})for(const std::uint8_t options_state:{0,4})
    for(const std::int32_t engine_status:{0,1}) {
      auto state=published_state(0,false);state.source_round_lifecycle_available=true;state.round_phase=lifecycle;
      auto &player=state.players[0];player.source_status.emplace();
      player.source_status->lifecycle=lifecycle;player.source_status->options_state=options_state;
      player.source_status->engine_status=engine_status;
      Mesh mesh;UiVariant variant;MultiplayerScoreboard board(cache,fonts,mesh,variant);
      FullScoreboardHeldState expected;FullScoreboardHeldInput input;
      input.local_view=input.input_user=0;input.processed_record_present=input.engine_present=true;
      input.lifecycle=lifecycle;input.options_state=options_state;input.engine_status=engine_status;
      input.current_unit=player.source_unit;input.player_flags=player.source_flags;
      const FullScoreboardClock clock{30,tick_seconds,std::int32_t(state.tick)};
      for(unsigned tick=0;tick<40;++tick) {
        check(full_scoreboard_update_held(expected,input,clock,FullScoreboardRounding::NearestEven,
            *original->full_scoreboard_constants),"Original lifecycle input leaf rejected declared fields");
        board.step(0,false,state,0);const auto draw=board.draw(state,ids,0,1280,720);
        check(draw.normal_hud==full_scoreboard_normal_hud(true,0,expected.counters[0],clock,
            *original->full_scoreboard_constants),"Native scoreboard lifecycle/options/status input differs");
        if(expected.counters[0])check(draw.layout.input_alpha==full_scoreboard_display_alpha(expected.counters[0],clock,
            *original->full_scoreboard_constants,full_scoreboard_host_power),"Native lifecycle fade differs from original gate leaf");
        else check(draw.layout.rows.empty(),"Native lifecycle displayed an unadmitted roster");
      }
    }
}
void captured_gates(const Cache &cache,const std::filesystem::path &fonts) {
  const std::array<unsigned,4> ids{0,1,2,3};
  for(unsigned scenario=0;scenario<8;++scenario) {
    auto state=published_state(0,false);auto &player=state.players[0];
    player.source_status.emplace();auto &gate=player.source_status->scoreboard_gate.emplace();
    gate.tick=101;gate.respawn_seconds=1;gate.current_unit=null_datum;gate.player_flags=1;
    gate.life_limit=2;gate.lives_remaining=1;gate.lifecycle=1;
    // Deliberately contradict the captured source phase with later native
    // projection fields. Only the captured15E6BC owners may select the gate.
    if(scenario==0) {
      state.tick=0;player.source_unit=0x90000000u;player.source_respawn_seconds=99;
      player.source_status->lifecycle=2;player.source_status->options_state=0;
      player.source_status->engine_status=2;
    } else {
      state.tick=200;player.source_unit=null_datum;player.source_respawn_seconds=0;
      switch(scenario) {
      case 1:gate.tick=90;break;
      case 2:gate.player_flags=0x4001;break;
      case 3:gate.death_observer_excluded=true;break;
      case 4:gate.lives_remaining=0;break;
      case 5:gate.current_unit=0x90000000u;break;
      case 6:gate.respawn_seconds=2;break;
      case 7:gate.player_flags=0;break;
      }
    }
    Mesh mesh;UiVariant variant;MultiplayerScoreboard board(cache,fonts,mesh,variant);
    for(unsigned tick=0;tick<12;++tick)board.step(0,false,state,0);
    const auto draw=board.draw(state,ids,0,1280,720);
    if(scenario==0)check(!draw.normal_hud && draw.layout.status==FullScoreboardStatus::Ready &&
        !draw.paint.draw.vertices.empty(),"Native scoreboard ignored the captured source gate in favor of later projection state");
    else check(draw.normal_hud && draw.layout.rows.empty() && draw.paint.draw.vertices.empty(),
        "Native scoreboard replaced a captured source rejection with later ready/dead state");
  }
}
void split_scroll(const Cache &cache,const std::filesystem::path &fonts) {
  Mesh mesh;UiVariant variant;MultiplayerScoreboard board(cache,fonts,mesh,variant);
  auto state=published_state(0,false);const std::array<unsigned,4> ids{0,1,2,3};
  for(unsigned i=4;i<16;++i) {
    state.players[i]=state.players[0];state.players[i].source_player=0x80000000u+i;
    state.players[i].source_unit=0x90000000u+i;state.players[i].name="Player "+std::to_string(i);
  }
  for(unsigned view=0;view<2;++view)for(unsigned i=0;i<8;++i)board.step(view,true,state,ids[view]);
  check(board.draw(state,ids,1,1280,720).layout.scroll==0,"Initial split scoreboard scroll changed");
  const auto down=multiplayer_scoreboard_scroll_axis(32767),up=multiplayer_scoreboard_scroll_axis(-32768);
  for(unsigned i=0;i<100;++i)board.step(1,true,state,1,down);
  const auto checkpoint=board.checkpoint();auto prepared=board.prepare_restore(checkpoint);
  check(board.restore_ready(prepared),"Actual scroll checkpoint was not prepared");
  const auto bottom=board.draw(state,ids,1,1280,720);
  check(bottom.layout.status==FullScoreboardStatus::Ready&&bottom.layout.scroll>0,"Right stick did not scroll16-player split roster");
  check(!board.restore_ready(prepared)&&!board.commit_restore(prepared),"Source scroll clamp did not retire prepared CPU revision");
  board.step(1,true,state,1,up);const auto reversed=board.draw(state,ids,1,1280,720);
  check(reversed.layout.scroll<bottom.layout.scroll,"Reversing at bottom retained an unbounded scroll accumulator");
  check(board.draw(state,ids,0,1280,720).layout.scroll==0,"Scoreboard scrolling leaked into another local view");
}
}
int main(int argc,char **argv)try {
  if(argc!=3)throw std::runtime_error("Supply map and authenticated gameplay XBE|native-mcc");
  const bool native_mcc=std::string_view(argv[2])=="native-mcc";
  if(!std::filesystem::exists(argv[1])||(!native_mcc&&!std::filesystem::exists(argv[2])))return 77;
  const std::filesystem::path map=argv[1];const auto original=native_mcc?nullptr:OriginalData::load_xbe(argv[2]);
  const Cache cache(map,{},original);
  check(!native_mcc||cache.is_mcc(),"native-mcc requires the prepared imported MCC cache");
  const auto fonts=gameplay_font_directory(cache);
  for(unsigned engine=0;engine<7;++engine)for(bool teams:{false,true})mode(cache,fonts,engine,teams);
  split_scroll(cache,fonts);
  lifecycle_gates(cache,fonts);
  captured_gates(cache,fonts);
  const Cache menu(map.parent_path()/"mainmenu.map",{},original);const auto frontend=menu.load_frontend();
  check(!native_mcc||menu.is_mcc(),"Native scoreboard preset catalog lost its matching MCC mainmenu");
  check(frontend.variants.size()==39,"Original authored preset catalog changed");
  const auto assets=cache.load_full_scoreboard_assets();std::array<unsigned,7> families{};
  for(const auto &variant:frontend.variants) {
    check(variant.engine<families.size(),"Authored preset has no original family");
    ++families[variant.engine];
    try{authored_mode(cache,fonts,assets,variant);}
    catch(const std::exception &e){throw std::runtime_error(variant.name+": "+e.what());}
  }
  check(families==std::array<unsigned,7>{9,6,5,5,4,5,5},"Original seven-family preset coverage changed");
  std::cout<<"PASS actual Cache/font/scoreboard constructors and draw: seven synthetic FFA/team modes plus all39 unchanged authored presets, four views, active/death/final column gates and formatting, original status/token/lifecycle titles, displayed countdown and per-view16-player scrolling; roster/status API states are declared inputs, not played matches\n";
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
