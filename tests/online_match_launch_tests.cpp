#define SDL_MAIN_HANDLED
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_account.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_departure.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/game_voice.hpp"
#include "halo2/discord_voice_blocks.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/audio.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_binding.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
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
  void discard()override{} // Immediate synthetic device has no queued samples.
};
}
int main(int argc,char **argv)try {
  check(argc==7 || argc==8,"Supply ROOT ORIGIN OWNER CALLBACK_PORT OUTPUT MODE [PLAYLIST]");
  SDL_SetMainReady();const std::filesystem::path root=argv[1],output=argv[5];
  const auto imported_root=std::getenv("HALO2_ONLINE_FLOW_UI_ROOT");
  const std::filesystem::path mcc_root=imported_root?imported_root:"";
  std::filesystem::path vista;
  if(mcc_root.empty()){std::ifstream sources(root/"assets/asset-sources.tsv");std::string line;
    while(std::getline(sources,line))if(line.starts_with("vista\t"))vista=line.substr(6);}
  const std::string owner=argv[3],mode=argv[6];
  check(mode!="party" || !mcc_root.empty(),"Party UI validation requires an explicit prepared MCC root");
  const auto maps=mcc_root.empty()?root/"assets/disc/maps":mcc_root/"halo2/h2_maps_win64_dx11";
  const auto menu_settings=mcc_root.empty()?vista/"maps/mainmenu.map":maps/"mainmenu.map";
  check(std::filesystem::is_regular_file(menu_settings),"Selected UI bank is unavailable");
  const auto runtime_ui=mcc_root.empty()?vista:mcc_root;
  const std::string first_playlist=argc==8?argv[7]:"ranked";
  const bool clan=mode=="clan" || mode=="clan-major" || mode=="clan-full";
  const bool group=mode=="party" || clan;
  const bool departure_only=mode=="render-departure" || mode=="team-render-departure";
  const bool team=group || mode=="team" || mode=="team-render" || mode=="team-render-departure";
  const unsigned participants=mode=="source-render"?8:mode=="clan-full"?16:mode=="clan-major"?14:clan?6:group?4:2;
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
    OnlineAccountOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;options.enable_parties=true;options.poll_interval=mode=="party"?20s:1500ms;
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
  nlohmann::json visible_frames={{"schema","H2_NATIVE_PARTY_FRAMES_1"},
    {"behavior_source","OG Xbox TU1.5"},{"source_xbe_sha256","21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"},
    {"owner",owner},{"frames",nlohmann::json::array()},{"inputs",nlohmann::json::array()},{"sounds",nlohmann::json::array()}};
  const auto record_frame=[&](const MenuState &actual,const std::filesystem::path &path,std::uint32_t clock) {
    nlohmann::json row={{"file",path.filename().string()},{"screen",actual.frame().screen},{"pane",actual.frame().context},
      {"ui_clock",clock},{"ui_countdown",actual.online_lobby_countdown()},
      {"progress_pane",actual.online_matchmaking_progress_pane()},
      {"source_session_state",int(actual.online_matchmaking_ui())},{"matched_countdown",actual.online_matchmaking_countdown()},
      {"worker_queue_status",unsigned(queue.snapshot().status)}};
    const auto map_color=actual.ui_map_intro_color();
    row["map_intro_argb_observed_after_painter"]=map_color?nlohmann::json(*map_color):nlohmann::json(nullptr);
    if(const auto &p=actual.online_party_observation()) {
      row["leader"]=p->leader;row["party_version"]=p->version;
      if(p->countdown)row["countdown"]={{"serial",p->countdown->serial},
        {"seconds",p->countdown->seconds?nlohmann::json(*p->countdown->seconds):nlohmann::json(nullptr)},
        {"mode",p->countdown->mode},{"complete",p->countdown->complete}};
      else row["countdown"]=nullptr;
    }
    if(actual.frame().vista_pcr) {
      row["results"]={{"players",actual.frame().vista_pcr->results->player_count},
        {"teams",actual.frame().vista_pcr->results->team_count},{"variant",actual.frame().vista_pcr->results->variant}};
      row["pcr_open_clock"]=actual.frame().pcr_open_clock;
      const auto color=actual.pcr_intro_color(actual.frame());
      row["pcr_intro_argb_observed_after_painter"]=color?nlohmann::json(*color):nlohmann::json(nullptr);
    }
    auto &frames=visible_frames["frames"];
    const auto previous=std::find_if(frames.begin(),frames.end(),[&](const auto &r){return r.at("file")==path.filename().string();});
    if(previous==frames.end())frames.push_back(std::move(row));else *previous=std::move(row);
    std::ofstream file(output/(owner+"-ui-frames.json"));file<<visible_frames.dump(2)<<'\n';check(bool(file),"Could not record actual native frame provenance");
  };
  std::optional<FrontendContent> party_content;
  std::shared_ptr<FrontendSession> party_frontend;
  MenuState *party_menu{};
  std::optional<FrontendLaunch> party_launch;
  if(group) {
    const Cache menu(maps/"mainmenu.map");party_content=menu.load_frontend();
    party_content->vista=load_vista_settings(menu_settings);
    // A finite custom validation round uses the real selected source variant.
    // Only its duration/round count are adjusted, identically on every client.
    for(auto &v:party_content->variants)if(v.name.starts_with("Team Slayer")){v.values.set(10,8);v.values.set(2,0);}
    party_frontend=std::make_shared<FrontendSession>();party_frontend->discord_login=login;
    party_frontend->retain_content(std::move(*party_content));party_content.reset();
    party_frontend->online_services=std::make_shared<NativeTu15OnlineServices>();
    party_frontend->online_services->login=login;party_frontend->online_services->account=account;
    party_frontend->online_services->matchmaking=queue_owner;
    party_menu=&party_frontend->create_state();
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
      // The account generation gate discards reads begun before a command.
      // Observe actual accepted membership here; unrelated periodic social
      // publications are neither a consent owner nor a synchronization clock.
      wait([&]{const auto s=account->snapshot();return s.party && s.party->members.size()==party_end-party_begin && !s.busy;});
      const auto catalog=queue.snapshot().playlists;
      const auto row=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return p.id==playlist;});
      check(row!=catalog.end(),"Party playlist absent");
      check(row->ranked || !row->level,"Casual party playlist exposed hidden skill to native client");
      party_menu->select(unsigned(row-catalog.begin()));party_menu->accept();
      check(party_menu->frame().screen==14,"Party playlist selection did not return directly to its original lobby");
      if(clan)check(row->clan_match && row->level,"Authenticated clan playlist lost clan-owned rank");
      wait_stage="per-member playlist consent";
      if(leader)wait([&]{party_menu->update_online_login();const auto s=account->snapshot();return s.party && !s.busy && std::all_of(s.party->members.begin(),s.party->members.end(),[&](const auto &m){return m.self || m.ready_playlist==playlist;});});
      check(party_menu->frame().screen==14,"Chosen playlist did not enter the original pregame lobby before searching");
      const bool rendered_party=mode=="party" && search_round==1 && std::getenv("DISPLAY");
      if(leader && !rendered_party) {
        // Confirmation publishes this leader's own consent asynchronously.
        // Start is refused while its actual account command is still busy.
        wait_stage="leader's acknowledged pregame consent";
        wait([&]{party_menu->update_online_login();party_menu->update_online_matchmaking();
          const auto s=account->snapshot();return s.party && !s.busy &&
            std::all_of(s.party->members.begin(),s.party->members.end(),[&](const auto &m){return m.ready_playlist==playlist;});});
        const auto &entries=party_menu->frame().entries;
        const auto start=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
        check(start!=entries.end(),"Confirmed party hopper has no original Start Game action");
        party_menu->select(unsigned(start-entries.begin()));party_menu->accept();
        wait_stage="published party pregame countdown";
        wait([&]{party_menu->update_online_login();party_menu->update_online_matchmaking();return party_menu->online_lobby_countdown()>0;});
        check(party_menu->online_lobby_countdown()<=10 && !party_menu->launch,"Party Start skipped its original ten-second countdown");
      }
      if(rendered_party) {
        FrontendOptions options;options.session=party_frontend;options.entry=FrontendOptions::Entry::online_return;
        options.maps=maps;options.ui_assets=mcc_root.empty()?vista:std::filesystem::path{};options.vista_settings=menu_settings;
        options.skip_intro=options.hidden=options.mute=true;options.width=1280;options.height=720;
        const bool capture_audio=owner=="one" && std::getenv("HALO2_ONLINE_MATCH_AUDIO");
        if(capture_audio) {
          options.mute=false;
          const Cache admitted_menu(maps/"mainmenu.map");
          nlohmann::json sources={{"source_cache","admitted MCC"},{"slots",nlohmann::json::array()}};
          check(admitted_menu.is_mcc(),"Actual rendered party audio lost its admitted MCC owner");
          for(const auto slot:{10u,12u}) {
            const auto datum=party_frontend->content().sounds.at(slot);
            const auto encoded=admitted_menu.load_sound(datum);const auto prepared=decode_sound(encoded);
            write_wav(prepared,output/(owner+"-ui-slot-"+std::to_string(slot)+".wav"));
            sources["slots"].push_back({{"slot",slot},{"datum",datum},{"name",encoded.name},{"codec",encoded.codec},
              {"encoded_bytes",encoded.data.size()},{"sample_rate",encoded.sample_rate},{"channels",encoded.channels},
              {"source_class",encoded.sound_class},{"actual_decoded_frames",prepared.frames()}});
          }
          std::ofstream(output/(owner+"-ui-audio-source.json"))<<sources.dump(2)<<'\n';
        }
        unsigned phase{};int before_repeat{};std::uint64_t before_repeat_serial{};std::string capture_tag="idle";
        std::uint32_t observed_ui_clock{};
        const auto started=std::chrono::steady_clock::now();auto restart_observed=started;
        const auto version_before=account->snapshot().party->version;
        const auto capture_path=[&](const std::string &tag,const std::string &who){return output/(who+"-party-lobby-"+tag+".ppm");};
        const auto all_captured=[&](const std::string &tag){return std::all_of(owners.begin(),owners.begin()+participants,
          [&](const auto &who){return std::filesystem::is_regular_file(capture_path(tag,who));});};
        const auto key=[&](SDL_Keycode sym) {SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=sym;
          check(SDL_PushEvent(&e)==1,"Rendered lobby rejected its actual SDL keyboard event");
          e.type=SDL_KEYUP;check(SDL_PushEvent(&e)==1,"Rendered lobby rejected its actual SDL key release");
          visible_frames["inputs"].push_back({{"sdl_key",sym},{"ui_clock",observed_ui_clock},
            {"screen",party_menu->frame().screen},{"ui_countdown",party_menu->online_lobby_countdown()}});
        };
        const auto select_action=[&](MenuState &actual,int action) {
          const auto &entries=actual.frame().entries;
          const auto entry=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.destination==action;});
          check(entry!=entries.end(),"Rendered lobby lost its source button action");
          const auto target=unsigned(entry-entries.begin());check(target<2,"Rendered lobby action is not one of its two source buttons");
          if(actual.frame().selected!=target)key(target?SDLK_RIGHT:SDLK_LEFT);
        };
        options.session_update=[&](MenuState &actual,std::uint32_t clock,const auto &) {
          observed_ui_clock=clock;
          if(std::chrono::steady_clock::now()-started>90s)throw FormatError("Rendered shared lobby countdown timed out at phase "+std::to_string(phase));
          const auto s=account->snapshot();
          if(!s.party || s.party->version!=version_before)throw FormatError("Shared countdown changed the accepted party or its consent version");
          if(queue.snapshot().status==OnlineMatchmakingStatus::Failed)throw FormatError(queue.snapshot().message);
          const auto left=actual.online_lobby_countdown();
          const auto &bound=actual.online_party_observation();
          for(const auto cue:actual.ui_sound_observation())if(cue==FrontendUiSound::PregameCountdown || cue==FrontendUiSound::MatchmakingAdvance) {
            nlohmann::json receipt={{"cue",unsigned(cue)},{"ui_clock",clock},{"screen",actual.frame().screen},{"ui_countdown",left},
              {"progress_pane",actual.online_matchmaking_progress_pane()},{"match_countdown",actual.online_matchmaking_countdown()}};
            if(const auto &p=actual.online_party_observation();p && p->countdown)
              receipt["publication"]={{"serial",p->countdown->serial},{"seconds",p->countdown->seconds?nlohmann::json(*p->countdown->seconds):nlohmann::json(nullptr)}};
            visible_frames["sounds"].push_back(std::move(receipt));
          }
          if(phase==0 && !actual.ui_map_intro_color() && all_captured("idle") && !s.busy &&
             std::all_of(s.party->members.begin(),s.party->members.end(),[&](const auto &m){return m.ready_playlist==playlist;})) {
            if(leader){select_action(actual,-17);key(SDLK_RETURN);}phase=1;
          } else if(phase==1 && left>0) {capture_tag="start";phase=2;}
          else if(phase==2 && all_captured("start") && !s.busy) {if(leader)key(SDLK_x);phase=3;}
          else if(phase==3 && left<0 && !s.busy && !s.party->countdown) {capture_tag="cancel";phase=4;}
          else if(phase==4 && all_captured("cancel") && !s.busy) {if(leader){select_action(actual,-17);key(SDLK_RETURN);}phase=5;}
          else if(phase==5 && left>0) {
            restart_observed=std::chrono::steady_clock::now();phase=6;
            if(!leader)std::this_thread::sleep_for(1600ms); // Real delayed UI observer, service/identity workers continue.
          } else if(phase==6 && left>0 && left<10 && std::chrono::steady_clock::now()-restart_observed>=1600ms &&
                    s.party->countdown && s.party->countdown->seconds && left==*s.party->countdown->seconds) {
            // The worker can publish between this frame's menu update and
            // our observation. Capture once both actual observers agree.
            check(bound && bound->countdown,"Late frame lost its bound countdown");
            before_repeat=left;before_repeat_serial=bound->countdown->serial;
            capture_tag="late";phase=7;
          } else if(phase==7 && all_captured("late") && !s.busy) {
            check(bound && bound->countdown,"Rendered repeat lost the actual bound countdown");
            if(leader){select_action(actual,23);key(SDLK_RETURN);}phase=8;
          } else if(phase==8 && left>0 && left<before_repeat && !s.busy && bound && bound->countdown &&
                    bound->countdown->serial!=before_repeat_serial) {capture_tag="shorten";phase=9;}
          else if(phase==9 && all_captured("shorten") && !s.busy) {if(!leader)key(SDLK_x);phase=10;}
          else if(phase==10 && bound && bound->countdown && bound->countdown->mode==1 && left==10) {capture_tag="member-request";phase=11;}
          else if(phase==11 && all_captured("member-request") && !s.busy) {
            if(left>3 && leader){select_action(actual,23);key(SDLK_RETURN);}
            if(left==3){capture_tag="floor3";phase=12;}
          } else if(phase==12 && all_captured("tick-3"))phase=13;
          if(actual.frame().screen==210 && phase>=12) {capture_tag="zero-search";phase=14;}
          return false;
        };
        options.frame_capture=[&](const MenuState &actual) {
          if(actual.ui_map_intro_color())return std::filesystem::path{};
          std::string tag=capture_tag;
          if(phase>=12 && actual.frame().screen==14 && actual.online_lobby_countdown()>=1 && actual.online_lobby_countdown()<=3)
            tag="tick-"+std::to_string(actual.online_lobby_countdown());
          if(std::filesystem::is_regular_file(capture_path(tag,owner)) && actual.frame().screen==210)
            tag="actual-pane-"+std::to_string(actual.online_matchmaking_progress_pane())+"-source-state-"+std::to_string(int(actual.online_matchmaking_ui()));
          const auto path=capture_path(tag,owner);
          if(std::filesystem::is_regular_file(path))return std::filesystem::path{};
          record_frame(actual,path,observed_ui_clock);return path;
        };
        check(run_frontend(options).has_value() && party_menu->launch,"Rendered shared lobby did not reach the actual native match handoff");
        if(capture_audio)std::ofstream(output/(owner+"-ui-audio-complete"))<<"Actual rendered frontend returned its matched launch\n";
        for(const auto &tag:{"idle","start","cancel","late","shorten","member-request","tick-3","tick-2","tick-1","zero-search"})
          check(std::filesystem::is_regular_file(capture_path(tag,owner)),"Rendered shared countdown omitted an actual UI phase");
        for(const auto stage:{3,2,1})check(std::any_of(visible_frames["sounds"].begin(),visible_frames["sounds"].end(),[&](const auto &r){
          return r.at("cue")==unsigned(FrontendUiSound::PregameCountdown) && r.at("ui_countdown")==stage;
        }),"Actual rendered countdown did not request original UI sound10 at stage3/2/1");
        check(std::count_if(visible_frames["sounds"].begin(),visible_frames["sounds"].end(),[](const auto &r){
          return r.at("cue")==unsigned(FrontendUiSound::MatchmakingAdvance) && r.at("screen")==210 && r.at("progress_pane")==3;
        })==1,"Actual matched pane3 did not request original UI sound12 exactly once");
        std::cout<<"PASS actual rendered party lobby: shared Start/X cancel/restart, delayed observer, repeated A, member mode1 request, stages3/2/1/shared zero -> native match\n";
      }
      std::cout<<"Native party "<<(leader?"leader":"member")<<" chose "<<playlist<<" through playlist/pregame/search owners"<<std::endl;
    } else {
      std::vector<OnlineLocalPlayer> guests;
      if(mode=="source-render")for(unsigned n=1;n<4;++n)guests.push_back({"Guest "+owner+" "+std::to_string(n),{}});
      queue.refresh("Same Pilot",std::move(guests));wait([&]{return queue.snapshot().status==OnlineMatchmakingStatus::Ready;});
      const auto catalog=queue.snapshot().playlists;
      const auto row=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return p.id==playlist;});
      check(row!=catalog.end(),"Requested playlist absent");
      check(row->ranked || !row->level,"Casual playlist exposed hidden skill to native client");
      queue.join(playlist);
    }
  };
  // Both party-A and matched-B countdowns use real service publications.
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
  if(!group)queue.sign_out(); // A real accepted party keeps its API session through game B/PCR.
  check(session->roster().size()==participants,"Match did not retain every authenticated player");
  for(const auto &admitted:session->roster())
    check(admitted.matchmaking_level.has_value()==(first_playlist!="casual" && !admitted.guest),
      "Authenticated known ranked/unranked admission lost or fabricated its public hopper level");
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
    auto frontend=group?party_frontend:std::make_shared<FrontendSession>();frontend->discord_login=login;
    if(!group) {
      const Cache menu(maps/"mainmenu.map");frontend->retain_content(menu.load_frontend());
      frontend->content().vista=load_vista_settings(menu_settings);
      auto &initial=frontend->create_state();initial.accept();initial.accept();initial.text("Same Pilot");initial.commit_text();initial.back();
    }
    auto &state=frontend->state();
    state.set_discord_login(login);
    const bool retained_requeue=mode!="source-render";
    std::uint64_t party_version_at_report{};
    std::uint64_t account_revision_before_done{};
    auto pump_retained=[&] {
      state.update_online_login();state.update_online_matchmaking();
      if(queue.snapshot().status==OnlineMatchmakingStatus::Failed)throw FormatError(queue.snapshot().message);
    };
    if(retained_requeue) {
      if(!account) {
        OnlineAccountOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;
        options.enable_parties=true;options.poll_interval=1500ms;
        account=std::make_shared<OnlineAccount>(login,options);account->connect("Same Pilot");
        wait_stage="retained account authentication";
        wait([&]{return account->snapshot().authenticated && !account->snapshot().busy;});
      }
      state.set_online_account(account);state.set_online_matchmaking(queue_owner);
      frontend->online_services=std::make_shared<NativeTu15OnlineServices>();
      frontend->online_services->login=login;frontend->online_services->account=account;
      frontend->online_services->matchmaking=queue_owner;
      if(!group) {
      state.return_online();state.open(219);
      wait_stage="retained completed-match hopper catalog";
      wait([&]{pump_retained();return queue.snapshot().status==OnlineMatchmakingStatus::Ready;});
      const auto catalog=queue.snapshot().playlists;
      const auto hopper=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return p.id==first_playlist;});
      check(hopper!=catalog.end() && hopper->eligible,"Completed match hopper is unavailable in its actual authenticated catalog");
      state.select(unsigned(hopper-catalog.begin()));state.accept();
      check(state.frame().screen==14,"Actual Optimatch selection did not return directly to the lobby");
      check(state.frame().screen==14 && queue.snapshot().status==OnlineMatchmakingStatus::Ready && !state.launch,
        "Priming the real retained hopper started a queue before lobby Start");
      }
      wait_stage="retained party hopper consent";
      wait([&]{pump_retained();const auto s=account->snapshot();return s.party && !s.busy &&
        std::all_of(s.party->members.begin(),s.party->members.end(),[&](const auto &m){return m.ready_playlist==first_playlist;});});
      party_version_at_report=account->snapshot().party->version;
      // Every process has selected its real hopper without a ticket. The
      // same retained state will now own the actual completed publication.
      barrier("retained-hopper-before-report");
    }
    bool done{};auto report=online_postgame_screen(*frontend,session,[&]{
      done=true;
      if(retained_requeue)account_revision_before_done=account->snapshot().revision;
      state.return_online_lobby();
    });
    check(report->online,"ONLINE postgame lost its Vista asset provenance");
    check(report->results->player_count==participants && report->results->variant==2,"Vista report omitted a real participant");
    for(unsigned row=0;row<report->results->player_count;++row) {
      const auto &display=report->results->players[row];
      const auto admitted=std::find_if(session->roster().begin(),session->roster().end(),[&](const auto &p){
        return std::equal(p.identity.begin(),p.identity.end(),display.begin()+0x100);
      });
      check(admitted!=session->roster().end(),"Ranked PCR lost its actual admitted identity");
      std::uint32_t rank{};for(unsigned i=0;i<4;++i)rank|=std::to_integer<unsigned>(display[0x78+i])<<(8*i);
      check(rank==(admitted->matchmaking_level?*admitted->matchmaking_level-1:0xffffffffu),
        "Actual admitted ranked hopper level/unranked absence did not reach PCR");
    }
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
      const auto retained_services=frontend->online_services;
      FrontendOptions options;options.session=frontend;options.entry=FrontendOptions::Entry::postgame_report;options.postgame_report=report;
      options.maps=maps;options.ui_assets=mcc_root.empty()?vista:std::filesystem::path{};
      options.vista_settings=menu_settings;options.width=1280;options.height=720;
      options.skip_intro=options.hidden=options.mute=true;options.screenshot=output/(owner+"-postgame.ppm");
      // The real screen open starts the original clock. Sleeping before
      // run_frontend would merely age the preallocated report invisibly.
      options.frames=0;
      const auto pane_count=team?6u:5u;std::set<unsigned> report_panes;
      std::uint32_t observed_ui_clock{};const auto pcr_started=std::chrono::steady_clock::now();
      const auto pane_path=[&](unsigned pane){return output/(owner+"-pcr-pane-"+std::to_string(pane)+".ppm");};
      const auto intro_path=[&](const std::string &phase){return output/(owner+"-pcr-intro-"+phase+".ppm");};
      options.frame_capture=[&](const MenuState &actual) {
        if(!actual.frame().vista_pcr)return std::filesystem::path{};
        const auto color=actual.pcr_intro_color(actual.frame());
        check(color.has_value(),"Actual PCR16 unexpectedly selected prior-stats intro bypass");
        const auto alpha=*color>>24;
        const auto map_color=actual.ui_map_intro_color();
        const auto map_alpha=map_color?*map_color>>24:0u;
        const std::string phase=alpha==255?"black":alpha>32 && alpha<224 && map_alpha<224?"fade":
            alpha==0 && !map_color?"clear":"";
        if(!phase.empty() && !std::filesystem::is_regular_file(intro_path(phase))) {
          const auto path=intro_path(phase);record_frame(actual,path,observed_ui_clock);return path;
        }
        if(alpha || map_color || std::filesystem::is_regular_file(pane_path(actual.frame().context)))return std::filesystem::path{};
        const auto path=pane_path(actual.frame().context);record_frame(actual,path,observed_ui_clock);return path;
      };
      options.session_update=[&](MenuState &actual,std::uint32_t clock,const auto &) {
        observed_ui_clock=clock;
        if(std::chrono::steady_clock::now()-pcr_started>30s)throw FormatError("Rendered actual PCR tab traversal timed out");
        if(!actual.frame().vista_pcr)return false;
        const auto pane=actual.frame().context;
        if(std::filesystem::is_regular_file(pane_path(pane)) && report_panes.insert(pane).second) {
          if(report_panes.size()==pane_count)return true;
          SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_RIGHT;
          check(SDL_PushEvent(&event)==1,"Rendered PCR rejected its real horizontal SDL input");
        }
        return false;
      };
      run_frontend(options);
      for(const auto *phase:{"black","fade","clear"})check(std::filesystem::is_regular_file(intro_path(phase)),
        "Actual native PCR open did not render its source black/partial/clear transition");
      check(report_panes.size()==pane_count,"Actual completed report did not render every admitted source tab");
      check(frontend->discord_login==login && login->authorization().has_value(),"World/menu return discarded Discord identity");
      if(retained_requeue)check(frontend->online_services==retained_services &&
        retained_services->account==account && retained_services->matchmaking==queue_owner &&
        account->snapshot().authenticated && state.online_identity(),
        "Rendered report replaced or signed out its retained real account/queue services");
      // A party follower's Done (40C01A host false): dialog98 over the report.
      bool asked{};options.screenshot=output/(owner+"-postgame-leave-party.ppm");
      options.frames=0;const auto question_started=std::chrono::steady_clock::now();
      options.frame_capture=[&](const MenuState &menu) {
        if(!asked || menu.ui_map_intro_color() || std::filesystem::is_regular_file(options.screenshot))
          return std::filesystem::path{};
        record_frame(menu,options.screenshot,observed_ui_clock);return options.screenshot;
      };
      options.session_update=[&](MenuState &menu,std::uint32_t clock,const auto &) {
        observed_ui_clock=clock;
        if(std::chrono::steady_clock::now()-question_started>30s)
          throw FormatError("Rendered PCR follower question did not become visible after both source intros");
        if(!asked && menu.frame().vista_pcr && !menu.ui_map_intro_color() &&
           (*menu.pcr_intro_color(menu.frame())>>24)==0) {
          menu.vista_pcr_dialog({3,0x98,4,1,0x40c00d,0,0,0});asked=true;
        }
        return asked && std::filesystem::is_regular_file(options.screenshot);
      };
      run_frontend(options);
      if(retained_requeue)check(frontend->online_services==retained_services &&
        retained_services->account==account && retained_services->matchmaking==queue_owner &&
        account->snapshot().authenticated && state.online_identity(),
        "Second rendered report invocation replaced or signed out its retained real ONLINE services");
      check(asked && state.frame().entries.size()==2 && state.frame().entries[0].destination==-18,"Rendered report did not ask with dialog98");
      state.back();
    } else frontend->enter(FrontendOptions::Entry::postgame_report,report);
    // Real horizontal input must reach all six source tabs (five in FFA),
    // including Hit Stats. Direct pane setters would miss a disconnected
    // input route while still producing correct isolated painter captures.
    const auto first_pane=state.frame().context;
    std::vector<unsigned> visited;
    for(unsigned i=0;i<(team?6u:5u);++i) {
      visited.push_back(state.frame().context);state.navigate(1,0);
    }
    check(state.frame().context==first_pane && std::find(visited.begin(),visited.end(),5)!=visited.end() &&
      (std::find(visited.begin(),visited.end(),0)!=visited.end())==team,"PCR real input omitted a tab or exposed FFA team stats");
    // A party member who is not the leader (40C01A host false) asks with
    // dialog98 instead; its Cancel keeps the report.
    state.vista_pcr_dialog({3,0x98,4,1,0x40c00d,0,0,0});
    check(state.frame().title==frontend->content().dialogs.at(0x98).title && state.frame().entries.size()==2 &&
          state.frame().entries[0].destination==-18,"PCR follower Done did not ask with dialog98");
    state.back();check(state.frame().vista_pcr && !done,"Dialog98 Cancel left the report");
    if(retained_requeue && group) {
      // Poll the genuine match-room presence before the leader leaves; a
      // follower cannot manufacture the leader's session permission.
      wait_stage="party postgame presence";
      wait([&]{state.update_online_login();const auto s=account->snapshot();return s.party &&
        std::all_of(s.party->members.begin(),s.party->members.end(),[](const auto &m){return m.postgame==true;});});
      barrier("party-report-presence-observed");
      if(!leader) {
        state.back();
        check(!state.frame().vista_pcr && state.frame().entries.size()==2 &&
          state.frame().entries[0].destination==-18 && !done,"A real party follower's Done did not ask with dialog98");
        state.back();check(state.frame().vista_pcr && !done,"A real follower's dialog98 Cancel left its report");
      }
      barrier("party-report-done-permissions");
      if(leader)state.back();
      else {
        wait_stage="party follower observes leader Done";
        wait([&]{state.update_online_login();return done;});
      }
    } else state.back();
    check(done && !state.frame().vista_pcr,"Original PCR Done did not allocate fresh ONLINE navigation");
    check(state.frame().screen==14 && state.has_online_prior_stats(),"PCR Done did not retain prior stats in the returned pregame lobby");
    const auto lobby_instance=state.frame().instance;
    const auto results=report->results;
    report.reset();session.reset();
    check(state.review_online_postgame() && state.frame().screen==vista_menu(185) &&
      state.frame().vista_pcr->results==results && !state.frame().vista_pcr->voice_update &&
      !state.frame().vista_pcr->input.continue_session && !state.frame().vista_pcr->leave_session,
      "Prior stats did not reuse the real completed publication without retaining match transport callbacks");
    const FrontendPointerTarget old_done{FrontendPointerAction::Back,0,state.revision};
    state.navigate(-1,0);
    check(state.frame().context==5,"Prior stats did not wrap to the real hit statistics pane");
    state.back();
    check(state.frame().screen==14 && state.frame().instance==lobby_instance && state.has_online_prior_stats() &&
      !state.activate_pointer(old_done),"Prior stats Done discarded or replaced the lobby, or accepted stale report input");
    check(state.review_online_postgame() && state.frame().vista_pcr->results==results,"Reviewing prior stats consumed the publication");
    state.back();
    if(retained_requeue) {
      wait_stage="returned lobby retained hopper";
      // Require fresh server observations after the real Done callback,
      // rather than accepting cached pre-report consent or party version.
      wait([&]{pump_retained();const auto s=account->snapshot();return queue.snapshot().status==OnlineMatchmakingStatus::Ready &&
        s.party && !s.busy && s.revision>=account_revision_before_done+2;});
      const auto returned_party=account->snapshot().party;
      check(returned_party->version==party_version_at_report &&
        std::all_of(returned_party->members.begin(),returned_party->members.end(),[&](const auto &m){return m.ready_playlist==first_playlist;}),
        "PCR Done/prior stats changed the actual party version or discarded member hopper consent");
      barrier("prior-stats-returned-lobby");
      const auto &entries=state.frame().entries;
      const auto start=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
      check(start!=entries.end(),"The completed match's retained lobby lost Start Game");
      if(account->snapshot().party->leader) {
        state.select(unsigned(start-entries.begin()));state.accept();
        wait_stage="shared post-report countdown publication";
        wait([&]{pump_retained();return !account->snapshot().busy && state.online_lobby_countdown()>0;});
        check(state.online_lobby_countdown()<=10 && !state.launch,"Post-report Start skipped its ten-second native countdown");
        const auto setup=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==23;});
        check(setup!=entries.end(),"Retained lobby lost its Game Setup focus target");
        const auto before=state.online_lobby_countdown();state.select(unsigned(setup-entries.begin()));state.accept();
        wait_stage="shared repeated Start publication";
        wait([&]{pump_retained();return !account->snapshot().busy && state.online_lobby_countdown()<before;});
        check(state.frame().screen==14 && state.online_lobby_countdown()<=before-1,
          "Active countdown A followed Game Setup focus instead of shortening the native countdown");
        while(state.online_lobby_countdown()>3) {
          const auto previous=state.online_lobby_countdown();state.accept();
          wait([&]{pump_retained();return !account->snapshot().busy && state.online_lobby_countdown()<previous;});
        }
        check(state.frame().screen==14 && state.online_lobby_countdown()>0 && state.online_lobby_countdown()<=3,
          "Active countdown A did not preserve the original three-second shortening minimum");
        const auto serial=account->snapshot().party->countdown->serial;state.accept();
        wait([&]{pump_retained();return !account->snapshot().busy;});
        check(account->snapshot().party->countdown->serial==serial,
          "Pressing A at the minimum restarted the shared countdown clock");
      }
      wait_stage="same retained frontend queues next completed-match hopper";
      wait([&]{pump_retained();if(!state.launch)return false;session=state.launch->online_match;state.launch.reset();return bool(session);});
      check(queue.snapshot().playlist_id==first_playlist,"Post-report Start changed the retained hopper");
    } else {
      search(clan?first_playlist:"casual");
      wait([&]{session=take_session();return bool(session);});
    }
    check(session->roster().size()==participants && session->roster()[0].identity!=old_identity,"Requeue reused old room identity or lost roster");
    if(retained_requeue && !group) {
      // A transferred queue pauses on its old Loading snapshot. A real
      // abandon/disconnect return must reset that intent without requiring
      // a renderer-owned signout, then permit Start on the retained hopper.
      barrier("post-transfer-before-abandon");
      check(queue.snapshot().status==OnlineMatchmakingStatus::Loading,"Fixture did not reach the actual transferred queue boundary");
      const auto abandoned_identity=session->roster()[0].identity;
      frontend->enter(FrontendOptions::Entry::online_return);
      session.reset();
      check(state.frame().screen==14 && !state.launch,"Abandoned transfer did not return to the retained pregame lobby");
      wait_stage="abandoned transferred queue returns to Ready";
      wait([&]{pump_retained();return queue.snapshot().status==OnlineMatchmakingStatus::Ready;});
      const auto &entries=state.frame().entries;
      const auto start=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
      check(start!=entries.end(),"Abandon return discarded the retained hopper's Start action");
      state.select(unsigned(start-entries.begin()));state.accept();
      wait_stage="shared abandon-return countdown publication";
      wait([&]{pump_retained();return state.online_lobby_countdown()>0;});
      check(state.online_lobby_countdown()<=10,"Abandon return could not start the next native pregame countdown");
      wait_stage="abandon return starts a fresh queue intent";
      wait([&]{pump_retained();const auto s=queue.snapshot();return s.playlist_id==first_playlist &&
        (s.status==OnlineMatchmakingStatus::Searching || s.status==OnlineMatchmakingStatus::CheckingConnections ||
         s.status==OnlineMatchmakingStatus::AwaitingTransport || s.status==OnlineMatchmakingStatus::Loading);});
      wait_stage="fresh match after abandoned transferred queue";
      wait([&]{pump_retained();if(!state.launch)return false;session=state.launch->online_match;state.launch.reset();return bool(session);});
      check(session->roster().size()==participants && session->roster()[0].identity!=abandoned_identity,
        "Abandon return reused the transferred room or lost the actual admitted roster");
      barrier("abandoned-transfer-started-again");
      std::cout<<"PASS abandoned transferred queue -> Ready -> retained lobby Start -> fresh authenticated matched handoff\n";
    }
    queue.sign_out();
    std::cout<<(retained_requeue?
      "PASS completed match -> actual Vista PCR16 -> Done -> prior stats -> same lobby Start -> retained hopper -> fresh matched room\n":
      "PASS eight-seat completed publication -> actual Vista PCR16 -> Done -> prior stats -> fresh matched room\n");
  };
  if(mode=="source-render") {
    if(!std::getenv("DISPLAY"))return 77;
    MultiplayerOptions runtime;runtime.map=maps/"lockout.map";runtime.shared_maps=maps;runtime.online_match=session;
    runtime.gameplay_xbe="/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe";
    runtime.ui_assets=runtime_ui;runtime.width=1280;runtime.height=720;runtime.hidden=true;runtime.mute=true;runtime.replay=true;
    runtime.source_object_world=true; // Replay controls must not select the fallback world.
    bool native_world{};runtime.source_world_observer=[&](const auto &){native_world=true;};
    runtime.local_players=4;runtime.variant={"Slayer",0,{}};runtime.variant.values.set(10,100);runtime.variant.values.set(2,0);
    std::array<std::uint8_t,9> appearance{};
    for(auto slot:session->local_slots())runtime.selected_profiles.push_back(native_selected_player_configuration(session->roster()[slot].name,appearance,-1));
    runtime.screenshot=output/(owner+"-source-online.ppm");
    const bool retention_test=std::getenv("HALO2_RUNTIME_RETENTION");
    const bool stalled_host_test=retention_test&&std::getenv("HALO2_RUNTIME_STALLED_HOST");
    const bool source_host=session->self()==session->host();
    std::optional<std::chrono::steady_clock::time_point> started;unsigned ticks{},players{},frames{};bool quit{},stalled{};
    runtime.input_observer=[&](auto step,auto,const auto &state){ticks=step;
      if(!started)started=std::chrono::steady_clock::now();
      players=std::max(players,unsigned(std::count_if(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;})));
      if(stalled_host_test&&source_host&&!stalled&&std::chrono::steady_clock::now()-*started>25s){
        stalled=true;std::this_thread::sleep_for(12s); // Game loop stalls; authenticated peer/control workers stay alive.
      }
      if(!quit&&std::chrono::steady_clock::now()-*started>(retention_test&&!source_host?120s:45s)){
        SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"Source capture could not end its actual game loop");quit=true;}};
    runtime.frame_observer=[&](const auto &timing){frames+=timing.submitted;};
    MultiplayerRuntimeOutcome outcome;std::shared_ptr<const SourceAuthorityRetainedWorld> recovery;
    if(retention_test)run_multiplayer(runtime,outcome,recovery);
    else run_multiplayer(runtime,outcome);
    std::cout<<"Rendered source capture: ticks="<<ticks<<" players="<<players<<" frames="<<frames<<"\n";
    check(ticks>30&&players==8&&frames>10&&outcome.state.has_value(),"Rendered TU1.5 source session did not advance all couch players");
    check(session->self()!=session->host()||native_world,"Rendered host silently omitted its actual TU1.5 source owner");
    if(retention_test){
      if(source_host)check(outcome.reason==MultiplayerExitReason::window_quit&&!recovery,
          "An intentional host quit manufactured a recovery value");
      else {
        check(outcome.reason==MultiplayerExitReason::online_disconnected&&recovery&&
            outcome.authority==MultiplayerOutcomeAuthority::client_snapshot&&!outcome.authoritative_banks,
            "Real runtime disconnect discarded its source recovery or promoted display/PCR state");
        if(stalled_host_test)check(session->phase()==OnlineMatchSession::Phase::Running,
            "Stalled game-loop test did not preserve its still-live direct/control session");
        check(recovery->cut.context==*session->authority_context()&&recovery->admissions==*session->source_players()&&
            recovery->local_views==session->local_slots()&&recovery->checkpoint.match.simulation.ticks==recovery->cut.seal.tick,
            "Runtime recovery changed its admitted context, source identities, local views or completed tick");
        std::cout<<"Runtime recovery cut: validated_tick="<<recovery->cut.seal.tick
            <<" displayed_tick="<<outcome.state->tick<<" local_views="<<recovery->local_views.size()<<'\n';
        // Runtime renderer, source replay worker and native display world have
        // all been destroyed. Import only the owning recovery value on this
        // separate executor; no old pointer or fixture supplies world input.
        MultiplayerOptions continuation;continuation.online_match=runtime.online_match;
        continuation.map=runtime.map;continuation.shared_maps=runtime.shared_maps;continuation.gameplay_xbe=runtime.gameplay_xbe;
        continuation.bsp=runtime.bsp;continuation.variant=runtime.variant;continuation.starting_weapon=runtime.starting_weapon;
        continuation.session_random_seed=runtime.session_random_seed;continuation.local_players=runtime.local_players;
        continuation.selected_teams=runtime.selected_teams;continuation.selected_profiles=runtime.selected_profiles;
        auto recipient=MultiplayerPreparedWorld::source_shadow(continuation,MultiplayerPreparedWorld::SourceShadowViews::LocalPeer);
        auto staging=MultiplayerPreparedWorld::source_shadow(continuation,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost);
        SourceMatchAuthority target(recipient->match,*recipient->source_objects),producer(staging->match,*staging->source_objects);
        std::vector<SourcePlayerAdmission> locals;for(auto slot:recovery->local_views)locals.push_back(recovery->admissions.players.at(slot));
        auto prepared=target.prepare_with_local_views(recovery->checkpoint,source_authority_binding(recovery->cut.context),
            locals,producer,recovery->admissions.players,recovery->producer_views);
        check(target.commit(std::move(prepared)),"Real rendered runtime recovery did not import/rebind");
        for(const auto &p:session->roster())if(online_admission_peer(p)==session->host()){
          const auto deaths=recipient->match.player(p.slot).deaths;recipient->match.depart(p.slot);
          check(recipient->match.player(p.slot).deaths==deaths,"Runtime recovery departure manufactured deaths");
        }
        const auto tick=recipient->match.simulation().ticks();std::array<Controls,16> inputs{};
        for(unsigned i=0;i<10;++i){for(unsigned slot=0;slot<8;++slot)inputs[slot].yaw=recipient->match.unit(slot).yaw;recipient->match.step(inputs);}
        check(recipient->match.simulation().ticks()==tick+10&&!recipient->match.finished(),
            "Actual rendered recovery could not continue the native simulation");
        std::cout<<"PASS real rendered runtime disconnect handoff survives teardown, imports/rebinds four local views, retains original departure results and continues ten native ticks\n";
      }
    }
    session.reset();std::cout<<"PASS actual rendered TU1.5 ONLINE source capture workload with eight admitted couch seats\n";return 0;
  }
  if(mode=="render" || mode=="team-render" || departure_only) {
    if(!std::getenv("DISPLAY"))return 77;
    std::filesystem::create_directories(output);
    MultiplayerOptions runtime;runtime.map=maps/"lockout.map";runtime.shared_maps=maps;runtime.online_match=session;
    runtime.gameplay_xbe=root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
    runtime.ui_assets=runtime_ui;
    runtime.width=1280;runtime.height=720;runtime.hidden=true;runtime.mute=true;runtime.replay=true;
    runtime.variant.name=team?"Team Slayer":"Slayer";
    runtime.variant.values.set(10,8);runtime.variant.values.set(2,0);runtime.variant.values.set(91,int(team));
    std::array<std::uint8_t,9> appearance{};runtime.selected_profiles.push_back(native_selected_player_configuration("Same Pilot",appearance,team?session->roster()[session->self()].team:-1));
    if(team)runtime.selected_teams.push_back(std::int8_t(session->roster()[session->self()].team));
    runtime.screenshot=output/(owner+"-online.ppm");
    unsigned ticks{},players{};bool fire{};
    runtime.input_observer=[&](auto step,auto controls,const auto &state){ticks=step;fire|=controls[0].fire;players=std::max(players,unsigned(std::count_if(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;})));};
    if(!departure_only) {
      MultiplayerRuntimeOutcome outcome;run_multiplayer(runtime,outcome);
      check(ticks>10 && players==2 && fire && outcome.state.has_value(),"Rendered matched gameplay never advanced real players and controls");
      check(outcome.reason==MultiplayerExitReason::online_postgame && outcome.state->game_over_complete(),"Runtime did not hand off actual completed results");
      complete_and_requeue(true);
    }
    runtime.online_match=session;runtime.frames=0;runtime.screenshot=output/(owner+"-requeued.ppm");runtime.variant={team?"Team Slayer":"Slayer",0,{}};runtime.variant.values.set(91,int(team));
    if(departure_only){runtime.variant.values.set(10,0);runtime.variant.values.set(2,0);}
    runtime.desktop=std::make_shared<DesktopSession>();
    const bool quit_host=!std::getenv("HALO2_TEST_QUIT_NONHOST");
    bool quitting_player{};
    unsigned leave_stage{};runtime.input_observer=[&](auto step,auto controls,const auto& state){
      ticks=step;fire|=controls[0].fire;
      players=std::max(players,unsigned(std::count_if(state.players.begin(),state.players.end(),[](const auto& p){return p.connected;})));
      if((session->self()==session->host())!=quit_host)return;
      quitting_player=true;
      const auto key=[&](SDL_Keycode value){SDL_Event event{};event.type=SDL_KEYDOWN;
        event.key.windowID=SDL_GetWindowID(runtime.desktop->window(runtime.width,runtime.height,true));
        event.key.keysym.sym=value;check(SDL_PushEvent(&event)==1,"Could not deliver native pause/leave input");};
      if(step>=60 && leave_stage==0){key(SDLK_ESCAPE);++leave_stage;}
      else if(step>=75 && leave_stage==1){key(SDLK_DOWN);key(SDLK_DOWN);key(SDLK_RETURN);++leave_stage;}
      else if(step>=90 && leave_stage==2){key(SDLK_UP);key(SDLK_RETURN);++leave_stage;}
    };
    bool departure_game_over_visible{};
    bool departure_capture_requested{};
    const auto departure_capture=output/(owner+"-departure-game-over.ppm");
    runtime.scoreboard_observer=[&](unsigned,bool normal,const FullScoreboardLayout& layout,const FullScoreboardPaintResult& paint){
      departure_game_over_visible|=!normal && layout.title.text==u"GAME OVER" && layout.input_alpha>.9f && !paint.draw.vertices.empty();
    };
    runtime.frame_capture=[&](const MultiplayerState& world){
      if(!departure_capture_requested && departure_game_over_visible && world.finished) {
        departure_capture_requested=true;return departure_capture;
      }
      return std::filesystem::path{};
    };
    MultiplayerRuntimeOutcome second;run_multiplayer(runtime,second);
    check(second.state && second.state->tick>10,"ONLINE departure match did not launch real gameplay");
    check(ticks>10 && players==2 && fire,"Departure case did not advance real players and controls");
    if(quitting_player)check(second.reason==MultiplayerExitReason::escape,"Confirmed local quit lost its actual exit reason");
    else {
      if(second.reason==MultiplayerExitReason::online_disconnected)
        check(run_multiplayer_departure_game_over(runtime,*second.state,session->departed_players())==MultiplayerDepartureReturn::completed,
            "Last opponent host departure skipped its GAME OVER presentation");
      else check(second.reason==MultiplayerExitReason::online_postgame && second.state->game_over_complete(),
          "Last opponent departure skipped normal match completion");
      check(departure_game_over_visible,"Last opponent departure never rendered GAME OVER");
      std::ifstream image(departure_capture,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
      image>>magic>>width>>height>>maximum;image.get();
      int drawable_width{},drawable_height{};
      SDL_Vulkan_GetDrawableSize(runtime.desktop->window(runtime.width,runtime.height,true),&drawable_width,&drawable_height);
      check(image && magic=="P6" && width==unsigned(drawable_width) && height==unsigned(drawable_height) && maximum==255,
          "Departure GAME OVER did not produce its actual native framebuffer");
      std::vector<unsigned char> pixels(std::size_t(width)*height*3);
      image.read(reinterpret_cast<char*>(pixels.data()),std::streamsize(pixels.size()));
      check(bool(image),"Departure GAME OVER framebuffer was truncated");
      const auto [dark,light]=std::minmax_element(pixels.begin(),pixels.end());
      check(*light>*dark+64,"Departure GAME OVER framebuffer omitted its visible scoreboard");
      session->close_postgame();
    }
    runtime.online_match.reset();session.reset();
    std::cout<<"PASS real ONLINE game -> Vista pause/confirmed Quit -> last opponent GAME OVER -> clean native return outcome\n";
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
  check(session->direct_udp()==(o.compatibility=="halo2-native-queue-v12"),"Match did not use the admitted transport");
  if(mode=="blocks"){
    // Exercise the actual native receiver before any codec worker starts.
    // A malicious host bypasses voice() and injects an attributed packet with
    // a bogus origin proof over its authenticated direct transport.
    barrier("voice-origin-probe-ready");
    if(hosting){
      std::vector<std::byte> forged;
      const auto append=[&](std::uint64_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)forged.push_back(std::byte(value>>(8*i)));};
      append(0x324e3248,4);append(native_multiplayer_protocol,2);append(6,1);append(0,1);
      append(other,1);append(1,4);append(0,1);append(std::bit_cast<std::uint32_t>(1.f),4);append(std::bit_cast<std::uint32_t>(1.f),4);
      append(0x42,1);forged.resize(forged.size()+64);
      session->send(forged,other);
    }
    std::this_thread::sleep_for(300ms);barrier("voice-origin-probe-delivered");
    check(session->voice_packets().empty(),"Native receiver accepted forged host microphone origin");
    check(session->phase()==OnlineMatchSession::Phase::Running,"Invalid host voice ended a healthy native session");
    std::cout<<"PASS native receiver rejects forged host voice origin over actual direct UDP\n";
  }
  if(mode=="nonhost-loss"){
    wait([&]{return session->authority_supported(other);});barrier("direct-peer-ready-before-departure");
    if(!hosting){session.reset();std::cout<<"PASS nonhost closed its real direct UDP and signaling session\n";return 0;}
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(std::chrono::steady_clock::now()<until){check(session->phase()==OnlineMatchSession::Phase::Running,
        "Nonhost direct link closure ended the host session");std::this_thread::sleep_for(10ms);}
    check(session->master_connected(),"Nonhost departure dropped the host signaling connection");
    check(!session->authority_supported(other),"Closed physical peer remained a live authority transfer target");
    std::cout<<"PASS native host session survives actual nonhost DTLS/signaling closure\n";return 0;
  }
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
  unsigned offline_ticks{},offline_audible{},offline_at{},offline_audio_at{};bool offline_seen{};
  std::vector<std::byte> authority_fixture;
  bool authority_published=false,authority_received=false;
  if(mode=="authority"){authority_fixture.resize(512*1024);for(unsigned i=0;i<authority_fixture.size();++i)authority_fixture[i]=std::byte((i*37+i/13)&255);}
  for(unsigned n=0;n<210;++n) {
    const auto now=std::chrono::steady_clock::now();
    if(mode=="authority"){
      if(hosting&&!authority_published&&n>5&&session->authority_supported(other)){
        const auto context=session->authority_context();check(bool(context),"Direct authority context absent");
        const std::array<unsigned,1> targets{other};session->publish_authority(*context,match.simulation().ticks(),authority_fixture,targets);authority_published=true;
      }
      if(!hosting)if(auto cut=session->received_authority()){
        check(*cut->bytes==authority_fixture,"Session authority bytes differ");authority_received=true;
        // This fixture is only a byte owner; it does not claim a decoded,
        // prepared world or invoke acknowledge_authority for these bytes.
      }
      check(session->validated_authority().empty(),"Byte fixture falsely acknowledged a prepared world");
    }
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
    if(std::getenv("HALO2_TEST_MASTER_OUTAGE") && !session->master_connected()) {
      if(!offline_seen){offline_seen=true;offline_at=tick;offline_audio_at=audio->audible;}
      offline_ticks=std::max(offline_ticks,tick-offline_at);offline_audible=std::max(offline_audible,audio->audible-offline_audio_at);
    }
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
  if(mode=="authority"){
    check(hosting?authority_published:authority_received,"Authority delivery did not complete beside native gameplay/voice");
    std::cout<<"PASS direct session authority byte delivery beside native gameplay and synthetic-device Opus voice; no unvalidated world acknowledgement\n";
  }
  if(std::getenv("HALO2_TEST_MASTER_OUTAGE")) {
    check(offline_seen && offline_ticks>30 && offline_audible>20,"Master outage stalled player-hosted simulation or direct voice");
    std::cout<<"PASS master signaling outage: direct gameplay advanced "<<offline_ticks<<" ticks and voice delivered "<<offline_audible<<" audible frames\n";
  }
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
  complete_and_requeue((mode=="team" || mode=="party") && std::getenv("DISPLAY"));
  check(!voice.snapshot().enabled,"Done left the departed session microphone enabled");
  session->loaded(fingerprint,variant);wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;});
  barrier("fresh-room-running-before-host-loss");
  // QoS chooses the new host independently of fixture account order. Closing
  // a nonhost now deliberately preserves the host's running session.
  if(session->self()==session->host())session.reset();
  else {
    wait([&]{return session->phase()==OnlineMatchSession::Phase::Failed;});
    MultiplayerOptions aborted;aborted.map=maps/"lockout.map";aborted.shared_maps=maps;aborted.online_match=session;
    aborted.selected_profiles.push_back(profile);
    MultiplayerRuntimeOutcome outcome;run_multiplayer(aborted,outcome);
    check(outcome.reason==MultiplayerExitReason::online_disconnected,"Pre-world disconnect escaped clean handoff ownership");
    check(!outcome.state || !outcome.state->finished,"Pre-world disconnect fabricated a completed match");
    std::cout<<"PASS disconnect before native world handoff -> clean return without postgame fabrication\n";
  }
  if(mode=="party") {
    session.reset();barrier("before-real-lobby-back");
    auto &state=*party_menu;
    auto pump=[&]{state.update_online_login();state.update_online_matchmaking();};
    const auto asked=[&](unsigned id){const auto &d=party_frontend->content().dialogs.at(id);return state.frame().screen==d.screen && state.frame().title==d.title;};
    const auto choose_retained_hopper=[&] {
      state.return_online_lobby();state.open(219);
      wait_stage="B regression real retained catalog";
      wait([&]{pump();return queue.snapshot().status==OnlineMatchmakingStatus::Ready && !state.frame().entries.empty();});
      const auto catalog=queue.snapshot().playlists;
      const auto selected=std::find_if(catalog.begin(),catalog.end(),[&](const auto &p){return p.id==first_playlist;});
      check(selected!=catalog.end(),"B regression lost the actual retained hopper");
      const auto &entries=state.frame().entries;
      const auto row=std::find_if(entries.begin(),entries.end(),[&](const auto &e){return e.label==selected->name;});
      check(row!=entries.end(),"B regression catalog is not selectable");state.select(unsigned(row-entries.begin()));state.accept();
      check(state.frame().screen==14,"B regression selected hopper did not return directly to the lobby");
      wait_stage="B regression own acknowledged consent";
      wait([&]{pump();const auto s=account->snapshot();return s.party && !s.busy &&
        std::any_of(s.party->members.begin(),s.party->members.end(),[&](const auto &m){return m.self && m.ready_playlist==first_playlist;});});
    };
    const auto start=[&]{const auto &entries=state.frame().entries;const auto row=std::find_if(entries.begin(),entries.end(),[](const auto &e){return e.destination==-17;});
      check(row!=entries.end(),"B regression lobby lost Start");state.select(unsigned(row-entries.begin()));state.accept();};
    choose_retained_hopper();barrier("before-real-lobby-back-consent");
    if(std::getenv("DISPLAY")) {
      FrontendOptions options;options.session=party_frontend;options.entry=FrontendOptions::Entry::online_return;
      options.maps=maps;options.vista_settings=menu_settings;options.skip_intro=options.hidden=options.mute=true;
      options.width=960;options.height=720;
      unsigned phase{};std::uint32_t ui_clock{};std::string tag="ready";
      auto changed=std::chrono::steady_clock::now();const auto began=changed;
      const auto version=account->snapshot().party->version;
      const auto path=[&](const std::string &label,const std::string &who){return output/(who+"-party-back-"+label+".ppm");};
      const auto all=[&](const std::string &label){return std::all_of(owners.begin(),owners.begin()+participants,
        [&](const auto &who){return std::filesystem::is_regular_file(path(label,who));});};
      const auto captured=[&]{return std::filesystem::is_regular_file(path(tag,owner));};
      const auto show=[&](std::string next){if(tag!=next){tag=std::move(next);changed=std::chrono::steady_clock::now();}};
      const auto key=[&](SDL_Keycode value){SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=value;
        check(SDL_PushEvent(&event)==1,"Party B flow rejected actual SDL key input");event.type=SDL_KEYUP;
        check(SDL_PushEvent(&event)==1,"Party B flow rejected actual SDL key release");
        visible_frames["inputs"].push_back({{"phase","party-back-"+tag},{"sdl_key",value},{"screen",state.frame().screen},{"ui_clock",ui_clock}});};
      const auto button=[&](const MenuState &actual,int destination){const auto &rows=actual.frame().entries;
        const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto&e){return e.destination==destination;});
        check(found!=rows.end(),"Rendered B flow lost an original lobby button");const auto target=unsigned(found-rows.begin());
        check(target<2,"Rendered B flow selected a non-button row");if(actual.frame().selected!=target)key(target?SDLK_RIGHT:SDLK_LEFT);key(SDLK_RETURN);};
      options.frame_capture=[&](const MenuState &actual)->std::filesystem::path {
        if(actual.ui_map_intro_color() || captured() || std::chrono::steady_clock::now()-changed<1000ms)return {};
        const auto target=path(tag,owner);record_frame(actual,target,ui_clock);return target;
      };
      options.session_update=[&](MenuState &actual,std::uint32_t clock,const auto&) {
        ui_clock=clock;
        const auto s=account->snapshot();const auto &f=actual.frame();const auto left=actual.online_lobby_countdown();
        const auto lobby=actual.lobby_view();const auto q=queue.snapshot();
        const auto diagnostic=[&] {
          nlohmann::json value={{"phase",phase},{"screen",f.screen},{"description",f.description},
            {"queue_status",unsigned(q.status)},{"queue_message",q.message},{"queue_playlist",q.playlist_id},
            {"account_busy",s.busy},{"account_message",s.party_message},{"selected_hopper",actual.expand("\xee\x90\xba")},
            {"source_start_permission",lobby && lobby->input.start_permission},{"countdown",left}};
          if(s.party) {
            value["party_version"]=s.party->version;value["leader"]=s.party->leader;value["members"]=nlohmann::json::array();
            for(const auto&m:s.party->members)value["members"].push_back({{"self",m.self},{"online",m.online},{"ready_playlist",m.ready_playlist}});
          }
          if(const auto &bound=actual.online_party_observation();bound) {
            value["bound_version"]=bound->version;value["bound_ready"]=nlohmann::json::array();
            for(const auto&m:bound->members)value["bound_ready"].push_back(m.ready_playlist);
          }
          return value.dump();
        };
        if(f.screen==8)throw FormatError("Rendered party B/Start actual refusal: "+diagnostic());
        if(std::chrono::steady_clock::now()-began>150s)throw FormatError("Rendered party B/reinvite timed out: "+diagnostic());
        check(!actual.launch,"Rendered B/reinvite flow queued before a new explicit Start");
        // The rendered source button consumes the menu-bound acknowledged
        // readiness, not another process's file barrier. Social snapshots can
        // lag the peer's already completed ready command by one polling turn.
        if(phase==0 && !actual.ui_map_intro_color() && all("ready") && s.party && !s.busy && (!leader || (lobby && lobby->input.start_permission))) {if(leader)button(actual,-17);phase=1;}
        else if(phase==1 && left>0 && !s.busy){show("start");phase=2;}
        else if(phase==2 && all("start") && !s.busy){if(leader)key(SDLK_ESCAPE);phase=3;}
        else if(phase==3 && f.screen==14 && left<0 && !s.busy){show("cancelled");phase=4;}
        else if(phase==4 && all("cancelled") && !s.busy){if(leader)button(actual,-17);phase=5;}
        else if(phase==5 && left>0 && !s.busy){show("restart");phase=6;}
        else if(phase==6 && all("restart") && !s.busy){if(!leader)key(SDLK_ESCAPE);phase=7;}
        else if(phase==7 && !s.busy && s.party && s.party->countdown && s.party->countdown->mode==1 &&
                (leader?f.screen==14:asked(0x8b))){show("first-question");phase=8;}
        else if(phase==8 && all("first-question") && !s.busy){if(!leader)key(SDLK_ESCAPE);phase=9;}
        else if(phase==9 && f.screen==14 && s.party && s.party->version==version && !s.busy){show("kept-membership");phase=10;}
        else if(phase==10 && all("kept-membership") && !s.busy){if(!leader)key(SDLK_ESCAPE);phase=11;}
        else if(phase==11 && !s.busy && (leader?f.screen==14:asked(0x8b))){show("second-question");phase=12;}
        else if(phase==12 && all("second-question") && !s.busy){if(!leader){if(f.selected)key(SDLK_UP);key(SDLK_RETURN);}phase=13;}
        else if(phase==13 && !s.busy && (leader?(s.party && s.party->members.size()==1 && asked(0xae)):(!s.party && f.screen==6))){show("left");phase=14;}
        else if(phase==14 && all("left") && !s.busy){if(leader)key(SDLK_RETURN);phase=15;}
        else if(phase==15 && !s.busy && (!leader || (f.screen==14 && left<0))) {
          if(leader){check(account->party_command({"invite",owners[party_begin+1]}),"Rendered B flow fresh native invitation was refused");
            visible_frames["service_commands"].push_back({{"phase","fresh-reinvite"},{"action","invite"},{"recipient_fixture_owner",owners[party_begin+1]}});}
          phase=16;
        } else if(phase==16 && !s.busy && (leader || actual.invite_notice().has_value())){show("fresh-invite");phase=17;}
        else if(phase==17 && all("fresh-invite") && !s.busy){if(!leader)key(SDLK_F9);phase=18;}
        else if(phase==18 && s.party && s.party->members.size()==2 && !s.busy && f.screen==14) {
          check(std::all_of(s.party->members.begin(),s.party->members.end(),[](const auto&m){return m.ready_playlist.empty();}),
            "Actual fresh invitation acceptance fabricated retained playlist consent");
          show("rejoined");phase=19;
        } else if(phase==19 && all("rejoined") && !s.busy){button(actual,23);phase=20;}
        else if(phase==20 && f.screen==23){show("setup");if(captured()){check(f.entries.size()==3 && f.entries[0].value==9,"Fresh consent lost source enum6 Setup rows");if(f.selected)for(unsigned i=0;i<f.selected;++i)key(SDLK_UP);key(SDLK_RETURN);phase=21;}}
        else if(phase==21 && f.screen==237 && !f.entries.empty()) {
          show("playlist");if(captured()) {const auto catalog=queue.snapshot().playlists;
            const auto chosen=std::find_if(catalog.begin(),catalog.end(),[&](const auto&p){return p.id==first_playlist;});
            check(chosen!=catalog.end(),"Rendered fresh consent lost the authenticated retained hopper");
            const auto row=std::find_if(f.entries.begin(),f.entries.end(),[&](const auto&e){return e.label==chosen->name;});
            check(row!=f.entries.end(),"Rendered original listing omitted the retained hopper");const auto index=unsigned(row-f.entries.begin());
            if(index>f.selected)for(unsigned i=f.selected;i<index;++i)key(SDLK_DOWN);else for(unsigned i=index;i<f.selected;++i)key(SDLK_UP);
            key(SDLK_RETURN);phase=22;
          }
        } else if(phase==22 && f.screen==14 && s.party && !s.busy && std::any_of(s.party->members.begin(),s.party->members.end(),
            [&](const auto&m){return m.self && m.ready_playlist==first_playlist;})){show("fresh-consent");phase=23;}
        else if(phase==23 && all("fresh-consent")) {
          check(queue.snapshot().status==OnlineMatchmakingStatus::Ready && left<0 && !actual.launch,
            "Fresh rendered rejoin autoqueued before a new explicit Start");return true;
        }
        return false;
      };
      check(!run_frontend(options).has_value() && phase==23,"Rendered B flow did not retain its actual unqueued lobby");
      for(const auto label:{"ready","start","cancelled","restart","first-question","kept-membership","second-question","left","fresh-invite","rejoined","setup","playlist","fresh-consent"})
        check(std::filesystem::is_regular_file(path(label,owner)),"Rendered party B/rejoin flow omitted an actual owner capture");
      std::cout<<"PASS actual four-owner SDL/Vulkan party B: leader cancel; member question/cancel/confirm; authoritative leave/peer AE; native invitation and SDL acceptance; original Setup23/list237 fresh consent without autoqueue\n";
    } else {
    wait_stage="B regression actual shared countdown";if(leader)start();
    wait([&]{pump();return state.online_lobby_countdown()>0 && !account->snapshot().busy;});
    if(leader){state.back();check(state.frame().screen==14,"Active leader B asked to leave instead of canceling source countdown");}
    wait([&]{pump();return state.online_lobby_countdown()<0 && !account->snapshot().busy;});
    barrier("real-lobby-back-cancelled");if(leader)start();
    wait([&]{pump();return state.online_lobby_countdown()>0 && !account->snapshot().busy;});
    const auto version=account->snapshot().party->version;
    if(!leader) {
      state.back();check(asked(0x8b),"Member B did not open original Leave confirmation");
      wait([&]{pump();const auto s=account->snapshot();return s.party && !s.busy && s.party->countdown && s.party->countdown->mode==1;});
      state.back();check(state.frame().screen==14 && account->snapshot().party->version==version,
        "Canceling original Leave confirmation changed accepted membership");
      state.back();check(asked(0x8b),"Member second B lost original Leave confirmation");
      wait([&]{pump();return !account->snapshot().busy;});state.select(0);state.accept();
      wait_stage="B confirmed authoritative party departure";
      wait([&]{pump();return !account->snapshot().party && !account->snapshot().busy && state.frame().screen==6;});
      check(!state.launch,"Confirmed B departure queued the counted-down hopper");
    } else {
      wait_stage="B peer departure original notice";
      wait([&]{pump();const auto s=account->snapshot();return s.party && s.party->members.size()==1 && asked(0xae);});
      state.accept();check(state.frame().screen==14 && state.online_lobby_countdown()<0 && !state.launch,
        "Actual member departure did not cancel the shared countdown and retain original pregame");
    }
    barrier("real-lobby-back-left");
    if(leader){check(account->party_command({"invite",owners[party_begin+1]}),"B regression fresh reinvitation refused");}
    else {
      wait_stage="B regression real fresh invitation";
      wait([&]{pump();return state.invite_notice().has_value() && !account->snapshot().busy;});
      check(state.act_invite_notice(true),"B regression fresh accepted invitation refused");
    }
    wait_stage="B regression rejoined accepted party";
    wait([&]{pump();const auto s=account->snapshot();return s.party && s.party->members.size()==2 && !s.busy;});
    const auto rejoined=account->snapshot().party;
    check(std::all_of(rejoined->members.begin(),rejoined->members.end(),[](const auto &m){return m.ready_playlist.empty();}),
      "Fresh B rejoin fabricated retained playlist consent");
    choose_retained_hopper();barrier("real-lobby-back-rejoined-consent");
    check(queue.snapshot().status==OnlineMatchmakingStatus::Ready && !state.launch && state.online_lobby_countdown()<0,
      "Fresh B rejoin autoqueued before a new explicit Start");
    std::cout<<"PASS two-native party B: active leader cancels; member cancel/confirm leaves both sessions; peer AE; real reinvite/rejoin requires fresh consent without autoqueue\n";
    }
  }
  std::cout<<"PASS matched native "<<(hosting?"host":"client")<<" players="<<roster<<" tick="<<tick<<" moved="<<moved<<" minimum_loaded="<<minimum_loaded<<" audible_frames="<<audio->audible<<"; opt-in, radio, "<<(o.compatibility=="halo2-native-queue-v12"?"direct UDP":"legacy relay")<<", mute/unmute, speaker labels and queue teardown\n";
  std::this_thread::sleep_for(500ms);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
