#define SDL_MAIN_HANDLED
#include "halo2/online_matchmaking.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include "halo2/online_account.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_terminal_recovery.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include <nlohmann/json.hpp>
#include <SDL.h>
#include <curl/curl.h>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <thread>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
const char *stage="sign-in";
std::optional<std::uint64_t> resident_kib() {
  std::ifstream status("/proc/self/status");std::string key,line;
  while(status>>key) {
    if(key=="VmRSS:"){std::uint64_t value;if(status>>value)return value;return {};}
    std::getline(status,line);
  }
  return {};
}
std::optional<std::uint64_t> peak_resident_kib() {
#if defined(__unix__) || defined(__APPLE__)
  rusage use{};if(getrusage(RUSAGE_SELF,&use)!=0)return {};
#if defined(__APPLE__)
  return std::uint64_t(use.ru_maxrss)/1024;
#else
  return std::uint64_t(use.ru_maxrss);
#endif
#else
  return {};
#endif
}
template<class F>void wait(F f){const auto end=std::chrono::steady_clock::now()+60s;while(std::chrono::steady_clock::now()<end){if(f())return;std::this_thread::sleep_for(5ms);}throw FormatError(std::string("Couch test timed out: ")+stage);}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
}
int main(int argc,char **argv)try {
  check(argc==9,"Supply ROOT ORIGIN OWNER PORT OUTPUT VISTA XBE MODE");
  SDL_SetMainReady();
  const std::filesystem::path root=argv[1],maps=std::getenv("HALO2_COUCH_TEST_MCC_MAPS")?
      std::filesystem::path(std::getenv("HALO2_COUCH_TEST_MCC_MAPS")):root/"assets/disc/maps",output=argv[5],vista=argv[6];
  // The UI cache and source-exact gameplay projection have independent owners.
  // Keep menu admission on its validated input while selecting the actual map
  // and sibling resources explicitly for rendered source-world qualification.
  const std::filesystem::path gameplay_maps=std::getenv("HALO2_ONLINE_FLOW_GAMEPLAY_MAPS")?
      std::filesystem::path(std::getenv("HALO2_ONLINE_FLOW_GAMEPLAY_MAPS")):maps;
  const std::string owner=argv[3],mode=argv[8];const bool render=mode=="render";
  const bool eos=mode=="terminal-eos"||mode=="terminal-eos-abrupt";
  const bool abrupt=mode=="terminal-abrupt"||mode=="terminal-eos-abrupt";
  const bool terminal=mode=="terminal"||mode=="terminal-abrupt"||eos;
  if(eos&&(!std::getenv("H2_EOS_LIVE_TEST")||std::string_view(std::getenv("H2_EOS_LIVE_TEST"))!="1"))return 77;
  struct EosShutdown {
    bool active{};
    ~EosShutdown(){if(active&&!online_protocol_bridge::eos_process_shutdown()){std::cerr<<"EOS fixture shutdown retained an SDK owner.\n";std::_Exit(1);}}
  } eos_shutdown;
  if(eos){online_protocol_bridge::eos_sdk_register_host();eos_shutdown.active=true;}
  const bool family=mode=="family";
  const std::string family_preset=std::getenv("HALO2_COUCH_FAMILY_PRESET")?std::getenv("HALO2_COUCH_FAMILY_PRESET"):"";
  const bool custom=std::getenv("HALO2_COUCH_TEST_VARIANT")!=nullptr;
  const std::string map=std::getenv("HALO2_COUCH_TEST_MAP")?std::getenv("HALO2_COUCH_TEST_MAP"):"lockout";
  check(!map.empty() && map.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos,"Invalid couch-test map key");
  if(!std::filesystem::exists(maps/(map+".map")))return 77;
  check(std::filesystem::is_regular_file(gameplay_maps/(map+".map")),"Explicit gameplay cache does not contain the selected admitted map");
  std::string browser;DiscordLoginOptions auth;auth.callback_port=std::stoul(argv[4]);
  auto make_login=[&](const std::string &token,unsigned subject){
    return std::make_shared<DiscordLogin>([&](const auto &url){browser=url;return true;},auth,
      [token,subject](const DiscordHttpRequest &r,std::stop_token)->DiscordHttpResponse {
        if(r.url.ends_with("/token"))return {200,"{\"access_token\":\""+token+"\",\"token_type\":\"Bearer\",\"scope\":\"identify\",\"expires_in\":3600}"};
        return {200,"{\"id\":\""+std::to_string(123456789012345678ull+subject)+"\",\"username\":\"test\",\"global_name\":\"Test\"}"};
      });
  };
  auto complete_login=[&](const auto &login){
    stage="OAuth browser launch";wait([&]{return !browser.empty();});
    const auto begin=browser.find("state=")+6;
    const auto url="http://127.0.0.1:"+std::string(argv[4])+"/discord/callback?code=test&state="+browser.substr(begin,browser.find('&',begin)-begin);
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
    curl_easy_setopt(curl.get(),CURLOPT_URL,url.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");
    curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
    check(curl_easy_perform(curl.get())==CURLE_OK,"Couch OAuth callback failed");
    wait([&]{return login->authorization().has_value();});
  };
  auto login=make_login(owner,owner=="one"?0:1);login->start();complete_login(login);
  auto frontend=std::make_shared<FrontendSession>();frontend->discord_login=login;
  const Cache menu(maps/"mainmenu.map");frontend->retain_content(menu.load_frontend());
  frontend->content().vista=load_vista_settings(terminal||family?maps/"mainmenu.map":vista/"maps/mainmenu.map");
  std::optional<SourceGameVariant> family_before,family_expected;bool family_teams{};
  if(family) {
    const auto selected=std::find_if(frontend->content().variants.begin(),frontend->content().variants.end(),[&](const auto &v){
      std::string_view name=v.name;if(name.ends_with(" \xee\x80\x88"))name.remove_suffix(4);return name==family_preset;
    });
    check(selected!=frontend->content().variants.end()&&!family_preset.empty(),"Requested source family preset is absent from the actual menu cache");
    family_before=source_game_variant(*selected);check(source_game_variant_valid(*family_before),"Actual family preset is not source-valid");
    family_expected=*family_before;source_game_variant_set(*family_expected,10,3);selected->values.set(10,3);
    check(source_game_variant(*selected).bytes==family_expected->bytes,"Finite family input changed an original rule other than parameter10");
    family_teams=source_game_variant_get(*family_expected,91)!=0;
  } else for(auto &v:frontend->content().variants)if(v.name.starts_with("Team Slayer")){
    v.values.set(10,terminal?5:8);v.values.set(2,0);
    if(terminal)v.values.set(0,1); // Genuine two-round progress before host loss.
  }
  if(custom)for(auto &v:frontend->content().bundled_variants){v.values.set(10,8);v.values.set(2,0);}
  auto &state=frontend->create_state();state.accept();state.accept();state.text("Couch "+owner);state.commit_text();state.back();
  state.set_discord_login(login);
  OnlineAccountOptions account_options;account_options.endpoint=argv[2];account_options.allow_loopback_http=true;
  account_options.enable_parties=true;account_options.poll_interval=1500ms;
  auto account=std::make_shared<OnlineAccount>(login,account_options);account->connect("Couch "+owner);
  state.set_online_account(account);stage="authenticated primary account";
  wait([&]{state.update_online_login();return account->snapshot().authenticated && !account->snapshot().busy;});
  OnlineMatchmakingOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;options.maps={map};options.poll_interval=20ms;
  options.terminal_recovery=terminal;
  if(eos)options.compatibility="halo2-native-queue-v13";
  auto queue=std::make_shared<OnlineMatchmaking>(login,options);state.set_online_matchmaking(queue);state.update_online_login();state.open(186);
  if(family||terminal) {
    frontend->online_services=std::make_shared<NativeTu15OnlineServices>();
    frontend->online_services->login=login;frontend->online_services->account=account;frontend->online_services->matchmaking=queue;
  }
  for(unsigned p=1;p<4;++p) {
    check(state.controller_slot(100+p,true)==p,"Controller was not assigned its chosen local seat");
    check(state.frame().screen==10,"ONLINE did not open the original profile chooser");
    const auto &entries=state.frame().entries;
    const auto guest=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-80;});
    check(guest!=entries.end(),"ONLINE profile chooser omitted Guest");
    state.select(unsigned(guest-entries.begin()));state.accept();
    check(state.online_couch_screen(state.frame()),"Local player options did not open");
    state.select(0);state.accept();
  }
  check(state.local_players().size()==4,"Four local profiles did not remain selected");
  std::shared_ptr<DiscordLogin> secondary;
  if(mode=="mixed") {
    state.set_local_discord_login_factory([&]{secondary=make_login(owner+"-second",owner=="one"?2:3);return secondary;});
    state.open_online_local_player(1);state.select(1);browser.clear();state.accept();complete_login(secondary);
    state.update_online_login();state.select(1);state.accept();
  }
  unsigned searches{};
  auto search=[&](bool retained=false) {
    const bool quick=mode=="core" && searches++==0;
    if(!retained)state.return_online();state.update_online_login();
    if(quick){state.select(0);state.accept();}
    else if(!retained)state.open(219);
    stage="catalog";
    if(!quick && !retained) {
    wait([&]{state.update_online_login();state.update_online_matchmaking();const auto s=queue->snapshot();if(s.status==OnlineMatchmakingStatus::Failed)throw FormatError(s.message);return s.status==OnlineMatchmakingStatus::Ready;});
    const auto catalog=queue->snapshot().playlists;
    const auto row=std::find_if(catalog.begin(),catalog.end(),[](const auto &p){return p.id=="casual";});
    check(row!=catalog.end() && row->eligible,"Couch casual playlist unavailable");
    state.select(unsigned(row-catalog.begin()));state.accept();check(state.frame().screen==14,"Playlist selection did not return directly to its original pregame lobby");
    }
    check(state.frame().screen==14,"Quickmatch skipped its original pregame lobby");
    stage="actual party hopper consent";
    wait([&]{state.update_online_login();state.update_online_matchmaking();const auto s=account->snapshot();
      return s.party && s.party->leader && !s.busy && !s.party->members.empty() &&
        std::all_of(s.party->members.begin(),s.party->members.end(),[](const auto &m){return !m.ready_playlist.empty();});});
    const auto &entries=state.frame().entries;
    const auto begin=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
    check(begin!=entries.end(),"Couch pregame lobby has no native Start Game action");
    state.select(unsigned(begin-entries.begin()));state.accept();
    stage="published shared pregame countdown";
    wait([&]{state.update_online_login();state.update_online_matchmaking();return state.online_lobby_countdown()>0;});
    check(state.online_lobby_countdown()<=10 && !state.launch,"Couch Start skipped its original pregame countdown");
    stage="eight-player admission";
    // Advance only the pregame UI clock. The final matched-room countdown
    // remains gated by the service-published actual remaining time.
    std::uint32_t menu_clock{};
    wait([&]{state.ui_clock(menu_clock+=250);state.update_online_login();state.update_online_matchmaking();if(queue->snapshot().status==OnlineMatchmakingStatus::Failed)throw FormatError(queue->snapshot().message);return state.launch.has_value();});
    auto launch=std::move(*state.launch);state.launch.reset();if(!family&&!terminal)queue->sign_out();
    check(launch.local_players==4 && launch.selected_profiles.size()==4 && launch.selected_teams.size()==(family?(family_teams?4u:0u):custom?0u:4u) && launch.input_instances.size()==4,"Native handoff omitted local players/profiles/teams/controllers");
    if(family)check(source_game_variant(launch.variant).bytes==family_expected->bytes,"Actual family launch differs from the source cache preset plus its declared finite time");
    check(launch.online_match->roster().size()==8 && launch.online_match->local_slots().size()==4,"Room omitted couch seats");
    const auto slots=launch.online_match->local_slots();
    for(unsigned i=0;i<4;++i) {
      const auto &p=launch.online_match->roster()[slots[i]];
      check(p.guest==(i>0 && !(i==1 && mode=="mixed")),"Guest classification differs from verified local login");
      check(p.name==state.local_players()[i].name && (custom || (family&&!family_teams) || p.team==launch.selected_teams[0]),"Couch name/team lost");
      check(launch.input_instances[i]==(i?int(100+i):-1),"Controller binding changed during queue handoff");
    }
    return launch;
  };
  auto launch=search();auto session=launch.online_match;
  check(launch.map.filename()==map+".map","Native launch changed the selected admitted gameplay map");
  if(family) {
    if(!std::getenv("DISPLAY"))return 77;
    const auto type=source_game_variant_type(*family_expected);const auto old_identity=session->roster()[0].identity;
    MultiplayerOptions runtime;runtime.map=gameplay_maps/launch.map.filename();runtime.shared_maps=gameplay_maps;
    if(!menu.is_mcc())runtime.ui_assets=vista;
    if(!menu.is_mcc())runtime.gameplay_xbe=argv[7];
    runtime.online_match=session;runtime.variant=launch.variant;runtime.local_players=4;runtime.source_object_world=true;
    runtime.selected_profiles=launch.selected_profiles;runtime.selected_teams=launch.selected_teams;runtime.input_instances=launch.input_instances;
    runtime.width=640;runtime.height=480;runtime.hidden=runtime.mute=true;runtime.frames=6000;
    bool source{},visible{},captured{};unsigned peak{},frames{};MultiplayerState current;
    std::uint8_t fade_seen{},fade_opening{},fade_partial{},fade_clear{},fade_ending{};bool fade_captured{};
    nlohmann::json fade_frames=nlohmann::json::object();
    const auto deadline=std::chrono::steady_clock::now()+180s;
    runtime.source_world_observer=[&](const auto &){source=true;};
    runtime.input_observer=[&](auto,auto,const MultiplayerState &s){
      check(std::chrono::steady_clock::now()<deadline,"Natural source family game did not reach its actual ending");current=s;
      peak=std::max(peak,unsigned(std::count_if(s.players.begin(),s.players.end(),[](const auto &p){return p.connected;})));
      check(s.source_engine==type&&s.source_round_lifecycle_available,"Actual source family owner/round publication changed");
    };
    runtime.gameplay_fade_observer=[&](unsigned view,std::uint32_t tick,std::optional<float> alpha,const SourceGameplayFadeStateTu15 &fade) {
      check(view<4&&current.source_gameplay_fade.has_value(),"Actual ONLINE viewport lost its source fade input owner");
      const auto bit=std::uint8_t(1u<<view);
      if(!(fade_seen&bit)){fade_seen|=bit;check(alpha&&*alpha==1.f,"Actual ONLINE local view did not start with its original activation black");fade_opening|=bit;}
      if(!current.finished&&alpha&&*alpha>0.f&&*alpha<1.f)fade_partial|=bit;
      if(!alpha)fade_clear|=bit;
      if(current.finished&&alpha&&*alpha==1.f&&current.game_over_countdown>0)fade_ending|=bit;
      const auto record=[&](const std::string &name){if(!fade_frames.contains(name))fade_frames[name]={{"tick",tick},{"view",view},
        {"phase",current.round_phase},{"counter",fade.phase_counter},{"alpha_bits",alpha?nlohmann::json(std::bit_cast<std::uint32_t>(*alpha)):nlohmann::json(nullptr)},
        {"source_sequence",current.source_gameplay_fade->newest},{"game_over_countdown",current.game_over_countdown}};};
      if(fade_opening&bit)record("opening-"+std::to_string(view));
      if(!current.finished&&alpha&&*alpha>0.f&&*alpha<1.f)record("opening-partial-"+std::to_string(view));
      if(!alpha)record("clear-"+std::to_string(view));
      if(fade_ending&bit)record("ending-"+std::to_string(view));
    };
    runtime.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint){
      if(view||normal||!current.finished||layout.status!=FullScoreboardStatus::Ready||paint.draw.vertices.empty())return;
      check(layout.title.text==u"GAME OVER"&&layout.rows.size()==8&&layout.team_rows.size()==(family_teams?2u:0u),
          "Actual family end omitted the original automatic title/player/team layout");
      visible=layout.input_alpha>=.9f;
    };
    runtime.frame_capture=[&](const MultiplayerState &s){
      if(visible&&!captured&&s.finished){captured=true;return output/(owner+"-family-game-over.ppm");}
      if(fade_ending==15&&!fade_captured&&s.finished){fade_captured=true;return output/(owner+"-family-ending-world-black.ppm");}
      return std::filesystem::path{};
    };
    runtime.frame_observer=[&](const auto &f){frames+=f.submitted;};
    MultiplayerRuntimeOutcome outcome;check(run_multiplayer(runtime,outcome)==0&&outcome.reason==MultiplayerExitReason::online_postgame&&
        outcome.state&&outcome.state->game_over_complete()&&outcome.state->source_engine==type&&peak==8&&frames>10&&visible&&captured,
        "Actual two-owner source family world did not complete its original rendered end and acknowledged report");
    check(fade_opening==15&&fade_partial==15&&fade_clear==15&&fade_ending==15&&fade_captured,
      "Actual ONLINE host/joined couch did not render all4 original opening-black/partial/visible/final-black viewport states");
    check(session->self()!=session->host()||source,"Family host silently omitted the actual native source world");
    check(session->result_recorded()==true,"All admitted family owners did not produce an actual unanimous commit receipt");
    online_postgame_validate(*outcome.state,launch.variant,session->roster(),session->postgame());
    state.ui_clock(SDL_GetTicks());bool done{};
    auto report=online_postgame_screen(*frontend,session,[&]{done=true;state.return_online_lobby();});
    check(report->online&&report->result_recorded==true&&report->results->player_count==8&&report->results->team_count==(family_teams?2u:0u),
        "Actual source family report did not retain all admitted source result rows and known commit status");
    const auto retained=frontend->online_services;
    FrontendOptions front;front.session=frontend;front.entry=FrontendOptions::Entry::postgame_report;front.postgame_report=report;
    front.maps=maps;front.vista_settings=maps/"mainmenu.map";front.width=640;front.height=480;front.hidden=front.mute=front.skip_intro=true;
    std::set<unsigned> panes;unsigned phase{};std::uint64_t lobby_instance{};
    const auto report_deadline=std::chrono::steady_clock::now()+60s;
    const auto pane_path=[&](unsigned pane){return output/(owner+"-family-pcr-"+std::to_string(pane)+".ppm");};
    const auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;check(SDL_PushEvent(&e)==1,"Could not deliver real report SDL input");};
    front.frame_capture=[&](const MenuState &menu)->std::filesystem::path{
      if(!menu.frame().vista_pcr||menu.ui_map_intro_color())return {};
      if(menu.frame().screen==vista_menu(185)){const auto path=output/(owner+"-family-prior-stats.ppm");return std::filesystem::is_regular_file(path)?std::filesystem::path{}:path;}
      const auto intro=menu.pcr_intro_color(menu.frame());if(!intro||(*intro>>24)!=0)return {};
      const auto path=pane_path(menu.frame().context);return std::filesystem::is_regular_file(path)?std::filesystem::path{}:path;
    };
    front.session_update=[&](MenuState &menu,std::uint32_t clock,const auto &){
      check(std::chrono::steady_clock::now()<report_deadline,"Actual family PCR tabs/Done/prior-stats did not complete");
      const auto intro=menu.pcr_intro_color(menu.frame());
      if(phase==0&&menu.frame().vista_pcr&&intro&&(*intro>>24)==0&&!menu.ui_map_intro_color()&&
          std::filesystem::is_regular_file(pane_path(menu.frame().context))&&panes.insert(menu.frame().context).second) {
        if(panes.size()==(family_teams?6u:5u)){phase=1;key(SDLK_ESCAPE);}else key(SDLK_RIGHT);
      } else if(phase==1&&!menu.frame().vista_pcr&&menu.frame().screen==14) {
        check(done&&!session->postgame_presence()&&menu.has_online_prior_stats(),"Actual family Done retained stale session presence or lost prior stats");
        lobby_instance=menu.frame().instance;phase=2;key(SDLK_TAB);
      } else if(phase==2&&menu.frame().vista_pcr&&menu.frame().screen==vista_menu(185)&&
          std::filesystem::is_regular_file(output/(owner+"-family-prior-stats.ppm"))) {phase=3;key(SDLK_ESCAPE);}
      else if(phase==3&&!menu.frame().vista_pcr&&menu.frame().screen==14) {
        check(menu.frame().instance==lobby_instance,"Prior stats replaced the actual retained family lobby");return true;
      }
      return false;
    };
    run_frontend(front);
    check(phase==3&&panes.size()==(family_teams?6u:5u)&&frontend->online_services==retained&&
        account->snapshot().authenticated&&state.local_players().size()==4&&login->authorization(),
        "Rendered family report lost a real tab or its authenticated account/couch/party owner");
    report.reset();front.postgame_report.reset();front.session_update={};runtime.online_match.reset();launch.online_match.reset();session.reset();
    auto requeued=search(true);check(requeued.online_match->local_slots().size()==4&&requeued.online_match->roster()[0].identity!=old_identity,
        "Retained family lobby did not start a fresh authenticated eight-seat matched room");
    const auto barrier=std::filesystem::path(std::getenv("HALO2_COUCH_RENDER_BARRIER"));{std::ofstream ready(barrier/(owner+"-family-requeued"));ready<<"ready\n";}
    stage="both source family requeues";wait([&]{return std::filesystem::is_regular_file(barrier/"one-family-requeued")&&std::filesystem::is_regular_file(barrier/"two-family-requeued");});
    requeued.online_match.reset();queue->sign_out();
    nlohmann::json evidence={{"passed",true},{"preset",family_preset},{"source_engine",type},{"team_mode",family_teams},
      {"before_source_variant",family_before->bytes},{"finite_source_variant",family_expected->bytes},{"declared_override",{{"parameter",10},{"value",3}}},
      {"admitted_players",8},{"physical_owners",2},{"retained_local_players",4},{"result_recorded",true},{"rendered_pcr_panes",panes},
      {"visible_original_game_over",captured},{"submitted_frames",frames},{"viewport_fades",fade_frames},
      {"all4_source_fades",fade_opening==15&&fade_partial==15&&fade_clear==15&&fade_ending==15},{"ending_world_black_captured",fade_captured},{"same_lobby_prior_stats",true},{"fresh_authenticated_requeue",true},
      {"scope","Actual prepared MCC source family, authenticated2-owner/8-seat direct match, natural original end, automatic original GameOver, unanimous source report, actual rendered PCR pages/SDL Done/PriorStats and retained-party requeue. Only source time parameter10 is declared; no objective stats/winner/phase supplied."}};
    std::ofstream receipt(output/(owner+"-family.json"));receipt<<evidence.dump(2)<<'\n';check(bool(receipt),"Could not retain bounded source family flow evidence");
    std::this_thread::sleep_for(500ms);std::cout<<"PASS source family "<<family_preset<<" -> natural native original end -> unanimous report -> rendered original PCR/Done/prior-stats -> same authenticated lobby requeue\n";return 0;
  }
  if(terminal) {
    if(!std::getenv("DISPLAY"))return 77;
    check(session->direct_udp()&&session->terminal_recovery_enabled(),"Cold-terminal fixture lacks its direct qualification contract");
    const auto barrier=std::filesystem::path(std::getenv("HALO2_COUCH_RENDER_BARRIER"));
    const bool hosting=session->self()==session->host();
    MultiplayerOptions runtime;runtime.map=gameplay_maps/launch.map.filename();runtime.shared_maps=gameplay_maps;
    if(!menu.is_mcc())runtime.ui_assets=vista;
    if(!menu.is_mcc())runtime.gameplay_xbe=argv[7];
    runtime.online_match=session;runtime.variant=launch.variant;runtime.local_players=4;runtime.source_object_world=true;
    runtime.selected_profiles=launch.selected_profiles;runtime.selected_teams=launch.selected_teams;runtime.input_instances=launch.input_instances;
    runtime.width=640;runtime.height=480;runtime.hidden=runtime.mute=true;runtime.frames=15000;
    nlohmann::json evidence={{"schema","H2_COLD_TERMINAL_RECOVERY_1"},{"owner",owner},{"host",hosting},{"mode",mode},
      {"transport",eos?"eos-p2p-v1":"direct-udp-v1"},{"device_id_authentication",eos},{"account_v14_qualified",false}};
    const auto began=std::chrono::steady_clock::now();bool quit{},expired{},paired{},host_marked{};unsigned peak{},frames{};
    std::optional<MultiplayerState> projected_terminal_state;
    std::optional<AuthorityCutSeal> previous_ready,previous_endpoint;std::uint64_t previous_received{};
    const auto elapsed_ms=[&]{return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();};
    const auto progress=[](const MultiplayerState &s){
      nlohmann::json players=nlohmann::json::array();
      for(unsigned i=0;i<8;++i){const auto &p=s.players[i];players.push_back({{"score",p.score},{"total",p.total_score},
          {"wins",p.wins},{"kills",p.kills},{"deaths",p.deaths}});}
      return nlohmann::json{{"tick",s.tick},{"round_mode",s.round_mode},{"round_index",s.round_index},
          {"round_phase",s.round_phase},{"round_countdown",s.round_countdown},{"clock_seconds",s.clock_seconds},
          {"time_ticks",s.time_ticks},{"team_scores",s.team_scores},{"team_totals",s.team_total_scores},
          {"team_wins",s.team_wins},{"team_statistics",s.team_statistics},{"players",std::move(players)}};
    };
    evidence["snapshot_receipts"]=nlohmann::json::array();evidence["source_validation"]=nlohmann::json::array();
    runtime.replica_observer=[&](const MultiplayerState &s,const MultiplayerSnapshotOrigin &origin,bool){
      projected_terminal_state=s;
      evidence["last_projected_progress"]=progress(s);
      if(evidence["snapshot_receipts"].size()<512)evidence["snapshot_receipts"].push_back(
          {{"tick",origin.tick().value_or(0)},{"sequence",origin.sequence().value_or(0)},{"elapsed_ms",elapsed_ms()}});
    };
    evidence["declared_loading_deadline_seconds"]=185;
    if(const auto memory=resident_kib())evidence["resident_kib"]["before_run"]=*memory;
    if(const auto memory=peak_resident_kib())evidence["peak_resident_kib"]["before_run"]=*memory;
    runtime.source_recovery_observer=[&](const SourceAuthorityReceiverState &s){
      const auto ready=s.ready?std::optional(s.ready->seal):std::nullopt;
      const auto endpoint=s.endpoint;const auto endpoint_cut=endpoint?std::optional(endpoint->world().cut.seal):std::nullopt;
      if((ready!=previous_ready||endpoint_cut!=previous_endpoint||s.received!=previous_received)&&evidence["source_validation"].size()<512){
        evidence["source_validation"].push_back({{"elapsed_ms",elapsed_ms()},{"received",s.received},
          {"ready_tick",ready?nlohmann::json(ready->tick):nlohmann::json(nullptr)},
          {"ready_id",ready?nlohmann::json(ready->id):nlohmann::json(nullptr)},
          {"endpoint_tick",endpoint_cut?nlohmann::json(endpoint_cut->tick):nlohmann::json(nullptr)},
          {"endpoint_id",endpoint_cut?nlohmann::json(endpoint_cut->id):nlohmann::json(nullptr)},
          {"replayed_phases",s.replayed_phases},{"replaying",s.replaying},{"decode_us",s.decode_microseconds},
          {"replay_us",s.replay_microseconds},{"endpoint_copies",s.endpoint_copies},
          {"endpoint_copy_us",s.endpoint_copy_microseconds},{"endpoint_bank_bytes",s.endpoint_bank_bytes},
          {"endpoint_budget_skips",s.endpoint_budget_skips},{"endpoint_copy_failures",s.endpoint_copy_failures},{"endpoint_error",s.endpoint_error},{"error",s.error}});
        previous_ready=ready;previous_endpoint=endpoint_cut;previous_received=s.received;
      }
      if(!paired&&endpoint) {
        const auto &cut=endpoint->world().cut;const auto &proof=endpoint->proof().cut();
        check(cut.peer==session->self()&&session->authority_context()==cut.context&&proof.peer==cut.peer&&
            proof.context==cut.context&&proof.seal==cut.seal&&cut.seal.tick==0&&
            endpoint->world().checkpoint.match.simulation.ticks==0,"Initial owning paired checkpoint has a different admission or source tick0");
        if(const auto memory=resident_kib())evidence["resident_kib"]["initial_paired_validation"]=*memory;
        if(const auto memory=peak_resident_kib())evidence["peak_resident_kib"]["initial_paired_validation"]=*memory;
        paired=true;evidence["validated_checkpoint"]={{"tick",cut.seal.tick},{"bytes",cut.seal.bytes},
          {"warm_us",s.warm_microseconds},{"workspace_us",s.workspace_microseconds},{"decode_us",s.decode_microseconds},
          {"prepare_us",s.prepare_microseconds},{"elapsed_ms",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count()}};
        std::ofstream ready(barrier/"terminal-paired-ready");ready<<"validated\n";check(bool(ready),"Could not report actual paired checkpoint");
      }
    };
    runtime.input_observer=[&](auto,auto,const MultiplayerState &s){
      peak=std::max(peak,unsigned(std::count_if(s.players.begin(),s.players.end(),[](const auto &p){return p.connected;})));
      const bool progressed=s.source_round_lifecycle_available&&s.round_phase==1&&s.round_index==1&&frames>10;
      if(hosting&&progressed&&!host_marked&&std::filesystem::is_regular_file(barrier/"terminal-current-progress-adopted")){
        evidence["last_host_progress"]=progress(s);
        {std::ofstream cost(output/(owner+"-terminal-host-cost.json"));cost<<evidence.dump(2)<<'\n';check(bool(cost),"Could not record actual second-round host progress");}
        std::ofstream identity(barrier/("terminal-host-"+owner));identity<<"host\n";check(bool(identity),"Could not record actual progressed host");host_marked=true;
      }
      const bool leave=hosting&&progressed&&!abrupt&&std::filesystem::is_regular_file(barrier/"terminal-current-progress-adopted");
      expired=std::chrono::steady_clock::now()-began>210s;
      if(!quit&&(leave||expired)){SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"Could not leave the actual host runtime");quit=true;}
    };
    runtime.frame_observer=[&](const auto &f){
      if(!hosting&&f.submitted&&projected_terminal_state&&projected_terminal_state->source_round_lifecycle_available&&
          projected_terminal_state->round_index==1&&projected_terminal_state->round_phase==1){
        evidence["last_submitted_progress"]=progress(*projected_terminal_state);
        std::ofstream marker(barrier/"terminal-current-progress-adopted");marker<<"submitted certified second-active frame\n";
        check(bool(marker),"Could not record actual submitted second-round endpoint");
      }
      if(!frames&&f.submitted) {
        evidence["first_frame_ms"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();
        if(const auto memory=resident_kib())evidence["resident_kib"]["first_frame"]=*memory;
        if(const auto memory=peak_resident_kib())evidence["peak_resident_kib"]["first_frame"]=*memory;
        if(hosting){std::ofstream cost(output/(owner+"-terminal-host-cost.json"));cost<<evidence.dump(2)<<'\n';check(bool(cost),"Could not record bounded host checkpoint cost");}
      }
      frames+=f.submitted;
    };
    MultiplayerRuntimeOutcome outcome;std::shared_ptr<const SourceAuthorityRetainedWorld> saved;
    const auto runtime_result=run_multiplayer(runtime,outcome,saved);
    evidence["runtime_result"]=runtime_result;evidence["runtime_reason"]=unsigned(outcome.reason);
    if(outcome.state)evidence["last_adopted_progress"]=progress(*outcome.state);
    evidence["retained_cut_tick"]=saved?nlohmann::json(saved->cut.seal.tick):nlohmann::json(nullptr);
    {std::ofstream timing(output/(owner+"-terminal-timing.json"));timing<<evidence.dump(2)<<'\n';check(bool(timing),"Could not record actual terminal cadence/freshness diagnostic");}
    check(runtime_result==0&&!expired&&peak==8&&frames>10,"Actual cold-terminal network world failed or timed out");
    evidence["network_runtime_ms"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();
    if(const auto memory=resident_kib())evidence["resident_kib"]["after_network_runtime"]=*memory;
    if(const auto memory=peak_resident_kib())evidence["peak_resident_kib"]["after_network_runtime"]=*memory;
    if(hosting) {
      check(outcome.reason==MultiplayerExitReason::window_quit&&!saved&&!session->terminal_observation(),"Intentional departing host manufactured recovery evidence");
      std::cout<<"PASS actual host renderer quit after independent source checkpoint validation\n";return 0;
    }
    const auto observed=session->terminal_observation();
    check(paired&&saved&&observed&&outcome.reason==MultiplayerExitReason::online_disconnected&&
      outcome.authority==MultiplayerOutcomeAuthority::client_snapshot&&!outcome.authoritative_banks,
      "Actual host loss lacked typed direct retirement and independently validated source checkpoint");
    auto cold=runtime;cold.initial_gameplay_fade=outcome.local_gameplay_fade;cold.prepared_world=SourceTerminalRecovery::prepare(runtime,*saved,*observed);
    check(outcome.state&&outcome.state->source_round_lifecycle_available&&outcome.state->round_index==1&&
        outcome.state->round_phase==1,"Terminal fixture did not preserve genuine second-round progress");
    check(progress(cold.prepared_world->match.state())==progress(*outcome.state),
        "Terminal source import rolled adopted player/team scores, wins, round or clock back");
    check(evidence.contains("last_submitted_progress")&&progress(*outcome.state)==evidence["last_submitted_progress"],
        "Terminal outcome differs from the actual last submitted certified frame");
    check(outcome.local_gameplay_fade&&outcome.state->source_gameplay_fade&&
        outcome.local_gameplay_fade->sequence()==outcome.state->source_gameplay_fade->newest,
        "Terminal handoff lost the actual submitted local viewport fade owner");
    evidence["exact_adopted_progress"]=progress(*outcome.state);
    if(const auto memory=resident_kib())evidence["resident_kib"]["cold_terminal_prepared"]=*memory;
    evidence["retired_physical_peers"]=observed->retired_peers();evidence["cut_tick"]=saved->cut.seal.tick;
    evidence["display_tick"]=outcome.state?outcome.state->tick:0;saved.reset();
    std::array<int,16> kills{},deaths{};for(unsigned i=0;i<8;++i){kills[i]=cold.prepared_world->match.player(i).kills;deaths[i]=cold.prepared_world->match.player(i).deaths;}
    cold.online_match.reset();cold.source_recovery_observer={};cold.input_observer={};cold.frame_observer={};
    std::optional<VistaPcrConsumedSnapshot> report;bool visible{},captured{};MultiplayerState current;
    cold.input_observer=[&](auto,auto,const MultiplayerState &s){current=s;};
    cold.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint){
      if(view||normal||!current.finished||layout.status!=FullScoreboardStatus::Ready||paint.draw.vertices.empty())return;
      check(layout.title.text==u"GAME OVER"&&layout.rows.size()==8&&layout.team_rows.size()==2,"Cold source ending omitted original GAME OVER roster/title/teams");visible=true;
      if(layout.input_alpha<.9f){visible=false;return;}
      evidence["end_screen"]={{"tick",current.tick},{"phase",current.round_phase},{"round_index",current.round_index},
        {"countdown",current.game_over_countdown},{"rows",layout.rows.size()},{"teams",layout.team_rows.size()},{"title","GAME OVER"}};
    };
    cold.frame_capture=[&](const MultiplayerState &s){if(visible&&!captured&&s.finished){captured=true;return output/(owner+"-terminal-game-over.ppm");}return std::filesystem::path{};};
    cold.session_world_update=[&](const MultiplayerMatch &match){
      if(!match.state().game_over_complete())return false;
      for(unsigned i=0;i<8;++i)check(match.player(i).kills==kills[i]&&match.player(i).deaths==deaths[i],"Departure recovery manufactured kills or deaths");
      report=online_postgame_publish(match,launch.variant,session->roster());return true;
    };
    MultiplayerRuntimeOutcome ended;check(run_multiplayer(cold,ended)==0&&report&&visible&&captured&&ended.state&&ended.state->game_over_complete(),
      "Cold original sole-team ending did not reach actual rendered completion/PCR");
    state.ui_clock(SDL_GetTicks());
    bool done{};auto pcr=online_terminal_postgame_screen(*frontend,*ended.state,std::move(*report),session->roster(),[&]{done=true;state.return_online_lobby();});
    check(pcr->result_recorded==false&&pcr->results->player_count==8&&pcr->lobby_review&&pcr->lobby_review->result_recorded==false,
      "Cold terminal PCR omitted retired source rows or represented a competitive commit");
    const auto retained_services=frontend->online_services;
    FrontendOptions front;front.session=frontend;front.entry=FrontendOptions::Entry::postgame_report;front.postgame_report=pcr;
    front.maps=maps;front.vista_settings=maps/"mainmenu.map";front.width=640;front.height=480;
    front.hidden=front.mute=front.skip_intro=true; // Optional2Dpack stays empty; native content owns pixels.
    std::set<unsigned> panes;unsigned presentation_phase{};std::uint64_t retained_lobby{};
    const auto pcr_deadline=std::chrono::steady_clock::now()+60s;
    const auto pcr_path=[&](unsigned pane){return output/(owner+"-terminal-pcr-"+std::to_string(pane)+".ppm");};
    const auto key=[&](SDL_Keycode code){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;check(SDL_PushEvent(&e)==1,"Could not deliver real terminal report SDL input");};
    front.frame_capture=[&](const MenuState &menu)->std::filesystem::path {
      if(!menu.frame().vista_pcr||menu.ui_map_intro_color())return {};
      if(menu.frame().screen==vista_menu(185)) {
        const auto file=output/(owner+"-terminal-prior-stats.ppm");
        return std::filesystem::is_regular_file(file)?std::filesystem::path{}:file;
      }
      // Wait the actual original report intro/black overlay; no supplied UI clock.
      const auto intro=menu.pcr_intro_color(menu.frame());if(!intro||(*intro>>24)!=0)return {};
      const auto file=pcr_path(menu.frame().context);
      return std::filesystem::is_regular_file(file)?std::filesystem::path{}:file;
    };
    front.session_update=[&](MenuState &menu,std::uint32_t clock,const auto &) {
      check(std::chrono::steady_clock::now()<pcr_deadline,"Actual terminal PCR tabs/Done/prior stats timed out");
      const auto intro=menu.pcr_intro_color(menu.frame());
      if(presentation_phase==0&&menu.frame().vista_pcr&&intro&&(*intro>>24)==0&&!menu.ui_map_intro_color()&&
          std::filesystem::is_regular_file(pcr_path(menu.frame().context))&&panes.insert(menu.frame().context).second) {
        for(unsigned row=0;row<session->roster().size();++row)if(!session->roster()[row].matchmaking_level)
          check(!pcr->appearance(row).rank_visible,"Unrecorded terminal report invented an unknown rank");
        if(panes.size()==6){presentation_phase=1;key(SDLK_ESCAPE);}else key(SDLK_RIGHT);
      } else if(presentation_phase==1&&!menu.frame().vista_pcr&&menu.frame().screen==14) {
        check(done&&menu.has_online_prior_stats()&&menu.local_players().size()==4,
            "Rendered terminal Done lost actual account/profiles/lobby/prior report");
        retained_lobby=menu.frame().instance;presentation_phase=2;key(SDLK_TAB);
      } else if(presentation_phase==2&&menu.frame().vista_pcr&&menu.frame().screen==vista_menu(185)&&
          std::filesystem::is_regular_file(output/(owner+"-terminal-prior-stats.ppm"))) {
        presentation_phase=3;key(SDLK_ESCAPE);
      } else if(presentation_phase==3&&!menu.frame().vista_pcr&&menu.frame().screen==14) {
        check(menu.frame().instance==retained_lobby,"Terminal prior stats replaced its retained lobby");return true;
      }
      return false;
    };
    check(!run_frontend(front).has_value()&&presentation_phase==3&&panes.size()==6&&frontend->online_services==retained_services,
        "Actual terminal PCR/six panes/SDL Done/prior stats did not preserve native services");
    evidence["rendered_pcr_panes"]=panes;evidence["same_lobby_prior_stats"]=true;evidence["unknown_rank_hidden"]=true;
    pcr.reset();front.postgame_report.reset();front.session_update={};cold.prepared_world.reset();launch.online_match.reset();runtime.online_match.reset();session.reset();
    // Requeue the actual retained hopper. With its old peer gone, this must
    // produce a fresh authenticated searching intent, never a fabricated room.
    stage="terminal retained-lobby requeue";
    wait([&]{state.ui_clock(SDL_GetTicks());state.update_online_login();state.update_online_matchmaking();return queue->snapshot().status==OnlineMatchmakingStatus::Ready;});
    const auto &entries=state.frame().entries;const auto start=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
    check(start!=entries.end(),"Terminal retained lobby lost native Start");state.select(unsigned(start-entries.begin()));state.accept();
    wait([&]{state.ui_clock(SDL_GetTicks());state.update_online_login();state.update_online_matchmaking();return queue->snapshot().status==OnlineMatchmakingStatus::Searching;});
    queue->cancel_queue();wait([&]{return queue->snapshot().status==OnlineMatchmakingStatus::Ready;});queue->sign_out();
    evidence["total_ms"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();
    if(const auto memory=peak_resident_kib())evidence["peak_resident_kib"]["after_terminal"]=*memory;
    evidence["passed"]=true;evidence["result_recorded"]=false;evidence["admitted_players"]=8;evidence["retained_local_players"]=4;
    evidence["scope"]="Opt-in direct-UDP cold terminal reconstruction; actual paired source checkpoint, actual all-other-owner loss, original depart/natural sole-team ending and automatic GPU GameOver. No migration, rank/history commit or original Xbox framebuffer equality.";
    std::ofstream receipt(output/(owner+"-terminal.json"));receipt<<evidence.dump(2)<<'\n';check(bool(receipt),"Could not record bounded terminal evidence");
    std::cout<<"PASS actual host loss -> typed retirement -> cold source owner -> natural original GAME OVER -> unrecorded eight-row PCR -> retained four-seat lobby -> fresh authenticated requeue\n";return 0;
  }
  if(render) {
    if(!std::getenv("DISPLAY"))return 77;
    MultiplayerOptions runtime;runtime.map=gameplay_maps/launch.map.filename();runtime.shared_maps=gameplay_maps;
    if(!menu.is_mcc())runtime.ui_assets=vista;
    if(!menu.is_mcc())runtime.gameplay_xbe=argv[7];
    runtime.online_match=session;runtime.variant=launch.variant;runtime.local_players=launch.local_players;
    runtime.selected_profiles=launch.selected_profiles;runtime.selected_teams=launch.selected_teams;runtime.input_instances=launch.input_instances;
    runtime.width=1280;runtime.height=720;runtime.hidden=runtime.mute=true;runtime.screenshot=output/(owner+"-couch.ppm");
    runtime.capture_tick=120;runtime.input_script=output/(owner+"-input.txt");
    {std::ofstream script(runtime.input_script);for(unsigned t=0;t<120;++t)for(unsigned p=0;p<4;++p)script<<t<<' '<<p<<' '<<(t%20<10)<<" 0 0 "<<p*1.1f<<" 0 move "<<(.2f+.1f*p)<<" 0\n";}
    std::set<unsigned> views;std::array<bool,4> controlled{};unsigned peak{},ticks{};
    runtime.view_observer=[&](unsigned view,const Camera &,float,std::int16_t){views.insert(view);};
    runtime.input_observer=[&](auto t,auto controls,const auto &world){ticks=t;for(unsigned i=0;i<4;++i)controlled[i]|=controls[i].fire && controls[i].forward>.1f;peak=std::max(peak,unsigned(std::count_if(world.players.begin(),world.players.end(),[](const auto &p){return p.connected;})));};
    MultiplayerRuntimeOutcome outcome;run_multiplayer(runtime,outcome);
    if(std::getenv("HALO2_COUCH_REQUIRE_SOURCE_ADMISSIONS")){
      const auto accepted=session->source_players();const auto context=session->authority_context();
      check(accepted&&context&&accepted->context==*context&&accepted->players.size()==8,"Native runtime omitted the admitted source player exchange");
      for(unsigned slot=0;slot<8;++slot)check(accepted->players[slot].slot==slot&&(accepted->players[slot].player&65535u)==slot,
          "Native runtime source allocations differ from the accepted couch slots");
      std::cout<<"PASS native runtime exported and accepted all eight source allocations over direct encrypted peers\n";
    }
    check(views==std::set<unsigned>{0,1,2,3} && peak==8 && ticks>10 && std::all_of(controlled.begin(),controlled.end(),[](bool v){return v;}),"Rendered four-view match lost a view, player, or independent input");
    check(outcome.reason==MultiplayerExitReason::captured_tick && outcome.state && outcome.state->tick>=120,"Four-view finite capture did not preserve its native snapshot");
    const auto barrier=std::filesystem::path(std::getenv("HALO2_COUCH_RENDER_BARRIER"));
    {std::ofstream ready(barrier/owner);ready<<"ready\n";}
    stage="both rendered captures";wait([&]{return std::filesystem::exists(barrier/"one") && std::filesystem::exists(barrier/"two");});
    launch.online_match.reset();session.reset();std::this_thread::sleep_for(500ms);
    std::cout<<"PASS four 640x360 views in 1280x720; all four independent controls; eight replicated players; finite native capture; no fabricated result\n";
    return 0;
  } else {
    const Cache cache(maps/(map+".map"),maps,OriginalData::load_xbe(argv[7]));const auto content=cache.load_content(0,true);
    auto rules=slayer_rules(launch.variant,true);rules.weapon=cache.slayer_starting_weapon(custom?5:rules.starting_weapon);if(custom)rules.weapon_choices=cache.multiplayer_weapon_choices();
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    // Match the native prepared world's actual representation/weapon bank.
    // Fiesta can select any authored weapon, so a bare SMG-only fixture does
    // not satisfy the original animation timing owner's binding contract.
    for(unsigned representation:{2u,3u})
      if(representation<content.player_representations.size() &&
          content.player_representations[representation].biped.datum!=null_datum)
        for(const auto &[datum,weapon]:content.weapons)
          if(!weapon.first_person_models.empty() && weapon.first_person_models.front().model!=null_datum &&
              weapon.first_person_models.front().animation!=null_datum)
            match.configure_replicated_first_person(cache.load_first_person_binding(datum,representation));
    const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(maps/(map+".map"),maps,0,rules.weapon),launch.variant);
    session->loaded(fingerprint,launch.variant);stage="loading";wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;});
    const auto slots=session->local_slots();const bool hosting=session->self()==session->host();
    std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
    if(hosting) {for(unsigned i=0;i<4;++i)match.join(session->roster()[slots[i]].name,custom?-1:launch.selected_teams[i],launch.selected_profiles[i],slots[i]);host=std::make_unique<LanHost>(match,session,fingerprint,session->roster());}
    else client=std::make_unique<LanClient>(session,session->host(),fingerprint,launch.selected_teams,launch.selected_profiles);
    std::array<Controls,16> controls{};std::array<Controls,4> local{};stage="grouped gameplay handshake";
    wait([&]{if(host){host->receive(controls);host->broadcast();std::this_thread::sleep_for(30ms);return host->peers()==1;}client->send(local);client->receive();return client->ready();});
    if(client)check(std::vector<unsigned>(client->slots().begin(),client->slots().end())==slots,"Client slots changed from authenticated group");
    std::array<Vec3,4> initial{};std::array<bool,4> moved{};bool recorded{};unsigned peak{},tick{};
    const auto start=std::chrono::steady_clock::now();
    for(unsigned n=0;n<210;++n) {
      for(unsigned i=0;i<4;++i){local[i].forward=.2f+.1f*i;local[i].yaw=i*1.1f;local[i].fire=n%20<10;}
      if(host){for(unsigned i=0;i<4;++i)controls[slots[i]]=local[i];host->receive(controls);match.step(controls);host->broadcast();}
      else {client->send(local);client->receive();}
      const auto &world=client?client->state():match.state();tick=world.tick;peak=std::max(peak,unsigned(std::count_if(world.players.begin(),world.players.end(),[](const auto &p){return p.connected;})));
      for(unsigned i=0;i<4;++i)if(recorded)moved[i]|=length(world.players[slots[i]].position-initial[i])>.001f;else initial[i]=world.players[slots[i]].position;
      recorded=true;if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());
      std::this_thread::sleep_until(start+(n+1)*33333us);
    }
    check(peak==8 && tick>150 && std::all_of(moved.begin(),moved.end(),[](bool v){return v;}),"Eight-player match did not replicate all four local movements");
    bool published{};stage="unanimous source results";
    wait([&]{
      if(host && session->phase()==OnlineMatchSession::Phase::Running){host->receive(controls);match.step(controls);host->broadcast();std::this_thread::sleep_for(28ms);if(!published && match.state().game_over_complete()){session->complete(online_postgame_publish(match,launch.variant,session->roster()).source);published=true;}}
      else if(client){client->send(local);client->receive();}
      if(session->phase()==OnlineMatchSession::Phase::Reviewing){online_postgame_validate(client?client->state():match.state(),launch.variant,session->roster(),session->postgame());session->confirm_result();}
      if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());return session->phase()==OnlineMatchSession::Phase::Postgame;
    });
    std::cout<<"PASS two native PCs, eight players, one remote connection, all four independent movement inputs, source result agreement\n";
  }
  bool done{};auto report=online_postgame_screen(*frontend,session,[&]{done=true;state.return_online_lobby();});
  check(report->online && report->results->player_count==8,"Local Vista PCR omitted guest details");
  frontend->enter(FrontendOptions::Entry::postgame_report,report);state.back();
  check(!session->postgame_presence(),"Completed report retained stale live presence after its transport closed");
  check(done && state.frame().screen==14 && state.has_online_prior_stats() &&
    state.local_players().size()==4 && login->authorization(),"PCR Done lost its lobby, actual prior stats, couch profiles or primary login");
  report.reset();launch.online_match.reset();session.reset();
  auto requeued=search();check(requeued.online_match->local_slots().size()==4,"Requeue lost couch group");
  const auto barrier=std::filesystem::path(std::getenv("HALO2_COUCH_RENDER_BARRIER"));
  {std::ofstream ready(barrier/(owner+"-requeued"));ready<<"ready\n";}
  stage="both native requeue handoffs";
  wait([&]{return std::filesystem::exists(barrier/"one-requeued") && std::filesystem::exists(barrier/"two-requeued");});
  requeued.online_match.reset();std::this_thread::sleep_for(500ms);
  std::cout<<"PASS Vista PCR keeps all eight players; Done preserves four selected profiles and Discord login; native requeue\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
