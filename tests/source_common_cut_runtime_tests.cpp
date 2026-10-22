#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_receipt_continuity.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/peer_common_cut.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>
#include <utility>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
std::string stage="setup";
void check(bool value,const char *why){if(!value)throw FormatError(why);}
bool same_peer_envelopes(const SoundClassFades &a,const SoundClassFades &b){
    for(std::size_t i=0;i<sound_class_count;++i){
        const auto &x=a.state()[i],&y=b.state()[i];
        if(std::bit_cast<std::uint32_t>(x.target)!=std::bit_cast<std::uint32_t>(y.target)||
            std::bit_cast<std::uint32_t>(x.current)!=std::bit_cast<std::uint32_t>(y.current)||
            std::bit_cast<std::uint32_t>(x.remaining)!=std::bit_cast<std::uint32_t>(y.remaining))return false;
    }
    return true;
}
const auto deadline=std::chrono::steady_clock::now()+240s;
std::function<void(std::string_view)> diagnostic;
unsigned diagnostic_reports{};
void diagnose(std::string_view reason)noexcept {
    try{if(diagnostic)diagnostic(reason);}catch(...){std::cerr<<"DIAGNOSTIC common-cut status unavailable\n";}
}
template<class F>void wait(F &&ready,const char *why){auto next_report=std::chrono::steady_clock::now()+5s;
    for(;;){bool done{};try{done=ready();}catch(...){diagnose("predicate exception");throw;}if(done)return;
        const auto now=std::chrono::steady_clock::now();
        if(now>=deadline){diagnose("deadline");check(false,why);}
        if(now>=next_report&&diagnostic_reports<24){++diagnostic_reports;diagnose("waiting");next_report=now+15s;}
        std::this_thread::sleep_for(5ms);
    }}
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();
    std::array<std::uint8_t,9> appearance;std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
    return native_selected_player_configuration(name,appearance,-1);}

