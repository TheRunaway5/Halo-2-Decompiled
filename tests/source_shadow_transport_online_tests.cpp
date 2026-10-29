#define SDL_MAIN_HANDLED
#include "source_match_rebind_fixture.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/multiplayer_network.hpp"
#include <curl/curl.h>
#include <rtc/global.hpp>
#include <fstream>
#include <thread>
#include <nlohmann/json.hpp>
#ifdef HALO2_SHADOW_ONLINE_GPU
#include "player_shadows_runtime_capture.hpp"
#endif

namespace {
using namespace std::chrono_literals;
using Wall=std::chrono::steady_clock;
template<class F>void wait(F ready,const char *stage){const auto until=Wall::now()+120s;
    while(!ready()){check(Wall::now()<until,stage);std::this_thread::sleep_for(5ms);}}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
void report(const std::filesystem::path &path,const nlohmann::json &value){
    const auto temporary=path.string()+".tmp";
    {std::ofstream out(temporary);out<<value.dump(2)<<'\n';check(bool(out),"Could not write bounded native ONLINE shadow report");}
    std::filesystem::rename(temporary,path);
}
// Lose the real ordinary native snapshots, while admitting the independent
// ONLINE control lane and reliable native-cut datagrams unchanged. Its exact
// embedded gameplay body must pass the ordinary decoder before publication.
class LostOrdinarySnapshots final:public MultiplayerDatagrams {
    std::shared_ptr<OnlineMatchSession> session_;
public:
    std::uint64_t dropped{};
    bool lose=true;
    explicit LostOrdinarySnapshots(std::shared_ptr<OnlineMatchSession> session):session_(std::move(session)){}
    void send(std::span<const std::byte> bytes,unsigned peer)override{session_->send(bytes,peer);}
    bool receive(std::vector<std::byte>& bytes,unsigned& peer)override {
        for(unsigned n=0;n<256&&session_->receive(bytes,peer);++n) {
            if(lose&&bytes.size()>=7 && Bytes(bytes).u32(0)==0x324e3248u &&
                (bytes[6]==std::byte{3} || bytes[6]==std::byte{14})) {++dropped;continue;}
            return true;
        }
        return false;
    }
};
#ifdef HALO2_SHADOW_ONLINE_GPU
void gpu(const std::shared_ptr<OnlineMatchSession>& session,const std::filesystem::path& files,const std::string& account) {
    const auto* map=std::getenv("HALO2_SHADOW_GPU_MAP");const auto* mode=std::getenv("HALO2_SHADOW_GPU_MODE");
    check(map&&mode&&std::filesystem::is_regular_file(map),"ONLINE GPU proof requires its actual prepared MCC map");
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"ONLINE GPU proof requires owned scratch");
    const auto own=std::filesystem::path(scratch)/("online-"+account);std::filesystem::create_directories(own);
    const auto other=account=="one"?"two":"one";
    const auto own_ready=files/("shadow-gpu-ready-"+account),other_ready=files/(std::string("shadow-gpu-ready-")+other);
    const auto own_submitted=files/("shadow-gpu-submitted-"+account),other_submitted=files/(std::string("shadow-gpu-submitted-")+other);
    const bool hosting=session->self()==session->host();
    SDL_SetMainReady();
    MultiplayerOptions options;options.map=map;options.shared_maps=options.map.parent_path();options.online_match=session;
    options.width=640;options.height=480;options.local_players=1;options.hidden=true;options.mute=true;options.validation=true;
    options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};options.starting_weapon=5;options.session_random_seed=0x12345678u;
    options.source_object_savegame_directory=own/"world";
    options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
    shadow_test::Capture capture;capture.mode=mode;capture.picture=own/"player-shadow.ppm";
    capture.local_slot=session->local_slots().at(0);capture.joining=!hosting;capture.exact_tick=false;capture.expected_players=2;
    capture.capture_ready=[&] {
        if(!std::filesystem::exists(own_ready))report(own_ready,{{"ready",true}});
        return std::filesystem::exists(other_ready);
    };
    capture.after_capture=[&] {
        // Both actual renderer fences finish before either live native socket
        // or foreground world retires. This barrier supplies no gameplay state.
        report(own_submitted,{{"submitted",true}});
        const auto deadline=Wall::now()+30s;
        while(!std::filesystem::exists(other_submitted)) {
            check(Wall::now()<deadline,"Other actual ONLINE GPU frame did not finish before peer retirement");
            std::this_thread::sleep_for(5ms);
        }
    };
    auto result=shadow_test::capture(options,capture);
    result["role"]=hosting?"host":"joining-client";result["account"]=account;
    result["scope"]="Actual ONLINE direct UDP MCC runtime, one real local view per PC, both native sockets live through completed GPU frames; fixture Discord, no full-game parity claim";
    report(files/("native-shadow-"+account+".json"),result);
    wait([&]{return std::filesystem::exists(files/(std::string("native-shadow-")+other+".json"));},"Other ONLINE GPU endpoint did not retain its capture receipt");
}
#endif
void run(int argc,char **argv){
    check(argc==7,"Supply ROOT ORIGIN OWNER CALLBACK_PORT XBE PRIVATE_FIXTURE_DIRECTORY");
    const std::filesystem::path root=argv[1],files=argv[6];
    const auto* admitted_assets=std::getenv("HALO2_TEST_ASSET_SOURCE_ROOT");
    const auto asset_root=admitted_assets?std::filesystem::path(admitted_assets):root;
    const auto map=asset_root/"assets/disc/maps/lockout.map";
    const std::string account=argv[3];std::string browser;DiscordLoginOptions auth;auth.callback_port=std::stoul(argv[4]);
    auto login=std::make_shared<DiscordLogin>([&](const auto &url){browser=url;return true;},auth,
        [&](const DiscordHttpRequest &r,std::stop_token)->DiscordHttpResponse{
            if(r.url.ends_with("/token"))return {200,"{\"access_token\":\""+account+"\",\"token_type\":\"Bearer\",\"scope\":\"identify\",\"expires_in\":3600}"};
            return {200,"{\"id\":\""+std::to_string(123456789012345678ull+(account=="one"?0:1))+"\",\"username\":\"test\",\"global_name\":\"Test\"}"};});
    login->start();wait([&]{return !browser.empty();},"OAuth launch timeout");const auto start=browser.find("state=")+6;
    const auto callback="http://127.0.0.1:"+std::string(argv[4])+"/discord/callback?code=test&state="+browser.substr(start,browser.find('&',start)-start);
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);curl_easy_setopt(curl.get(),CURLOPT_URL,callback.c_str());
    curl_easy_setopt(curl.get(),CURLOPT_PROXY,"");curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT_MS,2000L);curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,discard);
    check(curl_easy_perform(curl.get())==CURLE_OK,"Fixture authorization callback failed");wait([&]{return login->authorization().has_value();},"OAuth grant timeout");
    OnlineMatchmakingOptions options;options.endpoint=argv[2];options.allow_loopback_http=true;options.maps={"lockout"};options.poll_interval=100ms;
    OnlineMatchmaking queue(login,options);std::vector<OnlineLocalPlayer> guests;
    for(unsigned i=1;!std::getenv("HALO2_SHADOW_GPU_MODE")&&i<4;++i)guests.push_back({"Guest "+account+" "+std::to_string(i),{}});
    queue.refresh("Pilot "+account,guests);wait([&]{return queue.snapshot().status==OnlineMatchmakingStatus::Ready;},"Native queue login timeout");queue.join("casual");
    std::shared_ptr<OnlineMatchSession> session;auto queue_diagnostic_at=Wall::now();
    wait([&]{
        session=queue.take_launch();
        if(Wall::now()>=queue_diagnostic_at) {
            const auto state=queue.snapshot();
            std::cout<<"ONLINE queue progress account="<<account<<" status="<<unsigned(state.status)
                <<" message="<<state.message<<" queued="<<state.queued_players<<" match="<<bool(state.match)
                <<" countdown_supported="<<state.start_countdown_supported<<" countdown="
                <<(state.start_countdown_ms?std::to_string(*state.start_countdown_ms):"unknown")<<std::endl;
            if(state.gathering)std::cout<<"ONLINE gathering seats="<<state.gathering->players.size()
                <<" required="<<state.gathering->minimum_players<<" maximum="<<state.gathering->maximum_players<<std::endl;
            queue_diagnostic_at=Wall::now()+5s;
        }
        return bool(session);
    },"Actual measured native match handoff timeout");queue.sign_out();
    wait([&]{return session->phase()==OnlineMatchSession::Phase::Loading;},"Measured direct host selection timeout");
    if(std::getenv("HALO2_SHADOW_GPU_MODE")) {
#ifdef HALO2_SHADOW_ONLINE_GPU
        check(session->roster().size()==2&&session->local_slots().size()==1&&session->direct_udp(),
            "ONLINE GPU proof did not admit two real one-view physical owners");
        gpu(session,files,account);return;
#else
        throw FormatError("CPU-only transport fixture cannot claim an ONLINE GPU frame");
#endif
    }
    check(session->roster().size()==8&&session->local_slots().size()==4&&session->direct_udp(),"Real session did not admit two four-seat physical owners");
    const auto self=session->self(),host=session->host();const bool hosting=self==host;const unsigned other=self==0?4:0;
    const auto locals=session->local_slots();std::vector<unsigned> host_locals;
    for(const auto &p:session->roster())if(online_admission_peer(p)==host)host_locals.push_back(p.slot);
    check(std::getenv("HALO2_TEST_SCRATCH"),"ONLINE native transport requires owned scratch");
    Owner world(argv[5],map.c_str(),locals,hosting?1000:900000,session->roster(),{},
        hosting?std::span<const unsigned>(locals):std::span<const unsigned>{});
    std::unique_ptr<Owner> workspace;
    if(!hosting)workspace=std::make_unique<Owner>(argv[5],map.c_str(),host_locals,500000,session->roster());
    // These are disposable local prototypes, deliberately different from the
    // actual independently published native identity on the producer PC.
    if(!hosting){auto &bank=world.world->memory();for(auto slot:locals){
        const auto row=source_raw_datum_lookup(world.world->source_player_pool(),world.match->player(slot).source_datum,storage(bank));
        auto identity=bank.view(row+4u,12);std::vector<std::byte> different(identity.begin(),identity.end());different[0]^=std::byte{0x80};bank.write(row+4u,different);
    }}
    const UiVariant variant{"Slayer",0,{{0x59,5}}};
    const auto fingerprint=multiplayer_fingerprint(map,map.parent_path(),0,world.cache.slayer_starting_weapon(5),argv[5]);
    std::unique_ptr<LanHost> native_host;std::unique_ptr<LanClient> native_client;
    std::shared_ptr<LostOrdinarySnapshots> snapshot_loss;
    if(hosting){
        native_host=std::make_unique<LanHost>(*world.match,session,fingerprint,session->roster(),variant);
        native_host->enable_shadow_lane(*world.authority,host_locals,session->roster());
    }else{
        std::array<std::int8_t,4> teams;std::array<SourcePlayerConfiguration,4> profiles;
        for(unsigned view=0;view<4;++view){teams[view]=std::int8_t(session->roster().at(locals[view]).team);
            profiles[view]=*world.match->player(locals[view]).source_profile_request;}
        snapshot_loss=std::make_shared<LostOrdinarySnapshots>(session);
        native_client=std::make_unique<LanClient>(snapshot_loss,host,fingerprint,teams,profiles);
        native_client->enable_shadow_lane(session);
    }
    session->loaded(fingerprint,variant);
    wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;},"Real direct session loading timeout");
    wait([&]{return session->authority_supported(other);},"Direct native authority capability timeout");
    std::array<Controls,16> controls{};std::array<Controls,4> input{};
    std::uint64_t imports{};bool source_admitted{},continuity_bound{},repaired_before_continuity{};
    bool repaired_gameplay_fenced{},historical_behind_gameplay{};
    std::optional<AcceptedShadowCut> previous;auto next_step=Wall::now(),diagnostic_at=next_step+5s;
    const auto deadline=next_step+150s;
    const auto done=files/"native-shadow-receiver-checked.json",host_done=files/"native-shadow-host-checked.json";
    while(true){
        const auto now=Wall::now();check(now<deadline,"ONLINE paired native presentation timed out");
        check(session->phase()==OnlineMatchSession::Phase::Running,"Actual ONLINE room left Running during native pairing");
        if(hosting){
            native_host->receive(controls);
            if(!source_admitted&&native_host->peers()==1){
                const auto players=world.authority->allocated_player_admissions();check(players.size()==8,"Actual ONLINE native admission omitted a couch seat");
                check(session->publish_source_players(*session->authority_context(),players),"Independent ONLINE source admission failed");source_admitted=true;
            }
            if(now>=next_step){
                for(unsigned slot=0;slot<16;++slot)if(world.match->player(slot).connected&&
                    world.match->player(slot).source_current_object!=null_datum)controls[slot].yaw=world.match->unit(slot).yaw;
                world.step(controls);native_host->update_shadow_roster(session->roster());native_host->broadcast();
                if(native_host->shadow_publication_pending())native_host->capture_shadow_state();
                next_step=now+33ms;
            }
            const auto stats=native_host->shadow_transport_statistics();
            if(std::filesystem::exists(done)&&stats.acknowledged>=3){
                report(files/("native-shadow-"+account+".json"),{{"passed",true},{"role","host"},{"scope","Production ONLINE direct-peer native gameplay/native-cut transport; fixture Discord; no GPU acceptance claim"},
                    {"captures",stats.captures},{"encoded",stats.encoded},{"published",stats.published},{"native_ack_receipts",stats.acknowledged},
                    {"full_bytes",stats.full_bytes},{"wire_bytes",stats.wire_bytes},{"capture_us",stats.capture_us},{"encode_us",stats.encode_us},
                    {"largest_capture_gap_ms",stats.largest_capture_gap_ms},{"backpressure",stats.backpressure},{"rejected",stats.rejected},{"error",stats.error}});
                report(host_done,{{"passed",true}});break;
            }
        }else{
            native_client->send(input);native_client->receive();
            check(native_client->error().empty(),native_client->error().c_str());
            if(!continuity_bound&&native_client->shadow_transport_statistics().received) {
                check(snapshot_loss->dropped&&native_client->transport_ready()&&!native_client->ready()&&
                    native_client->received_origins().empty()&&!native_client->state_origin().tick(),
                    "Repaired first ONLINE native body published before independently bound continuity");
                repaired_before_continuity=true;
                native_client->bind_continuity(*session->authority_context(),self,session->roster());continuity_bound=true;
            }
            if(imports<3)for(const auto &event:native_client->received_shadow_cuts()){
                const auto origin=event.origin();
                const auto cut=native_client->shadow_cut(origin);check(cut&&native_client->owns_shadow_cut(*cut,origin),"ONLINE historical body escaped its exact native cut");
                check(origin.context()==session->authority_context()&&origin.tick()&&origin.sequence(),"ONLINE origin lost its independently admitted current authority");
                check(origin.accepted_state().tick==*origin.tick()&&cut->local_views().size()==4&&std::ranges::equal(cut->local_views(),locals),"ONLINE native cut changed recipient couch ownership");
                const auto gameplay=native_client->state_origin();
                check(gameplay.tick()&&gameplay.sequence()&&!gameplay.same(origin)&&!gameplay.same_connection(origin),
                    "Historical native cut substituted an ordinary gameplay receipt");
                const auto proof_kind_rejected=[&](const MultiplayerSnapshotOrigin& receipt) {
                    try{native_client->prove_snapshot(LanHost::Continuation{},ValidatedAuthorityCut{},receipt);}
                    catch(const FormatError& e){return std::string_view(e.what()).find("Historical native or repaired gameplay receipt")!=std::string_view::npos;}
                    return false;
                };
                check(proof_kind_rejected(origin),"Historical ONLINE native body escaped the candidate/recovery fence");
                if(!imports) {
                    check(gameplay.sequence()==origin.sequence()&&gameplay.tick()==origin.tick()&&proof_kind_rejected(gameplay),
                        "Lost ordinary snapshots did not repair from their actual newer encoder body with a distinct authority fence");
                    repaired_gameplay_fenced=true;snapshot_loss->lose=false;
                } else if(std::int32_t(*gameplay.sequence()-*origin.sequence())>0) {
                    check(native_client->state().tick>=*origin.tick(),"Historical ONLINE native cut rolled gameplay clock back");
                    if(!native_client->received_origins().empty())check(gameplay.same(native_client->received_origins().back()),
                        "Historical ONLINE native cut replaced a newer ordinary receive-batch origin");
                    historical_behind_gameplay=true;
                }
                check(!native_client->acknowledged_shadow_cut(*cut)&&!native_client->owns_native_render_pair(*cut,gameplay),
                    "ONLINE native pair claimed completion before real native import");
                check(session->source_players()&&std::ranges::equal(cut->admissions(),session->source_players()->players),"Presentation trusted native identities from its cut instead of the independent ONLINE lane");
                const auto saved=decode_source_authority_image(cut->archive_image(),source_shadow_archive_codec_limits());
                check(saved.match.simulation.ticks==*origin.tick()&&saved.match.admitted_binding==cut->binding(),"ONLINE native cut was not the accepted body tick");
                std::vector<SourcePlayerAdmission> local_admissions;
                for(auto slot:locals){const auto at=std::ranges::find(cut->admissions(),slot,&SourcePlayerAdmission::slot);check(at!=cut->admissions().end(),"ONLINE native cut omitted local identity");local_admissions.push_back(*at);}
                auto prepared=world.authority->prepare_with_local_views(saved,cut->binding(),local_admissions,*workspace->authority,cut->admissions(),cut->producer_views());
                check(world.authority->commit(std::move(prepared)),"Actual ONLINE native recipient prepare/commit failed");
                local_views(world,locals);check(world.authority->execution_tick()==*origin.tick(),"ONLINE native recipient imported another tick");
                if(previous){check(previous->same_world(*cut)&&previous->same_generation(*cut),"Unchanged ONLINE native world lost its exact admission generation");
                    check(!native_client->owns_shadow_cut(*cut,previous->origin()),"An ONLINE native cut forged a different accepted gameplay body");}
                check(native_client->acknowledge_shadow_cut(*cut),"Actual ONLINE native preparation did not ACK");
                check(native_client->acknowledged_shadow_cut(*cut)&&native_client->owns_native_render_pair(*cut,gameplay),
                    "Actual ONLINE native import/ACK did not authorize its live historical/gameplay pair");previous=*cut;++imports;
                std::cout<<"PASS production ONLINE paired cut "<<imports<<" tick="<<*origin.tick()<<" sequence="<<*origin.sequence()<<" actual four-view native rebind/import before ACK\n";
                if(imports==3){report(done,{{"passed",true},{"imports",imports}});break;}
            }
            if(imports==3&&std::filesystem::exists(host_done)){
                const auto stats=native_client->shadow_transport_statistics();
                check(repaired_before_continuity&&continuity_bound&&snapshot_loss->dropped&&stats.acknowledged>=3&&
                    repaired_gameplay_fenced&&historical_behind_gameplay,
                    "ONLINE first-body-loss control did not progress real native imports and ACKs");
                check(!session->received_authority(),"Presentation lane populated unrelated recovery authority readiness");
                report(files/("native-shadow-"+account+".json"),{{"passed",true},{"role","joining-client"},{"scope","Production ONLINE direct-peer paired snapshot origins, independent accepted eight-seat native identity, actual four-view native import before ACK; no GPU acceptance claim"},
                    {"imports",imports},{"received",stats.received},{"acknowledged",stats.acknowledged},{"rejected",stats.rejected},{"error",stats.error},
                    {"ordinary_snapshot_datagrams_dropped",snapshot_loss->dropped},{"repaired_before_continuity",repaired_before_continuity},
                    {"repaired_gameplay_candidate_recovery_fenced",repaired_gameplay_fenced},
                    {"later_historical_cut_preserved_newer_gameplay",historical_behind_gameplay}});break;
            }
        }
        if(now>=diagnostic_at){const auto stats=hosting?native_host->shadow_transport_statistics():native_client->shadow_transport_statistics();
            std::cout<<"ONLINE native stream progress "<<(hosting?"host":"client")<<" captures="<<stats.captures<<" encoded="<<stats.encoded<<" published="<<stats.published
                <<" received="<<stats.received<<" nativeACK="<<stats.acknowledged<<" imports="<<imports<<" rejected="<<stats.rejected<<" error="<<stats.error<<std::endl;diagnostic_at=now+5s;}
        std::this_thread::sleep_for(2ms);
    }
}
}
int main(int argc,char **argv){std::cout<<std::unitbuf;int status=0;
    try{run(argc,argv);}catch(const std::exception &e){std::cerr<<e.what()<<'\n';status=1;}
    rtc::Cleanup().wait();return status;
}
