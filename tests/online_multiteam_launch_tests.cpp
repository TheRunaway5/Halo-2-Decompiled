#define SDL_MAIN_HANDLED
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_account.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/native_tu15_online_services.hpp"
#include "halo2/full_scoreboard.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include <SDL.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <thread>
#include <utility>
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
const char *stage="sign-in";
template<class F>void wait(F f){const auto end=std::chrono::steady_clock::now()+150s;while(std::chrono::steady_clock::now()<end){if(f())return;std::this_thread::sleep_for(5ms);}throw FormatError(std::string("Multi-Team timed out: ")+stage);}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
struct CouchControllers {
  std::array<int,3> devices{-1,-1,-1};
  std::array<SDL_Joystick*,3> sticks{};
  std::array<SDL_JoystickID,3> instances{};
  CouchControllers(){
    check(SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1")==SDL_TRUE,"Hidden SDL controller polling was not enabled");
    for(unsigned i=0;i<sticks.size();++i){
      devices[i]=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
      check(devices[i]>=0,"SDL couch controller did not attach");
      sticks[i]=SDL_JoystickOpen(devices[i]);check(sticks[i],"SDL couch controller did not open");
      char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(sticks[i]),guid,sizeof(guid));
      const auto mapping=std::string(guid)+",Multi-Team validation controller,a:b0,b:b1,x:b2,y:b3,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
      check(SDL_GameControllerAddMapping(mapping.c_str())>=0&&SDL_IsGameController(devices[i]),"SDL couch controller mapping failed");
      instances[i]=SDL_JoystickInstanceID(sticks[i]);neutral(i);
    }
  }
  void neutral(unsigned i){for(int a=0;a<SDL_CONTROLLER_AXIS_MAX;++a)
    check(SDL_JoystickSetVirtualAxis(sticks[i],a,a<SDL_CONTROLLER_AXIS_TRIGGERLEFT?0:SDL_JOYSTICK_AXIS_MIN)==0,"SDL couch controller neutral delivery failed");}
  ~CouchControllers(){for(auto *p:sticks)if(p)SDL_JoystickClose(p);for(auto i=devices.rbegin();i!=devices.rend();++i)if(*i>=0)SDL_JoystickDetachVirtual(*i);}
};
}
int main(int argc,char **argv)try {
  check(argc==7,"Supply MCC_ROOT ORIGIN OWNER PORT OUTPUT BARRIER");
  if(!std::getenv("DISPLAY"))return 77;
  SDL_SetMainReady();
  auto desktop=std::make_shared<DesktopSession>();desktop->window(960,720,true);CouchControllers pads;
  const std::filesystem::path root=argv[1],maps=root/"halo2/h2_maps_win64_dx11",out=argv[5],barriers=argv[6];
  const std::filesystem::path gameplay_maps=std::getenv("HALO2_ONLINE_FLOW_GAMEPLAY_MAPS")?
      std::filesystem::path(std::getenv("HALO2_ONLINE_FLOW_GAMEPLAY_MAPS")):maps;
  check(std::filesystem::is_regular_file(gameplay_maps/"lockout.map"),"Explicit gameplay cache lacks the admitted Multi-Team map");
  const std::string owner=argv[3];
  const std::array<std::string,4> owners{"one","two","three","four"};
  const auto owner_index=std::find(owners.begin(),owners.end(),owner)-owners.begin();check(owner_index<4,"Unknown owner");
  std::filesystem::create_directories(out);
  auto barrier=[&](const std::string &phase){std::ofstream(barriers/(owner+"-"+phase))<<"ready\n";stage="four native owners barrier";wait([&]{return std::all_of(owners.begin(),owners.end(),[&](const auto &p){return std::filesystem::exists(barriers/(p+"-"+phase));});});};
  std::string browser;DiscordLoginOptions auth;auth.callback_port=std::stoul(argv[4]);
  auto login=std::make_shared<DiscordLogin>([&](const auto &url){browser=url;return true;},auth,
      [&,owner_index](const DiscordHttpRequest &r,std::stop_token)->DiscordHttpResponse {
        if(r.url.ends_with("/token"))return {200,"{\"access_token\":\""+owner+"\",\"token_type\":\"Bearer\",\"scope\":\"identify\",\"expires_in\":3600}"};
        return {200,"{\"id\":\""+std::to_string(123456789012345678ull+owner_index)+"\",\"username\":\"test\",\"global_name\":\"Test\"}"};
      });
  login->start();wait([&]{return !browser.empty();});
  const auto begin=browser.find("state=")+6;
  const auto callback="http://127.0.0.1:"+std::string(argv[4])+"/discord/callback?code=test&state="+browser.substr(begin,browser.find('&',begin)-begin);
  {std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
    curl_easy_setopt(curl.get(),CURLOPT_URL,callback.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
    check(curl_easy_perform(curl.get())==CURLE_OK,"Native OAuth callback failed");}
  wait([&]{return login->authorization().has_value();});
  auto frontend=std::make_shared<FrontendSession>();frontend->discord_login=login;
  const Cache menu(maps/"mainmenu.map");frontend->retain_content(menu.load_frontend());
  frontend->content().vista=load_vista_settings(maps/"mainmenu.map");
  // Finite validation rules, explicitly not the unavailable downloaded hopper
  // variant buffer. Every native endpoint confirms this identical source blob.
  for(auto &v:frontend->content().variants)if(v.name.starts_with("Team Slayer")){v.values.set(0,1);v.values.set(10,8);v.values.set(2,0);}
  auto &state=frontend->create_state();state.accept();state.accept();state.text("Couch "+owner);state.commit_text();state.back();state.set_discord_login(login);
  // Bind the primary through the real host-controller owner before entering
  // ONLINE. SDL_PushEvent does not change SDL's polled keyboard state, whereas
  // each virtual controller supplies actual independent polled axis values.
  check(state.controller_slot(pads.instances[0],true)==0,"Primary controller did not bind its actual profile");
  OnlineAccountOptions ao;ao.endpoint=argv[2];ao.allow_loopback_http=true;ao.enable_parties=true;ao.poll_interval=20s;
  auto account=std::make_shared<OnlineAccount>(login,ao);account->connect("Couch "+owner);state.set_online_account(account);
  wait([&]{state.update_online_login();return account->snapshot().authenticated&&!account->snapshot().busy;});
  OnlineMatchmakingOptions qo;qo.endpoint=argv[2];qo.allow_loopback_http=true;qo.maps={"lockout"};qo.poll_interval=20ms;
  auto queue=std::make_shared<OnlineMatchmaking>(login,qo);state.set_online_matchmaking(queue);state.open(186);
  frontend->online_services=std::make_shared<NativeTu15OnlineServices>();
  frontend->online_services->login=login;frontend->online_services->account=account;frontend->online_services->matchmaking=queue;
  for(unsigned seat=1;seat<3;++seat){
    check(state.controller_slot(pads.instances[seat],true)==seat,"Controller assigned wrong physical couch seat");
    const auto &rows=state.frame().entries;const auto guest=std::find_if(rows.begin(),rows.end(),[](const auto &e){return e.destination==-80;});
    check(guest!=rows.end(),"Original ONLINE chooser omitted Guest");state.select(unsigned(guest-rows.begin()));state.accept();state.select(0);state.accept();
  }
  check(state.local_players().size()==3,"Original chooser lost three couch profiles");
  std::set<std::string> capture_requests;std::string pcr_prefix;std::ofstream phases(out/(owner+"-phases.tsv"));phases<<"phase\tscreen\tcontext\tplayers\n";
  nlohmann::json capture_rows=nlohmann::json::object();std::uint32_t observed_ui_clock{};
  const auto record_capture=[&](const std::string&phase,nlohmann::json row){
    capture_rows[phase]=std::move(row);std::ofstream file(out/(owner+"-capture-phases.json"));file<<capture_rows.dump(2)<<'\n';
    check(bool(file),"Actual capture provenance could not be retained");
  };
  const auto capture_path=[&](const std::string&phase){return out/(owner+"-"+phase+".ppm");};
  const auto capture_exists=[&](const std::string&phase){return std::filesystem::is_regular_file(capture_path(phase));};
  const auto capture_once=[&](const std::string&phase,unsigned screen,int context,unsigned players)->std::filesystem::path{
    if(phase.empty()||capture_exists(phase))return {};
    if(capture_requests.insert(phase).second){phases<<phase<<'\t'<<screen<<'\t'<<context<<'\t'<<players<<'\n';phases.flush();}
    // A requested readback is not a submitted frame. Retry until the actual
    // renderer writes the file, including after an out-of-date swapchain.
    return capture_path(phase);
  };
  auto capture=[&](const MenuState &s)->std::filesystem::path{
    std::string phase;const auto &f=s.frame();const auto snap=queue->snapshot();
    if(!f.vista_pcr&&s.ui_map_intro_color())return {};
    if(f.screen==14)phase=s.online_lobby_countdown()>0?"pregame-countdown":"pregame";
    else if(f.screen==210)phase="matchmaking-pane-"+std::to_string(s.online_matchmaking_progress_pane())+"-state-"+std::to_string(int(s.online_matchmaking_ui()));
    else if(f.vista_pcr) {
      if(f.screen==vista_menu(185)) {
        check(!s.pcr_intro_color(f),"Prior statistics did not bypass the original PCR intro");
        phase=pcr_prefix+"prior-"+std::to_string(f.context);
      } else {
        const auto color=s.pcr_intro_color(f);check(color.has_value(),"Completed PCR omitted its actual intro clock");
        const auto alpha=*color>>24;
        const auto map_color=s.ui_map_intro_color();const auto map_alpha=map_color?*map_color>>24:0u;
        const std::string intro=alpha==255?"black":alpha>32&&alpha<224&&map_alpha<224?"fade":alpha==0&&!map_color?"clear":"";
        if(!intro.empty()&&!std::filesystem::is_regular_file(out/(owner+"-"+pcr_prefix+"pcr-intro-"+intro+".ppm")))
          phase=pcr_prefix+"pcr-intro-"+intro;
        else if(!alpha&&!map_color)phase=pcr_prefix+"pcr-"+std::to_string(f.context);
        else return {};
      }
    }
    const auto path=capture_once(phase,f.screen,f.context,unsigned(snap.match?snap.match->players.size():snap.gathering?snap.gathering->players.size():0));
    if(!path.empty()){
      const auto map_color=s.ui_map_intro_color(),pcr_color=s.pcr_intro_color(f);
      record_capture(phase,{{"file",path.filename().string()},{"domain","frontend"},{"screen",f.screen},{"pane",f.context},
          {"ui_clock",observed_ui_clock},{"map_intro_argb",map_color?nlohmann::json(*map_color):nlohmann::json(nullptr)},
          {"pcr_open_native_clock",f.vista_pcr?nlohmann::json(f.pcr_open_clock):nlohmann::json(nullptr)},
          {"pcr_intro_argb_observed_after_painter",pcr_color?nlohmann::json(*pcr_color):nlohmann::json(nullptr)}});
    }
    return path;
  };
  auto front_options=[&]{FrontendOptions fo;fo.session=frontend;fo.maps=maps;fo.vista_settings=maps/"mainmenu.map";fo.width=960;fo.height=720;fo.skip_intro=fo.hidden=fo.mute=true;fo.desktop=desktop;fo.frame_capture=capture;return fo;};
  auto search=[&](bool first){
    barrier(first?"first-search":"requeue-search");
    auto fo=front_options();fo.entry=FrontendOptions::Entry::session_return;
    fo.session_mainmenu_ready=[&](MenuState&s){if(first){s.return_online();s.select(0);s.accept();}else s.return_online_lobby();};
    bool requested{};std::optional<std::chrono::steady_clock::time_point> lobby_seen;const auto deadline=std::chrono::steady_clock::now()+150s;
    fo.session_update=[&](MenuState&s,std::uint32_t clock,const auto &){
      observed_ui_clock=clock;
      check(std::chrono::steady_clock::now()<deadline,"Native matchmaking screen never completed");
      const auto snapshot=queue->snapshot();if(snapshot.status==OnlineMatchmakingStatus::Failed)throw FormatError(snapshot.message);
      const auto a=account->snapshot();if(!requested&&s.frame().screen==14&&!s.ui_map_intro_color()&&a.party&&a.party->leader&&!a.busy&&
          !a.party->members.empty()&&a.party->members.front().ready_playlist=="multi-team"){
        if(!lobby_seen)lobby_seen=std::chrono::steady_clock::now();
        if(std::chrono::steady_clock::now()-*lobby_seen>500ms){const auto &rows=s.frame().entries;const auto start=std::find_if(rows.begin(),rows.end(),[](const auto&e){return e.destination==-17;});
          check(start!=rows.end(),"Original lobby omitted Start Game");s.select(unsigned(start-rows.begin()));s.accept();requested=true;}
      }
      return false;
    };
    stage="actual original pregame/search/configuration/countdown";auto result=run_frontend(fo);check(result.has_value(),"Real frontend did not return its admitted launch");
    auto launch=std::move(*result);check(requested&&launch.online_match&&launch.local_players==3&&launch.selected_profiles.size()==3&&launch.selected_teams.size()==3&&launch.input_instances.size()==3,"Four-team handoff lost couch selections");
    auto session=launch.online_match;check(session->roster().size()==12&&session->local_slots().size()==3,"Four-team admission did not contain twelve authenticated seats");
    std::array<unsigned,4> teams{};for(const auto &p:session->roster()){check(p.team>=0&&p.team<4,"Multi-Team admission outside original four-team range");++teams[unsigned(p.team)];}
    check(teams==std::array<unsigned,4>{3,3,3,3},"Native coordinator did not produce four equal teams");
    for(unsigned i=0;i<3;++i){const auto &p=session->roster()[session->local_slots()[i]];check(p.team==launch.selected_teams[i]&&p.team==launch.selected_teams[0]&&p.guest==(i>0)&&p.name==state.local_players()[i].name,"Native greedy balancer split a fitting three-seat party or changed identity");}
    return launch;
  };
  auto launch=search(true);const auto old_identity=launch.online_match->roster()[0].identity;
  auto run=[&](bool retirement){
    stage=retirement?"native opposing couch retirement":"native twelve-seat rounds";
    auto session=launch.online_match;const bool host=session->self()==session->host();
    // run_frontend resolves the selected file against its own menu cache.
    // The explicit gameplay cache owns that same admitted map and its HUD.
    check(launch.map.filename()=="lockout.map","Frontend selected a different validation map");
    MultiplayerOptions runtime;runtime.map=gameplay_maps/launch.map.filename();runtime.shared_maps=gameplay_maps;runtime.online_match=session;
    runtime.variant=launch.variant;if(retirement){runtime.variant.values.set(0,0);runtime.variant.values.set(10,90);}runtime.source_object_world=true;
    runtime.local_players=3;runtime.selected_profiles=launch.selected_profiles;runtime.selected_teams=launch.selected_teams;runtime.input_instances=launch.input_instances;
    runtime.width=960;runtime.height=720;runtime.hidden=runtime.mute=true;runtime.desktop=desktop;
    for(unsigned i=0;i<pads.sticks.size();++i){pads.neutral(i);check(SDL_JoystickSetVirtualAxis(pads.sticks[i],SDL_CONTROLLER_AXIS_LEFTY,Sint16(-16000-4000*i))==0,"SDL couch movement delivery failed");}
    std::set<unsigned> views;std::array<bool,3> controlled{};unsigned peak{},ticks{},frames{},leave_stage{};bool source{},ended{},round_board{},game_board{};std::array<std::pair<int,int>,16> before{};std::optional<unsigned> first_source_tick,finished_tick,controls_observed_at,pause_source_tick;const auto deadline=std::chrono::steady_clock::now()+150s;
    unsigned fade_opening{},fade_partial{},fade_clear{},fade_ending{};
    struct FadeRender {bool observed{};std::uint32_t tick{};std::optional<float> alpha;std::int16_t counter{};};
    std::array<FadeRender,3> render_fades{};
    bool initial_black_window_closed{},opening_fade_window_closed{};
    std::ofstream fade_phases(out/(owner+(retirement?"-retire-fade-phases.tsv":"-game-fade-phases.tsv")));
    fade_phases<<"phase\tview\ttick\talpha\tsource_counter\tgame_over_countdown\n";
    runtime.source_world_observer=[&](const auto &){source=true;};runtime.view_observer=[&](auto v,const auto &,float,std::int16_t){views.insert(v);};runtime.frame_observer=[&](const auto &t){frames+=t.submitted;};
    runtime.gameplay_fade_observer=[&](unsigned view,std::uint32_t tick,std::optional<float> alpha,const SourceGameplayFadeStateTu15 &fade){
      check(view<3,"Actual Multi-Team fade observation escaped its local view bank");
      check(!render_fades[view].observed,"Source view fade callback repeated within one actual render");
      render_fades[view]={true,tick,alpha,fade.phase_counter};
    };
    runtime.scoreboard_observer=[&](unsigned view,bool normal_hud,const FullScoreboardLayout&layout,const FullScoreboardPaintResult&paint){if(view)return;const bool visible=!normal_hud&&layout.status==FullScoreboardStatus::Ready&&layout.input_alpha>.99f&&!paint.draw.vertices.empty();round_board=visible&&layout.title.text==u"ROUND OVER";game_board=visible&&layout.title.text==u"GAME OVER";};
    runtime.frame_capture=[&](const MultiplayerState&w)->std::filesystem::path{
      const auto prefix=std::string(retirement?"retire-":"game-");
      const auto observed=std::exchange(render_fades,{});
      check(w.source_gameplay_fade.has_value(),"Actual submitted world omitted its source fade owner");
      unsigned black{},partial{},clear{};
      for(unsigned view=0;view<observed.size();++view){
        const auto &sample=observed[view];const auto bit=1u<<view;
        check(sample.observed&&sample.tick==w.tick,"Actual source fade receipt does not belong to this rendered frame");
        const auto record=[&](unsigned &mask,const char*name){if(mask&bit)return;mask|=bit;fade_phases<<name<<'\t'<<view<<'\t'<<w.tick<<'\t'<<(sample.alpha?*sample.alpha:-1.f)<<'\t'<<sample.counter<<'\t'<<w.game_over_countdown<<'\n';fade_phases.flush();};
        if(!(fade_opening&bit)){check(sample.alpha&&*sample.alpha==1.f,"Actual couch view did not begin with source activation black");record(fade_opening,"opening-black");}
        if(sample.alpha&&*sample.alpha==1.f)black|=bit;
        if(sample.alpha&&*sample.alpha>0.f&&*sample.alpha<1.f){partial|=bit;if(!w.finished&&w.round_index==0)record(fade_partial,"opening-partial");}
        if(!sample.alpha){clear|=bit;record(fade_clear,"world-clear");}
        if(w.finished&&(black&bit)&&w.game_over_countdown>0)record(fade_ending,"ending-world-black");
      }
      const auto request=[&](const std::string&phase){
        const auto path=capture_once(prefix+phase,w.round_phase,w.round_index,12);
        if(!path.empty()){
          nlohmann::json samples=nlohmann::json::array();
          for(unsigned view=0;view<observed.size();++view){const auto &s=observed[view];samples.push_back({{"view",view},
              {"tick",s.tick},{"alpha_bits",s.alpha?nlohmann::json(std::bit_cast<std::uint32_t>(*s.alpha)):nlohmann::json(nullptr)},
              {"phase_counter",s.counter}});}
          record_capture(prefix+phase,{{"file",path.filename().string()},{"domain","source-world"},{"tick",w.tick},
              {"finished",w.finished},{"game_over_countdown",w.game_over_countdown},{"round_phase",w.round_phase},
              {"round_index",w.round_index},{"black_view_mask",black},{"partial_view_mask",partial},{"clear_view_mask",clear},
              {"view_fades",std::move(samples)}});
        }
        return path;
      };
      if(!w.finished&&w.round_index==0){
        if(black!=7)initial_black_window_closed=true;
        if(clear&1u)opening_fade_window_closed=true;
        if(!initial_black_window_closed&&!capture_exists(prefix+"initial-black"))
          return request("initial-black");
        if(!opening_fade_window_closed&&!capture_exists(prefix+"opening-fade")&&!capture_exists(prefix+"opening-fade-view0")){
          if(partial==7)return request("opening-fade");
          if(partial&1u)return request("opening-fade-view0");
        }
        if(clear==7&&!capture_exists(prefix+"world-visible"))return request("world-visible");
      }
      if(w.finished&&game_board&&black==7&&!capture_exists(prefix+"ending-world-black"))
        return request("ending-world-black");
      if((w.finished&&!game_board)||(!w.finished&&w.source_round_lifecycle_available&&w.round_phase==2&&!round_board))return {};
      const auto phase=std::string(w.finished?(w.game_over_fade_started?"game-over":"game-over-countdown"):w.source_round_lifecycle_available&&w.round_phase==2?"round-over":w.source_round_lifecycle_available&&w.round_phase==3?"round-preparation":"playing");
      return request(phase);
    };
    runtime.input_observer=[&](auto step,auto controls,const auto &world){
      check(std::chrono::steady_clock::now()<deadline,"Actual native world did not complete");ticks=step;peak=std::max(peak,unsigned(std::count_if(world.players.begin(),world.players.end(),[](const auto&p){return p.connected;})));
      if(!first_source_tick)first_source_tick=world.tick;
      for(unsigned i=0;i<3;++i)controlled[i]|=controls[i].forward>.1f;
      // All three views use ordinary independently bound SDL controllers;
      // no source controls or prohibited ONLINE input script are supplied.
      // Hold through actual initial source preparation/input admission. A
      // five-step pulse can finish before the world accepts player movement.
      if(!controls_observed_at && std::all_of(controlled.begin(),controlled.end(),[](bool value){return value;}))controls_observed_at=step;
      if(controls_observed_at && step==*controls_observed_at+5)for(unsigned i=0;i<pads.sticks.size();++i)pads.neutral(i);
      if(step==0 || step==30 || step==60) {
        std::cout<<"SDL input proof owner="<<owner<<" step="<<step<<" round_phase="<<world.round_phase;
        for(unsigned i=0;i<pads.sticks.size();++i)std::cout<<" view"<<i<<"={axis:"<<SDL_JoystickGetAxis(pads.sticks[i],SDL_CONTROLLER_AXIS_LEFTY)
          <<",opened:"<<(SDL_GameControllerFromInstanceID(pads.instances[i])!=nullptr)<<",forward:"<<controls[i].forward<<'}';
        std::cout<<'\n';
      }
      if(retirement&&host&&step>90&&!ended){for(unsigned i=0;i<12;++i)before[i]={world.players[i].kills,world.players[i].deaths};ended=true;}
      if(world.finished&&!finished_tick){
        finished_tick=world.tick;
        if(retirement&&host){
          check(first_source_tick&&world.tick-*first_source_tick<60u*30u,
              "Retirement proof reached the unrelated ninety-second game time limit");
          unsigned retired{};
          for(unsigned i=0;i<12;++i)if(session->roster()[i].team!=launch.selected_teams.front()){
            check((world.players[i].source_flags&2)!=0,"Original first game-over tick retained an active opponent");++retired;
          }
          check(retired==9,"Original first game-over tick did not retain all nine retired opponents");
        }
      }
      if(retirement&&!host){const auto key=[&](SDL_Keycode k){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=SDL_GetWindowID(runtime.desktop->window(runtime.width,runtime.height,true));e.key.keysym.sym=k;check(SDL_PushEvent(&e)==1,"Failed to deliver real native pause input");};
        const auto base=120+unsigned(owner_index)*15;if(step>=base&&leave_stage==0){pause_source_tick=world.tick;key(SDLK_ESCAPE);++leave_stage;}else if(step>=base+15&&leave_stage==1&&pause_source_tick&&world.tick>*pause_source_tick){key(SDLK_DOWN);key(SDLK_DOWN);key(SDLK_RETURN);++leave_stage;}else if(step>=base+30&&leave_stage==2){key(SDLK_UP);key(SDLK_RETURN);++leave_stage;}}
    };
    // Observe the production retained-device map wait. Capture requests do
    // not alter its source clocks/readiness, and each file belongs to this
    // invocation's output directory under the wrapper's owned scratch.
    const auto loading_prefix=owner+(retirement?"-retire-loading-":"-game-loading-");
    const auto loading_before=out/(loading_prefix+"before-match.ppm");
    const auto loading_after=out/(loading_prefix+"before-pcr.ppm");
    check(desktop->source_loading_wait_configured(),"Rendered matchmaking did not admit the MCC source loading font");
    desktop->begin_source_loading_wait();desktop->capture_loading(loading_before);
    MultiplayerRuntimeOutcome outcome;run_multiplayer(runtime,outcome);
    check(std::filesystem::is_regular_file(loading_before),"Actual pre-match loading did not submit its requested frame");
    check(desktop->source_loading_wait_active(),"Actual ONLINE world return omitted its retained map wait");
    const auto loading_snapshot=desktop->loading_snapshot();
    check(loading_snapshot && !loading_snapshot->fade_active && !loading_snapshot->fade_complete,
          "Universal MP map wait borrowed the unrelated Vista/campaign fade clock");
    std::cout<<"Actual Multi-Team controls owner="<<owner<<" retirement="<<retirement<<" ticks="<<ticks
      <<" peak="<<peak<<" frames="<<frames<<" views="<<views.size()<<" controls="
      <<controlled[0]<<controlled[1]<<controlled[2]<<" leave_stage="<<leave_stage
      <<" exit_reason="<<unsigned(outcome.reason)<<'\n';
    check(ticks>30&&peak==12&&frames>10&&views==std::set<unsigned>{0,1,2}&&std::all_of(controlled.begin(),controlled.end(),[](bool v){return v;}),"Actual rendered Multi-Team lost views, twelve players, or independent controls");
    const auto image_prefix=std::string(retirement?"retire-":"game-");
    check(fade_opening==7&&fade_partial==7&&fade_clear==7&&capture_exists(image_prefix+"initial-black")&&
        (capture_exists(image_prefix+"opening-fade")||capture_exists(image_prefix+"opening-fade-view0"))&&capture_exists(image_prefix+"world-visible"),
        "Actual three-view couch did not submit original initial black, accurately labelled partial fade and clear");
    check(!host||source,"Rendered host omitted its original TU1.5 world");
    if(retirement&&!host){check(leave_stage==3&&outcome.reason==MultiplayerExitReason::escape,"Native confirmed Quit did not leave opposing couch");runtime.online_match.reset();launch.online_match.reset();session.reset();state.return_online_lobby();barrier("retirement-finished");return;}
    check(outcome.reason==MultiplayerExitReason::online_postgame&&outcome.state&&outcome.state->game_over_complete()&&outcome.state->results_finalized,"Original native engine did not complete four-team result/PCR handoff");
    check(fade_ending==7&&capture_exists(image_prefix+"ending-world-black"),"Completed three-view couch did not submit source ending-world black before loading");
    if(!retirement)check(capture_exists("game-round-over")&&capture_exists("game-round-preparation"),
      "Actual original intermission/preparation phases were not published and rendered on every native owner");
    if(retirement){const auto &w=*outcome.state;check(ended&&finished_tick&&w.tick>=*finished_tick+200&&w.winner_team==launch.selected_teams.front()&&w.retained_team_mask==15,"Last original team did not win after source seven-second game-over countdown");for(unsigned i=0;i<12;++i)check(std::pair<int,int>{w.players[i].kills,w.players[i].deaths}==before[i],"Leaving opposing teams fabricated kills/deaths");check(session->result_recorded()==false,"Departed owners incorrectly authorized persistent results");}
    else check(session->result_recorded()!=false,"All original owners did not persist unanimous native result");
    bool done{};std::optional<std::uint64_t> returned_lobby;
    auto report=online_postgame_screen(*frontend,session,[&]{done=true;state.return_online_lobby();returned_lobby=state.frame().instance;});check(report->results->player_count==12&&report->results->team_count==4,"Native PCR omitted twelve players or four original team records");
    pcr_prefix=retirement?"retire-":"game-";
    auto fo=front_options();fo.entry=FrontendOptions::Entry::postgame_report;fo.postgame_report=report;
    desktop->capture_loading(loading_after);
    unsigned phase{};std::set<unsigned> report_panes;const auto pcr_started=std::chrono::steady_clock::now();
    const auto key=[&](SDL_Keycode k){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;check(SDL_PushEvent(&e)==1,"Actual PCR/lobby SDL input failed");};
    fo.session_update=[&](MenuState&s,std::uint32_t clock,const auto&){
      observed_ui_clock=clock;
      check(std::chrono::steady_clock::now()-pcr_started<30s,"Actual PCR/prior traversal timed out");
      if(!phase&&s.frame().vista_pcr&&s.frame().screen!=vista_menu(185)){
        const auto pane=s.frame().context;
        if(std::filesystem::is_regular_file(out/(owner+"-"+pcr_prefix+"pcr-"+std::to_string(pane)+".ppm"))&&report_panes.insert(pane).second){
          if(report_panes.size()==6){key(SDLK_ESCAPE);phase=1;}else key(SDLK_RIGHT);
        }
      }else if(phase==1&&s.frame().screen==14){
        check(done&&returned_lobby&&s.frame().instance==*returned_lobby,"PCR Done did not retain its actual lobby instance");
        key(SDLK_TAB);phase=2;
      }else if(phase==2&&s.frame().screen==vista_menu(185)&&std::filesystem::is_regular_file(out/(owner+"-"+pcr_prefix+"prior-"+std::to_string(s.frame().context)+".ppm"))){
        check(!s.pcr_intro_color(s.frame()),"Prior statistics restarted the original completed-report intro");key(SDLK_ESCAPE);phase=3;
      }else if(phase==3&&s.frame().screen==14){
        check(returned_lobby&&s.frame().instance==*returned_lobby,"Prior statistics replaced the retained pregame lobby");return true;
      }
      return false;
    };
    run_frontend(fo);check(done&&state.frame().screen==14&&state.has_online_prior_stats()&&state.local_players().size()==3&&login->authorization(),"Original PCR Done lost retained lobby, profiles, consent or login");
    check(report_panes.size()==6&&phase==3,"Actual SDL report traversal did not render all six tabs and return from prior statistics");
    for(const auto *intro:{"black","fade","clear"})check(std::filesystem::is_regular_file(out/(owner+"-"+pcr_prefix+"pcr-intro-"+intro+".ppm")),"Actual PCR did not render its original black/fade/clear intro");
    check(std::filesystem::is_regular_file(loading_after) && !desktop->loading_active(),
          "Actual post-match loading did not present and join before the new PCR/lobby owner");
    const auto loading_log=out/(owner+"-loading-phases.tsv");
    const bool new_loading_log=!std::filesystem::exists(loading_log);
    std::ofstream loading_phases(loading_log,std::ios::app);
    if(new_loading_log)loading_phases<<"phase\tfile\tsource_caption_owner\n";
    loading_phases<<"before-match\t"<<loading_before.filename().string()<<"\tTU1.5 229DB0\n"
        <<"before-pcr\t"<<loading_after.filename().string()<<"\tTU1.5 229DB0\n";
    runtime.online_match.reset();launch.online_match.reset();session.reset();report.reset();
    if(retirement)barrier("retirement-finished");
  };
  if(std::getenv("HALO2_MULTITEAM_RETIREMENT_ONLY"))run(true);
  else {run(false);barrier("completed-pcr");launch=search(false);check(launch.online_match->roster()[0].identity!=old_identity,"Retained-lobby requeue reused old matched room");run(true);}
  queue->sign_out();account->sign_out();
  if(std::getenv("HALO2_MULTITEAM_RETIREMENT_ONLY"))std::cout<<"PASS focused actual twelve-seat native opposing couch retirement and unrecorded original ending; full rounds/requeue excluded from this focused case\n";
  else std::cout<<"PASS original TU1.5 native four-team ×3 gameplay -> 12-seat PCR -> prior stats -> same lobby requeue -> all opposing teams native confirmed Quit -> last-team game-over, no fabricated deaths or departed-owner persistence\n";
  return 0;
}catch(const std::exception&e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
