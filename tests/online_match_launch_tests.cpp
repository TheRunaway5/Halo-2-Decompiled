#define SDL_MAIN_HANDLED
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_account.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/game_voice.hpp"
#include "halo2/discord_voice_blocks.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/desktop_session.hpp"
#include <SDL.h>
#include <curl/curl.h>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
const char *wait_stage="OAuth authorization";
template<class F>void wait(F f){const auto end=std::chrono::steady_clock::now()+30s;while(std::chrono::steady_clock::now()<end){if(f())return;std::this_thread::sleep_for(5ms);}throw FormatError(std::string("Native ONLINE handoff timed out: ")+wait_stage);}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
struct Synthetic:VoiceDevices {
  std::atomic<unsigned> starts{},audible{},captures{};bool transmitting{};unsigned phase{},noise=12345;
  void start()override{++starts;}
  void transmit(bool v)override{transmitting=v;}
  bool capture(VoicePcm &pcm)override {
    if(!transmitting)return false;
    // A stationary sine is classified as non-speech by Opus DTX. Vary the
    // harmonic content and envelope to exercise long-lived voice recovery.
    for(auto &v:pcm){noise=noise*1664525u+1013904223u;const float t=float(phase++)/48000.f;
      const float a=.5f+.5f*std::sin(t*6.2831853f*4.3f);
      v=a*(.15f*std::sin(t*6.2831853f*(140+30*std::sin(t*3)))+.12f*(float(noise>>8)/16777216.f*2-1));}
    ++captures;return true;
  }
  void playback(const VoicePcm &pcm)override {float energy{};for(auto v:pcm)energy+=v*v;if(energy>.001f)++audible;}
  void stop()override{transmitting=false;}
};
}
int main(int argc,char **argv)try {
  check(argc==7 || argc==8,"Supply ROOT ORIGIN OWNER CALLBACK_PORT OUTPUT MODE [PLAYLIST]");
  SDL_SetMainReady();const std::filesystem::path root=argv[1],output=argv[5];
  const std::string owner=argv[3],mode=argv[6];
  const std::string first_playlist=argc==8?argv[7]:"ranked";
  const bool clan=mode=="clan" || mode=="clan-major" || mode=="clan-full";
  const bool group=mode=="party" || clan;
  const bool team=group || mode=="team" || mode=="team-render";
  const unsigned participants=mode=="clan-full"?16:mode=="clan-major"?14:clan?6:group?4:2;
  const std::vector<std::string> owners={"one","two","three","four","five","six","seven","eight","nine","ten","eleven","twelve","thirteen","fourteen","fifteen","sixteen"};
  const auto found=std::find(owners.begin(),owners.end(),owner);check(found!=owners.end(),"Unknown fixture owner");
  const unsigned owner_index=unsigned(found-owners.begin());
  const unsigned first_size=mode=="clan-full"?8:mode=="clan-major"?6:clan?3:2;
  const unsigned party_begin=owner_index<first_size?0:first_size;
  const unsigned party_end=owner_index<first_size?first_size:participants;
  const bool leader=owner_index==party_begin;
  auto barrier=[&](const std::string &stage) {
    const auto directory=std::getenv("HALO2_NATIVE_FIXTURE_BARRIER");if(!directory)return;
    const auto path=std::filesystem::path(directory)/stage;std::filesystem::create_directories(path);
    {std::ofstream arrived(path/owner);arrived<<"ready\n";check(bool(arrived),"Fixture barrier could not record arrival");}
    wait_stage=stage.c_str();
    wait([&]{return std::all_of(owners.begin(),owners.begin()+participants,[&](const auto &p){return std::filesystem::is_regular_file(path/p);});});
    wait_stage="native continuation";
  };
  unsigned search_round{};
  const auto maps=root/"assets/disc/maps";
  if(!std::filesystem::exists(maps/"lockout.map"))return 77;
  std::string browser;DiscordLoginOptions authorization;authorization.callback_port=std::stoul(argv[4]);
  auto login=std::make_shared<DiscordLogin>([&](const auto &url){browser=url;return true;},authorization,
      [&](const DiscordHttpRequest &r,std::stop_token)->DiscordHttpResponse {
        if(r.url.ends_with("/token"))return {200,"{\"access_token\":\""+owner+"\",\"token_type\":\"Bearer\",\"scope\":\"identify\",\"expires_in\":3600}"};
        return {200,"{\"id\":\""+std::to_string(123456789012345678ull+owner_index)+"\",\"username\":\"test\",\"global_name\":\"Test\"}"};
      });
  login->start();const auto begin=browser.find("state=")+6;
  const auto callback="http://127.0.0.1:"+std::string(argv[4])+"/discord/callback?code=test&state="+browser.substr(begin,browser.find('&',begin)-begin);
  std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
  curl_easy_setopt(curl.get(),CURLOPT_URL,callback.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
  check(curl_easy_perform(curl.get())==CURLE_OK,"Fixture OAuth callback failed");wait([&]{return login->authorization().has_value();});
  std::shared_ptr<OnlineAccount> account;
  if(group) {
    OnlineAccountOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;options.enable_parties=true;options.poll_interval=1500ms;
    account=std::make_shared<OnlineAccount>(login,options);account->connect("Same Pilot");
    wait_stage="account authentication";
    wait([&]{return account->snapshot().authenticated && !account->snapshot().busy;});
    // Native invites require previously registered ONLINE recipients. Every
    // synthetic account must finish that real login before leaders invite.
    barrier("online-accounts-ready");
    if(clan) {
      if(leader) {
        check(account->command({"create",party_begin==0?"Native Clan A":"Native Clan B"}),"Native clan create was not submitted");
        wait_stage="clan creation";
        wait([&]{return account->snapshot().clan && !account->snapshot().busy;});
        for(unsigned i=party_begin+1;i<party_end;++i) {
          check(account->command({"invite",owners[i]}),"Native clan invite was not submitted");
          wait_stage="clan invitation response";
          wait([&]{return !account->snapshot().busy;});
          check(account->snapshot().message=="Clan updated.","Native clan invitation failed after registered-account barrier");
        }
      } else {
        wait_stage="clan invitation arrival";
        wait([&]{return !account->snapshot().invitations.empty() && !account->snapshot().busy;});
        check(account->command({"accept",account->snapshot().invitations.front().id}),"Native clan acceptance was not submitted");
      }
      wait_stage="accepted full clan roster";
      wait([&]{const auto s=account->snapshot();return s.clan && s.clan->members.size()==party_end-party_begin && !s.busy;});
    }
    if(leader) {
      check(account->party_command({"create"}),"Native party create was not submitted");
      wait_stage="party creation";
      wait([&]{return account->snapshot().party && !account->snapshot().busy;});
      for(unsigned i=party_begin+1;i<party_end;++i) {
        check(account->party_command({"invite",owners[i]}),"Native party invite was not submitted");
        wait([&]{return !account->snapshot().busy;});
      }
    } else {
      wait_stage="party invitation arrival";
      std::uint64_t attempted{};
      // Simultaneous invitees legitimately invalidate each other's version.
      // Simulate selecting/confirming the refreshed invitation; production
      // never silently retries acceptance against a changed party roster.
      wait([&]{
        const auto s=account->snapshot();if(s.party && !s.busy)return true;
        if(!s.busy && !s.party_invitations.empty()) {
          const auto invitation=s.party_invitations.front();
          if(invitation.version!=attempted) {
            attempted=invitation.version;
            check(account->party_command({"accept",invitation.id,{},{},invitation.version}),"Native party acceptance was not submitted");
          }
        }
        return false;
      });
    }
    wait_stage="accepted full party roster";
    wait([&]{const auto s=account->snapshot();return s.party && s.party->members.size()==party_end-party_begin && !s.busy;});
    check(account->snapshot().party->leader==leader,"Native party leader authority differs");
  }
  OnlineMatchmakingOptions o;o.endpoint=argv[2];o.allow_loopback_http=true;o.maps={"lockout"};o.poll_interval=20ms;
  if(const auto *profile=std::getenv("HALO2_MATCHMAKING_GATHERINGS");profile && std::string(profile)=="1")
    o.compatibility="halo2-native-queue-v8";
  auto queue_owner=std::make_shared<OnlineMatchmaking>(login,o);auto &queue=*queue_owner;
  std::optional<FrontendContent> party_content;
  std::unique_ptr<MenuState> party_menu;
  std::optional<FrontendLaunch> party_launch;
  if(group) {
    const Cache menu(maps/"mainmenu.map");party_content=menu.load_frontend();
    party_content->vista=load_vista_settings(root/"assets/imported/a13438f3719c48bcb914624ed35022c7/vista/maps/mainmenu.map");
    // A finite custom validation round uses the real selected source variant.
    // Only its duration/round count are adjusted, identically on every client.
    for(auto &v:party_content->variants)if(v.name.starts_with("Team Slayer")){v.values.set(10,8);v.values.set(2,0);}
    party_menu=std::make_unique<MenuState>(*party_content);
    party_menu->accept();party_menu->accept();party_menu->text("Same Pilot");party_menu->commit_text();party_menu->back();
    party_menu->set_discord_login(login);party_menu->set_online_account(account);party_menu->set_online_matchmaking(queue_owner);
    party_menu->update_online_login();
  }
  auto search=[&](const std::string &playlist) {
    if(group) {
      party_menu->return_online();party_menu->update_online_login();party_menu->open(219);
      wait_stage="authenticated catalog";
      wait([&]{party_menu->update_online_login();party_menu->update_online_matchmaking();return queue.snapshot().status==OnlineMatchmakingStatus::Ready;});
      barrier("authenticated-catalog-"+std::to_string(++search_round));
      // All independent testers finish reconnecting before selecting fresh
      // consent. Two publications exclude an older already-in-flight read.
      const auto previous=account->snapshot().revision;
      wait([&]{return account->snapshot().revision>=previous+2 && !account->snapshot().busy;});
      const auto catalog=queue.snapshot().playlists;
      const auto row=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return p.id==playlist;});
      check(row!=catalog.end(),"Party playlist absent");
      check(row->ranked || !row->level,"Casual party playlist exposed hidden skill to native client");
      party_menu->select(unsigned(row-catalog.begin()));party_menu->accept();
      check(party_menu->frame().screen==237,"Party playlist details did not open");
      if(clan)check(row->clan_match && row->level &&
        party_menu->online_matchmaking_text(party_menu->frame(),0).find("Your clan level:")!=std::string::npos,
        "Vista clan details lost clan-owned rank");
      wait_stage="per-member playlist consent";
      if(leader)wait([&]{party_menu->update_online_login();const auto s=account->snapshot();return s.party && !s.busy && std::all_of(s.party->members.begin(),s.party->members.end(),[&](const auto &m){return m.self || m.ready_playlist==playlist;});});
      party_menu->accept();check(party_menu->frame().screen==210,"Party Find Match did not open native search");
      std::cout<<"Native party "<<(leader?"leader":"member")<<" chose "<<playlist<<" through Vista playlist/search owners"<<std::endl;
    } else {
      queue.refresh("Same Pilot");wait([&]{return queue.snapshot().status==OnlineMatchmakingStatus::Ready;});
      const auto catalog=queue.snapshot().playlists;
      const auto row=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return p.id==playlist;});
      check(row!=catalog.end(),"Requested playlist absent");
      check(row->ranked || !row->level,"Casual playlist exposed hidden skill to native client");
      queue.join(playlist);
    }
  };
  auto take_session=[&] {
    if(!group)return queue.take_launch();
    party_menu->update_online_login();party_menu->update_online_matchmaking();
    if(queue.snapshot().status==OnlineMatchmakingStatus::Failed)throw FormatError(queue.snapshot().message);
    if(party_menu->launch){party_launch=std::move(party_menu->launch);party_menu->launch.reset();return party_launch->online_match;}
    return std::shared_ptr<OnlineMatchSession>{};
  };
  search(first_playlist);
  wait_stage="match handoff";
  std::shared_ptr<OnlineMatchSession> session;wait([&]{session=take_session();return bool(session);});
  queue.sign_out();
  check(session->roster().size()==participants,"Match did not retain every authenticated player");
  if(group) {
    check(party_launch && party_launch->variant.name=="Team Slayer" && party_launch->selected_teams.size()==1 &&
          party_launch->selected_teams[0]==session->roster()[session->self()].team,"Native party UI launch lost actual source variant or assigned team");
    std::vector<unsigned> sizes;
    for(unsigned t=0;t<2;++t)sizes.push_back(unsigned(std::count_if(session->roster().begin(),session->roster().end(),[&](const auto &p){return p.team==t;})));
    std::sort(sizes.begin(),sizes.end());
    check(sizes==std::vector<unsigned>{first_size,participants-first_size},"Party team partition is not two whole teams");
  }
  auto complete_and_requeue=[&](bool render) {
    check(session->phase()==OnlineMatchSession::Phase::Postgame,"Match did not receive its completed publication");
    auto frontend=std::make_shared<FrontendSession>();frontend->discord_login=login;
    const Cache menu(maps/"mainmenu.map");frontend->retain_content(menu.load_frontend());
    frontend->content().vista=load_vista_settings(root/"assets/imported/3c4bc518c80645f9b12caccab6d61f57/vista/maps/mainmenu.map");
    auto &state=frontend->create_state();state.accept();state.accept();state.text("Same Pilot");state.commit_text();state.back();
    state.set_discord_login(login);
    bool done{};auto report=online_postgame_screen(*frontend,session,[&]{done=true;state.return_online();});
    check(report->online,"ONLINE postgame lost its Vista asset provenance");
    check(report->results->player_count==participants && report->results->variant==2,"Vista report omitted a real participant");
    if(team) {
      check(report->results->team_count==2,"Vista report omitted admitted teams");
      const auto &names=frontend->content().vista->vista_team_names;
      check(names[0]=="Red Team" && names[1]=="Blue Team" && names[8]=="Observers","Referenced Vista team-name locale differs");
    }
    if(const auto audio=session->retained_voice()) {
      report->voice_update(true);
      check(report->voice(0).visible,"Actual Vista PCR voice owner omitted admitted identity");
    }
    const auto old_identity=session->roster()[0].identity;
    if(render) {
      FrontendOptions options;options.session=frontend;options.entry=FrontendOptions::Entry::postgame_report;options.postgame_report=report;
      options.maps=maps;options.ui_assets=root/"assets/imported/3c4bc518c80645f9b12caccab6d61f57/vista";
      options.vista_settings=options.ui_assets/"maps/mainmenu.map";options.width=1280;options.height=720;
      options.skip_intro=options.hidden=options.mute=true;options.screenshot=output/(owner+"-postgame.ppm");
      // Observe the genuine four-second PCR fade before a finite capture.
      std::this_thread::sleep_for(4500ms);options.frames=30;
      run_frontend(options);
      check(frontend->discord_login==login && login->authorization().has_value(),"World/menu return discarded Discord identity");
    } else frontend->enter(FrontendOptions::Entry::postgame_report,report);
    state.back();check(done && !state.frame().vista_pcr,"Original PCR Done did not allocate fresh ONLINE navigation");
    session.reset();
    search(clan?first_playlist:"casual");
    wait([&]{session=take_session();return bool(session);});queue.sign_out();
    check(session->roster().size()==participants && session->roster()[0].identity!=old_identity,"Requeue reused old room identity or lost roster");
    std::cout<<"PASS completed match -> actual Vista PCR16 -> Done -> retained Discord sign-in -> fresh matched room\n";
  };
  if(mode=="render" || mode=="team-render") {
    if(!std::getenv("DISPLAY"))return 77;
    std::filesystem::create_directories(output);
    MultiplayerOptions runtime;runtime.map=maps/"lockout.map";runtime.shared_maps=maps;runtime.online_match=session;
    runtime.gameplay_xbe=root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
    runtime.ui_assets=root/"assets/imported/3c4bc518c80645f9b12caccab6d61f57/vista";
    runtime.width=1280;runtime.height=720;runtime.hidden=true;runtime.mute=true;runtime.replay=true;
    runtime.variant.name=team?"Team Slayer":"Slayer";
    runtime.variant.values.set(10,8);runtime.variant.values.set(2,0);runtime.variant.values.set(91,int(team));
    std::array<std::uint8_t,9> appearance{};runtime.selected_profiles.push_back(native_selected_player_configuration("Same Pilot",appearance,team?session->roster()[session->self()].team:-1));
    if(team)runtime.selected_teams.push_back(std::int8_t(session->roster()[session->self()].team));
    runtime.screenshot=output/(owner+"-online.ppm");
    unsigned ticks{},players{};bool fire{};
    runtime.input_observer=[&](auto step,auto controls,const auto &state){ticks=step;fire|=controls[0].fire;players=std::max(players,unsigned(std::count_if(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;})));};
    MultiplayerRuntimeOutcome outcome;
    run_multiplayer(runtime,outcome);
    check(ticks>10 && players==2 && fire && outcome.state.has_value(),"Rendered matched gameplay never advanced real players and controls");
    check(outcome.reason==MultiplayerExitReason::online_postgame && outcome.state->game_over_complete(),"Runtime did not hand off actual completed results");
    complete_and_requeue(true);
    runtime.online_match=session;runtime.frames=500;runtime.screenshot=output/(owner+"-requeued.ppm");runtime.variant={team?"Team Slayer":"Slayer",0,{}};runtime.variant.values.set(91,int(team));
    runtime.desktop=std::make_shared<DesktopSession>();
    bool escaped{};runtime.input_observer=[&](auto step,auto,const auto&){
      if(owner=="one" && step>=60 && !escaped){SDL_SetRelativeMouseMode(SDL_FALSE);SDL_Event event{};event.type=SDL_KEYDOWN;
        event.key.windowID=SDL_GetWindowID(runtime.desktop->window(runtime.width,runtime.height,true));
        event.key.keysym.sym=SDLK_ESCAPE;check(SDL_PushEvent(&event)==1,"Could not deliver real Escape event");escaped=true;}
    };
    MultiplayerRuntimeOutcome second;run_multiplayer(runtime,second);
    check(second.state && second.state->tick>10,"Requeued ONLINE match did not launch real gameplay");
    check(second.reason==(owner=="one"?MultiplayerExitReason::escape:MultiplayerExitReason::online_disconnected),"Leave/disconnect did not yield clean runtime ownership");
    runtime.online_match.reset();session.reset();
    std::cout<<"PASS second real ONLINE game -> Escape/disconnect -> clean native return outcome\n";
    std::cout<<"PASS rendered ONLINE host/client handoff players="<<players<<" ticks="<<ticks<<"\n";
    std::this_thread::sleep_for(2s);return 0;
  }
  const Cache cache(maps/"lockout.map",maps);const auto content=cache.load_content(0,true);
  UiVariant variant=group?party_launch->variant:UiVariant{team?"Team Slayer":"Slayer",0,{}};
  if(!group){variant.values.set(10,mode=="mismatch" && owner=="one"?9:8);variant.values.set(2,0);variant.values.set(91,int(team));}
  auto rules=slayer_rules(variant,team);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  MultiplayerMatch match(content,0,cache.spawns(),rules);
  const auto fingerprint=multiplayer_fingerprint(maps/"lockout.map",maps,0,rules.weapon);
  session->loaded(fingerprint,variant);
  if(mode=="mismatch") {
    wait([&]{return session->phase()==OnlineMatchSession::Phase::Failed;});
    check(session->postgame().empty(),"Mismatched game rules produced a completed result");
    std::cout<<"PASS differing authored Slayer settings refused before native simulation\n";return 0;
  }
  wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;});
  const auto self=session->self(),other=(self+1)%participants;const bool hosting=self==session->host();
  std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
  std::array<std::uint8_t,9> appearance{};const auto profile=native_selected_player_configuration("Same Pilot",appearance,team?session->roster()[self].team:-1);
  if(hosting){match.join("Same Pilot",team?session->roster()[self].team:-1,profile,self);host=std::make_unique<LanHost>(match,session,fingerprint,session->roster());}
  else {const std::int8_t selected=std::int8_t(session->roster()[self].team);client=std::make_unique<LanClient>(session,session->host(),fingerprint,team?std::span(&selected,1):std::span<const std::int8_t>{},std::span(&profile,1));}
  std::array<Controls,16> controls{};std::array<Controls,1> local{};
  wait([&]{if(host){host->receive(controls);host->broadcast();std::this_thread::sleep_for(30ms);return host->peers()==participants-1;}client->send(local);client->receive();return client->ready();});
  auto devices=std::make_unique<Synthetic>();auto *audio=devices.get();auto retained=std::make_shared<GameVoice>(std::move(devices));session->retain_voice(retained);auto &voice=*retained;
  std::unique_ptr<DiscordVoiceBlocks> blocks;
  if(mode=="blocks") {
    DiscordVoiceBlockOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;
    blocks=std::make_unique<DiscordVoiceBlocks>(login,options);
  }
  const auto blocked_subject=std::to_string(123456789012345679ull);
  unsigned block_checkpoint{};
  std::this_thread::sleep_for(60ms);check(audio->starts==0,"Match voice opened a microphone before opt-in");voice.enable(true);voice.radio(true,true);
  unsigned roster{},tick{},minimum_loaded=1000,muted_before{},audible_before{},capture_checkpoint{};bool mute_checked{},talking_seen{};Vec3 initial{};bool position_set{},moved{};
  const auto start=std::chrono::steady_clock::now();
  for(unsigned n=0;n<210;++n) {
    const auto now=std::chrono::steady_clock::now();
    voice.blocks(session->voice_blocks());
    if(blocks) {
      if(n==30 && owner=="one")check(blocks->update({blocked_subject}),"Private native block sync was not queued");
      if(n==45){wait([&]{return session->voice_blocks()==(1u<<other);});voice.blocks(session->voice_blocks());voice.mute(other,false);}
      if(n==50)block_checkpoint=audio->audible;
      if(n==65)check(audio->audible==block_checkpoint && !voice.snapshot().talking[other],"Discord block or manual unmute failed to silence native playback");
      if(n==68 && owner=="one")check(blocks->update({}),"Explicit native unblock was not queued");
      if(n==74)wait([&]{return session->voice_blocks()==0;});
    }
    if(n==5)voice.radio(false,true);
    if(n==75)for(unsigned p=0;p<participants;++p)if(p!=self)voice.mute(p,true);
    if(n==85)muted_before=audio->audible;
    if(n==115){check(audio->audible==muted_before,"Muted matched speaker remained audible");mute_checked=true;for(unsigned p=0;p<participants;++p)if(p!=self)voice.mute(p,false);audible_before=audio->audible;}
    if(n==125)voice.microphone_muted(true);
    if(n==132)capture_checkpoint=audio->captures;
    if(n==142){check(audio->captures==capture_checkpoint,"Muted microphone continued capture");voice.microphone_muted(false);}
    if(n==150)voice.focus(false);
    if(n==157)capture_checkpoint=audio->captures;
    if(n==168){check(audio->captures==capture_checkpoint,"Unfocused match continued microphone capture");voice.focus(true);voice.radio(true,true);}
    if(n==172)voice.radio(false,true);
    if(n==180)voice.enable(false);
    if(n==186)capture_checkpoint=audio->captures;
    if(n==194){check(audio->captures==capture_checkpoint && !voice.snapshot().transmitting,"Disabled match voice continued capture");voice.enable(true);voice.radio(true,true);}
    if(n==198)voice.radio(false,true);
    local[0].forward=.5f;local[0].fire=n%20<10;
    if(client){client->send(local);client->receive();}
    else {controls[self]=local[0];host->receive(controls);match.step(controls);host->broadcast();}
    const auto state=client?client->state():match.state();tick=state.tick;
    roster=std::count_if(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;});
    const auto &p=state.players[self];minimum_loaded=std::min(minimum_loaded,unsigned(std::max(0,p.loaded)));
    if(!position_set){initial=p.position;position_set=true;}else moved|=length(p.position-initial)>.001f;
    auto route=[&](GameVoiceFrame frame){
      std::array<GameVoicePlayer,16> players{};for(unsigned i=0;i<16;++i){players[i].connected=state.players[i].connected;players[i].active=!state.players[i].dead;players[i].position=state.players[i].position;}
      for(unsigned recipient=0;recipient<participants;++recipient)if(recipient!=frame.source)
        if(auto gains=game_voice_route(frame.source,recipient,players,frame.radio,false,false,8)) {
          auto routed=frame;routed.gains=*gains;if(recipient==self)voice.receive(std::move(routed));else session->voice(recipient,routed);
        }
    };
    for(auto &packet:session->voice_packets())if(!packet.preferences){if(host){packet.frame.source=packet.peer;route(std::move(packet.frame));}else voice.receive(std::move(packet.frame));}
    for(auto &frame:voice.outgoing()){frame.source=self;if(host)route(std::move(frame));else session->voice(session->host(),frame);}
    const auto speaking=voice.snapshot();
    for(unsigned p=0;p<participants;++p)if(p!=self)talking_seen|=speaking.talking[p];
    if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());
    std::this_thread::sleep_until(start+(n+1)*33333us);
  }
  std::cout<<"Observed native "<<(hosting?"host":"client")<<" roster="<<roster<<" tick="<<tick<<" moved="<<moved<<" minimum_loaded="<<minimum_loaded<<" audible="<<audio->audible<<" before_unmute="<<audible_before<<" captures="<<audio->captures<<std::endl;
  check(roster==participants && tick>150 && moved && minimum_loaded<60,"Matched native state did not replicate movement/fire and real roster");
  check(mute_checked && audio->audible>audible_before+10 && audio->captures>50,"Match voice failed capture/relay/playback or unmute recovery");
  check(group?talking_seen:voice.snapshot().talking[other],"Actual matched speaker indicator absent");
  bool published{};
  wait([&]{
    if(host && session->phase()==OnlineMatchSession::Phase::Running){host->receive(controls);match.step(controls);host->broadcast();
      std::this_thread::sleep_for(28ms); // Retain the real 30Hz simulation cadence.
      if(!published && match.state().game_over_complete()) {const auto report=online_postgame_publish(match,variant,session->roster());session->complete(report.source);published=true;}}
    else if(client) {client->send(local);client->receive();}
    if(session->phase()==OnlineMatchSession::Phase::Reviewing) {
      const auto &final=client?client->state():match.state();const auto report=session->postgame();
      online_postgame_validate(final,variant,session->roster(),report);
      auto forged=report;forged[0x402]^=std::byte{1};bool rejected{};
      try{online_postgame_validate(final,variant,session->roster(),forged);}catch(const FormatError&){rejected=true;}
      check(rejected,"Client confirmed a score that differs from its final native snapshot");
      session->confirm_result();
    }
    if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());
    return session->phase()==OnlineMatchSession::Phase::Postgame;
  });
  host.reset();client.reset();
  barrier("postgame-voice-ready");
  const auto before_postgame=audio->audible.load();unsigned muted_postgame{};
  for(unsigned i=0;i<160;++i) {
    if(blocks) {
      if(i==10 && owner=="one")check(blocks->update({blocked_subject}),"Postgame block sync was not queued");
      if(i==25)wait([&]{return session->voice_blocks()==(1u<<other);});
      if(i==30){voice.mute(other,false);block_checkpoint=audio->audible;}
      if(i==44)check(audio->audible==block_checkpoint && !voice.snapshot().talking[other],"Discord block was lost at postgame or overridden by unmute");
      if(i==46 && owner=="one")check(blocks->update({}),"Postgame unblock was not queued");
    }
    if(i==45)for(unsigned p=0;p<participants;++p)if(p!=self)voice.mute(p,true);
    if(i==60)muted_postgame=audio->audible;
    if(i==90){check(audio->audible==muted_postgame,"Muted postgame speaker remained audible");for(unsigned p=0;p<participants;++p)if(p!=self)voice.mute(p,false);}
    session->update_postgame_voice(true);
    std::this_thread::sleep_for(20ms);
  }
  check(audio->audible>before_postgame+20 && session->postgame_voice_enabled(other),"Passive postgame session voice did not survive world teardown");
  check(voice.snapshot().talking[other],"Postgame actual remote talking state absent");
  std::cout<<"PASS passive postgame voice across teams without world positions; mutual mute/unmute and actual talking state\n";
  if(blocks)std::cout<<"PASS private SDK block handoff -> authenticated service -> bilateral native playback suppression -> manual unmute isolation -> explicit unblock recovery in gameplay and postgame\n";
  barrier("postgame-voice-checked");
  complete_and_requeue(mode=="team" && std::getenv("DISPLAY"));
  check(!voice.snapshot().enabled,"Done left the departed session microphone enabled");
  session->loaded(fingerprint,variant);wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;});
  if(owner=="one")session.reset();
  else {
    wait([&]{return session->phase()==OnlineMatchSession::Phase::Failed;});
    MultiplayerOptions aborted;aborted.map=maps/"lockout.map";aborted.shared_maps=maps;aborted.online_match=session;
    aborted.selected_profiles.push_back(profile);
    MultiplayerRuntimeOutcome outcome;run_multiplayer(aborted,outcome);
    check(outcome.reason==MultiplayerExitReason::online_disconnected,"Pre-world disconnect escaped clean handoff ownership");
    check(!outcome.state || !outcome.state->finished,"Pre-world disconnect fabricated a completed match");
    std::cout<<"PASS disconnect before native world handoff -> clean return without postgame fabrication\n";
  }
  std::cout<<"PASS matched native "<<(hosting?"host":"client")<<" players="<<roster<<" tick="<<tick<<" moved="<<moved<<" minimum_loaded="<<minimum_loaded<<" audible_frames="<<audio->audible<<"; opt-in, radio, relay, mute/unmute, speaker labels and queue teardown\n";
  std::this_thread::sleep_for(500ms);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
