#include "source_match_rebind_fixture.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/multiplayer_network.hpp"
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
void write(const std::filesystem::path &path,std::span<const std::byte> bytes){const auto temp=path.string()+".tmp";
    {std::ofstream out(temp,std::ios::binary);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());check(bool(out),"Receiver fixture write failed");}
    std::filesystem::rename(temp,path);
}
std::vector<std::byte> read(const std::filesystem::path &path){std::ifstream in(path,std::ios::binary);std::vector<char> data((std::istreambuf_iterator<char>(in)),{});
    check(bool(in)||in.eof(),"Receiver fixture read failed");std::vector<std::byte> bytes(data.size());std::copy(data.begin(),data.end(),reinterpret_cast<char *>(bytes.data()));return bytes;}
void marker(const std::filesystem::path &path){const std::array<std::byte,1> done{std::byte{1}};write(path,done);}
std::array<std::byte,32> bodies(const MultiplayerPreparedWorld &owner){std::vector<std::byte> bytes;auto &world=*owner.source_objects;const auto &bank=world.memory();const auto services=storage(bank);
    for(const auto &object:world.live_objects()){put(bytes,object.object,4);put(bytes,object.definition,4);put(bytes,object.parent,4);
        const auto row=source_raw_datum_lookup(world.source_object_pool(),object.object,services);const auto size=word(bank,row+6,2);auto data=bank.view(word(bank,row+8),size);bytes.insert(bytes.end(),data.begin(),data.end());}
    for(unsigned slot=0;slot<8;++slot){const auto player=owner.match.player(slot).source_datum;put(bytes,player,4);
        auto row=bank.view(source_raw_datum_lookup(world.source_player_pool(),player,services),0x21c);const auto begin=bytes.size();bytes.insert(bytes.end(),row.begin(),row.end());bytes[begin+0x28]=bytes[begin+0x29]=std::byte{};}
    std::array<std::byte,32> hash{};check(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(hash.data())),"Receiver body hash failed");return hash;}
void alter(SourceMatchAuthority::Checkpoint &cut,std::uint32_t address){for(auto &region:cut.world.world.bank.regions)if(address>=region.address&&std::uint64_t(address)<std::uint64_t(region.address)+region.bytes.size()){
    region.bytes[address-region.address]^=std::byte{0x80};return;}throw FormatError("Forged remote identity fixture address missing");}
