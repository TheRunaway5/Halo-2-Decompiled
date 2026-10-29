#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void key(Uint32 window,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.keysym.scancode=SDL_SCANCODE_TAB;e.key.keysym.sym=SDLK_TAB;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;check(SDL_PushEvent(&e)==1,"Queue real Tab event");
}
}
int main(int argc,char **argv) {
  try {
    if(argc<4 || argc>8)throw std::runtime_error("Supply original MAP SHARED_MAPS UPDATE_XBE [--split4] [--wide] [--team] [--end|--end-capture]");
    const bool native_mcc=std::string_view(argv[3])=="native-mcc";
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || (!native_mcc && !std::filesystem::exists(argv[3])))return 77;
    SDL_SetMainReady();MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];
    if(native_mcc)check(Cache(options.map,options.shared_maps).is_mcc(),"native-mcc requires an imported MCC cache");
    else options.gameplay_xbe=argv[3];
    float vista_start=2;unsigned vista_vertices=0;
    if(const auto *assets=std::getenv("HALO2_TEST_HUD_UI_ASSETS")) {
      options.ui_assets=assets;
      auto font=load_retail_font(options.shared_maps/"fonts/conduit-13");
      const auto old_width=font.atlas.width;
      const auto sampled=load_retail_font(hud_sampled_font_path(options.ui_assets,6,"conduit-13"));
      check((is_mcc_classic_hud_pack(options.ui_assets)?
          apply_mcc_classic_font_sampling(font,sampled):apply_retail_font_sampling(font,sampled))>0,
            "Scoreboard runtime has no selected font artwork");
      vista_start=float(old_width+1)/font.atlas.width;
    }
    const auto check_artwork=[&](const FullScoreboardPaintResult &paint) {
      if(paint.draw.vertices.empty())return;
      check(!paint.emblems_unavailable,"Actual scoreboard omitted player emblem/death artwork");
      if(options.ui_assets.empty())return;
      unsigned sampled=0;
      for(const auto &v:paint.draw.vertices)sampled+=v.uv.x>=vista_start && v.uv.x<1;
      check(sampled>0,"Actual full scoreboard did not submit Vista glyph artwork");
      check(!paint.emblems_unavailable,"Actual scoreboard still omits Vista emblem/death artwork");
      vista_vertices+=sampled;
    };
    options.width=640;options.height=480;options.local_players=1;options.starting_weapon=5;
    bool team_play=false,end_mode=false,capture_end=false;
    for(int i=4;i<argc;++i) {
      if(std::string(argv[i])=="--split4")options.local_players=4;
      else if(std::string(argv[i])=="--wide"){options.width=1280;options.height=720;}
      else if(std::string(argv[i])=="--team")team_play=true;
      else if(std::string(argv[i])=="--end")end_mode=true;
      else if(std::string(argv[i])=="--end-capture")end_mode=capture_end=true;
      else check(false,"Unknown roster probe option");
    }
    if(team_play) {
      const Cache menu_cache(options.map.parent_path()/"mainmenu.map");const auto content=menu_cache.load_frontend();
      const auto variant=std::find_if(content.variants.begin(),content.variants.end(),
          [](const auto &v){return v.name.starts_with("Team Slayer ");});
      check(variant!=content.variants.end(),"Original authored Team Slayer row missing");
      MenuState menu(content);
      const auto choose=[&](unsigned row,unsigned screen){menu.select(row);menu.accept();check(menu.frame().screen==screen,"Actual Team Slayer menu route changed");};
      const auto entry=[&](int value) {
        const auto &rows=menu.frame().entries;const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto &row){return row.value==value;});
        check(found!=rows.end(),"Authored Team Slayer menu selector missing");return unsigned(found-rows.begin());
      };
      menu.accept();choose(0,167);menu.text("Rendered Team");menu.commit_text();menu.back();
      choose(2,30);choose(0,14);choose(1,23);choose(1,207);choose(entry(variant->engine),208);
      choose(entry(int(variant-content.variants.begin())),14);
      menu.auxiliary();menu.navigate(1,0);menu.navigate(1,0);menu.accept();
      menu.select(0);menu.accept();check(menu.launch && menu.launch->selected_teams==std::vector<std::int8_t>{2},"Normal menu Team Slayer handoff failed");
      options.variant=menu.launch->variant;options.selected_teams=menu.launch->selected_teams;
      check(menu.launch->local_players==1,"One signed-in profile invented additional users");
      // Additional views are explicit test participants, not frontend sign-ins.
      if(options.local_players==4)options.selected_teams={2,7,2,7};
    }
    for(unsigned i=0;i<options.local_players;++i) {
      const std::array<std::uint8_t,9> appearance{10,0,std::uint8_t(2+i),std::uint8_t(3+i),0,
          std::uint8_t(7+i),std::uint8_t(11+i),std::uint8_t(i&1),0};
      options.selected_profiles.push_back(native_selected_player_configuration("Emblem "+std::to_string(i+1),appearance,
          options.selected_teams.empty()?-1:options.selected_teams[i]));
    }
    options.mute=true;options.validation=std::getenv("HALO2_SCOREBOARD_VALIDATION");
    options.desktop=std::make_shared<DesktopSession>();
    auto *window=options.desktop->window(options.width,options.height,false);const auto window_id=SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    if(end_mode) {
      // Editable source parameter10 owns seconds. No native finished flag,
      // score, Tab press or finalized statistic is supplied to this runtime.
      options.variant.values.append(10,1);options.frames=120;
      unsigned full_mask{},calls{};bool finalized=false;MultiplayerState state;
      if(const auto* path=std::getenv("HALO2_SCOREBOARD_CAPTURE"))options.screenshot=path;
      if(capture_end) {
        check(!options.screenshot.empty(),"Pre-carnage capture requires an output path");
        options.frames=0;options.capture_tick=120;
      }
      options.input_observer=[&](std::uint32_t step,std::span<const Controls>,const MultiplayerState& current) {
        state=current;finalized|=current.results_finalized;
        check(step<900,"Automatic results were not submitted within30 seconds");
        if(current.finished) {
          check(current.results_finalized && current.tick>=31 &&
              current.game_over_countdown==std::max(240-int(current.tick),0),
              "Actual time expiry/postgame world omitted finalization or countdown");
          check(current.game_over_fade_started==(current.tick>=180),"Actual runtime fade boundary differs");
        }
      };
      options.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout& layout,const FullScoreboardPaintResult& paint) {
        ++calls;if(normal)return;
        check_artwork(paint);
        check(state.finished && state.results_finalized && layout.rows.size()==options.local_players &&
            !paint.draw.vertices.empty(),"Actual automatic final scoreboard lacks source roster/cells");
        for(const auto& row:layout.rows)check(row.widgets[4].text==u"0","Actual tied final score changed");
        check(layout.team_rows.size()==(team_play?(options.local_players==4?2u:1u):0u),"Automatic results lost selected teams");
        full_mask|=1u<<view;
      };
      // Finite capture pacing guarantees time for the actual one-second limit
      // plus the source display delay, independently of GPU frame throughput.
      options.frame_observer=[](const MultiplayerFrameTiming& frame) {if(frame.submitted)SDL_Delay(80);};
      check(run_multiplayer(options)==0 && finalized &&
          (capture_end ? state.tick==120&&!state.game_over_fade_started
                       : state.game_over_complete()&&state.game_over_fade_started) &&
          full_mask==((1u<<options.local_players)-1) && calls>10,
          "Actual production time expiry/automatic result submission failed");
      std::cout<<(capture_end
          ? "PASS actual automatic pre-carnage scoreboard on every local view without Tab, captured before the source fade\n"
          : "PASS actual rendered source time-limit ending, continuing world clock, completed postgame countdown, fade-start and automatic scoreboard on every local view without Tab\n");
      if(!options.ui_assets.empty())std::cout<<vista_vertices<<" Vista artwork vertices submitted by automatic scoreboard\n";
      std::cout<<"LIMIT original session teardown/return-to-lobby, physical audio and full raster/asset fidelity remain open\n";
      return 0;
    }
    unsigned tick{};MultiplayerState current;bool full{},released{},resumed{},partial{},raised{};unsigned frames{};
    std::optional<unsigned> held_start;
    SDL_AddEventWatch([](void *,SDL_Event *event) {
      if(event->type==SDL_WINDOWEVENT && (event->window.event==SDL_WINDOWEVENT_FOCUS_GAINED || event->window.event==SDL_WINDOWEVENT_FOCUS_LOST))
        std::cerr<<"SDL window="<<event->window.windowID<<" focus_event="<<int(event->window.event)<<'\n';
      else if((event->type==SDL_KEYDOWN || event->type==SDL_KEYUP) && event->key.keysym.scancode==SDL_SCANCODE_TAB)
        std::cerr<<"SDL window="<<event->key.windowID<<" Tab="<<(event->type==SDL_KEYDOWN?"down":"up")<<'\n';
      return 1;
    },nullptr);
    options.input_observer=[&](std::uint32_t step,std::span<const Controls> inputs,const MultiplayerState &state) {
      check(inputs.size()==options.local_players,"Actual scoreboard local inputs changed");tick=step;current=state;
      check(state.team_mode==team_play,"Production runtime dropped source Team Play");
      if(team_play) for(unsigned i=0;i<options.local_players;++i)
        check(state.players[i].source_membership==options.selected_teams[i] && state.players[i].source_ffa_identity==i,
              "Actual runtime join lost profile teams or separate player identities");
      check(step<120 || held_start,"Test window never obtained actual SDL keyboard focus");
      if(held_start && step==*held_start+30)key(window_id,false);
      if(held_start && step==*held_start+60)key(window_id,true);
      if(held_start && step==*held_start+91 && options.screenshot.empty()){SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"Quit actual match");}
    };
    options.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint) {
      ++frames;
      check_artwork(paint);
      if(view){check(normal && layout.rows.empty() && paint.draw.vertices.empty(),"Tab leaked into another local player's UI");return;}
      // A real interactive press occurs only after SDL assigns this window
      // keyboard focus. Synthesized focus events do not establish that owner.
      if(!raised){SDL_RaiseWindow(window);raised=true;}
      if(!held_start && SDL_GetKeyboardFocus()==window) {
        held_start=tick;std::cerr<<"Actual SDL focus owner acquired at tick="<<tick<<'\n';key(window_id,true);
      }
      if(!held_start)return;
      const unsigned held_tick=tick-*held_start;
      // The queued SDL_QUIT at91 releases the keyboard owner in
      // MultiplayerKeyboardMouse. The resume assertion covers the held phase
      // before that deliberate shutdown, including all16 ticks75..90.
      if((held_tick>=12 && held_tick<=29) || (held_tick>=75 && held_tick<=90)) {
        if(normal || layout.status!=FullScoreboardStatus::Ready || layout.rows.size()!=options.local_players || paint.draw.vertices.empty())
          std::cerr<<"Roster observation tick="<<tick<<" normal_hud="<<normal<<" layout="<<int(layout.status)
              <<" rows="<<layout.rows.size()<<" paint="<<int(paint.status)<<" vertices="<<paint.draw.vertices.size()<<'\n';
        check(!normal && layout.status==FullScoreboardStatus::Ready && layout.rows.size()==options.local_players && !paint.draw.vertices.empty(),
            "Held actual Tab did not submit source roster/font/cells and suppress normal HUD");
        unsigned local{};
        for(const auto &row:layout.rows) {
          const auto slot=row.datum&0xffffu;
          check(slot<options.local_players && row.datum==current.players[slot].source_player,"Source full roster identity changed");
          const auto &text=row.widgets[2].text;const auto &name=current.players[slot].name;
          check(text==std::u16string(name.begin(),name.end()),"Original sanitizer/native name delivery changed");
          check(row.widgets[4].text==u"0","Published real native score not delivered to source row");
          local+=row.current_player;
        }
        check(local==1,"Full datum current-player highlight lost");
        if(team_play) {
          check(layout.team_rows.size()==(options.local_players==4 ? 2u : 1u),"Actual rendered Team roster missing retained teams");
          for(const auto &row:layout.team_rows)check(row.team==2 || row.team==7,"Rendered roster invented a team assignment");
        } else check(layout.team_rows.empty(),"FFA rendered a Team roster");
        partial|=!paint.emblems_unavailable;
        if(held_tick<30)full=true;else resumed=true;
      }
      if(held_tick>=45 && held_tick<=59){check(normal && paint.draw.vertices.empty(),"Released Tab did not restore normal HUD");released=true;}
    };
    if(const auto *path=std::getenv("HALO2_SCOREBOARD_CAPTURE")){options.screenshot=path;options.capture_tick=120;}
    check(run_multiplayer(options)==0 && full && released && resumed && partial && frames>10,
        "Actual Tab hold/release/resume and selected artwork not observed");
    std::cout<<"PASS actual SDL Tab hold/release/resume, source full-identity roster/rank/name/score, original palette/font/cells, normal HUD suppression and independent local views\n";
    std::cout<<"LIMIT this probe covers manual active-Slayer display; watched-player observers, measured network quality and all Xbox math/raster parity remain separate\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
