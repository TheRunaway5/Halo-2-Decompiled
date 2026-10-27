#include "helpers/gameplay_fixture.hpp"
#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
void place(MultiplayerMatch& match,unsigned id,Vec3 position) {
  Spawn spawn;spawn.position=position;
  test::place_player(match,id,spawn);
}
void score_format_matrix(const Cache &cache) {
  const std::array<unsigned,1> ids{0};unsigned cases{};
  for(unsigned engine=0;engine<7;++engine) {
    UiVariant variant;variant.engine=engine;
    const auto type=source_game_variant_type(source_game_variant(variant));
    Mesh mesh;MultiplayerScoreboard board(cache,gameplay_font_directory(cache),mesh,variant);
    check(board.available(),"Mode scoreboard providers unavailable");
    for(unsigned tick=0;tick<8;++tick)board.step(0,true);
    for(bool teams:{false,true})for(bool finished:{false,true}) {
      MultiplayerState state;state.tick=100;state.source_engine=type;state.team_mode=teams;
      state.finished=state.results_finalized=finished;state.retained_team_mask=1;
      state.team_scores[0]=65;state.team_total_scores[0]=125;
      auto &player=state.players[0];player.connected=true;player.name="Mode score";
      player.source_player=0x10000;player.source_unit=0x10001;
      player.source_flags=1;player.source_membership=0;player.score=65;player.total_score=125;
      const auto draw=board.draw(state,ids,0,640,480);
      check(draw.layout.status==FullScoreboardStatus::Ready && draw.layout.rows.size()==1,
          "Mode scoreboard omitted its actual adapter row");
      const bool timed=type==3 || type==4 || type==8;
      const std::u16string expected=timed?(finished?u"2:05":u"1:05"):(finished?u"125":u"65");
      if(draw.layout.rows.front().widgets[4].text!=expected)
        throw FormatError("Mode scoreboard score format differs from TU1.5: source type="+
            std::to_string(type)+" team="+std::to_string(teams)+" finished="+std::to_string(finished));
      if(teams)check(draw.layout.team_rows.size()==1 && draw.layout.team_rows.front().widgets[3].text==expected,
          "Mode scoreboard team score format differs from TU1.5");
      ++cases;
    }
  }
  std::cout<<"PASS "<<cases<<" native mode scoreboard active/final FFA/team score-format cases\n";
}
}
int main(int argc,char**argv)try {
  check(argc==4,"Supply MAP SHARED_MAPS UPDATE_XBE|native-mcc");
  const bool native_mcc=std::string_view(argv[3])=="native-mcc";
  const bool formats_only=std::getenv("HALO2_SCOREBOARD_FORMAT_MATRIX")!=nullptr;
  if((!formats_only && !std::getenv("DISPLAY")) || !std::filesystem::exists(argv[1]) ||
      (!native_mcc && !std::filesystem::exists(argv[3])))return 77;
  SDL_SetMainReady();MultiplayerOptions options;
  options.map=argv[1];options.shared_maps=argv[2];if(!native_mcc)options.gameplay_xbe=argv[3];
  options.width=640;options.height=480;options.starting_weapon=5;options.mute=true;
  options.validation=std::getenv("HALO2_DEATH_VALIDATION");
  Cache cache(options.map,options.shared_maps,native_mcc?nullptr:OriginalData::load_xbe(options.gameplay_xbe));
  check(!native_mcc || cache.is_mcc(),"native-mcc requires the prepared imported MCC cache");
  if(formats_only){score_format_matrix(cache);return 0;}
  auto content=cache.load_content(0,true);
  // Explicit native debug arena. Actual stock SMG damage, authority, death,
  // countdown, source-slot respawn and the ordinary renderer remain live.
  content.collision.clear();content.kill_triggers.clear();
  auto rules=slayer_rules({"Slayer",0,{{89,5}}});
  rules.weapon=cache.slayer_starting_weapon(5);rules.respawn_ticks=30;rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Shooter");
  for(unsigned n=0;n<100;++n)match.step({});
  LanHost host(match,0,multiplayer_fingerprint(options.map,cache.shared_directory(),0,rules.weapon,options.gameplay_xbe));
  options.join=host.join_address();
  std::atomic<bool> advance{};std::exception_ptr failure;
  std::jthread worker([&](std::stop_token stop) {
    try {
      bool placed{},killed{};std::array<Controls,16> controls{};
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(controls);
        if(advance && match.player(1).connected) {
          if(!placed) {
            place(match,0,{0,0,0});place(match,1,{1,0,0});placed=true;
          }
          killed|=match.player(1).deaths!=0;controls[0].fire=!killed;
          match.step(controls);
        }
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){failure=std::current_exception();}
  });
  MultiplayerState state;bool dead{},full{},respawn{},restored{},quitting{};
  std::uint32_t original_unit=null_datum;unsigned calls{};
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& current) {
    state=current;advance=true;check(state.tick<450,"Rendered death cycle did not complete");
    const auto& victim=state.players[1];
    if(victim.connected && !dead && victim.source_unit!=null_datum)original_unit=victim.source_unit;
    if(victim.dead) {
      check(victim.source_unit==null_datum && victim.deaths==1 && state.players[0].kills==1,
          "Rendered snapshot did not originate from the host's actual SMG fatal hit");dead=true;
    }
    if(dead && !victim.dead) {
      check(victim.source_unit!=null_datum && victim.source_unit!=original_unit,
          "Rendered respawn did not replace the old salted unit");respawn=true;
    }
  };
  options.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout& layout,
      const FullScoreboardPaintResult& paint) {
    check(view==0,"Unexpected local death view");++calls;
    if(!normal) {
      check(dead && layout.rows.size()==2 && !paint.draw.vertices.empty(),
          "Automatic death scoreboard did not reach GPU UI submission");
      check(!paint.emblems_unavailable,"Automatic death scoreboard omitted original player/death icons");
      check(std::any_of(layout.rows.begin(),layout.rows.end(),[&](const auto& row) {
        return row.current_player && row.datum==state.players[1].source_player;
      }),"Rendered automatic scoreboard lost the victim highlight");full=true;
    }
    if(respawn && full && normal && paint.draw.vertices.empty()) {
      restored=true;
      if(!quitting){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Queue death-cycle quit");quitting=true;}
    }
  };
  const auto result=run_multiplayer(options);
  worker.request_stop();worker.join();if(failure)std::rethrow_exception(failure);
  check(result==0 && dead && full && respawn && restored && calls>10,
      "Actual rendered UDP fatal hit/automatic scoreboard/respawn/HUD cycle incomplete");
  std::cout<<"PASS actual UDP SMG death, automatic source scoreboard GPU submission without F3, salted respawn and HUD restoration\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