// A headless completed CPU-effects frontier. The exact source scene/device/
// prop replica and every network origin are real; this deliberately does not
// stand in for a renderer, a playing SDL voice or a full live adopter.
struct Foreground {
    MultiplayerRuntimeAuthority &runtime;
    std::shared_ptr<OnlineMatchSession> session;
    Mesh mesh;
    std::uint32_t primary=0x713719u,auxiliary=0x511599u;
    SourceEffectRandom random{primary,auxiliary};
    NativeWorldEffects world;
    NativeMuzzleEffects muzzle;
    SourceMapEffects map;
    SourceCombatAttachments combat;
    EffectPresentationContinuity effects;
    NativeEffectSounds sounds;
    EffectPresentationHistory history;
    MultiplayerPoseContinuity poses;
    MultiplayerRuntimeReceiptValues values;
    MultiplayerState display_current,display_previous;
    MultiplayerSnapshotOrigin display_origin;
    std::optional<MultiplayerSnapshotOrigin> current,previous;
    std::optional<PeerCommonCut> round;
    std::optional<EffectPresentationHistory::RuntimeReservation> reservation;
    std::shared_ptr<const MultiplayerRuntimeAuthority::NativeCutPin> native;
    std::optional<EffectPresentationHistory::RuntimeCheckpoint> bound;
    bool associate_enabled{};
    std::uint64_t accepted{};
    Foreground(MultiplayerRuntimeAuthority &r,std::shared_ptr<OnlineMatchSession> s)
        :runtime(r),session(std::move(s)),
        world(*r.world().cache,mesh,r.match().simulation().collision(),random),muzzle(*r.world().cache,random),
        map(*r.world().cache,r.world().content.objects,world),combat(*r.world().cache,world,{}),
        effects(muzzle,world,map,combat,random),sounds(*r.world().cache),
        history(effects,sounds,r.match().simulation(),128){
        // Declared local envelope requests exercise the receipt companion;
        // they do not claim a native game-over event or alter host classes.
        values.peer_sound_classes.set("game_event",.25f,3.f);
        values.peer_sound_classes.set("ambient",.5f,6.f);
        values.peer_sound_classes.advance(.125f);
    }
    void offer(){
        if(!current)return;
        const auto common=session->common_cut();if(!common||!common->pending)return;
        if(common->abort){native.reset();bound.reset();reservation.reset();return;}
        if(round==common->pending)return;
        native.reset();bound.reset();reservation.reset();round=common->pending;
        reservation=history.reserve_runtime(round->context,round->seal);
    }
    void capture(const MultiplayerSnapshotOrigin &origin){
        if(current&&current->same(origin))return;
        const auto &state=origin.accepted_state();
        check(state.source_scene_roots_authoritative,"Actual native host omitted its authoritative replica scene");
        auto &match=runtime.match();
        match.replicate_devices(state.devices);
        match.replicate_source_scene(state.source_scene_roots,state.source_body_poses);
        match.replicate_prop_removals(state.prop_removals);
        match.replicate_prop_regions(state.prop_regions);
        if(!current){history.context(session->authority_context());history.begin_runtime(*runtime.client());previous=origin;}
        else if(origin.tick()!=current->tick())previous=current;
        if(!current||origin.tick()!=current->tick()){
            world.advance(*origin.tick(),tick_seconds);muzzle.advance(*origin.tick(),tick_seconds);
        }
        MultiplayerRuntimeReceiptFrontier frontier;
        frontier.peer_sound_classes=values.peer_sound_classes;
        frontier.peer_game_over_fade_started=values.peer_game_over_fade_started;
        frontier.previous=*previous;frontier.replica=match.replica_checkpoint(origin);
        frontier.local_slots.assign(origin.local_slots().begin(),origin.local_slots().end());
        frontier.client_state_bound=frontier.joined=true;
        frontier.world_effect_tick=origin.tick();frontier.muzzle_pose_tick=origin.tick();
        auto &pose=poses.state();
        pose.scene_interpolation.reset(state.tick,match.simulation().scene().objects());
        pose.client_fp_reported_units[0]=*origin.sequence();
        pose.client_fp_views[0].unit=state.players[origin.local_slots().front()].source_unit;
        frontier.pose=poses.capture(origin);
        check(history.remember(origin,frontier),"Fresh accepted source receipt did not complete one typed history entry");
        values=frontier;display_current=state;display_previous=previous->accepted_state();display_origin=origin;
        current=origin;++accepted;
    }
    void receive(){
        auto &client=*runtime.client();client.receive();runtime.receive();
        if(!current){const auto origin=client.state_origin();
            if(origin.context()==session->authority_context()&&origin.context()&&!origin.local_slots().empty())capture(origin);
            return;
        }
        for(const auto &origin:client.received_origins())if(origin.context()==session->authority_context())capture(origin);
    }
    void service(){
        offer();runtime.service();offer();
        const auto pin=runtime.pending_native_cut();
        if(pin&&round&&reservation&&pin->cut().context==round->context&&pin->cut().seal==round->seal){
            native=pin;
            if(!bound)bound=history.bind_runtime(pin->host_proof(),*runtime.client(),*reservation);
            if(bound&&associate_enabled){const auto common=session->common_cut();
                if(common&&common->pending==round&&!common->abort&&!(common->associated&(1u<<session->self())))
                    check(runtime.associate_common_cut(pin,*bound),"Actual native/foreground owner association refused");}
        }
        const auto shadow=runtime.shadow_state();if(shadow)check(shadow->error.empty(),shadow->error.c_str());
    }
};
}

