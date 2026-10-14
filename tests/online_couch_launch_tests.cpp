#define SDL_MAIN_HANDLED
#include "halo2/online_matchmaking.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include <SDL.h>
#include <curl/curl.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
const char *stage="sign-in";
template<class F>void wait(F f){const auto end=std::chrono::steady_clock::now()+60s;while(std::chrono::steady_clock::now()<end){if(f())return;std::this_thread::sleep_for(5ms);}throw FormatError(std::string("Couch test timed out: ")+stage);}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
}
int main(int argc,char **argv)try {
  check(argc==9,"Supply ROOT ORIGIN OWNER PORT OUTPUT VISTA XBE MODE");
  SDL_SetMainReady();
  const std::filesystem::path root=argv[1],maps=root/"assets/disc/maps",output=argv[5],vista=argv[6];
  const std::string owner=argv[3],mode=argv[8];const bool render=mode=="render";
  const bool custom=std::getenv("HALO2_COUCH_TEST_VARIANT")!=nullptr;
  const std::string map=std::getenv("HALO2_COUCH_TEST_MAP")?std::getenv("HALO2_COUCH_TEST_MAP"):"lockout";
  check(!map.empty() && map.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos,"Invalid couch-test map key");
  if(!std::filesystem::exists(maps/(map+".map")))return 77;
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
  frontend->content().vista=load_vista_settings(vista/"maps/mainmenu.map");
  for(auto &v:frontend->content().variants)if(v.name.starts_with("Team Slayer")){v.values.set(10,8);v.values.set(2,0);}
  if(custom)for(auto &v:frontend->content().bundled_variants){v.values.set(10,8);v.values.set(2,0);}
  auto &state=frontend->create_state();state.accept();state.accept();state.text("Couch "+owner);state.commit_text();state.back();
  state.set_discord_login(login);
  OnlineMatchmakingOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;options.maps={map};options.poll_interval=20ms;
  auto queue=std::make_shared<OnlineMatchmaking>(login,options);state.set_online_matchmaking(queue);state.update_online_login();state.open(186);
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
  auto search=[&] {
    const bool quick=mode=="core" && searches++==0;
    state.return_online();state.update_online_login();state.open(quick?210:219);stage="catalog";
    if(!quick) {
    wait([&]{state.update_online_login();state.update_online_matchmaking();const auto s=queue->snapshot();if(s.status==OnlineMatchmakingStatus::Failed)throw FormatError(s.message);return s.status==OnlineMatchmakingStatus::Ready;});
    const auto catalog=queue->snapshot().playlists;
    const auto row=std::find_if(catalog.begin(),catalog.end(),[](const auto &p){return p.id=="casual";});
    check(row!=catalog.end() && row->eligible,"Couch casual playlist unavailable");
    state.select(unsigned(row-catalog.begin()));state.accept();check(state.frame().screen==237,"Playlist details missing");
    state.accept();check(state.frame().screen==210,"Native search did not open");
    }
    stage="eight-player admission";
    wait([&]{state.update_online_login();state.update_online_matchmaking();if(queue->snapshot().status==OnlineMatchmakingStatus::Failed)throw FormatError(queue->snapshot().message);return state.launch.has_value();});
    auto launch=std::move(*state.launch);state.launch.reset();queue->sign_out();
    check(launch.local_players==4 && launch.selected_profiles.size()==4 && launch.selected_teams.size()==(custom?0:4) && launch.input_instances.size()==4,"Native handoff omitted local players/profiles/teams/controllers");
    check(launch.online_match->roster().size()==8 && launch.online_match->local_slots().size()==4,"Room omitted couch seats");
    const auto slots=launch.online_match->local_slots();
    for(unsigned i=0;i<4;++i) {
      const auto &p=launch.online_match->roster()[slots[i]];
      check(p.guest==(i>0 && !(i==1 && mode=="mixed")),"Guest classification differs from verified local login");
      check(p.name==state.local_players()[i].name && (custom || p.team==launch.selected_teams[0]),"Couch name/team lost");
      check(launch.input_instances[i]==(i?int(100+i):-1),"Controller binding changed during queue handoff");
    }
    return launch;
  };
  auto launch=search();auto session=launch.online_match;
  if(render) {
    if(!std::getenv("DISPLAY"))return 77;
    MultiplayerOptions runtime;runtime.map=maps/launch.map;runtime.shared_maps=maps;runtime.gameplay_xbe=argv[7];runtime.ui_assets=vista;
    runtime.online_match=session;runtime.variant=launch.variant;runtime.local_players=launch.local_players;
    runtime.selected_profiles=launch.selected_profiles;runtime.selected_teams=launch.selected_teams;runtime.input_instances=launch.input_instances;
    runtime.width=1280;runtime.height=720;runtime.hidden=runtime.mute=true;runtime.screenshot=output/(owner+"-couch.ppm");
    runtime.capture_tick=120;runtime.input_script=output/(owner+"-input.txt");
    {std::ofstream script(runtime.input_script);for(unsigned t=0;t<120;++t)for(unsigned p=0;p<4;++p)script<<t<<' '<<p<<' '<<(t%20<10)<<" 0 0 "<<p*1.1f<<" 0 move "<<(.2f+.1f*p)<<" 0\n";}
    std::set<unsigned> views;std::array<bool,4> controlled{};unsigned peak{},ticks{};
    runtime.view_observer=[&](unsigned view,const Camera &,float,std::int16_t){views.insert(view);};
    runtime.input_observer=[&](auto t,auto controls,const auto &world){ticks=t;for(unsigned i=0;i<4;++i)controlled[i]|=controls[i].fire && controls[i].forward>.1f;peak=std::max(peak,unsigned(std::count_if(world.players.begin(),world.players.end(),[](const auto &p){return p.connected;})));};
    MultiplayerRuntimeOutcome outcome;run_multiplayer(runtime,outcome);
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
  bool done{};auto report=online_postgame_screen(*frontend,session,[&]{done=true;state.return_online();});
  check(report->online && report->results->player_count==8,"Local Vista PCR omitted guest details");
  frontend->enter(FrontendOptions::Entry::postgame_report,report);state.back();
  check(done && state.local_players().size()==4 && login->authorization(),"PCR Done lost couch profiles or primary login");
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
