#define SDL_MAIN_HANDLED
#include "helpers/objective_match_completion_tu15.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_account.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_vista_announcer.hpp"
#include <SDL.h>
#include <curl/curl.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void need(bool ok,const char* why){if(!ok)throw FormatError(why);}
template<class F>void wait(const char* stage,F ready) {
    std::cout<<"Objective ONLINE stage: "<<stage<<'\n';
    const auto end=std::chrono::steady_clock::now()+120s;
    while(std::chrono::steady_clock::now()<end) {
        if(ready())return;std::this_thread::sleep_for(10ms);
    }
    throw FormatError(std::string("Objective ONLINE timed out: ")+stage);
}
std::size_t discard(char*,std::size_t a,std::size_t b,void*){return a*b;}
}
int main(int argc,char** argv)try {
    std::cout<<std::unitbuf;need(argc==7,"Supply MCC_MAP ORIGIN OWNER CALLBACK_PORT PRESET REPORT");
    SDL_SetMainReady();const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Objective ONLINE requires owned scratch");
    const std::filesystem::path map=argv[1];const std::string owner=argv[3],preset=argv[5];
    auto frontend=std::make_shared<FrontendSession>();
    auto content=Cache(map.parent_path()/"mainmenu.map").load_frontend();
    content.vista=load_vista_settings(map.parent_path()/"mainmenu.map");frontend->retain_content(std::move(content));
    auto& menu=frontend->create_state();menu.accept();menu.select(0);menu.accept();
    menu.text("Objective "+owner);menu.commit_text();menu.back();
    std::string browser;DiscordLoginOptions auth;auth.callback_port=std::stoul(argv[4]);
    auto login=std::make_shared<DiscordLogin>([&](const auto& url){browser=url;return true;},auth,
        [owner](const DiscordHttpRequest& r,std::stop_token)->DiscordHttpResponse {
            if(r.url.ends_with("/token"))return {200,"{\"access_token\":\""+owner+"\",\"token_type\":\"Bearer\",\"scope\":\"identify\",\"expires_in\":3600}"};
            return {200,"{\"id\":\""+std::to_string(123456789012345678ull+(owner=="two"))+"\",\"username\":\"test\",\"global_name\":\"Test\"}"};
        });
    login->start();wait("OAuth browser",[&]{return !browser.empty();});
    const auto start=browser.find("state=")+6;
    const auto callback="http://127.0.0.1:"+std::string(argv[4])+"/discord/callback?code=test&state="+
        browser.substr(start,browser.find('&',start)-start);
    {std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);
     curl_easy_setopt(curl.get(),CURLOPT_URL,callback.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");
     curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
     need(curl_easy_perform(curl.get())==CURLE_OK,"Objective fixture OAuth callback failed");}
    wait("OAuth authorization",[&]{return login->authorization().has_value();});
    frontend->discord_login=login;menu.set_discord_login(login);
    OnlineAccountOptions account_options;account_options.endpoint=argv[2];account_options.allow_loopback_http=true;
    account_options.enable_parties=true;account_options.poll_interval=1500ms;
    auto account=std::make_shared<OnlineAccount>(login,account_options);account->connect("Objective "+owner);
    menu.set_online_account(account);
    wait("authenticated ONLINE account",[&]{menu.update_online_login();return account->snapshot().authenticated && !account->snapshot().busy;});
    OnlineMatchmakingOptions cfg;cfg.endpoint=argv[2];cfg.allow_loopback_http=true;cfg.maps={map.stem().string()};cfg.poll_interval=20ms;
    auto queue=std::make_shared<OnlineMatchmaking>(login,cfg);menu.set_online_matchmaking(queue);
    const auto search=[&] {
        menu.return_online();menu.update_online_login();menu.open(219);
        wait("catalog",[&]{menu.update_online_login();menu.update_online_matchmaking();
            const auto status=queue->snapshot();if(status.status==OnlineMatchmakingStatus::Failed)throw FormatError(status.message);
            return status.status==OnlineMatchmakingStatus::Ready;});
        const auto catalog=queue->snapshot().playlists;
        const auto selected=std::ranges::find(catalog,std::string("casual"),&OnlinePlaylist::id);
        need(selected!=catalog.end() && selected->eligible,"Objective playlist missing");
        menu.select(unsigned(selected-catalog.begin()));menu.accept();need(menu.frame().screen==237,"Objective playlist details missing");
        menu.accept();need(menu.frame().screen==14,"Objective playlist skipped its original pregame lobby");
        wait("actual party hopper consent",[&]{menu.update_online_login();menu.update_online_matchmaking();
            const auto s=account->snapshot();return s.party && s.party->leader && !s.busy &&
                std::all_of(s.party->members.begin(),s.party->members.end(),[](const auto &m){return m.ready_playlist=="casual";});});
        const auto& entries=menu.frame().entries;
        const auto begin=std::find_if(entries.begin(),entries.end(),[](const auto& e){return e.destination==-17;});
        need(begin!=entries.end(),"Objective pregame lobby has no native Start Game action");
        menu.select(unsigned(begin-entries.begin()));menu.accept();
        wait("published shared pregame countdown",[&]{menu.update_online_login();menu.update_online_matchmaking();return menu.online_lobby_countdown()>0;});
        need(menu.online_lobby_countdown()<=10 && !menu.launch,"Objective pregame Start skipped its original countdown");
        // Advance only the pregame UI countdown; the final matched-room
        // countdown still waits for its real service-published zero.
        std::uint32_t menu_clock{};
        wait("actual launch",[&]{menu.ui_clock(menu_clock+=250);menu.update_online_login();menu.update_online_matchmaking();
            const auto status=queue->snapshot();if(status.status==OnlineMatchmakingStatus::Failed)throw FormatError(status.message);
            if(menu.frame().title=="UNAVAILABLE")throw FormatError(menu.frame().description);
            return menu.launch.has_value();});
        auto launch=std::move(*menu.launch);menu.launch.reset();queue->sign_out();
        need(launch.variant.name==preset && launch.selected_profiles.size()==1 && launch.selected_teams.size()==1 &&
            launch.online_match->roster().size()==2,"Objective handoff lost its actual original preset/profile/team");
        return launch;
    };
    auto launch=search();auto session=launch.online_match;
    const bool hosting=session->self()==session->host();
    const auto type=source_game_variant_type(source_game_variant(launch.variant));
    need(type==1 || type==9,"Objective ONLINE selected another engine");
    MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();options.online_match=session;
    options.variant=launch.variant;options.selected_profiles=launch.selected_profiles;options.selected_teams=launch.selected_teams;
    options.source_object_savegame_directory=std::filesystem::path(scratch)/("online-objective-"+owner);
    std::shared_ptr<MultiplayerPreparedWorld> world;
    if(hosting)world=std::make_shared<MultiplayerPreparedWorld>(options);
    const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(map,map.parent_path(),0,
        std::uint32_t(source_game_variant_get(source_game_variant(launch.variant),89)),{}),launch.variant);
    session->loaded(fingerprint,launch.variant);
    wait("world admission",[&]{if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());return session->phase()==OnlineMatchSession::Phase::Running;});
    std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
    std::cout<<"Objective admitted roles: self="<<session->self()<<" host="<<session->host()<<'\n';
    if(hosting) {
        const auto slot=session->local_slots().at(0);const auto& p=session->roster().at(slot);
        need(world->match.join(p.name,p.team,launch.selected_profiles[0],slot)==slot,"Objective host allocation changed admitted slot");
        world->bind_source_local_views(session->local_slots());
        host=std::make_unique<LanHost>(world->match,session,fingerprint,session->roster(),launch.variant);
    } else client=std::make_unique<LanClient>(session,session->host(),fingerprint,launch.selected_teams,launch.selected_profiles);
    std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,1> remote{};
    // A real ordinary movement input acknowledges snapshot admission without
    // holding Use before an objective's approach/pickup interaction starts.
    remote[0].forward=.25f;
    std::array<Controls,multiplayer_capacity> desired_controls{};
    desired_controls.at(1u-session->self()).forward=.25f;
    const auto input_file=std::filesystem::path(scratch)/"declared-objective-inputs.jsonl";
    std::ofstream input_output;std::ifstream input_input;
    if(host){input_output.open(input_file);need(bool(input_output),"Cannot open declared objective input journal");}
    const auto publish_input=[&] {
        const auto& input=desired_controls.at(1u-session->self());
        const nlohmann::json row={{"yaw",input.yaw},{"pitch",input.pitch},{"forward",input.forward},{"right",input.right},
            {"pickup",input.pickup},{"pickup_ms",input.pickup_milliseconds}};
        input_output<<row.dump()<<'\n';input_output.flush();need(bool(input_output),"Cannot publish declared objective input");
    };
    const auto read_input=[&] {
        if(!input_input.is_open()){input_input.open(input_file);if(!input_input){input_input.clear();return;}}
        input_input.clear();
        while(true) {
            const auto begin=input_input.tellg();std::string line;
            if(!std::getline(input_input,line) || input_input.eof()) {
                input_input.clear();input_input.seekg(begin);break;
            }
            const auto row=nlohmann::json::parse(line);auto& input=remote[0];
            input.yaw=row.at("yaw");input.pitch=row.at("pitch");input.forward=row.at("forward");input.right=row.at("right");
            input.pickup=row.at("pickup");input.pickup_milliseconds=row.at("pickup_ms");
        }
    };
    std::unique_ptr<NativeVistaAnnouncer> audio;
    std::uint32_t audio_serial{},audio_tick{};
    unsigned audio_events{},selected_cues{},objective_events{},objective_cues{},objective_requests{};double pcm_peak{};
    if(client) {
        const Cache resources(map);
        NativeVistaAnnouncerConfiguration cfg{resources.shared_directory(),resources.scenario(),resources.original_data(),
            0,30,{0x27f,0x1f80},false,SDL_GetTicks(),{SdlSoundPlatformConfiguration::Delivery::Manual,512,{}}};
        cfg.source_tu15=true;audio=NativeVistaAnnouncer::open(std::move(cfg));
    }
    const auto consume_audio=[&] {
        if(!audio || !client->ready())return;
        const auto& state=client->state();
        const auto advance=[&](std::uint32_t target) {
            while(audio_tick<target) {
                audio->pre_world_tick();++audio_tick;
                for(float value:audio->render_manual(1600)) {
                    need(std::isfinite(value),"Remote objective PCM contains a nonfinite sample");
                    pcm_peak=std::max(pcm_peak,std::abs(double(value)));
                }
            }
        };
        for(const auto& event:state.source_engine_events)if(event.serial>audio_serial) {
            audio_serial=event.serial;advance(event.tick);++audio_events;
            const bool objective=event.words[0]==(type==1?3u:10u);objective_events+=objective;
            for(const auto& selection:audio->publish_source_event(event,client->slots())) {
                selected_cues+=selection.sound!=null_datum;
                objective_cues+=objective && selection.sound!=null_datum;
                objective_requests+=objective && selection.requested;
            }
        }
        advance(state.tick);
    };
    auto next_tick=std::chrono::steady_clock::now();
    std::uint64_t rejected{};
    unsigned observed_mask{},observed_peers=16;
    const auto receive_host=[&] {
        host->receive(controls);unsigned mask{};
        for(unsigned n=0;n<multiplayer_capacity;++n)if(world->match.player(n).connected)mask|=1u<<n;
        if(mask!=observed_mask || host->peers()!=observed_peers) {
            observed_mask=mask;observed_peers=host->peers();
            std::cout<<"Objective actual source roster: mask="<<mask<<" peers="<<observed_peers<<" tick="<<world->match.state().tick<<'\n';
        }
    };
    const auto source_step=[&] {
        publish_input();controls.at(session->self())=desired_controls.at(session->self());
        receive_host();world->match.step(controls);host->broadcast();
        const auto rate=Bytes(world->source_objects->tu15_world().memory().view(
            Bytes(world->source_objects->tu15_world().memory().view(0x51df54u,4)).u32(0)+2u,2)).u16(0);
        need(rate>0 && rate<=120,"Objective ONLINE lost its original configured tick rate");
        next_tick=std::max(next_tick+std::chrono::nanoseconds(1000000000/rate),std::chrono::steady_clock::now());
        std::this_thread::sleep_until(next_tick);
        if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());
    };
    const auto pump=[&] {
        if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());
        if(host) {
            receive_host();
            // Loading/admission is outside gameplay. Running the original
            // engine with only the first admitted team can legitimately end
            // the match before the other actual player has joined.
            if(host->peers()==1 && !world->match.state().game_over_complete())source_step();
            else host->broadcast();
        }
        else {
            read_input();
            client->send(remote);client->receive();
            if(client->rejected_packet_count()!=rejected) {
                rejected=client->rejected_packet_count();
                if(rejected<4)std::cerr<<"Objective client rejected packet: "<<client->last_packet_rejection()<<'\n';
            }
            if(!client->error().empty())throw FormatError(client->error()+": "+client->last_packet_rejection()+
                " direct_rejections="+std::to_string(session->rejected_direct_dispatches()));
            consume_audio();
        }
    };
    wait("source player admission",[&]{
        pump();
        if(client)return client->ready();
        // Only a client that accepted its actual snapshot can send an
        // authenticated ordinary input. Peer insertion alone is too early.
        const auto remote_slot=1u-session->self();
        return host->peers()==1 && controls.at(remote_slot).forward>.1f;
    });
    if(host) {
        const auto diagnostic=host->snapshot_diagnostics();
        std::cout<<"Objective source admission: tick="<<world->match.state().tick<<" snapshot_bytes="<<diagnostic.body_bytes
            <<" complete_fragments="<<diagnostic.fragments<<'\n';
    }
    nlohmann::json report={{"name",preset},{"host",hosting},{"passed",false},{"direct_udp",session->direct_udp()}};
    if(host) {
        const auto step=[&] {
            source_step();
        };
        desired_controls={};
        for(unsigned n=0;n<4;++n)step();
        // The separate process consumes declared controller samples and sends
        // them through the real client/host ingress. Source never consumes
        // the host's desired remote controls directly.
        report["match"]=test::complete_objective_match_tu15(*world,launch.variant,desired_controls,step);
        host->broadcast();session->complete(online_postgame_publish(world->match,launch.variant,session->roster()).source);
    }
    wait("unanimous source results",[&] {
        pump();const auto state=client?client->state():world->match.state();
        if(session->phase()==OnlineMatchSession::Phase::Reviewing && state.game_over_complete()) {
            online_postgame_validate(state,launch.variant,session->roster(),session->postgame());session->confirm_result();
        }
        return session->phase()==OnlineMatchSession::Phase::Postgame;
    });
    const auto final=client?client->state():world->match.state();
    report["tick"]=final.tick;report["winner_team"]=final.winner_team;
    if(client) {
        need(objective_events && objective_cues && objective_requests && pcm_peak>0,
            "Remote objective events did not reach their native PCM consumer");
        report["remote_audio"]={{"events",audio_events},{"selected_cues",selected_cues},{"objective_events",objective_events},
            {"objective_cues",objective_cues},{"objective_requests",objective_requests},{"pcm_peak",pcm_peak}};
    }
    audio.reset();
    host.reset();client.reset();const std::weak_ptr<MultiplayerPreparedWorld> retired=world;world.reset();
    need(retired.expired(),"Objective ONLINE return retained a departed source world");
    bool done{};auto pcr=online_postgame_screen(*frontend,session,[&]{done=true;menu.return_online_lobby();});
    need(pcr->results->player_count==2 && pcr->results->variant==type && pcr->results->team_count==2,
        "Objective ONLINE PCR lost original engine/team/players");
    frontend->enter(FrontendOptions::Entry::postgame_report,pcr);menu.back();
    need(done && menu.frame().screen==14 && menu.has_online_prior_stats() && menu.local_players().size()==1 && login->authorization(),
        "Objective PCR Done failed its retained pregame lobby return");
    pcr.reset();launch.online_match.reset();session.reset();
    auto second=search();need(second.online_match->roster().size()==2,"Objective fresh requeue failed");
    // Both actual new admissions must be observed before either endpoint leaves.
    const auto barrier=std::filesystem::path(scratch)/"online-objective-requeue";
    std::filesystem::create_directories(barrier);{std::ofstream out(barrier/owner);out<<"ready\n";}
    wait("both fresh admissions",[&]{return std::filesystem::is_regular_file(barrier/"one") && std::filesystem::is_regular_file(barrier/"two");});
    second.online_match.reset();report["passed"]=true;report["world_released"]=true;report["pcr_done"]=true;report["requeued"]=true;
    {std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain objective ONLINE report");}
    std::cout<<"PASS "<<preset<<": original completed source match, native encrypted peers, unanimous retained PCR, world release, Done and fresh admission\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