int main(int argc,char **argv)try{
    std::cout<<std::unitbuf;check(argc==5,"Supply ORIGIN DESCRIPTORS (XBE | --mcc) MAP");
    const bool selected_mcc=std::string_view(argv[3])=="--mcc";
    if(selected_mcc){
        check(std::filesystem::is_regular_file(argv[4]),"Explicit MCC common-cut map is missing");
        const auto map=std::filesystem::path(argv[4]);
        const Cache cache(map,map.parent_path(),std::shared_ptr<const OriginalData>{});
        check(cache.is_mcc()&&!cache.is_vista()&&cache.original_data()==OriginalData::native_tu15(),
            "MCC common-cut route requires imported MCC assets and compiled native TU1.5 gameplay");
        std::cout<<"OBSERVED selected MCC common-cut input; external_xbe=0; no asset import performed\n";
    }else{
        if(!std::filesystem::is_regular_file(argv[3])||!std::filesystem::is_regular_file(argv[4]))return 77;
        std::cout<<"REFERENCE offline Xbox TU1.5 common-cut route; not MCC runtime acceptance\n";
    }
    nlohmann::json descriptors;{std::ifstream input(argv[2]);input>>descriptors;}
    std::vector<OnlineAdmission> roster;
    for(const auto &row:descriptors.at(0).at("players")){
        OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");p.peer=row.at("peer");p.guest=row.at("guest");
        const std::string identity=row.at("identity");check(identity.size()==16,"Invalid fixture admission identity extent");
        for(unsigned i=0;i<8;++i)p.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));roster.push_back(p);
    }
    check(roster.size()==12&&descriptors.size()==3,"Expected three physical sessions with four couch players each");
    std::array<std::shared_ptr<OnlineMatchSession>,16> sessions;
    for(const auto &d:descriptors){OnlineMatchTransportOptions transport;transport.direct_udp=true;transport.peer_host_migration=true;
        const unsigned self=d.at("slot");sessions[self]=std::make_shared<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),self,roster,true,false,false,transport);}
    const auto healthy=[&]{for(const auto &session:sessions)if(session)
        check(session->phase()!=OnlineMatchSession::Phase::Failed,session->error().c_str());};
    stage="direct loading";wait([&]{healthy();for(const auto &session:sessions)if(session&&session->phase()!=OnlineMatchSession::Phase::Loading)return false;return true;},"Direct loading timeout");
    const UiVariant variant{"Slayer",0,{{0x59,5},{10,0},{2,0}}};constexpr std::uint64_t fingerprint=0x15c071502;
    for(const auto &session:sessions)if(session)session->loaded(fingerprint,variant);
    wait([&]{healthy();for(const auto &session:sessions)if(session&&session->phase()!=OnlineMatchSession::Phase::Running)return false;return true;},"Direct running timeout");
    const unsigned host=sessions[0]->host();std::array<unsigned,2> clients{};unsigned count{};
    for(unsigned p=0;p<16;++p)if(sessions[p]){check(sessions[p]->host()==host,"Measured host selection disagreed");if(p!=host){check(count<2,"Too many physical clients");clients[count++]=p;}}
    check(count==2,"Missing independent physical recipient");
    const auto generation=*sessions[host]->authority_context();
    const auto recipient_mask=std::uint16_t(generation.peers&~(1u<<host));
    check(std::popcount(recipient_mask)==2,"Common-cut physical mask counted guest seats");
    std::array<MultiplayerOptions,16> options;
    std::array<std::shared_ptr<MultiplayerPreparedWorld>,16> worlds;
    stage="native worlds";
    for(unsigned p=0;p<16;++p)if(sessions[p]){
        auto &o=options[p];if(!selected_mcc)o.gameplay_xbe=argv[3];
        o.map=argv[4];o.shared_maps=o.map.parent_path();o.source_object_world=true;
        o.variant=variant;o.local_players=4;o.session_random_seed=0x71;o.online_match=sessions[p];
        for(const auto &player:roster)if(online_admission_peer(player)==p)o.selected_profiles.push_back(profile(player.name));
        worlds[p]=std::make_shared<MultiplayerPreparedWorld>(o,std::nullopt,std::make_shared<Clock>());
    }
    for(unsigned view=0;view<4;++view){const auto slot=sessions[host]->local_slots()[view];
        check(worlds[host]->match.join(roster[slot].name,-1,options[host].selected_profiles[view],slot)==slot,"Native local host birth changed an admitted slot");}
    worlds[host]->bind_source_local_views(sessions[host]->local_slots());
    auto lan_host=std::make_unique<LanHost>(worlds[host]->match,sessions[host],fingerprint,roster,variant);
    std::array<std::unique_ptr<MultiplayerRuntimeAuthority>,16> owners;
    owners[host]=std::make_unique<MultiplayerRuntimeAuthority>(options[host],worlds[host],std::move(lan_host),nullptr);
    std::array<std::unique_ptr<Foreground>,16> foreground;
    for(auto p:clients){auto native_client=std::make_unique<LanClient>(sessions[p],host,fingerprint,std::span<const std::int8_t>{},options[p].selected_profiles);
        auto worker=std::make_unique<SourceAuthorityReplayWorker>(options[p],std::make_shared<Clock>(),std::make_shared<Clock>());
        owners[p]=std::make_unique<MultiplayerRuntimeAuthority>(options[p],worlds[p],nullptr,std::move(native_client),std::move(worker));
        foreground[p]=std::make_unique<Foreground>(*owners[p],sessions[p]);}
    auto &authority=*owners[host];const auto producer=&authority.world();
    std::array<Controls,4> neutral{};std::array<Controls,multiplayer_capacity> controls{};
    auto last_broadcast=std::chrono::steady_clock::now()-1s;
    auto last_host_frame=last_broadcast;
    std::uint64_t recording_drops{};
    const auto drive=[&](bool publish=true,bool force_broadcast=false,bool advance=false){
        healthy();for(auto p:clients){foreground[p]->offer();owners[p]->client()->send(neutral);}
        const auto now=std::chrono::steady_clock::now();
        if(advance||now-last_host_frame>=(force_broadcast?25ms:100ms)){
            auto frame=authority.begin_frame();check(bool(frame),"Healthy native host refused an ordinary frame");
            controls={};authority.host()->receive(controls);authority.admit_players();
            if(advance){for(const auto &player:roster)if(authority.match().player(player.slot).connected)
                    controls[player.slot].yaw=authority.match().unit(player.slot).yaw;
                authority.match().step(controls);}
            // A genuinely advanced cut must include an actually emitted
            // same-tick snapshot in its native host continuation. A skipped
            // time gate would retain the prior tick's unprovable stamp.
            if(advance||now-last_broadcast>=(force_broadcast?25ms:100ms)){authority.host()->broadcast();last_broadcast=now;}
            // Losing a bounded successor journal does not undo completed live
            // work or the separately held common cut. The later real source
            // step and repaired native exports still require the unfaulted
            // authority guard; no fault is reset or bypassed here.
            if(!frame->finish())++recording_drops;
            last_host_frame=now;
        }
        for(auto p:clients){foreground[p]->receive();foreground[p]->service();}
        if(publish)authority.service();
        check(authority.host()->peers()==2||!foreground[clients[0]]->current||!foreground[clients[1]]->current,"A serviced native peer expired during common-cut coordination");
    };
    diagnostic=[&](std::string_view reason){
        // Bounded metadata only: no tokens, identity/profile names, session
        // digests, origin bodies, native banks or endpoint URLs are printed.
        std::cerr<<"DIAGNOSTIC common-cut stage="<<stage<<" reason="<<reason
            <<" producer_tick="<<authority.match().simulation().ticks()
            <<" native_peers="<<authority.host()->peers()<<" recording_drops="<<recording_drops<<'\n';
        for(unsigned p=0;p<16;++p)if(sessions[p]){
            const auto &session=*sessions[p];const auto common=session.common_cut();
            std::cerr<<"  peer="<<p<<" phase="<<unsigned(session.phase())<<" host="<<session.host();
            if(common){const auto label=[](const std::optional<PeerCommonCut> &cut){
                    if(cut)std::cerr<<cut->seal.id<<'@'<<cut->seal.tick<<'/'<<cut->seal.bytes;else std::cerr<<'-';};
                std::cerr<<" epoch="<<common->context.epoch<<" revision="<<common->context.membership_revision
                    <<" recipients="<<common->recipients<<" committed=";label(common->committed);
                std::cerr<<" pending=";label(common->pending);
                std::cerr<<" associated="<<common->associated<<" applied="<<common->applied<<" released="<<common->released
                    <<" aborted="<<common->aborted<<" native_all="<<common->native_all_validated
                    <<" commit="<<common->commit<<" release="<<common->release<<" abort="<<common->abort<<" rejected="<<common->rejected;
            }
            std::cerr<<'\n';
            if(!foreground[p])continue;const auto &f=*foreground[p];const auto history=f.history.state();const auto shadow=owners[p]->shadow_state();
            std::cerr<<"    foreground current="<<(f.current?f.current->tick().value_or(0):0)<<':'<<(f.current?f.current->sequence().value_or(0):0)
                <<" captured="<<history.captured<<" retained="<<history.retained<<" evicted="<<history.evicted
                <<" reservation="<<(f.reservation?f.reservation->retained():0)<<" overflow="<<(f.reservation&&f.reservation->overflowed())
                <<" associate_enabled="<<f.associate_enabled<<" bound="<<(f.bound?f.bound->cut().seal.id:0)
                <<" native_pin="<<(f.native?f.native->cut().seal.id:0)<<'\n';
            if(shadow)std::cerr<<"    native warmed="<<shadow->warmed<<'/'<<shadow->workspace_warmed
                <<" received="<<shadow->received<<" prepared="<<shadow->prepared<<" decoding="<<shadow->decoding
                <<" replaying="<<shadow->replaying<<" ready="<<(shadow->ready?shadow->ready->seal.id:0)
                <<" host_ready="<<shadow->host_ready<<" rejected="<<(shadow->rejected?shadow->rejected->seal.id:0)
                <<" error_present="<<!shadow->error.empty()<<'\n';
        }
    };
    stage="actual twelve-player admission";
    wait([&]{drive(false);return authority.host()->peers()==2&&foreground[clients[0]]->current&&foreground[clients[1]]->current&&
        sessions[host]->source_players()&&sessions[host]->common_cut()&&sessions[clients[0]]->common_cut()&&sessions[clients[1]]->common_cut();},
        "Actual native admission/feature negotiation did not complete");
    check(authority.world().source_objects&&authority.match().simulation().ticks()==0,"Initial actual source world was advanced before its declared cut");
    const auto offered=[&]() -> std::optional<PeerCommonCut> {
        const auto state=sessions[host]->common_cut();if(!state||!state->pending)return {};
        for(auto p:clients){const auto other=sessions[p]->common_cut();if(!other||other->pending!=state->pending)return {};}
        return state->pending;
    };
    const auto all_released=[&](const PeerCommonCut &cut){
        for(const auto &session:sessions)if(session){const auto state=session->common_cut();
            // Host committed certifies every Released ACK. A successor Offer
            // may already have reset the current-round diagnostic masks.
            if(!state||state->context!=cut.context||state->committed!=cut)return false;}
        for(auto p:clients){const auto pair=owners[p]->common_cut_checkpoint(cut.context,cut.seal);
            if(!pair||pair->cut().peer!=p||pair->cut().context!=cut.context||pair->cut().seal!=cut.seal||
                !pair->proof().matches(pair->origin())||!pair->frontier().replica.matches(pair->origin())||
                !owners[p]->match().owns_replica_checkpoint(pair->frontier().replica))return false;}
        return true;
    };
    const auto ready_native=[&](const PeerCommonCut &cut){for(auto p:clients){const auto &f=*foreground[p];
        if(!f.native||f.native->cut().peer!=p||f.native->cut().context!=cut.context||f.native->cut().seal!=cut.seal)return false;}return true;};
    const auto ready_pair=[&](const PeerCommonCut &cut){if(!ready_native(cut))return false;
        for(auto p:clients){const auto &bound=foreground[p]->bound;
            if(!bound||bound->cut().peer!=p||bound->cut().context!=cut.context||bound->cut().seal!=cut.seal||
                !bound->proof().matches(bound->origin())||!bound->frontier().replica.matches(bound->origin()))return false;}
        return true;};
    const auto require_live=[&]{
        check(&authority.world()==producer&&authority.host()&&!authority.client(),"Common recovery-pin publication changed the live host role");
        for(auto p:clients){check(&owners[p]->world()==worlds[p].get()&&!owners[p]->host()&&owners[p]->client(),"Common recovery-pin publication activated a client world");
            check(sessions[p]->phase()==OnlineMatchSession::Phase::Running&&sessions[p]->authority_context()==generation,
                "Common metadata publication elected or fenced a native generation");
            if(foreground[p]->current)check(owners[p]->match().replica_checkpoint(*foreground[p]->current).matches(*foreground[p]->current),
                "Common recovery-pin service restored a historical replica into the live receipt");}
    };
    stage="first exact common offer";std::optional<PeerCommonCut> first;
    wait([&]{drive();first=offered();return first.has_value();},"Actual runtime publisher never offered its exact all-peer cut");
    check(first->context==generation&&first->recipients==recipient_mask,"Runtime Offer changed native generation or fixed physical membership");
    foreground[clients[0]]->associate_enabled=true;
    stage="independent native exports";
    wait([&]{drive();return ready_pair(*first)&&sessions[host]->common_cut()->native_all_validated&&
        (sessions[host]->common_cut()->associated&(1u<<clients[0]));},"Both receiver-native exports/first foreground association did not finish");
    auto first_native_a=foreground[clients[0]]->native,first_native_b=foreground[clients[1]]->native;
    check(first_native_a!=first_native_b&&first_native_a->cut().peer==clients[0]&&first_native_b->cut().peer==clients[1],"Independent receivers shared an in-process native proof");
    for(auto p:clients){const auto &f=*foreground[p];const auto &retained=f.native->retained_world();
        check(retained.host&&retained.checkpoint.match.simulation.ticks==first->seal.tick&&
            std::ranges::equal(retained.local_views,sessions[p]->local_slots())&&
            std::ranges::equal(retained.producer_views,sessions[host]->local_slots())&&
            f.bound->proof().matches(f.bound->origin())&&f.bound->frontier().replica.matches(f.bound->origin())&&
            f.native->host_proof().prove_snapshot(*owners[p]->client(),f.bound->origin()).matches(f.bound->origin()),
            "Native pin did not prove this receiver's own completed exact foreground receipt");}
    check(!owners[clients[1]]->associate_common_cut(first_native_a,*foreground[clients[1]]->bound)&&
        !owners[clients[1]]->associate_common_cut(first_native_b,*foreground[clients[0]]->bound),
        "Foreign native/foreground owners completed another recipient's association");
    check(!sessions[host]->common_cut()->commit&&sessions[host]->common_cut()->associated==std::uint16_t(1u<<clients[0]),
        "Native-all ACKs or the first foreground association manufactured the missing owner");
    std::cout<<"PASS native common-cut runtime owners: two independently validated receiver pins and exact foreground receipts\n";
    stage="delayed association beyond ordinary history eviction";
    const auto first_origin_a=foreground[clients[0]]->bound->origin(),first_origin_b=foreground[clients[1]]->bound->origin();
    const std::array first_frontiers{*foreground[clients[0]]->bound,*foreground[clients[1]]->bound};
    wait([&]{drive(true,true);return foreground[clients[0]]->history.state().captured>=260&&foreground[clients[1]]->history.state().captured>=260;},
        "Actual same-tick receipt history did not turn over while the second association was delayed");
    for(auto p:clients){auto &f=*foreground[p];
        check(f.history.state().retained==128&&f.history.state().evicted>=132&&f.reservation&&f.reservation->overflowed()&&f.reservation->retained()==128,
            "Ordinary history/reservation did not exercise its actual bounded eviction policy");
        check(!f.history.bind_runtime(f.native->host_proof(),*owners[p]->client())&&f.bound&&
            f.bound->proof().matches(f.bound->origin())&&f.bound->frontier().replica.matches(f.bound->origin()),
            "Already-bound exact owning checkpoint did not survive ordinary history eviction");}
    check(owners[clients[0]]->common_cut_checkpoint(generation,first->seal)&&!owners[clients[1]]->common_cut_checkpoint(generation,first->seal)&&
        foreground[clients[0]]->native==first_native_a&&foreground[clients[1]]->native==first_native_b&&!sessions[host]->common_cut()->commit,
        "Delayed association dropped its native pins or implicitly committed the second foreground");
    foreground[clients[1]]->associate_enabled=true;
    wait([&]{drive();require_live();return all_released(*first);},"First actual common round did not finish every owner release");
    for(auto p:clients){const auto saved=owners[p]->common_cut_checkpoint(generation,first->seal);
        check(saved&&saved->proof().matches(saved->origin())&&saved->origin().same(p==clients[0]?first_origin_a:first_origin_b),
            "Released runtime pair lost its exact original foreground receipt");}
    std::cout<<"PASS native common-cut delayed association survives history eviction: captured="<<foreground[clients[0]]->history.state().captured
        <<'/'<<foreground[clients[1]]->history.state().captured<<" retained=128 reservation=128; exact native pins and receipts retained\n";

    foreground[clients[1]]->associate_enabled=false;
    stage="second native cut before controlled foreground rejection";std::optional<PeerCommonCut> aborted;
    wait([&]{drive();const auto value=offered();if(value&&value->seal.id>first->seal.id)aborted=value;
        return aborted&&ready_native(*aborted);},
        "Second real offered cut did not retain both independent exact native pins");
    check(!sessions[host]->common_cut()->commit,"Delayed second owner could not prevent premature second Commit");
    unsigned missing_foregrounds{};
    for(auto p:clients){const auto prior=owners[p]->common_cut_checkpoint(generation,first->seal);
        const auto pending=owners[p]->common_cut_checkpoint(generation,aborted->seal);
        const auto local=sessions[p]->common_cut();const auto &f=*foreground[p];
        check(prior&&prior->origin().same(p==clients[0]?first_origin_a:first_origin_b)&&prior->proof().matches(prior->origin()),
            "New native work displaced the prior released exact foreground pair");
        check(f.native->retained_world().host&&f.native->retained_world().checkpoint.match.simulation.ticks==aborted->seal.tick&&
            f.native->host_proof().cut().peer==p&&f.native->host_proof().cut().seal==aborted->seal,
            "Second native pin lacks its actual independently validated paired world");
        const bool associated=local&&local->pending==aborted&&(local->associated&(1u<<p));
        if(associated)check(pending&&f.bound&&pending->origin().same(f.bound->origin())&&pending->proof().matches(pending->origin()),
            "Actual local association lost its privately retained exact foreground pair");
        else check(!pending,"Unassociated native cut fabricated a retained foreground pair");
        if(!f.bound)++missing_foregrounds;
    }
    check(!foreground[clients[1]]->associate_enabled,"Controlled rejecting recipient was allowed to associate");
    diagnose("before explicit pre-Commit foreground Reject");
    check(sessions[clients[1]]->reject_common_cut(*aborted),"Missing foreground association could not request pre-Commit repair");
    drive(false,true,true); // One real recorded source step, with no activation.
    check(authority.match().simulation().ticks()>first->seal.tick,"Later genuine native tick was not executed");
    stage="real advanced receipts before repaired native publication";
    wait([&]{drive(false);for(auto p:clients){const auto &f=*foreground[p];
        if(!f.current||f.current->tick()!=authority.match().simulation().ticks()||
            f.history.state().newest_tick!=authority.match().simulation().ticks())return false;}return true;},
        "Advanced native frame did not deliver actual completed receipts to both clients");
    stage="explicit pending-owner Abort retirement";
    wait([&]{drive(false);const auto common=sessions[host]->common_cut();
        return common&&common->pending==aborted&&common->abort&&common->aborted==recipient_mask;},
        "Every real receiver did not retire its aborted pending owners");
    for(auto p:clients){check(owners[p]->common_cut_checkpoint(generation,first->seal)&&!owners[p]->common_cut_checkpoint(generation,aborted->seal)&&
            !owners[p]->pending_native_cut(),"Abort lost old native/foreground owners or retained pending eligibility");}
    require_live();
    std::cout<<"PASS native common-cut rejection/Abort repair preserves prior native/foreground owners: unbound_foregrounds="<<missing_foregrounds<<'\n';

    stage="repaired later native common round";std::optional<PeerCommonCut> repaired;
    wait([&]{drive();const auto value=offered();if(value&&value->seal.id>aborted->seal.id)repaired=value;
        return repaired&&ready_pair(*repaired)&&(sessions[host]->common_cut()->associated&(1u<<clients[0]));},
        "Aborted native stream did not repair into a fresh independent common round");
    check(repaired->seal.tick>first->seal.tick&&!sessions[host]->common_cut()->commit,"Repair reused the aborted seal or implicitly associated the delayed owner");
    for(auto p:clients)check(owners[p]->common_cut_checkpoint(generation,first->seal).has_value(),"Repair discarded the prior released native pair before all new owners applied");
    foreground[clients[1]]->associate_enabled=true;
    wait([&]{drive();require_live();return all_released(*repaired);},"Second released native common round did not finish");
    for(auto p:clients){auto &f=*foreground[p];const auto saved=owners[p]->common_cut_checkpoint(generation,repaired->seal);
        check(saved&&saved->cut().peer==p&&saved->proof().matches(saved->origin())&&saved->frontier().replica.matches(saved->origin())&&
            !owners[p]->common_cut_checkpoint(generation,first->seal),"Second release retained stale internal eligibility or lost its exact complete pair");
        f.history.context(std::nullopt);f.reservation.reset();f.bound.reset();f.native.reset();
        const auto independently_retained=owners[p]->common_cut_checkpoint(generation,repaired->seal);
        check(independently_retained&&independently_retained->origin().same(saved->origin())&&
            independently_retained->proof().matches(saved->origin()),"Ordinary history retirement destroyed the runtime's strong common pair");}
    require_live();
    check(first_native_a->host_proof().prove_snapshot(*owners[clients[0]]->client(),first_origin_a).matches(first_origin_a)&&
        first_native_b->host_proof().prove_snapshot(*owners[clients[1]]->client(),first_origin_b).matches(first_origin_b),
        "Historical independently owned native proofs were invalidated by worker advancement or common release");
    std::cout<<"PASS native common-cut two released rounds retain exact proofs without live activation: first="<<first->seal.id<<'@'<<first->seal.tick
        <<" aborted="<<aborted->seal.id<<" repaired="<<repaired->seal.id<<'@'<<repaired->seal.tick<<" physical_recipients=2 couch_players=12\n";
    std::cout<<"OBSERVED source successor recording drops="<<recording_drops<<"; subsequent genuine source step and repaired native validation required\n";
    stage="composed historical receipt/pose/effect adoption";
    for(unsigned n=0;n<clients.size();++n){auto &f=*foreground[clients[n]];const auto &saved=first_frontiers[n];
        MultiplayerReceiptContinuity continuity({f.values,f.display_current,f.display_previous,f.display_origin},f.effects,f.sounds,f.poses);
        const auto &names=sound_class_names();const auto event_class=std::ranges::find(names,"game_event");
        check(event_class!=names.end(),"Peer envelope control has no actual game-event class");
        check(saved.frontier().peer_sound_classes.state()[std::size_t(event_class-names.begin())].remaining>0,
            "Saved peer envelope lacks its declared unfinished game-event fade");
        f.values.peer_sound_classes.set("",.8f,9.f);f.values.peer_sound_classes.advance(.5f);
        f.values.peer_game_over_fade_started=!saved.frontier().peer_game_over_fade_started;
        const auto later_peer_classes=f.values.peer_sound_classes;
        const bool later_peer_game_over=f.values.peer_game_over_fade_started;
        check(!same_peer_envelopes(later_peer_classes,saved.frontier().peer_sound_classes),
            "Peer envelope restore lacks a genuinely different live envelope control");
        const auto later=f.display_origin;
        check(!later.same(saved.origin())&&f.display_current.tick>saved.current().tick,
            "Receipt restore did not retain a genuinely later live state control");
        const auto unchanged=[&]{return f.display_origin.same(later)&&f.display_current.tick==later.tick()&&
            std::as_const(f.poses).state().client_fp_reported_units[0]==later.sequence()&&
            same_peer_envelopes(f.values.peer_sound_classes,later_peer_classes)&&
            f.values.peer_game_over_fade_started==later_peer_game_over;};
        {auto cancelled=continuity.prepare(saved,{},{});check(continuity.ready(cancelled,saved.cut()),"Exact saved receipt did not prepare");}
        check(unchanged(),"Cancelled receipt preparation changed live values or pose owners");
        bool foreign_rejected{};try{(void)continuity.prepare(first_frontiers[1-n],{},{});}catch(const FormatError &){foreign_rejected=true;}
        check(foreign_rejected&&unchanged(),"Foreign recipient receipt changed or prepared this foreground owner");
        auto stale=continuity.prepare(saved,{},{});f.poses.mutating();
        check(!continuity.ready(stale,saved.cut())&&!continuity.commit(std::move(stale),saved.cut())&&unchanged(),
            "A later pose mutation did not reject the entire receipt publication");
        auto prepared=continuity.prepare(saved,{},{});auto wrong=saved.cut();++wrong.seal.id;
        check(!continuity.commit(std::move(prepared),wrong)&&continuity.ready(prepared,saved.cut())&&unchanged(),
            "Wrong-cut publication consumed a valid prepared receipt or altered live values");
        bool foreign_ready=true;std::thread observer([&]{foreign_ready=continuity.ready(prepared,saved.cut());});observer.join();
        check(!foreign_ready&&continuity.ready(prepared,saved.cut()),"Receipt preparation crossed its executor");
        check(continuity.commit(std::move(prepared),saved.cut())&&!continuity.commit(std::move(prepared),saved.cut())&&
            f.display_origin.same(saved.origin())&&f.display_current.tick==saved.current().tick&&
            f.display_previous.tick==saved.previous().tick&&f.values.previous.same(saved.frontier().previous)&&
            f.values.local_slots==saved.frontier().local_slots&&f.values.world_effect_tick==saved.frontier().world_effect_tick&&
            std::as_const(f.poses).state().client_fp_reported_units[0]==saved.origin().sequence()&&
            same_peer_envelopes(f.values.peer_sound_classes,saved.frontier().peer_sound_classes)&&
            f.values.peer_game_over_fade_started==saved.frontier().peer_game_over_fade_started,
            "Composed receipt publication did not atomically restore exact historical owners once");
        auto expected=saved.frontier().peer_sound_classes;expected.advance(.03125f);
        f.values.peer_sound_classes.advance(.03125f);
        check(same_peer_envelopes(f.values.peer_sound_classes,expected),
            "Restored peer envelope did not continue the original fade from its exact historical phase");
    }
    require_live();
    std::cout<<"PASS native common-cut composed receipt adoption: exact effects/pose/frontiers/deferred cues; cancellation, foreign owner, stale mutation, wrong seal and executor controls\n";
    std::cout<<"PASS native common-cut peer audio envelopes: all54 target/current/remaining triples and game-over latch; continued fade suffix\n";
    if(selected_mcc)std::cout<<"PASS selected-MCC-native common-cut receipt route: three physical sessions and twelve couch players\n";
    std::cout<<"LIMIT headless CPU/native owner-pin integration; no full renderer/HUD/input/playing-audio adopter, host-loss activation, WAN or physical controller/headset acceptance\n";
    return 0;
}catch(const std::exception &error){std::cerr<<stage<<": "<<error.what()<<'\n';return 1;}
