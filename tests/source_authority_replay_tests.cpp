#include "source_match_rebind_fixture.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include <curl/curl.h>
#include <openssl/sha.h>
#include <rtc/global.hpp>
#include <fstream>
#include <thread>

namespace {
using namespace std::chrono_literals;
using Wall=std::chrono::steady_clock;
template<class F>void wait(F ready,const char *stage){const auto until=Wall::now()+180s;
    while(!ready()){check(Wall::now()<until,stage);std::this_thread::sleep_for(5ms);}}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
void put(std::vector<std::byte> &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
auto seal_bytes(const AuthorityCutSeal &seal){std::vector<std::byte> bytes;
    put(bytes,seal.id,8);put(bytes,seal.tick,8);put(bytes,seal.bytes,8);
    bytes.insert(bytes.end(),seal.digest.begin(),seal.digest.end());return bytes;}
void write(const std::filesystem::path &path,std::span<const std::byte> bytes){const auto temp=path.string()+".tmp";
    {std::ofstream out(temp,std::ios::binary);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());check(bool(out),"Receiver fixture write failed");}
    std::filesystem::rename(temp,path);
}
std::vector<std::byte> read(const std::filesystem::path &path){std::ifstream in(path,std::ios::binary);std::vector<char> data((std::istreambuf_iterator<char>(in)),{});
    check(bool(in)||in.eof(),"Receiver fixture read failed");std::vector<std::byte> bytes(data.size());std::copy(data.begin(),data.end(),reinterpret_cast<char *>(bytes.data()));return bytes;}
void marker(const std::filesystem::path &path){const std::array<std::byte,1> done{std::byte{1}};write(path,done);}
// Encode an attack through the trusted local tape codec, bypassing the new
// sender-side policy. The recipient must reject the complete legacy batch
// before executing even the valid prefix or touching its qualified candidate.
std::vector<std::byte> legacy_replay_payload(const SourceReplayBatch &batch){
    auto shell=batch;for(auto &step:shell.steps)step.phase={};
    auto bytes=encode_source_replay_batch(shell);check(bytes.size()>=188,"Replay envelope fixture header changed");bytes.resize(188);
    for(const auto &step:batch.steps){const auto phase=encode_source_execution_phase(step.phase);
        put(bytes,step.before_tick,8);put(bytes,step.after_tick,8);put(bytes,phase.size(),4);bytes.insert(bytes.end(),phase.begin(),phase.end());}
    return bytes;
}
void alter(SourceMatchAuthority::Checkpoint &cut,std::uint32_t address){for(auto &region:cut.world.world.bank.regions)if(address>=region.address&&std::uint64_t(address)<std::uint64_t(region.address)+region.bytes.size()){
    region.bytes[address-region.address]^=std::byte{0x80};return;}throw FormatError("Forged remote identity fixture address missing");}
class CheckedDatagrams final:public MultiplayerDatagrams {
    std::shared_ptr<OnlineMatchSession> session_;
public:
    bool recording{};
    std::vector<std::pair<unsigned,std::vector<std::byte>>> packets;
    explicit CheckedDatagrams(std::shared_ptr<OnlineMatchSession> s):session_(std::move(s)){}
    void send(std::span<const std::byte> bytes,unsigned peer)override {
        check(bytes.size()>=7,"Native gameplay library emitted a truncated datagram");
        const auto protocol=std::to_integer<unsigned>(bytes[4])+(std::to_integer<unsigned>(bytes[5])<<8);
        if(protocol!=native_multiplayer_protocol)throw FormatError("Native gameplay library protocol "+std::to_string(protocol)+" differs from executor protocol "+std::to_string(native_multiplayer_protocol));
        if(recording)packets.emplace_back(peer,std::vector<std::byte>(bytes.begin(),bytes.end()));
        session_->send(bytes,peer);
    }
    bool receive(std::vector<std::byte> &bytes,unsigned &peer)override{return session_->receive(bytes,peer);}
};
class QuietDatagrams final:public MultiplayerDatagrams {
public:
    std::vector<std::pair<unsigned,std::vector<std::byte>>> packets;
    void send(std::span<const std::byte> bytes,unsigned peer)override{packets.emplace_back(peer,std::vector<std::byte>(bytes.begin(),bytes.end()));}
    bool receive(std::vector<std::byte> &,unsigned &)override{return false;}
};
auto packet_bytes(const auto &packets){
    std::vector<std::byte> result;
    const auto put=[&](std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)result.push_back(std::byte(value>>(i*8)));};
    for(const auto &[peer,bytes]:packets){put(peer,1);put(bytes.size(),4);result.insert(result.end(),bytes.begin(),bytes.end());}return result;
}
auto body_hash(std::span<const std::byte> bytes){std::array<std::byte,32> result{};
    check(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(result.data())),"Cannot hash paired native fixture");return result;}
