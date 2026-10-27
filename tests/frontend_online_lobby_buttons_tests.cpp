#define SDL_MAIN_HANDLED
#include "halo2/frontend_session.hpp"
#include "halo2/cache.hpp"
#include "halo2/native_tu15_online_services.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include <SDL.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
std::size_t discard(char*,std::size_t a,std::size_t b,void*){return a*b;}
void key(SDL_Keycode value){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=value;check(SDL_PushEvent(&e)==1,"Actual SDL key delivery failed");e.type=SDL_KEYUP;check(SDL_PushEvent(&e)==1,"Actual SDL key release failed");}
void rendered_buttons(const std::shared_ptr<FrontendSession>&frontend,
    const std::shared_ptr<OnlineAccount>&account,const std::shared_ptr<OnlineMatchmaking>&queue,
    const std::filesystem::path&maps) {
  check(std::getenv("DISPLAY"),"Actual rendered lobby button fixture requires DISPLAY");SDL_SetMainReady();
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Rendered button captures require the owned test lifecycle");
  const auto output=std::filesystem::path(scratch)/"lobby-buttons-gpu";std::filesystem::create_directories(output);
  frontend->content().vista=load_vista_settings(maps/"mainmenu.map");
  frontend->online_services=std::make_shared<NativeTu15OnlineServices>();
  frontend->online_services->login=frontend->discord_login;
  frontend->online_services->account=account;frontend->online_services->matchmaking=queue;
  FrontendOptions options;options.session=frontend;options.entry=FrontendOptions::Entry::session_return;
  options.session_mainmenu_ready=[](MenuState&s){s.return_online();};
  options.maps=maps;options.vista_settings=maps/"mainmenu.map";
  options.hidden=options.skip_intro=options.mute=true;options.width=960;options.height=720;
  unsigned phase{},privacy_phase{};int before{};std::string tag;std::set<std::string> captured;
  std::uint64_t same_mode_serial{},same_mode_revision{};
  auto changed=std::chrono::steady_clock::now();const auto started=changed;
  nlohmann::json observations=nlohmann::json::array();
  const auto present=[&](std::string next){if(tag!=next){tag=std::move(next);changed=std::chrono::steady_clock::now();}return captured.contains(tag);};
  options.session_update=[&](MenuState&s,std::uint32_t,const auto&){
    check(std::chrono::steady_clock::now()-started<100s,"Rendered source Start/Game Setup path timed out");
    const auto &f=s.frame();const auto a=account->snapshot();const auto q=queue->snapshot();
    switch(phase){
    case 0:if(f.screen==186&&present("online")){key(SDLK_RETURN);++phase;}break;
    case 1:if(f.screen==14&&s.lobby_view()->input.start_permission&&present("pregame")){key(SDLK_RIGHT);key(SDLK_RETURN);++phase;}break;
    case 2:
      if(privacy_phase==0 && f.screen==23 && present("setup")) {
        check(f.title=="\xee\x90\x9f SETUP"&&f.entries.size()==3&&f.entries[0].value==9,"Rendered Setup lost its original header or enum6 rows");
        check(s.expand(f.title)=="Xbox Live: Optimatch SETUP","Rendered original Setup lost its exact protocol wording");key(SDLK_x);++privacy_phase;
      } else if(privacy_phase==1 && f.screen==25 && present("privacy-open")) {
        check(f.title=="PARTY PRIVACY" && f.entries.size()==3 && f.selected==0 && a.party && a.party->privacy && a.party->privacy->mode==0,
            "Actual SDL privacy route lost source25 or the acknowledged Open setting");key(SDLK_DOWN);key(SDLK_RETURN);++privacy_phase;
      } else if(privacy_phase==2 && f.screen==14 && a.party && a.party->privacy && a.party->privacy->mode==1 && !a.busy && present("privacy-invite-only")) {
        if(f.selected==0)key(SDLK_RIGHT);key(SDLK_RETURN);++privacy_phase;
      } else if(privacy_phase==3 && f.screen==23 && present("privacy-setup")){key(SDLK_x);++privacy_phase;}
      else if(privacy_phase==4 && f.screen==25 && present("privacy-current")) {
        check(f.selected==1 && a.party && a.party->privacy && !a.busy,"Privacy editor did not focus the actual Invite Only mode");
        same_mode_serial=a.party_command_serial;same_mode_revision=a.party->privacy->revision;key(SDLK_RETURN);++privacy_phase;
      } else if(privacy_phase==5 && f.screen==14 && a.party && a.party->privacy && !a.busy && present("privacy-same-mode-return")) {
        check(a.party->privacy->mode==1 && a.party->privacy->revision>same_mode_revision &&
            a.party_command_serial==same_mode_serial+1 && a.party_command_successful==true,
            "Same-mode source privacy callback skipped its actual authenticated setter/counter");
        if(f.selected==0)key(SDLK_RIGHT);key(SDLK_RETURN);++privacy_phase;
      } else if(privacy_phase==6 && f.screen==23 && present("privacy-setup-reset")){key(SDLK_x);++privacy_phase;}
      else if(privacy_phase==7 && f.screen==25 && present("privacy-reset")){check(f.selected==1,"Privacy reset lost actual mode focus");key(SDLK_UP);key(SDLK_RETURN);++privacy_phase;}
      else if(privacy_phase==8 && f.screen==14 && a.party && a.party->privacy && a.party->privacy->mode==0 && !a.busy && present("privacy-open-restored")) {
        if(f.selected==0)key(SDLK_RIGHT);key(SDLK_RETURN);++privacy_phase;
      } else if(privacy_phase==9 && f.screen==23 && present("setup-restored")){key(SDLK_RETURN);++phase;}
      break;
    case 3:if(f.screen==237&&!f.entries.empty()&&present("playlist")){check(f.entries.size()==4,"Rendered source237 is not the authenticated listing");key(SDLK_DOWN);key(SDLK_UP);key(SDLK_RETURN);++phase;}break;
    case 4:if(f.screen==14&&s.lobby_view()->input.start_permission&&present("selected-lobby")){key(SDLK_LEFT);key(SDLK_RETURN);++phase;}break;
    case 5:if(f.screen==14&&s.online_lobby_countdown()>0&&present("countdown-start")&&!a.busy){check(q.playlist_id.empty(),"Rendered Start bypassed shared countdown");key(SDLK_x);++phase;}break;
    case 6:if(f.screen==14&&s.online_lobby_countdown()<0&&!a.busy&&present("countdown-cancel")){key(SDLK_RETURN);++phase;}break;
    case 7:if(f.screen==14&&s.online_lobby_countdown()>0&&present("countdown-restart")&&!a.busy){before=s.online_lobby_countdown();key(SDLK_RETURN);++phase;}break;
    case 8:if(f.screen==14&&!a.busy&&s.online_lobby_countdown()>0&&s.online_lobby_countdown()<before){if(s.online_lobby_countdown()>3){before=s.online_lobby_countdown();key(SDLK_RETURN);}else{tag.clear();++phase;}}break;
    case 9:if(f.screen==14&&s.online_lobby_countdown()>0)present("countdown-"+std::to_string(s.online_lobby_countdown()));else if(f.screen==210){tag.clear();++phase;}break;
    case 10:if(f.screen==210&&q.status==OnlineMatchmakingStatus::Searching&&present("search")){check(q.playlist_id=="ranked","Rendered shared clock queued a different hopper");key(SDLK_ESCAPE);++phase;}break;
    case 11:if(f.screen==7&&present("leave-confirm")){check(detail::og_matchmaking_layout::goal_frame(f,&s),"Actual B dialog lost its original goal ancestor");key(SDLK_UP);key(SDLK_RETURN);++phase;}break;
    case 12:if(f.screen==14&&q.status==OnlineMatchmakingStatus::Ready&&present("returned-lobby")){++phase;return true;}break;
    }
    return false;
  };
  options.frame_capture=[&](const MenuState&s)->std::filesystem::path{
    if(s.ui_map_intro_color())return {};
    if(tag.empty()||captured.contains(tag)||(!tag.starts_with("countdown-")&&std::chrono::steady_clock::now()-changed<1000ms))return {};
    // Countdown3/2/1 changes the already visible lobby. Other screens wait
    // their real authored intro; no diagnostic UI clock is supplied.
    captured.insert(tag);const auto file=tag+".ppm";
    observations.push_back({{"phase",tag},{"screen",s.frame().screen},{"selected",s.frame().selected},
        {"lobby_countdown",s.online_lobby_countdown()},{"queue_status",unsigned(queue->snapshot().status)},
        {"actual_friends_capability",s.online_friends_available()},{"source_map_intro_clear",true},{"file",file}});
    if(const auto &party=s.online_party_observation();party && party->privacy)
      observations.back()["privacy"]={{"mode",party->privacy->mode},{"revision",party->privacy->revision}};
    return output/file;
  };
  check(!run_frontend(options).has_value()&&phase==13,"Rendered source button path manufactured a gameplay launch");
  for(const auto*required:{"online","pregame","setup","privacy-open","privacy-invite-only","privacy-setup","privacy-current",
      "privacy-same-mode-return","privacy-setup-reset","privacy-reset","privacy-open-restored","setup-restored",
      "playlist","selected-lobby","countdown-start","countdown-cancel","countdown-restart","countdown-3","countdown-2","countdown-1","search","leave-confirm","returned-lobby"})
    check(captured.contains(required)&&std::filesystem::is_regular_file(output/(std::string(required)+".ppm")),"Actual rendered button flow omitted a required phase");
  std::ofstream(output/"observations.json")<<observations.dump(2)<<'\n';
  std::cout<<"PASS actual SDL/Vulkan source Setup23 -> privacy25 actual/same-mode acknowledged setters -> listing237 -> retained lobby Start/X/restart/3/2/1 -> search -> B/cancel -> retained lobby\n";
}
}
int main(int argc,char**argv)try{
  check(argc==6,"Supply MCC maps --matchmaking-url URL --callback-port PORT");
  check(std::string_view(argv[2])=="--matchmaking-url" && std::string_view(argv[4])=="--callback-port","Unexpected button fixture arguments");
  const auto legacy_value=std::getenv("H2_TEST_LEGACY_PARTY_COUNTDOWN");
  const bool legacy=legacy_value&&std::string_view(legacy_value)=="1";
  curl_global_init(CURL_GLOBAL_DEFAULT);
  const Cache cache(std::filesystem::path(argv[1])/"mainmenu.map");auto frontend=std::make_shared<FrontendSession>();frontend->retain_content(cache.load_frontend());
  auto &menu=frontend->create_state();menu.accept();menu.select(0);menu.accept();menu.text("Pilot");menu.commit_text();menu.back();
  check(menu.signed_in_profile()=="Pilot","Actual source profile chooser did not admit the local profile");
  for(const auto id:{7u,8u}){menu.open(id);check(!detail::og_matchmaking_layout::goal_frame(menu.frame(),&menu),"Unrelated dialog acquired the original matchmaking route");menu.back();}
  std::string browser;DiscordLoginOptions login_options;login_options.callback_port=std::stoul(argv[5]);
  auto login=std::make_shared<DiscordLogin>([&](const auto&value){browser=value;return true;},login_options,
      [](const DiscordHttpRequest&r,std::stop_token)->DiscordHttpResponse{
        if(r.url.ends_with("/token"))return {200,R"({"access_token":"t","token_type":"Bearer","scope":"identify","expires_in":3600})"};
        return {200,R"({"id":"123456789012345678","username":"verified_user","global_name":"Pilot"})"};
      });
  std::shared_ptr<OnlineAccount> account;
  std::shared_ptr<OnlineMatchmaking> queue;
  const char* stage="external account authorization";
  auto wait=[&](auto predicate){const auto until=std::chrono::steady_clock::now()+15s;for(;;){menu.update_online_login();menu.update_online_matchmaking();if(predicate())return;if(std::chrono::steady_clock::now()>=until){
    nlohmann::json detail{{"stage",stage},{"screen",menu.frame().screen},{"selected",menu.frame().selected}};
    if(account){const auto a=account->snapshot();detail["account"]={{"authenticated",a.authenticated},{"busy",a.busy},{"party",a.party.has_value()},{"message",a.message},{"party_message",a.party_message}};
      if(a.party){detail["account"]["leader"]=a.party->leader;detail["account"]["countdown_supported"]=a.party->countdown_supported;
        if(a.party->countdown)detail["account"]["countdown"]={{"serial",a.party->countdown->serial},{"seconds",a.party->countdown->seconds?nlohmann::json(*a.party->countdown->seconds):nlohmann::json(nullptr)},{"complete",a.party->countdown->complete}};
        detail["account"]["members"]=nlohmann::json::array();for(const auto&m:a.party->members)detail["account"]["members"].push_back({{"self",m.self},{"ready_playlist",m.ready_playlist}});}}
    if(queue){const auto q=queue->snapshot();detail["queue"]={{"status",unsigned(q.status)},{"playlist_id",q.playlist_id},{"message",q.message},{"catalog_count",q.playlists.size()}};}
    std::cerr<<"Lobby button timeout state (no credentials): "<<detail.dump()<<'\n';
    throw FormatError(std::string("Actual ONLINE lobby button path timed out: ")+stage);
  }std::this_thread::sleep_for(5ms);}};
  login->start();wait([&]{return !browser.empty();});
  const auto at=browser.find("state=")+6;const auto end=browser.find('&',at);
  const auto callback="http://127.0.0.1:"+std::string(argv[5])+"/discord/callback?code=test&state="+browser.substr(at,end-at);
  {std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
    curl_easy_setopt(curl.get(),CURLOPT_URL,callback.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
    check(curl_easy_perform(curl.get())==CURLE_OK,"Actual external account callback failed");}
  wait([&]{return login->authorization().has_value();});menu.set_discord_login(login);
  frontend->discord_login=login;
  OnlineAccountOptions account_options;account_options.endpoint=argv[3];account_options.allow_loopback_http=true;account_options.enable_parties=true;account_options.poll_interval=25ms;
  if(const auto gpu=std::getenv("H2_TEST_LOBBY_BUTTONS_GPU");gpu&&std::string_view(gpu)=="1")
    account_options.poll_interval=20s; // Actual production social cadence; commands wake it immediately.
  account=std::make_shared<OnlineAccount>(login,account_options);menu.set_online_account(account);
  OnlineMatchmakingOptions queue_options;queue_options.endpoint=argv[3];queue_options.allow_loopback_http=true;queue_options.maps={"lockout"};queue_options.poll_interval=25ms;
  queue=std::make_shared<OnlineMatchmaking>(login,queue_options);menu.set_online_matchmaking(queue);
  stage="authenticated account session";wait([&]{return account->snapshot().authenticated&&!account->snapshot().busy;});
  if(const auto gpu=std::getenv("H2_TEST_LOBBY_BUTTONS_GPU");gpu&&std::string_view(gpu)=="1") {
    check(!legacy,"Rendered button path requires the supported modern service");
    rendered_buttons(frontend,account,queue,argv[1]);
    queue->sign_out();account->sign_out();menu.set_online_matchmaking({});menu.set_online_account({});return 0;
  }
  menu.return_online();menu.select(0);menu.accept();
  stage="actual accepted party and playlist consent";
  wait([&]{const auto a=account->snapshot();return menu.frame().screen==14&&a.party&&a.party->leader&&!a.busy&&
    std::any_of(a.party->members.begin(),a.party->members.end(),[](const auto&m){return m.self&&m.ready_playlist=="ranked";});});
  check(menu.expand(menu.online_party_text(menu.frame(),48))=="Xbox Live: Optimatch",
      "Original pregame protocol token was replaced by the generic ONLINE label");
  const auto select=[&](int destination){const auto&rows=menu.frame().entries;const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto&e){return e.destination==destination;});check(found!=rows.end(),"Actual source menu omitted expected semantic action");menu.select(unsigned(found-rows.begin()));};
  if(legacy){
    const bool eligible=menu.lobby_view()->input.start_permission;
    const auto explanation=menu.online_party_text(menu.frame(),0);
    select(-17);menu.accept();
    check(menu.frame().screen==8&&menu.frame().description.find("shared lobby countdown")!=std::string::npos,
        "Start Game was silently consumed without a countdown or an actionable service error");
    check(detail::og_matchmaking_layout::goal_frame(menu.frame(),&menu),"Actual legacy-service error lost its original goal ancestor");
    check(!eligible&&explanation.find("shared lobby countdown")!=std::string::npos,
        "Disabled Start Game gave no visible explanation in the real pregame lobby");
    check(queue->snapshot().playlist_id.empty(),"Unsupported countdown fabricated a queued match");
  }else{
    select(23);menu.accept();check(menu.frame().screen==23&&menu.frame().title=="\xee\x90\x9f SETUP","Game Setup bypassed the original source squad-settings screen/header");
    check(menu.expand(menu.frame().title)=="Xbox Live: Optimatch SETUP",
        "Original Setup protocol token was replaced by the generic ONLINE label");
    check(menu.frame().entries.size()==3&&menu.frame().entries[0].value==9&&menu.frame().entries[1].value==6&&menu.frame().entries[2].value==7,
        "Original matchmaking Setup lost its three semantic source rows");
    for(unsigned row:{1u,2u}){
      menu.select(row);menu.accept();
      check(menu.frame().screen==8&&menu.frame().description.find("not available from this service")!=std::string::npos,
          "ONLINE Game Setup silently changed to an unrelated local session owner");
      menu.back();check(menu.frame().screen==23,"Unavailable ONLINE mode discarded retained Game Setup");
    }
    check(account->snapshot().party->privacy.has_value(),"Modern service omitted its actual party privacy owner");
    const auto party_version=account->snapshot().party->version;
    const auto open_privacy=[&] {
      if(menu.frame().screen==14){select(23);menu.accept();}
      check(menu.frame().screen==23,"Privacy route did not begin in actual original Setup");
      menu.auxiliary();const auto p=account->snapshot().party;
      check(menu.frame().screen==25 && p && p->privacy && menu.frame().entries.size()==3 &&
          menu.frame().selected==p->privacy->mode && detail::og_matchmaking_layout::goal_frame(menu.frame(),&menu),
          "Original X privacy route lost actual mode focus or source25 owner");
    };
    open_privacy();
    const auto unchanged=*account->snapshot().party->privacy;
    const auto no_op_serial=account->snapshot().party_command_serial;
    menu.accept();stage="same-mode privacy setter acknowledged its original counter update";
    wait([&]{const auto s=account->snapshot();return menu.frame().screen==14 && s.party && s.party->privacy &&
        s.party->privacy->context==unchanged.context && s.party->privacy->mode==unchanged.mode &&
        s.party->privacy->revision>unchanged.revision && s.party_command_serial==no_op_serial+1 &&
        s.party_command_successful==true && !s.busy;});
    const auto set_privacy=[&](unsigned mode) {
      open_privacy();const auto before=*account->snapshot().party->privacy;
      menu.select(mode);menu.accept();stage="acknowledged same-party privacy mutation";
      wait([&]{const auto s=account->snapshot();return menu.frame().screen==14 && s.party && !s.busy &&
          s.party->privacy && s.party->privacy->mode==mode && s.party->privacy->context==before.context &&
          s.party->privacy->revision>before.revision && s.party_command_successful==true;});
      const auto p=account->snapshot().party;
      check(p->version==party_version && std::all_of(p->members.begin(),p->members.end(),[](const auto&m){return m.ready_playlist=="ranked";}),
          "Party privacy mutation invalidated unrelated membership or hopper consent");
      constexpr std::array<std::uint32_t,3> ids{0x04000201,0x0f000202,0x06000203};
      check(menu.lobby_view()->input.party_privacy==int(mode) && menu.online_party_text(menu.frame(),10)==
          detail::og_matchmaking_layout::original_string(14,ids[mode]),
          "Acknowledged party privacy did not reach its exact source pregame caption");
    };
    set_privacy(1);set_privacy(2);set_privacy(0);
    open_privacy();
    // The snapshot is retained across iterator use; the RPC is a real second
    // controller/service observation while the privacy editor owns channel3.
    const auto privacy_catalog=queue->snapshot().playlists;
    const auto active_hopper=std::find_if(privacy_catalog.begin(),privacy_catalog.end(),[](const auto&p){return p.id=="ranked";});
    check(active_hopper!=privacy_catalog.end() && account->party_command({"countdown_start",{},active_hopper->id,active_hopper->revision}),
        "Privacy countdown preservation control could not start its actual party clock");
    stage="actual countdown while original privacy editor is open";
    wait([&]{const auto s=account->snapshot();return s.party && s.party->countdown && !s.busy && menu.online_lobby_countdown()>0;});
    const auto running=*account->snapshot().party->countdown;
    check(running.seconds.has_value(),"Observed shared countdown has no authoritative remaining seconds");
    menu.select(1);menu.accept();stage="privacy mutation preserves running shared clock";
    wait([&]{const auto s=account->snapshot();return menu.frame().screen==14 && s.party && s.party->privacy &&
        s.party->privacy->mode==1 && s.party->countdown && !s.busy;});
    check(account->snapshot().party->countdown->serial==running.serial && menu.online_lobby_countdown()>0 &&
        menu.online_lobby_countdown()<=*running.seconds && account->snapshot().party->version==party_version,
        "Privacy mutation canceled or restarted the real shared lobby countdown");
    menu.auxiliary();stage="cancel preserved clock after privacy mutation";
    wait([&]{return !account->snapshot().busy && !account->snapshot().party->countdown && menu.online_lobby_countdown()<0;});
    set_privacy(0);
    select(23);menu.accept();
    menu.select(0);menu.accept();wait([&]{return menu.frame().screen==237&&!menu.frame().entries.empty();});
    check(menu.frame().entries.size()==4&&menu.frame().entries[0].label==queue->snapshot().playlists[0].name,
        "Lobby playlist selector was replaced by a confirmation/details page");
    stage="chosen playlist return and consent";menu.select(0);menu.accept();wait([&]{return menu.frame().screen==14&&!account->snapshot().busy;});
    check(menu.lobby_view()->input.start_permission,"Authenticated supported solo party disabled Start Game");
    const auto catalog=queue->snapshot().playlists;
    const auto hopper=std::find_if(catalog.begin(),catalog.end(),[](const auto&p){return p.id=="ranked";});
    check(hopper!=catalog.end()&&account->party_command({"ready",{},hopper->id,hopper->revision}),"Busy-operation control did not submit real authenticated consent");
    menu.update_online_login();check(!menu.lobby_view()->input.start_permission,"Busy party operation left Start Game eligible");
    select(-17);menu.accept();
    check(menu.frame().screen==8&&menu.frame().description.find("current ONLINE operation")!=std::string::npos,
        "Busy Start Game was silently consumed");
    check(detail::og_matchmaking_layout::goal_frame(menu.frame(),&menu),"Actual busy-operation error lost its original goal ancestor");
    stage="busy consent completion";menu.back();wait([&]{return !account->snapshot().busy;});
    stage="published initial shared countdown";
    select(-17);menu.accept();wait([&]{return menu.online_lobby_countdown()>0&&!account->snapshot().busy;});
    check(menu.online_lobby_countdown()<=10&&queue->snapshot().playlist_id.empty(),"Start Game did not begin only the authoritative shared countdown");
    while(menu.online_lobby_countdown()>3){const auto before=menu.online_lobby_countdown();select(23);menu.accept();wait([&]{return !account->snapshot().busy&&menu.online_lobby_countdown()<before;});check(menu.frame().screen==14,"Active countdown A incorrectly opened Game Setup");}
    stage="shared countdown completion and search";wait([&]{return menu.frame().screen==210&&queue->snapshot().status==OnlineMatchmakingStatus::Searching;});
    check(queue->snapshot().playlist_id=="ranked"&&!menu.launch,"Actual shared countdown did not enqueue exactly the selected hopper");
    menu.back();menu.select(0);menu.accept();
    stage="authenticated search cancellation";wait([&]{return menu.frame().screen==14&&queue->snapshot().status==OnlineMatchmakingStatus::Ready;});
  }
  queue->sign_out();account->sign_out();menu.set_online_matchmaking({});menu.set_online_account({});
  std::cout<<(legacy?"PASS actual authenticated legacy-service Start reports unavailable shared lobby countdown without queuing\n":
      "PASS actual authenticated source Game Setup -> playlist -> retained lobby Start -> authoritative shared countdown -> queue -> cancel\n");
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
