#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/native_player_profile.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <deque>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
std::string stage="setup";
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void wait(F ready,const char *why){const auto until=std::chrono::steady_clock::now()+90s;
    while(!ready()){check(std::chrono::steady_clock::now()<until,why);std::this_thread::sleep_for(5ms);}}
template<class F>void rejects(F run){bool rejected{};try{run();}catch(const std::exception &){rejected=true;}
    check(rejected,"Invalid native takeover was accepted");}
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();
    std::array<std::uint8_t,9> appearance;std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
    return native_selected_player_configuration(name,appearance,-1);}
// Preserve real authenticated input datagrams until all three supplied pulses
// are present, then deliver them in one ordinary host receive/tick. Nothing is
// reconstructed or changed in the port's native control payload.
class HeldInputDatagrams final:public MultiplayerDatagrams {
    struct Packet{std::vector<std::byte> data;unsigned peer;};
    std::shared_ptr<OnlineMatchSession> session_;std::deque<Packet> held_;
public:
    explicit HeldInputDatagrams(std::shared_ptr<OnlineMatchSession> s):session_(std::move(s)){}
    void send(std::span<const std::byte> p,unsigned to)override{session_->send(p,to);}
    bool receive(std::vector<std::byte>&p,unsigned &from)override{
        if(held_.empty())return session_->receive(p,from);
        p=std::move(held_.front().data);from=held_.front().peer;held_.pop_front();return true;
    }
    unsigned retain_switches(unsigned from){
        std::vector<std::byte> p;unsigned peer;
        while(session_->receive(p,peer)){check(held_.size()<128,"Unbounded fixture input holding");held_.push_back({std::move(p),peer});}
        unsigned count{};
        for(const auto &packet:held_)if(packet.peer==from&&packet.data.size()>=46&&packet.data[6]==std::byte{2}&&
            (Bytes(packet.data).u16(44)&1024u))++count; // Native input: header7+nonce8+cookie8+sequence4+count1, then four floats/flags.
        return count;
    }
};
}
int main(int argc,char **argv)try{
    std::cout<<std::unitbuf;check(argc==5,"Supply ORIGIN DESCRIPTORS XBE MAP");
    if(!std::filesystem::exists(argv[3])||!std::filesystem::exists(argv[4]))return 77;
    nlohmann::json descriptors;{std::ifstream in(argv[2]);in>>descriptors;}
    std::vector<OnlineAdmission> roster;
    for(const auto &row:descriptors.at(0).at("players")){
        OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");p.peer=row.at("peer");p.guest=row.at("guest");
        const std::string identity=row.at("identity");check(identity.size()==16,"Invalid private admission fixture");
        for(unsigned i=0;i<8;++i)p.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));
        roster.push_back(p);
    }
    check(roster.size()==12&&descriptors.size()==3,"Expected three physical PCs with four couch seats each");
    std::array<std::shared_ptr<OnlineMatchSession>,16> sessions;
    for(const auto &d:descriptors){OnlineMatchTransportOptions transport;transport.direct_udp=true;transport.peer_host_migration=true;
        const unsigned self=d.at("slot");sessions[self]=std::make_shared<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),self,roster,true,false,false,transport);}
    const auto healthy=[&]{for(const auto &s:sessions)if(s)check(s->phase()!=OnlineMatchSession::Phase::Failed,s->error().c_str());};
    stage="direct loading";wait([&]{healthy();for(const auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Loading)return false;return true;},"Direct loading timeout");
    const UiVariant variant{"Slayer",0,{{0x59,5},{10,0},{2,0}}};constexpr std::uint64_t fingerprint=0x15c071501;
    for(const auto &s:sessions)if(s)s->loaded(fingerprint,variant);
    wait([&]{healthy();for(const auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Running)return false;return true;},"Direct running timeout");
    const unsigned old_host=sessions[0]->host();unsigned candidate=16,remote=16;
    for(unsigned p=0;p<16;++p)if(sessions[p]){check(sessions[p]->host()==old_host,"Measured hosts disagree");
        if(p!=old_host){if(candidate==16)candidate=p;else remote=p;}}
    check(candidate<16&&remote<16,"Missing native candidate or surviving client");
    const auto before=*sessions[old_host]->authority_context();
    MultiplayerOptions producer_options;producer_options.gameplay_xbe=argv[3];producer_options.map=argv[4];
    producer_options.variant=variant;producer_options.local_players=4;producer_options.session_random_seed=0x71;
    producer_options.online_match=sessions[old_host];
    std::array<std::vector<SourcePlayerConfiguration>,16> profiles;
    for(const auto &p:roster)profiles[online_admission_peer(p)].push_back(profile(p.name));
    producer_options.selected_profiles=profiles[old_host];
    stage="native source admission";
    auto producer=std::make_unique<MultiplayerPreparedWorld>(producer_options,std::nullopt,std::make_shared<Clock>());
    for(unsigned view=0;view<4;++view){const auto slot=sessions[old_host]->local_slots()[view];
        check(producer->match.join(roster[slot].name,-1,profiles[old_host][view],slot)==slot,"Initial native host birth changed a slot");}
    producer->bind_source_local_views(sessions[old_host]->local_slots());
    auto held_datagrams=std::make_shared<HeldInputDatagrams>(sessions[old_host]);
    auto lan_host=std::make_unique<LanHost>(producer->match,held_datagrams,fingerprint,roster,variant);
    auto promoted=std::make_unique<LanClient>(sessions[candidate],old_host,fingerprint,std::span<const std::int8_t>{},profiles[candidate]);
    LanClient survivor(sessions[remote],old_host,fingerprint,std::span<const std::int8_t>{},profiles[remote]);
    std::array<Controls,4> neutral{};std::array<Controls,16> controls{};
    wait([&]{promoted->send(neutral);survivor.send(neutral);lan_host->receive(controls);lan_host->broadcast();promoted->receive();survivor.receive();
        healthy();return lan_host->peers()==2&&promoted->ready()&&survivor.ready();},"Twelve actual native player/profile admissions timed out");
    survivor.bind_continuity(before,remote,roster);
    auto original=std::make_unique<SourceMatchAuthority>(producer->match,*producer->source_objects);
    std::vector<unsigned> slots;for(const auto &p:roster)slots.push_back(p.slot);
    const auto admitted=original->player_admissions(slots);
    check(sessions[old_host]->publish_source_players(before,admitted),"Actual native allocation export refused");
    wait([&]{healthy();for(const auto &s:sessions)if(s){const auto a=s->source_players();if(!a||a->players!=admitted)return false;}return true;},"Independent native admissions timed out");
    for(unsigned tick=0;tick<3;++tick){for(const auto &p:roster)controls[p.slot].yaw=producer->match.unit(p.slot).yaw;
        producer->match.step(controls);lan_host->broadcast();}
    auto switches=neutral;for(unsigned view=0;view<4;++view){switches[view].switch_grenade=true;
        switches[view].yaw=producer->match.unit(sessions[candidate]->local_slots()[view]).yaw;}
    for(unsigned packet=0;packet<3;++packet)promoted->send(switches);
    wait([&]{return held_datagrams->retain_switches(candidate)==3;},"Three real couch-input pulses were not delivered over DTLS");
    controls={};for(auto slot:sessions[old_host]->local_slots())controls[slot].yaw=producer->match.unit(slot).yaw;
    lan_host->receive(controls);
    for(auto slot:sessions[candidate]->local_slots())check(controls[slot].switch_grenade,"Predecessor lost the first real couch pulse");
    producer->match.step(controls);lan_host->broadcast();
    stage="capture paired native cut";
    const auto paired=capture_source_host_continuation(*lan_host,before,1);
    stage="encode native world";
    const auto complete=encode_source_authority(paired.world);
    MultiplayerOptions recipient_options=producer_options;recipient_options.online_match=sessions[candidate];recipient_options.selected_profiles=profiles[candidate];
    // The actual runtime keeps its initial replica/cache alive for resident
    // rendering resources while a private full source world is promoted.
    auto resident=std::make_shared<MultiplayerPreparedWorld>(recipient_options,std::nullopt,std::make_shared<Clock>());
    const auto resident_cache=resident->cache.get();const auto resident_match=&resident->match;
    stage="warm recipient native pair";
    const bool worker_retention=std::getenv("HALO2_TAKEOVER_WORKER_RETAIN");
    std::unique_ptr<SourceAuthorityReceiver> receiver;
    std::unique_ptr<SourceAuthorityReplayWorker> background;
    if(worker_retention)background=std::make_unique<SourceAuthorityReplayWorker>(recipient_options,std::make_shared<Clock>(),std::make_shared<Clock>());
    else receiver=std::make_unique<SourceAuthorityReceiver>(recipient_options,SourceAuthorityStreamMode::Replay,std::make_shared<Clock>(),std::make_shared<Clock>());
    MultiplayerRuntimeAuthority runtime(recipient_options,resident,{},std::move(promoted),std::move(background));resident.reset();
    bool foreign_rejected{};std::thread foreign([&]{try{runtime.world();}catch(const FormatError &){foreign_rejected=true;}});foreign.join();
    check(foreign_rejected,"Runtime exported a native owner to another executor");
    try{auto interrupted=runtime.begin_frame();check(bool(interrupted),"Missing interrupted frame");throw FormatError("Deliberate frame unwind");}
    catch(const FormatError &){}
    auto frame=runtime.begin_frame();check(bool(frame),"Running runtime refused its frame");
    rejects([&]{runtime.begin_frame();});rejects([&]{runtime.service();});
    rejects([&]{runtime.suspend_publication();});
    const auto state=[&]{return receiver?receiver->state():*runtime.shadow_state();};
    const auto update=[&]{if(receiver)receiver->update();};
    const auto checkpoint=[&](auto ready){return receiver->checkpoint(ready);};
    wait([&]{if(receiver)receiver->warm();const auto s=state();check(s.error.empty(),s.error.c_str());return s.warmed&&s.workspace_warmed;},"Private recipient worlds did not warm");
    const std::array<unsigned,1> targets{candidate};
    stage="encode and publish paired continuation";
    auto seal=sessions[old_host]->publish_authority(before,original->execution_tick(),encode_source_host_continuation(paired),targets);
    stage="paired native validation";
    wait([&]{update();const auto s=state();check(s.error.empty(),s.error.c_str());
        return s.ready&&s.ready->seal==seal&&s.host_ready;},"Paired native UDP cut did not validate");
    auto ready=*state().ready;
    if(receiver)check(encode_source_authority(checkpoint(ready))==complete,"Native receiver differs from complete source world");
    // Same bytes and tick still count as unrecorded native mutation. A receipt
    // cannot silently certify later private executor work. Repair from the
    // original authenticated producer before allowing any readiness/election.
    if(receiver){
        auto &bank=receiver->world()->source_objects->memory();const auto original_bytes=bank.view(0x4c65f0u,8);
        bank.write(0x4c65f0u,original_bytes);rejects([&]{checkpoint(ready);});
        wait([&]{return !sessions[old_host]->validated_authority().empty();},"Original cut receipt did not reach its producer");
        auto repair=paired;repair.serial=2;
        seal=sessions[old_host]->publish_authority(before,original->execution_tick(),encode_source_host_continuation(repair),targets);
        wait([&]{update();const auto s=state();check(s.error.empty(),s.error.c_str());return s.ready&&s.ready->seal==seal&&s.host_ready;},
            "Native receiver did not repair its mutated private world");
        ready=*state().ready;
        check(encode_source_authority(checkpoint(ready))==complete,"Repaired native cut differs from its producer");
    }
    PeerMigrationDecision invented;invented.previous=before;invented.value={candidate,seal};
    if(receiver)rejects([&]{receiver->prepare_peer_host_takeover(invented);});
    check(state().ready.has_value(),"Premature takeover consumed the validated native world");
    std::optional<SourceAuthorityRetainedWorld> retained;
    if(worker_retention){
        // Foreground recovery cannot join native destruction in its frame.
        rejects([&]{runtime.recover();});check(frame->finish(),"Recovery frame did not finish");frame.reset();
        std::shared_ptr<const SourceAuthorityRetainedWorld> value;
        wait([&]{value=runtime.recover();return bool(value);},"Runtime replay worker did not retire asynchronously");
        check(!runtime.shadow_state()&&runtime.recover()==value,"Recovery retained a mutable native executor or repeated its export");
        retained=*value;check(encode_source_authority(retained->checkpoint)==complete,"Retired native receiver differs from complete source world");
    }else wait([&]{retained=receiver->retain_validated_world();return bool(retained);},"Native receiver did not quiesce");
    check(retained->host.has_value()&&sessions[candidate]->prepare_peer_migration(retained->cut),"Actual retained native cut could not become ready");
    wait([&]{for(const auto &s:sessions)if(s){const auto state=s->peer_migration();if(!state||!state->ready[candidate])return false;}return true;},"Actual native readiness did not reach peers");
    stage="old host shutdown and peer agreement";
    lan_host.reset();original.reset();producer.reset();
    held_datagrams.reset();producer_options.online_match.reset();sessions[old_host].reset();
    wait([&]{healthy();return sessions[candidate]->peer_migration()->suspected;},"Host silence timeout");
    wait([&]{healthy();return sessions[candidate]->migration_candidate()==candidate;},"Measured survivors did not qualify the actual retained native cut");
    check(sessions[candidate]->propose_peer_migration(),"Automatically measured retained candidate could not propose");
    wait([&]{healthy();return sessions[candidate]->peer_migration()->decision&&sessions[remote]->peer_migration()->decision;},"Surviving peers did not agree");
    const auto decision=*sessions[candidate]->peer_migration()->decision;
    check(decision==*sessions[remote]->peer_migration()->decision&&decision.value.cut==seal,"Peers chose another native cut");
    auto forged=decision;++forged.value.cut.tick;
    if(receiver)rejects([&]{receiver->prepare_peer_host_takeover(forged);});
    else rejects([&]{SourcePeerHostTakeover::prepare(recipient_options,*retained,forged);});
    check((!receiver||receiver->state().ready)&&sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,"Rejected cut consumed the candidate or reopened media");
    stage="native takeover transaction";
    auto takeover=receiver?receiver->prepare_peer_host_takeover(decision):
        SourcePeerHostTakeover::prepare(recipient_options,*retained,decision,std::make_shared<Clock>(),std::make_shared<Clock>());
    if(receiver){check(!receiver->world()&&!receiver->state().ready,"Receiver retained a second mutable native owner");
        rejects([&]{receiver->prepare_peer_host_takeover(decision);});}
    check(takeover->world().local_slots==sessions[candidate]->local_slots()&&takeover->local().slots==takeover->world().local_slots,
        "New native host lost local/remote couch ownership");
    rejects([&]{takeover->host().broadcast();});
    for(const auto &p:roster)check(takeover->world().match.player(p.slot).source_datum==admitted[p.slot].player&&
        takeover->world().match.player(p.slot).deaths==paired.world.match.players[p.slot].deaths,"Takeover changed a native allocation or invented a death");
    check(takeover->world().match.simulation().ticks()==seal.tick&&sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,
        "Native preparation advanced simulation or opened media");
    // An outstanding frame, including one without a recording publisher,
    // prevents replacement. Rejection must leave ownership with the caller.
    if(frame){
        rejects([&]{runtime.stage_takeover(std::move(takeover));});
        check(bool(takeover)&&&runtime.match()==resident_match&&runtime.client()&&!runtime.host(),
            "Rejected staging changed the runtime's borrowed world or transport");
        rejects([&]{runtime.activate_takeover();});check(frame->finish()&&frame->finish(),"Runtime frame did not close exactly once");frame.reset();
    }
    check(!runtime.begin_frame(),"Fenced runtime admitted another native frame");
    auto *const staged=takeover.get();runtime.stage_takeover(std::move(takeover));
    check(!runtime.promoted_camera()&&!runtime.local_input_pending(),"Staging exposed promoted camera/input ownership");
    check(!takeover&&runtime.pending_takeover()==staged&&&runtime.match()==resident_match&&runtime.client(),
        "Staging exposed a private native owner before adapter rebinding");
    survivor.rebind_host(decision.previous,decision.next,decision.value.cut);
    check(runtime.activate_takeover()&&!runtime.activate_takeover(),"Native activation was rejected or committed twice");
    check(runtime.promoted_camera()==&staged->camera()&&runtime.local_input_pending(),"Runtime lost the restored local camera/input bank");
    rejects([&]{runtime.continue_local_input(controls);});
    check(!runtime.client()&&runtime.host()&&runtime.host()->peers()==1&&!runtime.pending_takeover(),"Replacement host lost its runtime role or surviving four-seat native peer");
    check(&runtime.match()==&staged->world().match&&&runtime.match()!=resident_match&&resident_cache->path()==runtime.world().cache->path(),
        "Runtime adoption changed immutable map resources or retained the old live Match");
    check(sessions[remote]->finish_peer_migration(decision),"Existing native client could not commit its rebind");
    runtime.service();
    stage="resumed native snapshots and encoded voice";
    auto &world=runtime.world();const auto start=world.match.simulation().ticks();unsigned voice_packets{};
    const auto held=staged->local().controls;
    for(unsigned tick=0;tick<12;++tick){auto scope=runtime.begin_frame();check(bool(scope),"Adopted runtime remained fenced");
        controls={};check(runtime.continue_local_input(controls)==(tick<2),"Retained pending input was omitted or repeated");
        if(tick<2){
            check(runtime.local_input_pending(),"Retained input released fresh hardware before its native step");
            for(unsigned view=0;view<world.local_slots.size();++view){const auto &c=controls[world.local_slots[view]];
                check(c.yaw==held[view].yaw&&c.pitch==held[view].pitch&&c.zoom==held[view].zoom&&
                    c.zoom_reset_serial==held[view].zoom_reset_serial,"Retained local look/zoom state changed during delivery");}
            for(auto slot:world.local_slots)check(controls[slot].switch_grenade,"Imported couch pulse did not reach its native tick");
            const auto delivered=staged->local().delivered_presses;
            auto repeated=controls;check(runtime.continue_local_input(repeated),"Repeated retained-input poll changed ownership");
            check(staged->local().delivered_presses==delivered,"Repeated retained-input poll consumed a second queued pulse");
            for(auto slot:world.local_slots)check(repeated[slot].yaw==controls[slot].yaw&&repeated[slot].zoom==controls[slot].zoom,
                "Repeated retained-input poll changed its current native tick");
        }
        for(auto slot:world.local_slots)controls[slot].yaw=world.match.unit(slot).yaw;
        survivor.send(neutral);runtime.host()->receive(controls);runtime.match().step(controls);runtime.host()->broadcast();
        check(runtime.local_input_pending()==(tick==0),"Imported input released hardware early or failed to release after its final native step");
        sessions[candidate]->voice(remote,GameVoiceFrame{candidate,tick+1,false,{1,1},{0x60}});
        wait([&]{survivor.receive();check(survivor.error().empty(),survivor.error().c_str());return survivor.state().tick==world.match.simulation().ticks();},"Surviving native client did not follow successor snapshots");
        voice_packets+=sessions[remote]->voice_packets().size();
        check(std::ranges::equal(survivor.slots(),sessions[remote]->local_slots()),"Existing remote couch slots changed after host loss");
    }
    wait([&]{voice_packets+=sessions[remote]->voice_packets().size();return voice_packets==12;},"Encoded direct voice did not resume with native host");
    check(world.match.simulation().ticks()==start+12&&!world.match.finished(),"Native migrated match did not survive");
    const auto successor=capture_source_host_continuation(*runtime.host(),decision.next,2);
    check(!encode_source_host_continuation(successor).empty(),"Migrated native host cannot publish its next paired cut");
    std::cout<<"PASS actual paired native UDP cut, independent twelve-player allocation admission, two-survivor majority after host session shutdown, transactional four-seat view/input/camera promotion, retained departures/results, existing four-seat client rebind, twelve connected native ticks and twelve encoded direct voice packets\n";
    std::cout<<(worker_retention?"PASS foreground runtime polls asynchronous replay-executor/native-world retirement and reconstructs the owning value on its executor\n":"PASS same-tick unrecorded native mutation rejected and repaired before readiness\n");
    std::cout<<"PASS runtime completed-frame lease, rejection without ownership loss, private staging, atomic Match/transport adoption and resident cache lifetime\n";
    std::cout<<"PASS restored runtime camera ownership, three real pre-cut DTLS couch pulses, two retained native input ticks, same-tick idempotence and fresh-input ownership after stepping\n";
    std::cout<<"LIMIT in-process executors with three real direct sessions; internal migration gate, automatic measured candidate choice, explicit native activation, no renderer/controller/headset/WAN or default runtime acceptance\n";
    return 0;
}catch(const std::exception &e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