void run(int argc,char **argv){
    check(argc==7,"Supply ROOT ORIGIN OWNER CALLBACK_PORT XBE PRIVATE_FIXTURE_DIRECTORY");
    const std::filesystem::path root=argv[1],files=argv[6];const auto map=root/"assets/disc/maps/lockout.map";
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
    for(unsigned i=1;i<4;++i)guests.push_back({"Guest "+account+" "+std::to_string(i),{}});
    queue.refresh("Pilot "+account,guests);wait([&]{return queue.snapshot().status==OnlineMatchmakingStatus::Ready;},"Native queue login timeout");queue.join("casual");
    std::shared_ptr<OnlineMatchSession> session;wait([&]{session=queue.take_launch();return bool(session);},"Actual measured native match handoff timeout");queue.sign_out();
    wait([&]{return session->phase()==OnlineMatchSession::Phase::Loading;},"Measured direct host selection timeout");
    check(session->roster().size()==8&&session->local_slots().size()==4&&session->direct_udp(),"Real session did not admit two four-seat physical owners");
    const auto self=session->self(),host=session->host();const bool hosting=self==host;const unsigned other=self==0?4:0;
    const auto locals=session->local_slots();std::vector<unsigned> host_locals;
    for(const auto &p:session->roster())if(online_admission_peer(p)==host)host_locals.push_back(p.slot);
    const UiVariant variant{"Slayer",0,{{0x59,5},{10,0},{2,0}}};
    MultiplayerOptions gameplay;gameplay.online_match=session;gameplay.map=map;gameplay.shared_maps=map.parent_path();gameplay.gameplay_xbe=argv[5];
    gameplay.variant=variant;gameplay.local_players=4;gameplay.session_random_seed=4242;
    const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance{};std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
    for(auto slot:locals)gameplay.selected_profiles.push_back(native_selected_player_configuration(session->roster()[slot].name,appearance,-1));
    auto target_clock=std::make_shared<Clock>(),workspace_clock=std::make_shared<Clock>();target_clock->time=900000;workspace_clock->time=500000;target_clock->cycle_stride=workspace_clock->cycle_stride=0;
    auto host_clock=std::make_shared<Clock>();host_clock->time=1000;host_clock->cycle_stride=0;
    std::unique_ptr<MultiplayerPreparedWorld> host_world;std::unique_ptr<SourceMatchAuthority> authority;std::unique_ptr<LanHost> lan_host;std::unique_ptr<LanClient> lan_client;
    std::unique_ptr<SourceAuthorityReceiver> receiver;
    std::unique_ptr<SourceAuthorityReplayWorker> background;
    const bool retain_inflight=std::getenv("HALO2_RECEIVER_RETAIN_INFLIGHT");
    const bool reject_prefetch=std::getenv("HALO2_RECEIVER_REJECT_PREFETCH");
    const unsigned replayed_before_retention=82u;
    if(hosting){host_world=std::make_unique<MultiplayerPreparedWorld>(gameplay,std::nullopt,host_clock);for(unsigned i=0;i<locals.size();++i){const auto slot=locals[i];
        check(host_world->match.join(session->roster()[slot].name,-1,gameplay.selected_profiles[i],slot)==slot,"Actual host local source birth failed");}
        host_world->bind_source_local_views(locals);authority=std::make_unique<SourceMatchAuthority>(host_world->match,*host_world->source_objects);
    }else if(std::getenv("HALO2_HOST_CONTINUATION_CODEC")) {
        // This case owns the received-authority queue directly and validates
        // the new whole-world/host container before any pipeline integration.
    }else if(std::getenv("HALO2_RECEIVER_ASYNC")) {
        {SourceAuthorityReplayWorker empty(gameplay,target_clock,workspace_clock);
            rejects([&]{empty.checkpoint_paired({}).get();});
            rejects([&]{empty.retain_validated_world().get();});
            rejects([&]{empty.retain_validated_world().get();});}
        background=std::make_unique<SourceAuthorityReplayWorker>(gameplay,target_clock,workspace_clock);
        check(!background->state().ready,"Background executor acknowledged before independent admission");
        rejects([&]{background->checkpoint({}).get();});
    }else {receiver=std::make_unique<SourceAuthorityReceiver>(gameplay,SourceAuthorityStreamMode::Replay,target_clock,workspace_clock);
        check(!receiver->warm()&&!receiver->world()&&!receiver->state().ready,"Receiver warmed or acknowledged without independent source admissions");
        std::atomic<bool> rejected=false;{std::jthread wrong([&]{try{receiver->update();}catch(const std::exception &){rejected=true;}});}check(rejected,"Receiver allowed native work on another executor");}
    bool replay_paused{};
    const auto receive_update=[&]{if(receiver){if(replay_paused)receiver->prefetch();else receiver->update();}};
    const auto receive_state=[&]{return background?background->state():receiver->state();};
    const auto pause_replay=[&](bool paused){
        replay_paused=paused;
        if(background){check(background->pause_replay(paused),"Background replay scheduling request was rejected");
            wait([&]{return background->state().replay_paused==paused;},"Background replay scheduling mode was not acknowledged");}
    };
    const auto receive_world_exists=[&]{return background?background->state().warmed:bool(receiver->world());};
    const auto receive_checkpoint=[&](const ValidatedAuthorityCut &cut){return background?background->checkpoint(cut).get():receiver->checkpoint(cut);};
    const auto receive_paired=[&](const ValidatedAuthorityCut &cut){return background?background->checkpoint_paired(cut).get():receiver->checkpoint_paired(cut);};
    Cache fingerprint_cache(map,map.parent_path(),OriginalData::load_xbe(argv[5]));const auto weapon=fingerprint_cache.slayer_starting_weapon(5);
    const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(map,map.parent_path(),0,weapon,argv[5]),variant);
    session->loaded(fingerprint,variant);wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;},"Receiver real session loading timeout");
    const auto datagrams=std::make_shared<CheckedDatagrams>(session);
    if(hosting)lan_host=std::make_unique<LanHost>(host_world->match,datagrams,fingerprint,session->roster(),variant);
    else lan_client=std::make_unique<LanClient>(datagrams,host,fingerprint,std::span<const std::int8_t>{},gameplay.selected_profiles);
    std::array<Controls,16> controls{};std::array<Controls,4> neutral{};
    auto pump=[&]{if(lan_host){lan_host->receive(controls);lan_host->broadcast();}else{lan_client->send(neutral);lan_client->receive();}};
    wait([&]{pump();if(lan_client&&!lan_client->error().empty())throw FormatError(lan_client->error());
        if(session->phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session->error());
        return hosting?lan_host->peers()==1:lan_client->ready();},"Actual native grouped source profile admission timeout");
    wait([&]{pump();return session->authority_supported(other);},"Receiver direct authority capability timeout");
    if(std::getenv("HALO2_HOST_CONTINUATION_STREAM")){
        check(hosting||background,"Paired stream case requires the native replay worker");
        wait([&]{return session->authority_host_continuation_supported(other);},"Paired stream capability was not negotiated");
        const auto context=*session->authority_context();const auto binding=source_authority_binding(context);
        if(hosting){
            std::vector<unsigned> slots;for(const auto &p:session->roster())slots.push_back(p.slot);
            check(session->publish_source_players(context,authority->player_admissions(slots)),"Paired stream identity export failed");
            wait([&]{return session->source_players_confirmed(other)&&std::filesystem::exists(files/"paired-stream-ready");},"Paired stream receiver not admitted");
            SourceAuthorityPublisher publisher(*authority,*lan_host,session,other,90s);
            check(publisher.capture(),"Paired publisher refused initial native world/host capture");
            for(unsigned i=0;i<70;++i){check(publisher.execute([&]{
                std::array<Controls,16> input{};for(unsigned slot=0;slot<8;++slot)input[slot].yaw=host_world->match.unit(slot).yaw;
                host_world->match.step(input);lan_host->broadcast();}),"Paired publisher did not record a complete native frame");}
            bool overlapped{};wait([&]{publisher.update();const auto state=publisher.state();
                check(state.error.empty(),state.error.c_str());overlapped|=publisher.in_flight_cuts()==2;
                check(publisher.in_flight_cuts()<=2,"Paired publisher exceeded two admitted cuts");
                return state.acknowledged_phases==70&&state.host_ready&&state.ready&&!state.awaiting_validation&&!state.encoding&&state.queued_phases==0;
            },"Paired publisher/worker did not catch up");
            check(overlapped,"Paired source stream never prefetched a successor");
            auto world=encode_source_authority(authority->checkpoint(binding));auto ready=*publisher.state().ready;
            const auto write_paired=[&]{const LanHost::ContinuationBinding proof{context,{1,authority->execution_tick(),world.size(),body_hash(world)}};
                write(files/"paired-stream-world",world);write(files/"paired-stream-host",LanHost::encode_continuation(publisher.host_checkpoint(ready),proof));};
            write_paired();
            auto wrong=ready;++wrong.seal.tick;rejects([&]{publisher.host_checkpoint(wrong);});
            marker(files/"paired-stream-complete");
            wait([&]{return std::filesystem::exists(files/"paired-stream-export-checked");},"Paired worker did not complete its nonterminal exact export");
            for(unsigned i=0;i<2;++i)check(publisher.execute([&]{
                std::array<Controls,16> input{};for(unsigned slot=0;slot<8;++slot)input[slot].yaw=host_world->match.unit(slot).yaw;
                host_world->match.step(input);lan_host->broadcast();}),"Paired publisher did not continue after a nonterminal receiver export");
            wait([&]{publisher.update();const auto state=publisher.state();check(state.error.empty(),state.error.c_str());
                return state.acknowledged_phases==72&&state.host_ready&&state.ready&&!state.awaiting_validation&&!state.encoding&&state.queued_phases==0;
            },"Paired worker stopped accepting replay after its nonterminal export");
            world=encode_source_authority(authority->checkpoint(binding));ready=*publisher.state().ready;write_paired();
            marker(files/"paired-stream-next-complete");
            wait([&]{return std::filesystem::exists(files/"paired-stream-downgrade-ready");},"Paired worker did not reach downgrade preflight");
            // An otherwise valid native-only full cut may not replace an
            // already paired stream. Only the real peer path supplies bytes.
            const std::array<unsigned,1> targets{other};const auto stripped=session->publish_authority(context,ready.seal.tick,world,targets);
            write(files/"paired-stream-rejected-seal",seal_bytes(stripped));
            wait([&]{return std::filesystem::exists(files/"paired-stream-downgrade-rejected");},"Paired worker accepted a stripped host owner");
            check(session->validated_authority().empty(),"A native-only downgrade produced a validated paired receipt");
            wait([&]{return std::filesystem::exists(files/"paired-stream-retained");},"Paired worker did not retain both owners");
            publisher.cancel();check(!publisher.state().host_ready,"Cancelled paired publisher retained host readiness");
            rejects([&]{publisher.host_checkpoint(ready);});
            std::cout<<"PASS production paired publisher: 72 actual native frames, historical end-of-batch host state, bounded two-cut prefetch and exact validated host checkpoint\n";
        }else{
            wait([&]{return session->source_players().has_value();},"Paired stream source admissions did not arrive");
            lan_client->bind_continuity(context,self,session->roster());
            MultiplayerSnapshotOrigin origin;unsigned snapshot_receipts{};
            // Keep the ordinary client pump alive while the other executor
            // decodes/replays. An immutable receipt survives empty receives;
            // deferring all gameplay ingress until after 70 fragmented source
            // snapshots would test the bounded datagram queue instead.
            const auto pump_receipts=[&]{pump();for(const auto &receipt:lan_client->received_origins()){
                origin=receipt;++snapshot_receipts;}};
            check(background->pause_replay(true),"Cannot pause paired worker before prefetch");
            wait([&]{return background->state().replay_paused;},"Paired replay pause was not acknowledged");marker(files/"paired-stream-ready");
            wait([&]{pump_receipts();const auto s=background->state();check(s.error.empty(),s.error.c_str());
                return s.received>=3&&s.replaying;},"Paired worker did not receive full baseline and two replay cuts");
            {const auto s=background->state();check(!s.host_ready&&!s.ready&&!s.host_proof,"Partially applied paired stream exported readiness or proof");}
            check(background->pause_replay(false),"Cannot resume paired native replay");
            wait([&]{pump_receipts();const auto s=background->state();check(s.error.empty(),s.error.c_str());
                return s.host_ready&&s.ready&&!s.replaying&&s.replayed_phases==70&&std::filesystem::exists(files/"paired-stream-complete");},
                "Paired worker did not validate all native phases and host owners");
            auto good=background->state();
            check(good.host_proof&&good.host_proof->cut().peer==self&&good.host_proof->cut().context==context&&
                good.host_proof->cut().seal==good.ready->seal,"Paired replay worker omitted its exact validated host proof");
            wait([&]{if(origin.tick()!=good.ready->seal.tick)pump_receipts();
                return origin.context()==context&&origin.tick()==good.ready->seal.tick;},"Paired replay snapshot receipt did not reach native client");
            check(good.host_proof->prove_snapshot(*lan_client,origin).matches(origin),"Native replay proof lost its exact received snapshot");
            rejects([&]{good.host_proof->prove_snapshot(*lan_client,MultiplayerSnapshotOrigin{});});
            const auto original_ready=*good.ready;
            auto exported=background->checkpoint_paired(original_ready).get();
            check(exported.host&&exported.cut.peer==self&&exported.cut.context==original_ready.context&&exported.cut.seal==original_ready.seal&&
                exported.admissions==*session->source_players()&&exported.producer_views==host_locals&&exported.local_views==locals,
                "Nonterminal paired export lost its independently validated cut, host table, admissions or views");
            const auto exported_world=encode_source_authority(exported.checkpoint);
            const auto exported_hash=body_hash(exported_world);
            const LanHost::ContinuationBinding exported_binding{context,{1,exported.checkpoint.match.simulation.ticks,exported_world.size(),exported_hash}};
            const auto exported_host=LanHost::encode_continuation(*exported.host,exported_binding);
            check(exported_world==read(files/"paired-stream-world")&&exported_host==read(files/"paired-stream-host"),
                "Nonterminal paired export changed the producer's complete world or paired host table");
            auto wrong=original_ready;wrong.peer=host;rejects([&]{background->checkpoint_paired(wrong).get();});
            wrong=original_ready;++wrong.context.membership_revision;rejects([&]{background->checkpoint_paired(wrong).get();});
            wrong=original_ready;wrong.seal.digest[0]^=std::byte{1};rejects([&]{background->checkpoint_paired(wrong).get();});
            wrong=original_ready;++wrong.seal.tick;rejects([&]{background->checkpoint_paired(wrong).get();});
            const auto unchanged=background->state();
            check(!background->stopped()&&!unchanged.replay_paused&&unchanged.ready&&unchanged.ready->seal==original_ready.seal&&
                unchanged.host_ready&&unchanged.host_proof&&unchanged.replayed_phases==70&&
                encode_source_authority(background->checkpoint_paired(original_ready).get().checkpoint)==exported_world,
                "Rejected or repeated paired export stopped replay or changed the validated world");
            marker(files/"paired-stream-export-checked");
            wait([&]{pump_receipts();const auto s=background->state();check(s.error.empty(),s.error.c_str());
                return s.host_ready&&s.ready&&!s.replaying&&s.replayed_phases==72&&std::filesystem::exists(files/"paired-stream-next-complete");},
                "Nonterminal paired export prevented later authenticated replay validation");
            good=background->state();
            check(good.ready->seal!=original_ready.seal&&good.ready->seal.tick==original_ready.seal.tick+2&&good.host_proof,
                "Paired continuation reused the exported cut instead of validating later native phases");
            rejects([&]{background->checkpoint_paired(original_ready).get();});
            auto continued=background->checkpoint_paired(*good.ready).get();
            check(continued.host&&continued.cut.seal==good.ready->seal&&continued.checkpoint.match.simulation.ticks==original_ready.seal.tick+2&&
                encode_source_authority(continued.checkpoint)==read(files/"paired-stream-world")&&
                body_hash(encode_source_authority(exported.checkpoint))==exported_hash&&
                LanHost::encode_continuation(*exported.host,exported_binding)==exported_host,
                "Later native replay relabeled an old export or invalidated its owning world/host values");
            wait([&]{if(origin.tick()!=good.ready->seal.tick)pump_receipts();
                return origin.context()==context&&origin.tick()==good.ready->seal.tick;},"Later paired snapshot did not reach the existing native client");
            check(good.host_proof->prove_snapshot(*lan_client,origin).matches(origin),"Later independently validated host proof lost its authentic receipt");
            marker(files/"paired-stream-downgrade-ready");
            wait([&]{const auto s=background->state();return s.received>good.received&&!s.decoding&&!s.error.empty();},"Paired stream did not reject a stripped host owner");
            const auto rejected=background->state();check(rejected.host_ready&&rejected.ready&&rejected.ready->seal==good.ready->seal&&
                rejected.ready->context==good.ready->context&&!rejected.replaying&&rejected.replayed_phases==72&&
                rejected.host_proof&&rejected.host_proof->cut().seal==good.host_proof->cut().seal,
                "Rejected downgrade replaced the last validated paired world");
            check(rejected.rejected&&rejected.rejected->peer==self&&rejected.rejected->context==context&&
                seal_bytes(rejected.rejected->seal)==read(files/"paired-stream-rejected-seal"),
                "Rejected paired downgrade omitted its exact delivered native failure receipt");
            check(encode_source_authority(background->checkpoint(*good.ready).get())==read(files/"paired-stream-world"),
                "Rejected native-only downgrade mutated the last validated world");
            marker(files/"paired-stream-downgrade-rejected");
            auto retained=background->retain_validated_world().get();check(retained.host.has_value(),"Paired retention dropped its host owner");
            rejects([&]{background->checkpoint_paired(*good.ready).get();});
            const auto encoded=encode_source_authority(retained.checkpoint);
            check(encoded==read(files/"paired-stream-world"),"Paired retained native world differs from the producer");
            const LanHost::ContinuationBinding proof{context,{1,retained.checkpoint.match.simulation.ticks,encoded.size(),body_hash(encoded)}};
            check(LanHost::encode_continuation(*retained.host,proof)==read(files/"paired-stream-host"),"Paired retention kept a stale or mismatched host table");
            background->cancel();{const auto s=background->state();check(!s.host_ready&&!s.ready&&!s.host_proof&&!s.rejected,"Cancelled paired worker retained readiness, proof or rejected-cut state");}background.reset();
            check(good.host_proof->prove_snapshot(*lan_client,origin).matches(origin),"Historical owning proof borrowed the destroyed native worker");
            check(body_hash(encode_source_authority(exported.checkpoint))==exported_hash&&
                LanHost::encode_continuation(*exported.host,exported_binding)==exported_host&&
                encode_source_authority(continued.checkpoint)==encoded,
                "Nonterminal paired exports retained a borrower into the destroyed native worker");
            std::cout<<"PASS paired stream retains "<<snapshot_receipts<<" accepted gameplay receipts beside native replay; exact host proof survives downgrade rejection and worker destruction\n";
            auto world=MultiplayerPreparedWorld::source_shadow(gameplay,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,workspace_clock);
            SourceMatchAuthority imported(world->match,*world->source_objects);
            auto prepared=imported.prepare(retained.checkpoint,binding,retained.admissions.players);
            check(imported.commit(std::move(prepared)),"Paired retained world failed separate-executor import");
            auto quiet=std::make_shared<QuietDatagrams>();auto resumed=LanHost::resume(world->match,quiet,fingerprint,session->roster(),*retained.host,binding,retained.admissions.players);
            check(encode_source_authority(imported.checkpoint(binding))==encoded,"Paired host reconstruction changed the imported world");
            marker(files/"paired-stream-retained");wait([&]{return session->phase()==OnlineMatchSession::Phase::Failed;},"Paired original host did not shut down");
            for(auto slot:host_locals){const auto deaths=world->match.player(slot).deaths;world->match.depart(slot);
                check(world->match.player(slot).deaths==deaths,"Paired old-host departure fabricated a death");}
            const auto tick=world->match.simulation().ticks();for(unsigned i=0;i<10;++i){std::array<Controls,16> input{};
                for(unsigned slot=0;slot<8;++slot)input[slot].yaw=world->match.unit(slot).yaw;world->match.step(input);resumed->broadcast();}
            check(world->match.simulation().ticks()==tick+10&&!world->match.finished(),"Paired recovery did not survive the original host");
            std::cout<<"PASS nonterminal exact paired exports: unchanged 70-phase boundary, continued receipt/replay to72, stale/foreign/seal rejection, owning world/host lifetime and preserved terminal retention\n";
            std::cout<<"PASS exact rejected native cut receipt: delivered paired downgrade context, physical receiver and complete seal; preserved validated world and retired failure on cancellation\n";
            std::cout<<"PASS production paired receiver/worker: 72 validated native phases, complete world+host equality, worker destruction, separate executor reconstruction, actual host loss and ten surviving ticks\n";
            std::cout<<"LIMIT producer view mapping and quiet local snapshot endpoint; live election/generation/controller/voice promotion remains external\n";
        }
        return;
    }
    if(std::getenv("HALO2_HOST_CONTINUATION_CODEC")){
        const auto context=*session->authority_context();const auto binding=source_authority_binding(context);
        const auto received_cut=[&]{std::optional<ReceivedAuthorityCut> cut;
            wait([&]{cut=session->received_authority();return bool(cut);},"Complete host continuation did not arrive over direct peers");
            check(cut->peer==host&&cut->context==context,"Host continuation arrived from a different authority");return *cut;};
        const auto validated=[&](const AuthorityCutSeal &seal){
            wait([&]{for(const auto &v:session->validated_authority())if(v.peer==other&&v.context==context&&v.seal==seal)return true;return false;},
                "Complete host continuation did not receive native validation");};
        if(hosting){
            std::vector<unsigned> slots;for(const auto &p:session->roster())slots.push_back(p.slot);
            check(session->publish_source_players(context,authority->player_admissions(slots)),"Host-cut native identity export failed");
            wait([&]{return session->source_players_confirmed(other)&&std::filesystem::exists(files/"host-cut-ready");},"Host-cut receiver admission timeout");
            const auto captured=capture_source_host_continuation(*lan_host,context,1);
            const std::array<unsigned,1> targets{other};
            const auto seal=session->publish_authority(context,authority->execution_tick(),encode_source_host_continuation(captured),targets);
            validated(seal);SourceReplayBatch next{{context,seal,seal,1},{}};datagrams->recording=true;
            for(unsigned i=0;i<4;++i){const auto before=authority->execution_tick();
                auto phase=authority->capture_execution_phase([&]{
                    std::array<Controls,16> input{};for(unsigned slot=0;slot<8;++slot)input[slot].yaw=host_world->match.unit(slot).yaw;
                    host_world->match.step(input);lan_host->broadcast();});
                next.steps.push_back({before,authority->execution_tick(),std::move(phase)});
            }
            write(files/"host-cut-world",encode_source_authority(authority->checkpoint(binding)));
            write(files/"host-cut-packets",packet_bytes(datagrams->packets));datagrams->recording=false;
            const auto replay=session->publish_authority(context,authority->execution_tick(),encode_source_replay_batch(next),targets);
            validated(replay);wait([&]{return std::filesystem::exists(files/"host-cut-replayed");},"Encoded host continuation did not replay");
            std::cout<<"PASS complete world/host bytes and four actual native phases sent over authenticated direct UDP, no master media relay\n";
        }else{
            wait([&]{return session->source_players().has_value();},"Host-cut source identities were not independently admitted");
            const auto accepted=*session->source_players();marker(files/"host-cut-ready");const auto first=received_cut();
            auto values=decode_source_host_continuation(*first.bytes,context,fingerprint,session->roster(),accepted.players);
            auto world=MultiplayerPreparedWorld::source_shadow(gameplay,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,target_clock);
            SourceMatchAuthority imported(world->match,*world->source_objects);auto prepared=imported.prepare(values.world,binding,accepted.players);
            check(imported.commit(std::move(prepared)),"Wire host continuation did not import its complete source world");
            const auto before=encode_source_authority(imported.checkpoint(binding));auto quiet=std::make_shared<QuietDatagrams>();
            auto resumed=LanHost::resume(world->match,quiet,fingerprint,session->roster(),values.host,binding,accepted.players);
            check(encode_source_authority(imported.checkpoint(binding))==before&&resumed->peers()==1,
                "Wire host continuation mutated/rejoined native players before actual execution");
            check(session->acknowledge_authority(context,first.seal),"Wire host continuation could not acknowledge native preparation");
            const auto next=received_cut();const auto batch=decode_source_replay_batch(*next.bytes,{context,first.seal,first.seal,1});
            check(batch.steps.size()==4,"Wire host continuation lost its complete native phases");
            for(const auto &step:batch.steps)imported.replay_execution_phase(step.phase,[&]{
                std::array<Controls,16> input{};for(unsigned slot=0;slot<8;++slot)input[slot].yaw=world->match.unit(slot).yaw;
                world->match.step(input);resumed->broadcast();});
            check(encode_source_authority(imported.checkpoint(binding))==read(files/"host-cut-world")&&
                packet_bytes(quiet->packets)==read(files/"host-cut-packets"),"Wire host continuation changed the full native world or complete actual snapshot bytes");
            check(session->acknowledge_authority(context,next.seal),"Wire host replay could not acknowledge actual native validation");
            marker(files/"host-cut-replayed");wait([&]{return session->phase()==OnlineMatchSession::Phase::Failed;},"Old host did not actually close its transport");
            for(auto slot:host_locals){const auto deaths=world->match.player(slot).deaths;world->match.depart(slot);
                check(world->match.player(slot).deaths==deaths,"Wire host loss fabricated a numerical death");}
            const auto tick=world->match.simulation().ticks();for(unsigned i=0;i<10;++i){std::array<Controls,16> input{};
                for(unsigned slot=0;slot<8;++slot)input[slot].yaw=world->match.unit(slot).yaw;world->match.step(input);resumed->broadcast();}
            check(world->match.simulation().ticks()==tick+10&&!world->match.finished(),"Decoded world/host did not survive original host loss");
            std::cout<<"PASS direct-peer full world/host container, independent eight-seat admission, native world/camera/connection import, four replayed native phases and exact snapshots, actual host shutdown, original departures and ten private surviving ticks\n";
            std::cout<<"LIMIT retained producer view mapping and quiet local snapshot endpoint; live generation/role/controller/voice promotion and WAN/hardware acceptance remain external\n";
        }
        return;
    }
    if(hosting){
        const auto context=*session->authority_context();const auto binding=source_authority_binding(context);
        std::vector<unsigned> slots;for(const auto &p:session->roster())slots.push_back(p.slot);
        check(session->publish_source_players(context,authority->player_admissions(slots)),"Replay source admission export failed");
        // Native game/profile admission is already complete. Keep simulation
        // quiescent while the other executor loads its private worlds; polling
        // LanHost's gameplay-input timeout here would manufacture a departure
        // during a slow/instrumented loading operation. DTLS stays live on its
        // independent production transport worker.
        wait([&]{return session->source_players_confirmed(other)&&std::filesystem::exists(files/"receiver-ready");},"Replay receiver not admitted");
        for(const auto &p:session->roster())check(!(host_world->match.player(p.slot).source_flags&2u),"Loading manufactured an admitted player's departure");
        // Gameplay admission is complete. From this point the peer transport
        // itself keeps the actual encrypted channel alive; no local test log
        // supplies source inputs to the receiver.
        for(unsigned i=0;i<20;++i){host_clock->time+=33;host_clock->filetime+=330000;
            for(unsigned slot=0;slot<8;++slot)controls[slot].yaw=host_world->match.unit(slot).yaw;host_world->match.step(controls);}
        auto bad=authority->checkpoint(binding);auto &bank=host_world->source_objects->memory();
        const auto row=source_raw_datum_lookup(host_world->source_objects->source_player_pool(),host_world->match.player(other).source_datum,storage(bank));
        alter(bad,row+4);const std::array<unsigned,1> targets{other};
        session->publish_authority(context,bad.match.simulation.ticks,encode_source_authority(bad),targets);
        wait([&]{return std::filesystem::exists(files/"bad-rejected");},"Replay mode accepted a forged full-roster identity");
        check(session->validated_authority().empty(),"Forged source identity produced a replay base receipt");
        SourceAuthorityPublisher publisher(*authority,session,other,SourceAuthorityStreamMode::Replay,90s);
        const auto frame=[&](unsigned number){
            host_clock->time+=33;host_clock->filetime+=330000;
            (void)host_world->match.state();
            for(unsigned slot=0;slot<8;++slot)controls[slot].yaw=host_world->match.unit(slot).yaw;
            controls[locals.front()].fire=number%8<2;controls[locals.front()].reload=number%16==12;
            host_world->match.step(controls);host_world->source_objects->advance_effect_particles(1.f/60.f);
            host_world->source_objects->begin_model_frame();
            for(unsigned view=0;view<4;++view){
                const auto eye=host_world->match.unit(host_locals[view]).body.position+Vec3{0,0,1.7f};
                host_world->source_objects->publish_native_observer_position(view,eye);
                const auto camera=source_frustum_camera({eye,{1,0,0},{0,0,1},.015625f,1024.f,{-1,1,-1,1}});
                SourcePortalVisibilitySet visible;visible.groups.push_back(camera);visible.frustums.push_back(camera.full_frustum);
                SourceClusterFrustumRanges cluster;cluster.source_cluster=0;cluster.counts[0]=1;visible.clusters.push_back(cluster);visible.sorted_cluster_indices.push_back(0);
                host_world->source_objects->publish_render_visibility(visible);host_world->match.advance_weather_view(1.+number/30.,std::int16_t(view));
                host_world->source_objects->render_model_view([&](auto &world){
                    (void)world.build_world_model(host_world->match.unit(host_locals[view]).source_datum,view,4,false,10.f,1.f);
                    (void)world.prepare_first_person_models(view,eye,{1,0,0},{0,0,1});
                });
            }
            host_world->match.advance_audio(1.f/60.f);(void)host_world->match.take_sounds();
            (void)host_world->match.take_prop_events();(void)host_world->match.take_grenade_events();
            (void)host_world->match.state();
            // Raw entrypoints/ABI/device writes are local-only. Their complete
            // native behavior remains covered by source_execution_calls_native;
            // this network cohort exercises the authorized typed call grammar.
            const auto origin=host_world->match.unit(host_locals[0]).body.position+Vec3{0,0,2.f};
            (void)host_world->source_objects->particle_collision(0x800005u,origin,{0,0,-100.f});
            (void)host_world->source_objects->particle_collision(0,origin,{0,0,1.f});
        };
        std::uint64_t acknowledged{};std::array<AuthorityCutSeal,4> bases;bool overlapped{};
        const auto stage=[&](unsigned number,unsigned count){
            check(publisher.capture(),"Replay base capture refused");
            const auto remember_base=[&]{if(!bases[number-1].id&&publisher.state().ready)bases[number-1]=publisher.state().ready->seal;};
            for(unsigned i=0;i<count;++i){
                const auto actual_frame=[&]{auto scope=publisher.record_frame();
                    if(!scope)throw FormatError("Complete native frame did not start recording: "+publisher.state().error);
                    frame(i);if(i%2==0)return; // Same RAII close used by an early game-loop continue.
                    if(!scope->finish())throw FormatError("Complete native frame did not finish recording: "+publisher.state().error);};
                actual_frame();remember_base();std::this_thread::sleep_for(5ms);
            }
            write(files/("complete-"+std::to_string(number)),encode_source_authority(authority->checkpoint(binding)));
            acknowledged+=count;
            wait([&]{publisher.update();remember_base();const auto state=publisher.state();check(state.error.empty(),state.error.c_str());
                overlapped|=publisher.in_flight_cuts()==2;check(publisher.in_flight_cuts()<=2,"Native publisher exceeded its two-cut bound");
                return state.acknowledged_phases==acknowledged&&state.ready&&!state.awaiting_validation&&!state.encoding&&
                    state.queued_phases==0&&std::filesystem::exists(files/("stage-"+std::to_string(number)+"-checked"));},"Ordered native peer replay did not catch up");
            std::cout<<"PASS native replay stage "<<number<<": "<<count<<" phases, tick="<<authority->execution_tick()<<", batches="<<publisher.state().replay_batches
                <<", last payload="<<publisher.state().payload_bytes<<" bytes; complete producer-view source world validated remotely\n";
        };
        stage(1,70);
        check(overlapped,"Actual native replay did not overlap successor delivery with previous validation");
        const auto valid=*publisher.state().ready;const auto stamp=authority->execution_boundary();bank.validate_mutable_view(0x4c65f0u,8);publisher.update();
        check(authority->execution_boundary()==stamp&&publisher.state().tracking_replay&&publisher.state().ready->seal==valid.seal,
            "Permission-only admission broke a live replay stream");
        const auto same=bank.view(0x4c65f0u,8);bank.write(0x4c65f0u,same);publisher.update();
        check(!publisher.state().tracking_replay&&!publisher.state().ready&&publisher.state().error=="Unrecorded native work between replay phases",
            "An inter-phase raw write retained a qualified replay stream");
        const auto before=authority->execution_tick();check(!publisher.execute([&]{host_clock->time+=33;host_clock->filetime+=330000;host_world->match.step(controls);}),
            "Unqualified stream reported a recording");check(authority->execution_tick()==before+1,"Stream invalidation stopped or repeated actual host work");
        stage(2,8);
        const auto before_failed=authority->execution_tick();check(!publisher.execute([&]{frame(100);const auto bytes=bank.view(0x4c65f0u,8);bank.write(0x4c65f0u,bytes);}),
            "Unknown in-phase raw write was qualified");
        check(authority->execution_tick()==before_failed+1&&!publisher.state().ready&&!publisher.state().tracking_replay,
            "Failed recording stopped the host or retained a promotable stream");
        stage(3,2);
        const auto last=*publisher.state().ready;
        check(publisher.execute([]{}),"Cancellation no-op phase was not recorded");publisher.update();publisher.cancel();
        wait([&]{publisher.update();return !publisher.state().encoding;},"Cancelled replay encoder did not finish");
        check(!publisher.state().ready&&!publisher.state().tracking_replay&&publisher.state().queued_phases==0,"Cancelled replay encoder revived its stream");
        marker(files/"cancel-checked");
        SourceReplayBinding chain{context,bases[2],last.seal,3};SourceReplayBatch wrong_chain{chain,{{last.seal.tick,last.seal.tick,{}}}};
        ++wrong_chain.binding.first_sequence;session->publish_authority(context,last.seal.tick,encode_source_replay_batch(wrong_chain),targets);
        wait([&]{return std::filesystem::exists(files/"chain-rejected");},"Wrong replay sequence was not rejected");
        check(session->validated_authority().empty(),"Wrong replay sequence acknowledged a world");
        const auto identity_row=source_raw_datum_lookup(host_world->source_objects->source_player_pool(),host_world->match.player(other).source_datum,storage(bank));
        const auto original_view=bank.view(identity_row+4,12);std::vector<std::byte> original(original_view.begin(),original_view.end()),changed=original;changed[0]^=std::byte{0x80};
        SourceReplayBatch identity{chain,{}};
        if(reject_prefetch)for(unsigned i=0;i<31;++i){const auto before=authority->execution_tick();
            auto phase=authority->capture_execution_phase([&]{frame(i+140);});
            identity.steps.push_back({before,authority->execution_tick(),std::move(phase)});}
        auto forged=authority->capture_execution_phase([&]{host_world->source_objects->tu15_world().write_source_call_input(identity_row+4,changed);});
        bank.write(identity_row+4,original);
        identity.steps.push_back({authority->execution_tick(),authority->execution_tick(),std::move(forged)});
        const auto invalid_tick=authority->execution_tick();
        const auto invalid_payload=legacy_replay_payload(identity);
        session->publish_authority(context,invalid_tick,invalid_payload,targets);
        wait([&]{return std::filesystem::exists(files/"replay-identity-rejected");},"Raw replay capability was not rejected before execution");
        check(session->validated_authority().empty(),"Replayed identity change acknowledged a world");
        stage(4,2);
        if(retain_inflight){
            wait([&]{return std::filesystem::exists(files/"retention-ready");},"Retention receiver did not establish its receive baseline");
            const auto final=*publisher.state().ready;
            publisher.cancel();
            // Two authenticated batches must survive host loss while the
            // first is still executing and the second has been prefetched.
            const auto record=[&](SourceReplayBinding binding,unsigned count){
                SourceReplayBatch batch{binding,{}};
                for(unsigned i=0;i<count;++i){const auto before=authority->execution_tick();
                    auto phase=authority->capture_execution_phase([&]{frame(i+200);});
                    batch.steps.push_back({before,authority->execution_tick(),std::move(phase)});}
                return batch;
            };
            auto batch=record({context,bases[3],final.seal,3},32);
            const auto first_tick=authority->execution_tick();const auto first_payload=encode_source_replay_batch(batch);
            auto next=record({context,bases[3],{},35},8);
            const auto first=session->publish_authority(context,first_tick,first_payload,targets);
            next.binding.previous=first;const auto next_payload=encode_source_replay_batch(next);
            wait([&]{return session->authority_delivered(other,context,first);},"Retention predecessor bytes were not delivered");
            session->publish_authority_ahead(context,first,authority->execution_tick(),next_payload,other);
            write(files/"complete-retained",encode_source_authority(authority->checkpoint(binding)));
        }
        wait([&]{return std::filesystem::exists(files/"ready-before-host-loss");},"Replay receiver did not retain the final source cut");
        std::cout<<"PASS native replay recording gaps, real-host continuation, complete resynchronization and encoder cancellation\n";
    }else {
        wait([&]{pump();return session->source_players().has_value();},"Replay source roster timeout");
        if(receiver){receiver->update();check(!receiver->world(),"Metadata started a replay world without a cut");
            if(!std::getenv("HALO2_RECEIVER_COLD_FROM_CUT")){check(receiver->warm(),"Producer replay target warm failed");check(receiver->warm(),"Recipient handoff workspace warm failed");}}
        marker(files/"receiver-ready");bool rejected=false,chain_rejected=false,identity_rejected=false;unsigned stage=1;
        std::uint64_t identity_receive_baseline{},identity_replay_baseline{};
        wait([&]{receive_update();const auto state=receive_state();
            if(!rejected&&!state.error.empty()){
                check(state.error.find("full source player identity")!=std::string::npos&&state.prepared==0&&!state.ready,"Replay mode bypassed independent full-roster identities");
                rejected=true;marker(files/"bad-rejected");}
            if(stage==4&&!chain_rejected&&state.error.find("Invalid source replay batch")!=std::string::npos){
                check(state.ready&&receive_world_exists()&&state.prepared==3,"Wrong sequence mutated the last qualified world");
                chain_rejected=true;identity_receive_baseline=state.received;identity_replay_baseline=state.replayed_phases;
                marker(files/"chain-rejected");}
            if(chain_rejected&&!identity_rejected&&state.error.find("local-only execution capability")!=std::string::npos){
                check(state.ready&&receive_world_exists()&&state.replayed_phases==identity_replay_baseline,
                    "Denied raw replay changed or discarded the previously qualified world");
                check(state.received>=identity_receive_baseline+1,"Denied packet was not received");
                identity_rejected=true;marker(files/"replay-identity-rejected");}
            if(stage<=4&&state.prepared==stage&&state.ready&&!state.replaying&&state.replay_sequence==(stage==1?70u:stage==2?8u:2u)){
                const auto expected=files/("complete-"+std::to_string(stage));if(!std::filesystem::exists(expected))return false;
                if(receiver)check(receiver->world()->local_slots==host_locals,"Catch-up rebound to recipient views before producer replay completed");
                check(encode_source_authority(receive_checkpoint(*state.ready))==read(expected),"Actual network replay changed the complete canonical world");
                check(!state.rejected,"Successful native repair retained an earlier rejected-cut receipt");
                rejects([&]{receive_paired(*state.ready);});
                marker(files/("stage-"+std::to_string(stage)+"-checked"));++stage;
            }
            return stage==5;
        },"Receiver did not reproduce the complete ordered native source stream");
        auto ready=*receive_state().ready;check(chain_rejected&&identity_rejected&&receive_state().replayed_phases==replayed_before_retention&&receive_state().replay_batches>0,
            "Receiver did not execute all recorded source phases");
        const auto received_before_retention=receive_state().received;
        if(retain_inflight){pause_replay(true);marker(files/"retention-ready");}
        wait([&]{return std::filesystem::exists(files/"cancel-checked");},"Replay sender cancellation not checked");
        std::optional<SourceAuthorityRetainedWorld> retained;
        std::optional<std::future<SourceAuthorityRetainedWorld>> retention;
        if(retain_inflight){
            wait([&]{receive_update();const auto state=receive_state();return state.replaying&&state.received>=received_before_retention+2;},
                "Retention receiver did not prefetch both admitted batches while paused");
            pause_replay(false);
            wait([&]{receive_update();const auto state=receive_state();return state.replaying&&state.replayed_phases>replayed_before_retention&&
                state.replayed_phases<replayed_before_retention+32&&state.received>=received_before_retention+2;},
                "Retention did not observe predecessor replay with a prefetched authenticated successor");
            pause_replay(true);
            if(background){
                retention.emplace(background->retain_validated_world());
                rejects([&]{background->retain_validated_world().get();});
                rejects([&]{background->checkpoint(ready).get();});
            }
            else check(!receiver->retain_validated_world(),"Partial replay exported an uncertified intermediate world");
        }
        marker(files/"ready-before-host-loss");wait([&]{receive_update();return session->phase()==OnlineMatchSession::Phase::Failed;},"Host transport did not close");
        if(retain_inflight){
            const auto interrupted=receive_state();
            check(interrupted.replaying&&interrupted.replayed_phases<replayed_before_retention+40&&
                (!retention||retention->wait_for(0ms)!=std::future_status::ready),
                "Host loss did not overlap the native in-flight replay batch");
            pause_replay(false);
            wait([&]{
                if(retention){if(retention->wait_for(0ms)!=std::future_status::ready)return false;retained=retention->get();return true;}
                receive_update();retained=receiver->retain_validated_world();return retained.has_value();
            },"Retention did not finish the admitted replay batch after actual host loss");
            ready=retained->cut;
            check(receive_state().replayed_phases==replayed_before_retention+40&&encode_source_authority(retained->checkpoint)==read(files/"complete-retained"),
                "Retained in-flight source batch changed the complete world");
        }else check(receive_state().ready&&receive_state().ready->seal==ready.seal,"Host loss discarded the replayed source candidate");
        auto wrong=ready;++wrong.seal.tick;
        std::unique_ptr<MultiplayerPreparedWorld> world;
        if(background){
            if(!retained){rejects([&]{background->checkpoint(wrong).get();});retained=background->retain_validated_world().get();}
            check(retained->cut.context==ready.context&&retained->cut.seal==ready.seal&&
                retained->admissions==*session->source_players()&&retained->producer_views==host_locals&&retained->local_views==locals,
                "Retained source continuation lost its independent roster, context or view bindings");
            rejects([&]{background->retain_validated_world().get();});
            rejects([&]{background->checkpoint(ready).get();});
            auto cut=std::move(retained->checkpoint);background->cancel();
            check(!background->state().ready,"Cancelled background retained readiness");
            check(!background->pause_replay(false),"Cancelled background replay accepted a scheduling request");
            rejects([&]{background->checkpoint(ready).get();});background.reset();
            rejects([&]{SourceAuthorityReplayWorker cancelled(gameplay);cancelled.cancel();cancelled.retain_validated_world().get();});
            world=MultiplayerPreparedWorld::source_shadow(gameplay,MultiplayerPreparedWorld::SourceShadowViews::LocalPeer,workspace_clock);
            auto staging=MultiplayerPreparedWorld::source_shadow(gameplay,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,target_clock);
            SourceMatchAuthority recipient(world->match,*world->source_objects),producer(staging->match,*staging->source_objects);
            const auto accepted=*session->source_players();std::vector<SourcePlayerAdmission> admitted;
            for(auto slot:locals)admitted.push_back(accepted.players.at(slot));
            auto prepared=recipient.prepare_with_local_views(cut,source_authority_binding(ready.context),admitted,producer,accepted.players,host_locals);
            check(recipient.commit(std::move(prepared))&&world->local_slots==locals,"Owning background checkpoint did not import/rebind on the recipient executor");
            std::cout<<"PASS background native replay, owning validated retention across worker destruction, independent roster/view bindings, cancellation and separate recipient executor import\n";
        }else {
            check(!receiver->take_world_for_local_views(wrong),"Different cut rebound the replayed world");
            world=receiver->take_world_for_local_views(ready);check(world&&world->local_slots==locals&&!receiver->world()&&!receiver->state().ready,
                "Replay-to-recipient local view handoff failed");
        }
        for(auto slot:host_locals){const auto deaths=world->match.player(slot).deaths;world->match.depart(slot);
            check(world->match.player(slot).deaths==deaths,"Departing host group fabricated deaths");}
        const auto tick=world->match.simulation().ticks();for(unsigned i=0;i<10;++i){workspace_clock->time+=33;workspace_clock->filetime+=330000;
            std::array<Controls,16> input{};for(unsigned slot=0;slot<8;++slot)input[slot].yaw=world->match.unit(slot).yaw;world->match.step(input);}
        check(world->match.simulation().ticks()==tick+10&&!world->match.finished(),"Replayed and rebound source world could not survive host loss");
        std::cout<<"PASS actual authenticated peer replay:"<<(retain_inflight?122:82)
            <<" accepted native match/effect/four-view/weather/audio phases, complete canonical worlds, wrong-chain and replayed-identity rejection without a receipt, full repair, validated retention, delayed recipient rebind, original host-group departure and ten surviving ticks; role/endpoint/voice promotion remains external\n";
    }
}
}
int main(int argc,char **argv){std::cout<<std::unitbuf;int status=0;try{run(argc,argv);}catch(const std::exception &e){std::cerr<<e.what()<<'\n';status=1;}rtc::Cleanup().wait();return status;}