class CheckedDatagrams final:public MultiplayerDatagrams {
    std::shared_ptr<OnlineMatchSession> session_;
public:
    explicit CheckedDatagrams(std::shared_ptr<OnlineMatchSession> s):session_(std::move(s)){}
    void send(std::span<const std::byte> bytes,unsigned peer)override {
        check(bytes.size()>=7,"Native gameplay library emitted a truncated datagram");
        const auto protocol=std::to_integer<unsigned>(bytes[4])+(std::to_integer<unsigned>(bytes[5])<<8);
        if(protocol!=native_multiplayer_protocol)throw FormatError("Native gameplay library protocol "+std::to_string(protocol)+" differs from executor protocol "+std::to_string(native_multiplayer_protocol));
        session_->send(bytes,peer);
    }
    bool receive(std::vector<std::byte> &bytes,unsigned &peer)override{return session_->receive(bytes,peer);}
};
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
    auto target_clock=std::make_shared<Clock>(),workspace_clock=std::make_shared<Clock>();target_clock->time=900000;workspace_clock->time=500000;
    std::unique_ptr<MultiplayerPreparedWorld> host_world;std::unique_ptr<SourceMatchAuthority> authority;std::unique_ptr<LanHost> lan_host;std::unique_ptr<LanClient> lan_client;
    std::unique_ptr<SourceAuthorityReceiver> receiver;
    if(hosting){host_world=std::make_unique<MultiplayerPreparedWorld>(gameplay);for(unsigned i=0;i<locals.size();++i){const auto slot=locals[i];
        check(host_world->match.join(session->roster()[slot].name,-1,gameplay.selected_profiles[i],slot)==slot,"Actual host local source birth failed");}
        host_world->bind_source_local_views(locals);authority=std::make_unique<SourceMatchAuthority>(host_world->match,*host_world->source_objects);
    }else {receiver=std::make_unique<SourceAuthorityReceiver>(gameplay,target_clock,workspace_clock);
        check(!receiver->warm()&&!receiver->world()&&!receiver->state().ready,"Receiver warmed or acknowledged without independent source admissions");
        std::atomic<bool> rejected=false;{std::jthread wrong([&]{try{receiver->update();}catch(const std::exception &){rejected=true;}});}check(rejected,"Receiver allowed native work on another executor");}
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
    if(hosting){std::vector<unsigned> slots;for(const auto &p:session->roster())slots.push_back(p.slot);
        check(session->publish_source_players(*session->authority_context(),authority->player_admissions(slots)),"Host source admission export failed");
        // This is a quiescent native preparation interval, not a simulation
        // input timeout. The production peer transport keeps DTLS alive while
        // the other source executor constructs its private worlds.
        wait([&]{return session->source_players_confirmed(other)&&std::filesystem::exists(files/"receiver-ready");},"Receiver publication gate was not ready");
        for(unsigned tick=0;tick<20;++tick){pump();for(unsigned i=0;i<8;++i)controls[i].yaw=host_world->match.unit(i).yaw;controls[locals[0]].fire=tick<10;host_world->match.step(controls);}
        // A well-formed/checksummed complete cut with a forged NONLOCAL native
        // identity must not enter the candidate or produce a world receipt.
        const auto context=*session->authority_context();auto bad=authority->checkpoint(source_authority_binding(context));
        const auto remote=host_locals.front();auto &bank=host_world->source_objects->memory();
        const auto row=source_raw_datum_lookup(host_world->source_objects->source_player_pool(),host_world->match.player(remote).source_datum,storage(bank));
        alter(bad,row+4);const std::array<unsigned,1> targets{other};session->publish_authority(context,bad.match.simulation.ticks,encode_source_authority(bad),targets);
        wait([&]{pump();return std::filesystem::exists(files/"bad-rejected");},"Forged nonlocal source identity was not rejected");
        check(session->validated_authority().empty(),"Forged remote source identity produced a world receipt");
        {SourceAuthorityPublisher publisher(*authority,session,other,90s);unsigned captures=0;const auto deadline=Wall::now()+180s;
            while(publisher.state().validations<2){pump();publisher.update();const auto state=publisher.state();check(state.error.empty(),state.error.c_str());
                if(captures<2&&state.validations==captures&&!state.encoding&&!state.awaiting_validation){
                    if(captures)host_world->source_objects->advance_effect_particles(1.f/60.f);
                    check(publisher.capture(),"Full prepared-world host capture failed");write(files/("bodies-"+std::to_string(host_world->match.simulation().ticks())),bodies(*host_world));++captures;
                }
                for(unsigned i=0;i<8;++i)controls[i].yaw=host_world->match.unit(i).yaw;controls[locals[0]].fire=host_world->match.simulation().ticks()%30<2;
                host_world->match.step(controls);check(Wall::now()<deadline,"Full source receiver publication timed out");std::this_thread::sleep_for(33ms);
            }
            const auto state=publisher.state();check(state.delta&&state.payload_bytes<state.full_bytes/4,"Prepared receiver did not retain the producer's canonical delta base");
            std::cout<<"PASS actual native host/grouped profiles/source admission/full-plus-delta preparation receipts: "<<state.full_bytes<<" full bytes -> "<<state.payload_bytes<<" delta bytes\n";
        }
        wait([&]{pump();return std::filesystem::exists(files/"cancel-ready");},"Receiver cancellation fixture was not ready");
        auto cancelled=authority->checkpoint(source_authority_binding(context));session->publish_authority(context,cancelled.match.simulation.ticks,encode_source_authority(cancelled),targets);
        wait([&]{pump();return std::filesystem::exists(files/"cancel-checked");},"In-flight decoder cancellation did not complete");
        check(session->validated_authority().empty(),"Cancelled decoder produced a world acknowledgement");
        marker(files/"host-checked");wait([&]{pump();return std::filesystem::exists(files/"ready-before-host-loss");},"Candidate did not retain its latest prepared world");
        // Return destroys every host session reference, including the native
        // sender. Receiver must keep the actual prepared world across failure.
    }else {
        wait([&]{pump();return session->source_players().has_value();},"Receiver independent source roster timeout");
        receiver->update();check(!receiver->world()&&!receiver->state().warmed&&!receiver->state().workspace_warmed,
            "Source metadata alone started cold world construction during ordinary gameplay");
        const bool cold_from_cut=std::getenv("HALO2_RECEIVER_COLD_FROM_CUT")!=nullptr;
        const auto change_prototype=[&]{
            check(receiver->world()&&receiver->world()->source_objects&&receiver->world()->source_objects->owns_physics(),"Production source-shadow initializer failed");
            check(!receiver->world()->matches(gameplay),"Shadow prototype was admitted as the visible runtime world");
            check(receiver->world()->local_slots==locals&&receiver->world()->source_savegames,"Shadow lost its accepted local views/private native files");
            auto &prototype=receiver->world()->source_objects->memory();for(auto slot:locals){const auto row=source_raw_datum_lookup(receiver->world()->source_objects->source_player_pool(),receiver->world()->match.player(slot).source_datum,storage(prototype));
                auto original=prototype.view(row+4,12);std::vector<std::byte> identity(original.begin(),original.end());identity[0]^=std::byte{0x80};prototype.write(row+4,identity);}
        };
        if(!cold_from_cut){
            check(receiver->warm(),"Explicit source-shadow warm failed");
            check(!receiver->state().workspace_warmed&&receiver->state().workspace_constructions==0,"One warm call constructed two native executors");
            pump();check(receiver->warm()&&receiver->state().workspace_warmed&&receiver->state().workspace_constructions==1,"Separate workspace warm failed");
            change_prototype();
        }
        // Publication permission is not a prepared-world receipt. The cold
        // variant intentionally has no executor when the first cut is sent.
        marker(files/"receiver-ready");bool rejected=false;unsigned prepared=0;
        wait([&]{pump();receiver->update();const auto state=receiver->state();
            if(!rejected&&state.received==1&&!state.decoding&&!state.error.empty()){
                check(state.error.find("full source player identity")!=std::string::npos,"Malformed cut failed before the independent full-roster identity gate");
                check(state.prepared==0&&!state.ready&&receiver->world()->match.simulation().ticks()==0,"Rejected remote identity changed or acknowledged the live candidate");
                if(cold_from_cut)change_prototype();
                rejected=true;marker(files/"bad-rejected");std::cout<<"PASS forged nonlocal source identity rejected before candidate mutation or world acknowledgement\n";}
            if(state.prepared>prepared){check(rejected&&state.ready&&receiver->world(),"Receiver prepared before rejecting the forged identity");
                check(state.workspace_constructions==1,"Receiver rebuilt its private workspace between cuts");
                const auto &seal=state.ready->seal;const auto expected=files/("bodies-"+std::to_string(seal.tick));
                check(std::filesystem::exists(expected)&&std::ranges::equal(bodies(*receiver->world()),read(expected)),"Receiver changed complete object/player bytes from the producer cut");
                ++prepared;std::cout<<"PASS production receiver cut "<<prepared<<" tick="<<seal.tick<<" decode="<<state.decode_microseconds<<"us warm="<<state.warm_microseconds<<"us workspace="<<state.workspace_microseconds<<"us constructions="<<state.workspace_constructions<<" prepare="<<state.prepare_microseconds<<"us delta="<<state.delta<<"; four rebound views and all admitted source identities\n";
            }
            return prepared==2;
        },"Production source receiver did not prepare two cuts");
        const auto ready=*receiver->state().ready;
        {SourceAuthorityReceiver cancelled(gameplay,target_clock,workspace_clock);marker(files/"cancel-ready");
            wait([&]{pump();cancelled.update();const auto state=cancelled.state();return state.received==1&&state.decoding;},"Cancellation did not reach an in-flight owning decoder job");
            cancelled.cancel();wait([&]{pump();cancelled.update();return !cancelled.state().decoding;},"Cancelled owning decoder did not finish");
            check(cancelled.state().prepared==0&&!cancelled.state().ready&&!cancelled.world()&&!cancelled.warm(),"Cancelled decoder revived a world, cache or readiness");
            check(!cancelled.state().warmed&&!cancelled.state().workspace_warmed,"Cancellation retained a warmed source executor");
            marker(files/"cancel-checked");std::cout<<"PASS in-flight owning decoder cancellation discards stale completion and never acknowledges a world\n";}
        marker(files/"ready-before-host-loss");
        wait([&]{receiver->update();return session->phase()==OnlineMatchSession::Phase::Failed;},"Real host disconnect did not terminate the original transport role");
        check(receiver->state().ready&&receiver->world()&&receiver->state().ready->seal==ready.seal,"Host loss destroyed the validated candidate world");
        auto wrong=ready;++wrong.seal.tick;check(!receiver->take_world(wrong),"A different cut took the prepared world");
        auto world=receiver->take_world(ready);check(world&&world->source_objects&&!receiver->world()&&!receiver->state().ready,"Exact prepared source world could not transfer ownership");
        check(!receiver->state().workspace_warmed,"Ownership transfer retained a stale reusable workspace");
        for(auto slot:host_locals){const auto player=world->match.player(slot).source_datum;const auto deaths=world->match.player(slot).deaths;
            world->match.depart(slot);check(world->match.player(slot).source_datum==player&&world->match.player(slot).deaths==deaths,"Departing host group released a retained player or added a numeric death");}
        const auto tick=world->match.simulation().ticks();for(unsigned i=0;i<10;++i){std::array<Controls,16> input{};for(unsigned slot=0;slot<8;++slot)input[slot].yaw=world->match.unit(slot).yaw;target_clock->time+=33;world->match.step(input);}
        check(world->match.simulation().ticks()==tick+10,"Transferred replacement world could not continue native source execution");
        check(!world->match.finished()&&std::count_if(session->roster().begin(),session->roster().end(),[&](const auto &p){return world->match.player(p.slot).connected&&!(world->match.player(p.slot).source_flags&2u);})==4,
            "Original host-group departure ended the actual surviving source world");
        receiver->update();check(!receiver->warm()&&!receiver->state().ready,"Stopped receiver revived stale state");
        std::cout<<"PASS actual host transport loss preserves the complete prepared source candidate; exact-cut owner transfer, original four-player host departure and ten surviving source ticks; no endpoint switch or authority promotion inferred\n";
    }
}
}
int main(int argc,char **argv){std::cout<<std::unitbuf;int status=0;try{run(argc,argv);}catch(const std::exception &e){std::cerr<<e.what()<<'\n';status=1;}rtc::Cleanup().wait();return status;}
