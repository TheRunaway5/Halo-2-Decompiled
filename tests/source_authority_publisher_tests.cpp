#include "source_match_rebind_fixture.hpp"
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
template<class F>void wait(F ready,const char *stage){const auto until=Wall::now()+120s;
    while(!ready()){check(Wall::now()<until,stage);std::this_thread::sleep_for(5ms);}}
std::size_t discard(char *,std::size_t a,std::size_t b,void *){return a*b;}
void put(std::vector<std::byte> &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
void write(const std::filesystem::path &path,std::span<const std::byte> bytes){const auto temp=path.string()+".tmp";
    {std::ofstream out(temp,std::ios::binary);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());check(bool(out),"Private fixture write failed");}
    std::filesystem::rename(temp,path);
}
std::vector<std::byte> read(const std::filesystem::path &path){std::ifstream in(path,std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(in)),{});check(bool(in)||in.eof(),"Private fixture read failed");
    std::vector<std::byte> out(bytes.size());std::copy(bytes.begin(),bytes.end(),reinterpret_cast<char *>(out.data()));return out;}
std::array<std::byte,32> bodies(Owner &owner){std::vector<std::byte> bytes;const auto &bank=owner.world->memory();const auto services=storage(bank);
    for(const auto &object:owner.world->live_objects()){
        put(bytes,object.object,4);put(bytes,object.definition,4);put(bytes,object.parent,4);
        const auto row=source_raw_datum_lookup(owner.world->source_object_pool(),object.object,services);
        const auto size=word(bank,row+6,2);auto body=bank.view(word(bank,row+8),size);bytes.insert(bytes.end(),body.begin(),body.end());
    }
    for(unsigned i=0;i<player_count;++i){const auto player=owner.match->player(i).source_datum;put(bytes,player,4);
        auto row=bank.view(source_raw_datum_lookup(word(bank,0x4f54f4),player,services),0x21c);const auto begin=bytes.size();bytes.insert(bytes.end(),row.begin(),row.end());
        bytes[begin+0x28]=bytes[begin+0x29]=std::byte{};
    }
    std::array<std::byte,32> value{};check(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(value.data())),"Fixture body hash failed");return value;
}
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
    Owner world(argv[5],map.c_str(),locals,hosting?1000:900000,session->roster());
    std::unique_ptr<Owner> workspace;if(!hosting)workspace=std::make_unique<Owner>(argv[5],map.c_str(),host_locals,500000,session->roster());
    // Both test processes share this OS's MAC. Deliberately make the receiver's
    // disposable prototype identities different, as they would be on another
    // PC, so accepting locally minted prototype identities cannot pass.
    if(!hosting){auto &bank=world.world->memory();for(auto slot:locals){
        const auto row=source_raw_datum_lookup(world.world->source_player_pool(),world.match->player(slot).source_datum,storage(bank));
        auto identity=bank.view(row+4u,12);std::vector<std::byte> different(identity.begin(),identity.end());different[0]^=std::byte{0x80};bank.write(row+4u,different);
    }}
    std::vector<SourceMatchAuthority::LocalPlayerAdmission> accepted;
    const UiVariant variant{"Slayer",0,{{0x59,5}}};const auto fingerprint=multiplayer_fingerprint(map,map.parent_path(),0,world.cache.slayer_starting_weapon(5),argv[5]);
    session->loaded(fingerprint,variant);wait([&]{return session->phase()==OnlineMatchSession::Phase::Running;},"Real direct session loading timeout");
    wait([&]{return session->authority_supported(other);},"Direct authority capability timeout");
    if(hosting){
        check(!session->source_players(),"Native source admissions appeared before host publication");
        {SourceAuthorityPublisher unadmitted(*world.authority,session,other,60s);
            check(!unadmitted.capture()&&!unadmitted.state().ready,"World publication bypassed native source admission");}
        std::vector<unsigned> slots;for(const auto &p:session->roster())slots.push_back(p.slot);
        const auto players=world.authority->player_admissions(slots);
        check(session->publish_source_players(*session->authority_context(),players),"Actual source identity publication failed");
        wait([&]{return session->source_players_confirmed(other);},"Direct source admission acknowledgement timeout");
    }else {
        std::optional<AcceptedSourcePlayers> players;wait([&]{players=session->source_players();return bool(players);},"Direct source admission timeout");
        check(players->context==*session->authority_context()&&players->players.size()==session->roster().size(),"Direct source roster context differs");
        const auto prototypes=world.authority->player_admissions(locals);
        for(unsigned i=0;i<locals.size();++i){const auto &entry=players->players.at(locals[i]);
            check(entry.identity!=prototypes[i].identity,"Receiver trusted its separately minted source prototype identity");accepted.push_back(entry);}
    }
    check(!std::filesystem::exists(files/"trusted-source-admissions"),"Source identity exchange still used a private fixture file");
    if(hosting){
        for(unsigned i=0;i<20;++i){std::array<Controls,16> input{};for(unsigned j=0;j<8;++j)input[j].yaw=world.match->unit(j).yaw;input[0].fire=i<10;world.step(input);}
        SourceAuthorityPublisher publisher(*world.authority,session,other,60s);unsigned captures=0,encoding_ticks=0;bool cancelling=false;
        std::atomic<bool> wrong_thread_rejected=false;
        {std::jthread wrong([&]{try{publisher.capture();}catch(const std::exception &){wrong_thread_rejected=true;}});}
        check(wrong_thread_rejected,"Publisher allowed source capture on another executor thread");
        const auto context=*session->authority_context();const auto admitted_binding=source_authority_binding(context);
        auto changed=context;++changed.epoch;check(source_authority_binding(changed)!=admitted_binding,"Epoch did not bind source admission");
        changed=context;++changed.membership_revision;check(source_authority_binding(changed)!=admitted_binding,"Membership revision did not bind source admission");
        changed=context;changed.session[0]^=std::byte{1};check(source_authority_binding(changed)!=admitted_binding,"Room did not bind source admission");
        auto next=Wall::now();const auto deadline=Wall::now()+90s;
        while(true){publisher.update();auto state=publisher.state();check(state.error.empty(),state.error.c_str());
            if(captures<2&&state.validations==captures&&!state.encoding&&!state.awaiting_validation){
                if(captures)world.world->advance_effect_particles(1.f/60.f);
                check(publisher.capture(),"Actual source capture was refused");const auto tick=world.match->simulation().ticks();write(files/("bodies-"+std::to_string(tick)),bodies(world));++captures;
                check(!publisher.capture(),"Overlapping native capture was accepted");
            }
            if(state.validations==2&&!cancelling){
                check(state.delta&&state.payload_bytes<state.full_bytes/4&&encoding_ticks>0,"Background native publication did not preserve ticks or delta reuse");
                std::cout<<"PASS real native source publisher: two validated cuts, last full="<<state.full_bytes<<" bytes, payload="<<state.payload_bytes<<" bytes, capture="<<state.capture_microseconds
                    <<"us, worker encoding="<<state.encoding_microseconds<<"us, "<<encoding_ticks<<" source ticks while encoding\n";
                check(publisher.capture(),"Cancellation capture could not start");publisher.cancel();cancelling=true;
            }
            if(cancelling&&!publisher.state().encoding){state=publisher.state();check(state.publications==2&&state.validations==2&&!state.ready,"Cancelled worker published or revived a ready world");
                const std::array<std::byte,1> done{std::byte{1}};write(files/"host-checked",done);break;}
            check(Wall::now()<deadline,"Source publisher integration timed out");
            std::array<Controls,16> input{};for(unsigned j=0;j<8;++j){input[j].yaw=world.match->unit(j).yaw;input[j].forward=j%2?.1f:0;}input[0].fire=world.match->simulation().ticks()%30<2;
            const auto encoding_before_step=publisher.state().encoding;
            world.step(input);if(encoding_before_step)++encoding_ticks;
            next+=33333us;std::this_thread::sleep_until(next);
        }
        wait([&]{return std::filesystem::exists(files/"receiver-checked");},"Receiver final checks timeout");
    }else {
        std::optional<AuthorityArchiveImage> base;unsigned received=0;const auto deadline=Wall::now()+120s;
        while(received<2){
            if(auto cut=session->received_authority()){
                check(cut->peer==host&&cut->context.host==host,"Unverified source host");
                const bool delta=cut->bytes->size()>=8&&std::to_integer<char>((*cut->bytes)[2])=='D';
                check(!delta||base.has_value(),"Delta arrived without its exact retained base");
                auto image=delta?apply_authority_delta(*base,*cut->bytes):AuthorityArchiveImage::read(*cut->bytes);
                const auto decoded=decode_source_authority(image.encode());const auto binding=source_authority_binding(cut->context);
                check(decoded.match.simulation.ticks==cut->seal.tick&&decoded.match.admitted_binding==binding,"Source cut tick/context binding differs");
                auto wrong=cut->seal;++wrong.tick;check(!session->acknowledge_authority(cut->context,wrong),"Different cut acknowledgement accepted");
                workspace->local_views(host_locals);auto prepared=world.authority->prepare_with_local_views(decoded,binding,accepted,*workspace->authority);
                check(world.authority->commit(std::move(prepared)),"Actual session source world preparation failed");
                const auto expected=files/("bodies-"+std::to_string(cut->seal.tick));wait([&]{return std::filesystem::exists(expected);},"Independent producer body expectation missing");
                check(std::ranges::equal(bodies(world),read(expected)),"Concurrent encoding changed source object/player bytes from capture time");
                check(session->acknowledge_authority(cut->context,cut->seal),"Prepared source cut acknowledgement refused");base=image;++received;
                std::cout<<"PASS actual session receiver prepared cut "<<received<<" tick="<<cut->seal.tick<<" delta="<<delta<<", four admitted local views, capture-time complete object/player bytes\n";
            }
            check(Wall::now()<deadline,"Actual source receiver timed out");std::this_thread::sleep_for(2ms);
        }
        wait([&]{return std::filesystem::exists(files/"host-checked");},"Host cancellation check timeout");
        check(!session->received_authority(),"Cancelled third source cut reached the receiver");
        const auto tick=world.match->simulation().ticks();for(unsigned i=0;i<10;++i){std::array<Controls,16> input{};for(unsigned j=0;j<8;++j)input[j].yaw=world.match->unit(j).yaw;world.step(input);}
        check(world.match->simulation().ticks()==tick+10,"Prepared shadow world could not continue its actual source executor");
        const std::array<std::byte,1> done{std::byte{1}};write(files/"receiver-checked",done);
    }
}
}
int main(int argc,char **argv){std::cout<<std::unitbuf;int status=0;
    try{run(argc,argv);}catch(const std::exception &e){std::cerr<<e.what()<<'\n';status=1;}
    // All session owners have now left scope. Wait for deferred RTC teardown
    // before testing process-lifetime crypto/thread resource cleanup.
    rtc::Cleanup().wait();return status;
}
