#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_receipt_continuity.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/peer_common_cut.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/cache.hpp"
#include "helpers/source_common_client.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdlib>
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
template<class F>void rejects(F &&work){bool refused{};try{work();}catch(const FormatError &){refused=true;}check(refused,"Invalid native preparation ownership accepted");}
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
struct CountingClock final:PhysicsLiveClockSource {
    mutable std::atomic_uint64_t calls{};
    std::uint32_t milliseconds()const noexcept override{++calls;return 1000;}
    std::uint64_t cycles()const noexcept override{++calls;return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{++calls;return 132000000000000000ull;}
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
    bool associate_enabled{},retirement_gated{};
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
        if(common->abort){
            if(!retirement_gated){native.reset();bound.reset();reservation.reset();}
            return;
        }
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
        // Distinct declared local selectors at authentic receipt boundaries.
        // The separate source-camera suffix regression covers the polynomial.
        for(unsigned view=0;view<origin.local_slots().size();++view){
            const auto &player=state.players[origin.local_slots()[view]];
            pose.view_zoom[view]=std::int16_t((*origin.sequence()+view)%3)-1;
            pose.view_zoom_unit[view]=player.source_unit;
            pose.view_zoom_reset[view]=player.zoom_reset_serial;
            pose.view_was_dead[view]=player.dead;
        }
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
    const bool prospective_mode=std::getenv("HALO2_COMMON_PROSPECTIVE")!=nullptr;
    const bool client_mode=std::getenv("HALO2_COMMON_CLIENT")!=nullptr;
    const bool client_abort_mode=std::getenv("HALO2_COMMON_CLIENT_ABORT")!=nullptr;
    const bool disconnect_mode=std::getenv("HALO2_COMMON_DISCONNECT")!=nullptr;
    const std::string inventory_mode=std::getenv("HALO2_COMMON_INVENTORY")?std::getenv("HALO2_COMMON_INVENTORY"):"";
    check(!disconnect_mode||(!prospective_mode&&!client_mode&&!client_abort_mode&&inventory_mode.empty()),
        "Ordinary disconnect recovery requires its separate unmodified native common-cut route");
    check(inventory_mode.empty()||inventory_mode=="abort"||inventory_mode=="freeze"||inventory_mode=="preparation","Unknown retained inventory fixture");
    check(inventory_mode.empty()||(selected_mcc&&!prospective_mode),"Retained inventory controls require their separate selected MCC route");
    check(!prospective_mode||selected_mcc,"Prospective native/audio binding control requires the selected MCC input");
    check(!client_mode||(selected_mcc&&prospective_mode&&inventory_mode.empty()&&!client_abort_mode),
        "Prospective client alternatives require their separate selected MCC/prospective route");
    check(!client_abort_mode||(selected_mcc&&inventory_mode=="abort"&&!prospective_mode&&!client_mode),
        "Prospective client Abort requires its separate actual pending inventory route");
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
    for(const auto &d:descriptors){OnlineMatchTransportOptions transport;transport.direct_udp=true;transport.peer_host_migration=!disconnect_mode;
        transport.survivor_cut_selection=inventory_mode=="freeze"||inventory_mode=="preparation";
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
    // The ordinary disconnect control deliberately prepares genuine owners
    // without election. Its explicit purpose must not enroll normal v14 play.
    using Preparation=MultiplayerRuntimeAuthority::RecoveryPreparation;
    const auto preparation=disconnect_mode?Preparation::PreparationOnly:Preparation::Automatic;
    for(const auto &session:sessions)if(session){
        check(MultiplayerRuntimeAuthority::recovery_preparation_enabled(session)==!disconnect_mode,
            "Automatic native recovery preparation differs from negotiated continuity");
        check(MultiplayerRuntimeAuthority::recovery_preparation_enabled(session,Preparation::PreparationOnly),
            "Explicit native preparation-only owner was refused");
        if(disconnect_mode)check(!session->continuity()&&!session->peer_migration(),
            "Preparation-only admission enabled transport continuity or election");
    }
    std::array<std::unique_ptr<MultiplayerRuntimeAuthority>,16> owners;
    owners[host]=std::make_unique<MultiplayerRuntimeAuthority>(options[host],worlds[host],std::move(lan_host),nullptr,nullptr,preparation);
    std::array<std::unique_ptr<Foreground>,16> foreground;
    for(auto p:clients){auto native_client=std::make_unique<LanClient>(sessions[p],host,fingerprint,std::span<const std::int8_t>{},options[p].selected_profiles);
        auto worker=std::make_unique<SourceAuthorityReplayWorker>(options[p],std::make_shared<Clock>(),std::make_shared<Clock>());
        owners[p]=std::make_unique<MultiplayerRuntimeAuthority>(options[p],worlds[p],nullptr,std::move(native_client),std::move(worker),preparation);
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
            const auto migration=sessions[p]->peer_migration();
            check(migration&&!migration->decision&&std::ranges::none_of(migration->ready,[](const auto &cut){return bool(cut);}),
                "Private native reconstruction advertised Ready or learned an unrequested election");
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
    for(auto p:clients){
        check(owners[p]->recovery_eligibility()==MultiplayerRuntimeAuthority::RecoveryEligibility::Unavailable,
            "Common native ownership without a learned certificate admitted a disconnect recovery wait");
        const auto before_pin=owners[p]->pending_native_cut();
        check(before_pin&&!owners[p]->recover()&&owners[p]->pending_native_cut()==before_pin&&owners[p]->shadow_state(),
            "Unavailable disconnect recovery exported a cut, retired a worker or consumed a genuine native pin");
    }
    if(disconnect_mode){
        check(std::ranges::all_of(clients,[&](unsigned p){return !sessions[p]->peer_migration();}),
            "Ordinary disconnect regression accidentally enabled a peer-migration decision producer");
        for(auto p:clients){
            auto frame=owners[p]->begin_frame();check(bool(frame),"Eligibility refusal fenced an ordinary live client frame");
            rejects([&]{(void)owners[p]->recovery_eligibility();});check(frame->finish(),"Eligibility frame did not finish");
            std::atomic<bool> foreign_refused{};
            {std::jthread foreign([&]{try{(void)owners[p]->recovery_eligibility();}catch(const FormatError &){foreign_refused=true;}});}
            check(foreign_refused,"Foreign executor inspected native disconnect recovery eligibility");
            owners[p]->cancel_recovery();
            check(owners[p]->recovery_eligibility()==MultiplayerRuntimeAuthority::RecoveryEligibility::Unavailable&&
                owners[p]->pending_native_cut(),"Ordinary recovery cancellation forged eligibility or discarded its native pin");
        }
        std::cout<<"PASS genuine ordinary disconnect recovery refusal: authenticated native common pins, no migration agreement, no exported fallback or recovery wait; creator/frame guards preserved\n";
        return 0;
    }
    std::cout<<"PASS native common-cut runtime owners: two independently validated receiver pins and exact foreground receipts\n";
    stage="private first pending common native import before Release";
    const auto candidate=clients[0];const ValidatedAuthorityCut preimported_cut{candidate,generation,first->seal};
    const auto prepared_names=[&]{const auto cut=owners[candidate]->prepared_common_native();
        return cut&&cut->peer==preimported_cut.peer&&cut->context==preimported_cut.context&&cut->seal==preimported_cut.seal;};
    auto imported_clock=std::make_shared<CountingClock>(),unused_clock=std::make_shared<CountingClock>();
    check(owners[candidate]->prepare_common_native(preimported_cut,imported_clock)&&imported_clock->calls>0&&prepared_names(),
        "Cold decision-free pending import did not execute real native clock consumers on its genuinely associated pair");
    check(owners[candidate]->prepare_common_native(preimported_cut,unused_clock)&&unused_clock->calls==0,
        "Repeated exact preparation reconstructed a second native target");
    std::optional<MccSoundVariationProvider::NativeBinding> prospective_binding;
    std::optional<SourceNativePublication::Boundary> prospective_boundary;
    Mesh prospective_mesh;
    std::unique_ptr<SourceWorldParticleDrawsTu15> prospective_particles;
    std::unique_ptr<SourceBspEffectLights> prospective_models;
    std::unique_ptr<SourceForegroundWorldHandoff> prospective_foreground;
    std::optional<SourceForegroundWorldHandoff::Prepared> prospective_foreground_prepared;
    MultiplayerObjectRuntime *prospective_particle{},*prospective_model{};
    std::uint64_t prospective_clock_calls{};
    if(prospective_mode){
        stage="private Rust prospective role before election";
        const auto role=derive_peer_prospective_host_role(generation,preimported_cut.seal,candidate);
        check(owners[candidate]->prepare_common_prospective(preimported_cut,unused_clock)&&
            owners[candidate]->prepared_common_prospective()==role&&unused_clock->calls==0&&prepared_names(),
            "Rust prospective role did not finalize the same private native import before election");
        check(!owners[candidate]->common_prospective_audio_binding(preimported_cut),
            "Unsealed prospective native owner minted an audio publication binding");
        check(!owners[candidate]->common_prospective_boundary(preimported_cut),
            "Unsealed prospective native owner minted a held foreground witness");
        // Headless routing/collision control: actual native target and cut,
        // deliberately disabled render policies. The separate resident-resource
        // fixture exercises the enabled model/particle branch.
        const auto cache=owners[candidate]->world().cache;
        prospective_particles=std::make_unique<SourceWorldParticleDrawsTu15>(*cache,owners[candidate]->world().content.objects);
        prospective_models=std::make_unique<SourceBspEffectLights>(*cache,prospective_mesh,0,std::span<const std::uint32_t>{});
        prospective_foreground=std::make_unique<SourceForegroundWorldHandoff>(cache,prospective_mesh,
            *prospective_particles,*prospective_models,prospective_mesh.materials.size(),
            SourceForegroundWorldHandoff::Policy{false,false},prospective_particle,prospective_model);
        prospective_foreground_prepared.emplace(owners[candidate]->prepare_common_foreground(preimported_cut,*prospective_foreground));
        check(prospective_foreground->ready(*prospective_foreground_prepared),
            "Actual prospective foreground preparation was not ready before sealing");
        check(owners[candidate]->seal_common_prospective(preimported_cut),"Prospective target did not seal its actual native scope");
        check(!prospective_foreground->ready(*prospective_foreground_prepared),
            "Unbound foreground preparation bypassed the genuine native exclusion");
        prospective_boundary=owners[candidate]->common_prospective_boundary(preimported_cut);
        check(prospective_boundary&&prospective_boundary->current()&&
            !prospective_foreground->bind_publication(*prospective_foreground_prepared,{})&&
            prospective_foreground->bind_publication(*prospective_foreground_prepared,*prospective_boundary)&&
            prospective_foreground->bind_publication(*prospective_foreground_prepared,*prospective_boundary)&&
            prospective_foreground->ready(*prospective_foreground_prepared),
            "Actual prospective foreground did not bind once to its genuine derived native boundary");
        prospective_particle=worlds[host]->source_objects.get();
        check(prospective_particle!=nullptr,"Routing refusal control lost its genuine current host world");
        check(!prospective_foreground->ready(*prospective_foreground_prepared)&&
            !prospective_foreground->commit(*prospective_foreground_prepared),
            "Bound prospective foreground accepted a changed outgoing routing slot");
        prospective_particle=nullptr;
        check(prospective_foreground->ready(*prospective_foreground_prepared),
            "Routing refusal consumed the exact prospective foreground owner");
        prospective_binding=owners[candidate]->common_prospective_audio_binding(preimported_cut);
        check(prospective_binding&&prospective_binding->current(),"Sealed prospective target did not mint a genuine native audio binding");
        prospective_clock_calls=imported_clock->calls.load();
        check(owners[candidate]->prepare_common_prospective(preimported_cut,unused_clock)&&
            owners[candidate]->seal_common_prospective(preimported_cut)&&unused_clock->calls==0&&
            imported_clock->calls==prospective_clock_calls&&prospective_binding->current(),
            "Repeated prospective preparation reconstructed native state or replaced its actual held scope");
        auto wrong=preimported_cut;++wrong.seal.tick;
        check(!owners[candidate]->seal_common_prospective(wrong)&&!owners[candidate]->common_prospective_audio_binding(wrong)&&
            !owners[candidate]->common_prospective_boundary(wrong)&&
            owners[candidate]->prepared_common_prospective()==role&&prospective_binding->current(),
            "Wrong cut changed the private prospective role or borrowed its actual native publication");
        bool foreign_refused{};std::thread foreign([&]{try{owners[candidate]->seal_common_prospective(preimported_cut);}
            catch(const FormatError &){foreign_refused=true;}});foreign.join();
        check(foreign_refused&&prospective_binding->current(),"Prospective native publication crossed its executor");
    }
    if(inventory_mode=="abort"){
        stage="actual retained pending inventory and Abort retirement";
        auto &runtime=*owners[candidate];
        check(!runtime.retain_common_inventory(0),"Zero inventory identity retained native owners");
        {auto frame=runtime.begin_frame();check(bool(frame),"Inventory frame exclusion control failed");
            rejects([&]{(void)runtime.retain_common_inventory(0xc071);});
            check(frame->finish(),"Inventory frame exclusion changed the live native owner");}
        auto lease=runtime.retain_common_inventory(0xc071);
        check(lease&&lease->current(),"Actual pending/imported pair did not create an opaque inventory lease");
        const auto inventory=runtime.common_inventory(*lease);
        check(inventory&&inventory->lease==0xc071&&std::ranges::count_if(inventory->cuts,[](const auto &cut){return bool(cut);})==1&&
            inventory->cuts[0]&&inventory->cuts[0]->cut==first->seal&&!inventory->cuts[0]->irreversible_common,
            "Pending/imported aliases were duplicated or native import fabricated a CommonCommit witness");
        const auto repeated=runtime.retain_common_inventory(0xc071);
        check(repeated&&repeated->current()&&!runtime.retain_common_inventory(0xc072)&&
            !owners[clients[1]]->common_inventory(*lease)&&!owners[clients[1]]->freeze_common_inventory(*lease),
            "An active native inventory was replaced or crossed runtime ownership");
        bool foreign_refused{},foreign_current{};std::thread foreign([&]{foreign_current=lease->current();try{(void)runtime.common_inventory(*lease);}
            catch(const FormatError &){foreign_refused=true;}});foreign.join();
        check(foreign_refused&&!foreign_current&&!runtime.freeze_common_inventory(*lease)&&lease->current()&&
            sessions[candidate]->phase()==OnlineMatchSession::Phase::Running&&!sessions[candidate]->peer_migration()->ready[candidate],
            "Inventory crossed executor, enabled unnegotiated selection, or manufactured full Ready");
        std::optional<SourceNativePublication::Boundary> abort_host_boundary;
        if(client_abort_mode){
            check(runtime.prepare_common_prospective(preimported_cut,unused_clock)&&
                runtime.seal_common_prospective(preimported_cut),"Actual Abort alternative did not retain its private host scope");
            abort_host_boundary=runtime.common_prospective_boundary(preimported_cut);
            check(abort_host_boundary&&abort_host_boundary->current()&&runtime.prepare_common_client(preimported_cut)&&
                runtime.prepared_common_client()==std::optional{derive_peer_prospective_client_role(generation,first->seal,candidate)}&&
                !runtime.begin_frame(),"Actual pending client alternative did not stage the resident replica separately");
            rejects([&]{(void)runtime.match().prepare_replica(foreground[candidate]->bound->frontier().replica);});
            rejects([&]{runtime.match().replicate_devices(foreground[candidate]->current->accepted_state().devices);});
            // Service-only retirement below never receives/captures another
            // display frame. Retire the other endpoint's outer proof aliases
            // explicitly before its independent native Abort acknowledgement.
            auto &other=*foreground[clients[1]];other.native.reset();other.bound.reset();other.reservation.reset();first_native_b.reset();
        }
        struct Retirement {
            MultiplayerRuntimeAuthority::CommonInventoryLease lease;
            Foreground &foreground;
            PeerCommonCut cut;
            std::optional<SourceNativePublication::Boundary> host;
            bool allow{},unexpected{};unsigned refused{},retired{};
        } retirement{*lease,*foreground[candidate],*first,abort_host_boundary};
        auto guard=runtime.hold_outer_retirement_gate([](void *opaque,const MultiplayerRuntimeAuthority::OuterRetirement &request)noexcept{
            auto &r=*static_cast<Retirement *>(opaque);
            if(request.reason!=MultiplayerRuntimeAuthority::OuterRetirement::Reason::Abort||
                request.context!=r.cut.context||request.seal!=r.cut.seal||!r.lease.current()||(r.host&&!r.host->current())){r.unexpected=true;return false;}
            if(!r.allow){++r.refused;return false;}
            r.foreground.native.reset();r.foreground.bound.reset();r.foreground.reservation.reset();++r.retired;return true;
        },&retirement);
        foreground[candidate]->retirement_gated=true;
        std::weak_ptr<const MultiplayerRuntimeAuthority::NativeCutPin> pin=first_native_a;first_native_a.reset();
        check(sessions[host]->abort_common_cut(*first),"Actual pending inventory Abort was refused");
        const auto service_retirement=[&]{
            if(!client_abort_mode){drive(false);return;}
            healthy();for(const auto &owner:owners)if(owner)owner->service();
        };
        wait([&]{service_retirement();return retirement.refused>0;},"Pending Abort did not reach inventory outer retirement");
        check(!retirement.unexpected&&lease->current()&&repeated->current()&&!pin.expired()&&
            !(sessions[candidate]->common_cut()->aborted&(1u<<candidate)),
            "Deferred retirement revoked an inventory owner or acknowledged Abort before actual outer retirement");
        if(client_abort_mode){
            check(abort_host_boundary->current()&&!runtime.prepared_common_client()&&!runtime.begin_frame(),
                "Deferred Abort exposed eligible client metadata or lost the genuine private host scope");
            rejects([&]{(void)runtime.match().prepare_replica(foreground[candidate]->bound->frontier().replica);});
        }
        retirement.allow=true;
        wait([&]{service_retirement();return sessions[host]->common_cut()->aborted==recipient_mask;},"Actual inventory owners did not retire before every Abort ACK");
        check(!retirement.unexpected&&retirement.retired==1&&!lease->current()&&!repeated->current()&&pin.expired()&&
            !runtime.common_inventory(*lease)&&!runtime.freeze_common_inventory(*lease)&&!runtime.prepared_common_native()&&
            !runtime.common_cut_checkpoint(generation,first->seal)&&!runtime.pending_native_cut(),
            "Aborted frozen inventory retained eligibility, native owners or its private imported target");
        if(client_abort_mode){
            check(!abort_host_boundary->current()&&!runtime.prepared_common_client()&&!runtime.prepared_common_prospective(),
                "Actual Abort retained either prepared role or its private native scope");
            auto frame=runtime.begin_frame();check(frame&&frame->finish(),"Actual Abort did not release the live replica mutation barrier");
            std::cout<<"PASS genuine prospective client Abort: actual private host scope and resident client replica survive outer refusal; both retire before ACK without another foreground capture; ordinary frame ownership resumes\n";
        }
        guard.reset();
        std::cout<<"PASS genuine retained inventory Abort: actual pending/imported pair deduplicated, no fabricated irreversible witness; creator/frame/foreign/replacement refusals; outer gate retains real pin until release, weak lease invalidated before Abort ACK\n";
        std::cout<<"LIMIT retained native/foreground inventory ownership only; no production driver, full Ready, rendered adoption, device input/audio, WAN or hardware acceptance\n";
        return 0;
    }
    if(inventory_mode=="preparation"){
        stage="first genuine Release before unpublished preparation scheduling";
        foreground[clients[1]]->associate_enabled=true;
        wait([&]{drive();require_live();return all_released(*first);},
            "Actual paired owners did not finish Release before preparation scheduling");
        auto &runtime=*owners[candidate];
        using Kind=OnlineMatchSession::MigrationPreparationKind;
        using Outcome=OnlineMatchSession::MigrationPreparationOutcome;
        // The old native host stays alive. Only its real gameplay emission is
        // paused; Common/DTLS service cannot manufacture a heard-host event.
        const auto suspect=[&]{const auto started=std::chrono::steady_clock::now();
            wait([&]{healthy();if(std::chrono::steady_clock::now()-started<510ms)return false;
                for(auto p:clients)if(!sessions[p]->peer_migration()->suspected)return false;return true;},
                "Actual native host silence did not create genuine suspicion");};
        const auto hear_host=[&]{wait([&]{healthy();
            for(auto p:clients)owners[p]->client()->send(neutral);
            auto frame=authority.begin_frame();check(bool(frame),"Current host frame unavailable during real liveness traffic");
            controls={};authority.host()->receive(controls);authority.host()->broadcast();
            if(!frame->finish())++recording_drops;
            for(auto p:clients)owners[p]->service();
            for(auto p:clients)if(sessions[p]->peer_migration()->suspected)return false;return true;
        },"Actual current-host native packet did not service suspicion");};
        suspect();
        const auto preparing=sessions[candidate]->migration_preparation_request();const auto request=preparing.request;
        check(preparing.outcome==Outcome::Preparing&&request&&request->kind()==Kind::Prepare&&request->context()==generation&&request->self()==candidate,
            "Genuine native-session suspicion did not mint its opaque preparation request");
        check(runtime.prepare_common_prospective(preimported_cut,unused_clock)&&unused_clock->calls==0&&
            runtime.seal_common_prospective(preimported_cut),
            "Unpublished preparation did not retain the same genuine prospective host target");
        const auto boundary=runtime.common_prospective_boundary(preimported_cut);
        check(boundary&&boundary->current()&&runtime.prepare_common_client(preimported_cut)&&
            runtime.prepared_common_client()==std::optional{derive_peer_prospective_client_role(generation,first->seal,candidate)},
            "Unpublished preparation did not retain both actual native role alternatives");
        rejects([&]{(void)runtime.match().prepare_replica(foreground[candidate]->bound->frontier().replica);});
        const auto lease=runtime.retain_common_inventory(0xc0a0+candidate);
        check(lease&&lease->current(),"Unpublished native preparation did not retain its genuine immutable inventory");
        const auto inventory=runtime.common_inventory(*lease);
        check(inventory&&inventory->lease!=request->id()&&inventory->cuts[0]&&
            inventory->cuts[0]->cut==first->seal&&inventory->cuts[0]->irreversible_common,
            "Actual released inventory did not preserve its own native lease and genuine Common provenance");
        stage="healthy current host races stale native preparation completion";
        hear_host();
        check(sessions[candidate]->complete_migration_preparation(*request,inventory)==Outcome::Retiring&&
            !(sessions[candidate]->survivor_selection()->inventory_peers&(1u<<candidate))&&
            !sessions[candidate]->peer_migration()->ready[candidate]&&!sessions[candidate]->peer_migration()->fenced&&
            sessions[candidate]->phase()==OnlineMatchSession::Phase::Running,
            "Stale native preparation published an inventory, Ready or fence after real host recovery");
        check(!runtime.prepare_common_native(preimported_cut)&&
            !runtime.retain_common_inventory(inventory->lease)&&!runtime.freeze_common_inventory(*lease)&&
            lease->current()&&boundary->current(),
            "Actual Session retirement reservation admitted native work before Runtime adopted its opaque request");
        struct Retirement {
            MultiplayerRuntimeAuthority::CommonInventoryLease lease;
            SourceNativePublication::Boundary boundary;
            Foreground &foreground;
            AuthorityTransferContext context;AuthorityCutSeal cut;
            bool allow{},unexpected{};unsigned refused{},retired{};
        } retirement{*lease,*boundary,*foreground[candidate],generation,first->seal};
        auto guard=runtime.hold_outer_retirement_gate([](void *opaque,const MultiplayerRuntimeAuthority::OuterRetirement &value)noexcept{
            auto &r=*static_cast<Retirement *>(opaque);
            if(value.reason!=MultiplayerRuntimeAuthority::OuterRetirement::Reason::CommonPreparationCancelled||
                value.context!=r.context||value.seal!=r.cut||!r.lease.current()||!r.boundary.current()){
                r.unexpected=true;return false;}
            if(!r.allow){++r.refused;return false;}
            r.foreground.native.reset();r.foreground.bound.reset();r.foreground.reservation.reset();++r.retired;return true;
        },&retirement);
        check(runtime.advance_common_preparation()==Outcome::Retiring&&retirement.refused==1&&!retirement.unexpected&&
            lease->current()&&boundary->current()&&!runtime.begin_frame()&&
            !runtime.retain_common_inventory(0xc0b0+candidate)&&!runtime.freeze_common_inventory(*lease)&&
            !runtime.prepare_common_native(preimported_cut)&&!runtime.prepare_common_client(preimported_cut),
            "Deferred outer retirement dropped a native owner or admitted a frame/new preparation/publication");
        rejects([&]{(void)runtime.match().prepare_replica(foreground[candidate]->bound->frontier().replica);});
        check(runtime.advance_common_preparation()==Outcome::Retiring&&retirement.refused==2&&
            lease->current()&&boundary->current(),"Repeated outer refusal replaced or released actual preparation owners");
        retirement.allow=true;
        check(runtime.advance_common_preparation()==Outcome::Retryable&&!retirement.unexpected&&retirement.retired==1&&
            !lease->current()&&!boundary->current()&&!runtime.prepared_common_client()&&!runtime.prepared_common_native()&&
            !runtime.prepared_common_prospective()&&!runtime.common_inventory(*lease),
            "Permitted actual retirement retained either native role, held scope or immutable inventory");
        guard.reset();
        {
            const auto pair=runtime.common_cut_checkpoint(generation,first->seal);
            check(pair&&pair->proof().matches(pair->origin()),"Cancellation discarded the ordinary genuine released recovery pair");
            auto probe=runtime.match().prepare_replica(pair->frontier().replica);
            check(runtime.match().replica_ready(probe),"Cancellation did not release the actual resident replica staging barrier");
        }
        {auto frame=runtime.begin_frame();check(frame&&frame->finish(),"Actual retirement did not restore ordinary live frame ownership");}
        check(sessions[candidate]->complete_migration_preparation(*request,inventory)==Outcome::Refused,
            "A retired opaque request could publish its stale native inventory");
        hear_host();suspect();
        const auto retrying=sessions[candidate]->migration_preparation_request();const auto retry=retrying.request;
        check(retrying.outcome==Outcome::Preparing&&retry&&retry->kind()==Kind::Prepare&&retry->id()>request->id(),
            "Fresh actual host silence did not create a new preparation identity after retirement");
        const auto retry_lease=runtime.retain_common_inventory(retry->id());
        check(retry_lease&&retry_lease->current()&&runtime.common_inventory(*retry_lease)&&
            !sessions[candidate]->peer_migration()->ready[candidate]&&!sessions[candidate]->peer_migration()->decision&&
            !(sessions[candidate]->survivor_selection()->inventory_peers&(1u<<candidate))&&
            sessions[candidate]->authority_context()==generation&&&runtime.world()==worlds[candidate].get(),
            "A genuine retry lost its ordinary pair, reused a retired lease or manufactured publication/Ready/adoption");
        check(runtime.advance_common_preparation()==Outcome::Published&&
            runtime.advance_common_preparation()==Outcome::Published&&retry_lease->current()&&
            sessions[candidate]->survivor_selection()->inventory_peers==(1u<<candidate)&&
            !sessions[candidate]->peer_migration()->ready[candidate]&&!sessions[candidate]->peer_migration()->decision&&
            sessions[candidate]->phase()==OnlineMatchSession::Phase::AwaitingWorld&&
            sessions[candidate]->authority_context()==generation&&&runtime.world()==worlds[candidate].get(),
            "The real preparation executor did not publish the fresh immutable inventory exactly once without manufacturing Ready/adoption");
        std::cout<<"PASS genuine native preparation scheduling: actual current-host silence and healthy packet; stale completion cannot publish; real private host scope, resident client staging and native inventory survive outer veto then retire; ordinary frame and fresh request/lease resume; actual executor publishes the new immutable inventory exactly once without Ready/adoption\n";
        std::cout<<"LIMIT explicit executor scheduling and paired native owners only; no production caller/default gate, complete foreground/renderer/input/playing-audio Ready, activation, WAN or hardware acceptance\n";
        return 0;
    }
    struct OuterOwners {
        Foreground &foreground;
        MultiplayerRuntimeAuthority &runtime;
        std::optional<PeerCommonCut> expected;
        std::weak_ptr<const MultiplayerRuntimeAuthority::NativeCutPin> pending_pin;
        unsigned refused{},retired{},reentry_refused{};
        bool allow{},unexpected{},native_alive_before_retirement{};
    } outer{*foreground[candidate],*owners[candidate]};
    const auto retire_outer=[](void *opaque,const MultiplayerRuntimeAuthority::OuterRetirement &retirement)noexcept{
        auto &owner=*static_cast<OuterOwners *>(opaque);
        if(retirement.reason!=MultiplayerRuntimeAuthority::OuterRetirement::Reason::Abort)return true;
        if(!owner.expected||owner.expected->context!=retirement.context||owner.expected->seal!=retirement.seal){
            owner.unexpected=true;return false;
        }
        // Deliberately attempt a forbidden getter: the callback must not
        // expose a partly retired native owner or enter the session again.
        try{(void)owner.runtime.world();owner.unexpected=true;}
        catch(const FormatError &){++owner.reentry_refused;}
        catch(...){owner.unexpected=true;}
        if(!owner.allow){++owner.refused;return false;}
        owner.native_alive_before_retirement=!owner.pending_pin.expired();
        owner.foreground.native.reset();owner.foreground.bound.reset();owner.foreground.reservation.reset();
        ++owner.retired;return true;
    };
    foreground[candidate]->retirement_gated=true;
    auto outer_guard=owners[candidate]->hold_outer_retirement_gate(retire_outer,&outer);
    rejects([&]{(void)owners[candidate]->hold_outer_retirement_gate(retire_outer,&outer);});
    require_live();
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
    std::array<std::optional<MultiplayerRuntimeAuthority::CommonInventoryLease>,16> historical_leases;
    if(inventory_mode.empty())for(auto p:clients){
        historical_leases[p]=owners[p]->retain_common_inventory(0xc090+p);
        check(historical_leases[p]&&historical_leases[p]->current(),"Actual first Released pair did not retain its genuine inventory");
        const auto inventory=owners[p]->common_inventory(*historical_leases[p]);
        check(inventory&&inventory->cuts[0]&&inventory->cuts[0]->cut==first->seal&&inventory->cuts[0]->irreversible_common&&
            !inventory->cuts[1]&&!inventory->cuts[2],"Rust inventory lost the genuine first Release or duplicated a private imported alias");}
    std::cout<<"PASS native common-cut delayed association survives history eviction: captured="<<foreground[clients[0]]->history.state().captured
        <<'/'<<foreground[clients[1]]->history.state().captured<<" retained=128 reservation=128; exact native pins and receipts retained\n";

    foreground[clients[1]]->associate_enabled=false;
    stage="second native cut before controlled foreground rejection";std::optional<PeerCommonCut> aborted;
    wait([&]{drive();const auto value=offered();if(value&&value->seal.id>first->seal.id)aborted=value;
        return aborted&&ready_native(*aborted);},
        "Second real offered cut did not retain both independent exact native pins");
    check(!sessions[host]->common_cut()->commit,"Delayed second owner could not prevent premature second Commit");
    // This replay successor may name an authentic early same-tick receipt
    // already outside the new reservation. Preserve the real missing-owner
    // rejection below; never invent an association just to import this cut.
    check(!owners[candidate]->prepare_common_native({candidate,generation,aborted->seal},unused_clock)&&
        prepared_names()&&unused_clock->calls==0,
        "An unrelated replay cut replaced the exact first native import");
    require_live();
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
    outer.expected=*aborted;outer.pending_pin=foreground[candidate]->native;
    const bool pending_bound=foreground[candidate]->bound.has_value();
    check(bool(foreground[candidate]->reservation),"Real pending native pin lacks its actual foreground reservation");
    diagnose("before explicit pre-Commit foreground Reject");
    check(sessions[clients[1]]->reject_common_cut(*aborted),"Missing foreground association could not request pre-Commit repair");
    drive(false,true,true); // One real recorded source step, with no activation.
    check(authority.match().simulation().ticks()>first->seal.tick,"Later genuine native tick was not executed");
    stage="real advanced receipts before repaired native publication";
    wait([&]{drive(false);for(auto p:clients){const auto &f=*foreground[p];
        if(!f.current||f.current->tick()!=authority.match().simulation().ticks()||
            f.history.state().newest_tick!=authority.match().simulation().ticks())return false;}return true;},
        "Advanced native frame did not deliver actual completed receipts to both clients");
    stage="deferred exact outer-owner Abort retirement";
    wait([&]{drive(false);check(!outer.unexpected,"Outer retirement callback received a foreign cut or allowed native reentry");
        const auto common=sessions[candidate]->common_cut();
        return common&&common->pending==aborted&&common->abort&&outer.refused;},
        "Real pending Abort did not reach the scoped outer-owner retirement gate");
    {const auto blocked_pin=owners[candidate]->pending_native_cut();
        check(blocked_pin&&blocked_pin==foreground[candidate]->native&&blocked_pin->cut().seal==aborted->seal&&
            foreground[candidate]->reservation&&foreground[candidate]->bound.has_value()==pending_bound&&
            !outer.pending_pin.expired()&&prepared_names()&&outer.retired==0&&
            !(sessions[candidate]->common_cut()->aborted&(1u<<candidate)),
            "Refused outer retirement lost a real native owner, selected import or prematurely acknowledged Abort");}
    check(!owners[candidate]->begin_frame()&&outer.refused>1&&outer.reentry_refused==outer.refused,
        "Deferred outer retirement admitted a native frame or failed its runtime reentry guard");
    outer.allow=true;
    stage="explicit pending-owner Abort retirement";
    wait([&]{drive(false);const auto common=sessions[host]->common_cut();
        return common&&common->pending==aborted&&common->abort&&common->aborted==recipient_mask;},
        "Every real receiver did not retire its aborted pending owners");
    for(auto p:clients){check(owners[p]->common_cut_checkpoint(generation,first->seal)&&!owners[p]->common_cut_checkpoint(generation,aborted->seal)&&
            !owners[p]->pending_native_cut(),"Abort lost old native/foreground owners or retained aborted pending eligibility");
        check(p==candidate?prepared_names():!owners[p]->prepared_common_native(),
            "An unrelated Abort discarded the selected first native import or created another preparation");}
    check(!outer.unexpected&&outer.retired==1&&outer.native_alive_before_retirement&&outer.pending_pin.expired()&&
        outer.reentry_refused==outer.refused+outer.retired&&!foreground[candidate]->native&&
        !foreground[candidate]->bound&&!foreground[candidate]->reservation,
        "Actual outer holders did not retire once before the matching native Abort acknowledgement");
    if(inventory_mode.empty())for(auto p:clients)check(historical_leases[p]->current()&&owners[p]->common_inventory(*historical_leases[p]),
        "Unrelated pending Abort invalidated the actual first Released inventory");
    outer_guard.reset();foreground[candidate]->retirement_gated=false;
    {auto replacement=owners[candidate]->hold_outer_retirement_gate(retire_outer,&outer);}
    require_live();
    std::cout<<"PASS native common-cut rejection/Abort repair preserves prior native/foreground owners: unbound_foregrounds="<<missing_foregrounds<<'\n';
    std::cout<<"PASS scoped exact outer-owner retirement gate: real pending native pin retained while refused, frame entry and callback reentry blocked; foreground aliases retire before Abort ACK; unrelated selected import preserved; registration released\n";

    stage="repaired later native common round";std::optional<PeerCommonCut> repaired;
    wait([&]{drive();const auto value=offered();if(value&&value->seal.id>aborted->seal.id)repaired=value;
        return repaired&&ready_pair(*repaired)&&(sessions[host]->common_cut()->associated&(1u<<clients[0]));},
        "Aborted native stream did not repair into a fresh independent common round");
    check(repaired->seal.tick>first->seal.tick&&!sessions[host]->common_cut()->commit,"Repair reused the aborted seal or implicitly associated the delayed owner");
    for(auto p:clients)check(owners[p]->common_cut_checkpoint(generation,first->seal).has_value(),"Repair discarded the prior released native pair before all new owners applied");
    foreground[clients[1]]->associate_enabled=true;
    wait([&]{drive();require_live();return all_released(*repaired);},"Second released native common round did not finish");
    stage="exact first native import retained through repaired Release";
    check(owners[candidate]->prepare_common_native(preimported_cut,unused_clock)&&unused_clock->calls==0,
        "Common-pair rotation discarded or reconstructed the exact first native import");
    for(unsigned field=0;field<4;++field){auto wrong=preimported_cut;
        if(field==0)wrong.peer=clients[1];
        if(field==1)++wrong.context.epoch;
        if(field==2)wrong.context.session.front()^=std::byte{1};
        if(field==3)++wrong.seal.bytes;
        check(!owners[candidate]->prepare_common_native(wrong,unused_clock)&&prepared_names()&&unused_clock->calls==0,
            "Foreign peer/context/epoch/cut consumed or replaced the exact private import");
    }
    {auto frame=owners[candidate]->begin_frame();check(bool(frame),"Healthy prepared client refused its live frame");
        rejects([&]{owners[candidate]->prepare_common_native(preimported_cut,unused_clock);});
        check(frame->finish()&&prepared_names(),"Outstanding-frame refusal lost the private native import");}
    bool foreign_import_refused{};std::thread foreign_import([&]{try{
        owners[candidate]->prepare_common_native(preimported_cut,unused_clock);
    }catch(const FormatError &){foreign_import_refused=true;}});foreign_import.join();
    check(foreign_import_refused&&prepared_names()&&unused_clock->calls==0,"A private native import crossed or changed executor");
    require_live();
    for(auto p:clients){auto &f=*foreground[p];const auto saved=owners[p]->common_cut_checkpoint(generation,repaired->seal);
        check(saved&&saved->cut().peer==p&&saved->proof().matches(saved->origin())&&saved->frontier().replica.matches(saved->origin()),
            "Second release lost its exact complete repaired native/foreground pair");
        const auto selected_first=owners[p]->common_cut_checkpoint(generation,first->seal);
        check(p==candidate?(selected_first&&selected_first->origin().same(first_origin_a)&&selected_first->proof().matches(first_origin_a)):
            (historical_leases[p]?(selected_first&&selected_first->origin().same(first_origin_b)&&selected_first->proof().matches(first_origin_b)):
                !selected_first),"Common rotation lost an explicitly imported/leased first pair or retained an unowned stale pair");
        f.history.context(std::nullopt);f.reservation.reset();f.bound.reset();f.native.reset();
        const auto independently_retained=owners[p]->common_cut_checkpoint(generation,repaired->seal);
        check(independently_retained&&independently_retained->origin().same(saved->origin())&&
            independently_retained->proof().matches(saved->origin()),"Ordinary history retirement destroyed the runtime's strong common pair");}
    check(prepared_names(),"Ordinary history retirement destroyed the exact private native import");
    require_live();
    check(first_native_a->host_proof().prove_snapshot(*owners[clients[0]]->client(),first_origin_a).matches(first_origin_a)&&
        first_native_b->host_proof().prove_snapshot(*owners[clients[1]]->client(),first_origin_b).matches(first_origin_b),
        "Historical independently owned native proofs were invalidated by worker advancement or common release");
    const auto first_cut_b=first_native_b->cut();
    if(inventory_mode.empty()){
        std::weak_ptr<const MultiplayerRuntimeAuthority::NativeCutPin> leased_pin=first_native_b;first_native_b.reset();
        check(!leased_pin.expired()&&!owners[clients[1]]->prepared_common_native()&&historical_leases[clients[1]]->current()&&
            owners[clients[1]]->common_cut_checkpoint(generation,first->seal),
            "Lease-only actual native pair did not survive committed-slot replacement and external pin retirement");
        for(auto p:clients){const auto inventory=owners[p]->common_inventory(*historical_leases[p]);
            check(inventory&&inventory->cuts[0]&&inventory->cuts[0]->cut==first->seal&&inventory->cuts[0]->irreversible_common&&
                !inventory->cuts[1]&&!inventory->cuts[2],"History/Release rotation mutated an already retained immutable inventory");}
        std::cout<<"PASS genuine retained inventory rotation: immutable first Release lease survives unrelated Abort and history/slot rotation; second receiver's actual native pin retained by lease alone after external pin retirement\n";
    }
    std::cout<<"PASS native common-cut two released rounds retain exact proofs without live activation: first="<<first->seal.id<<'@'<<first->seal.tick
        <<" aborted="<<aborted->seal.id<<" repaired="<<repaired->seal.id<<'@'<<repaired->seal.tick<<" physical_recipients=2 couch_players=12\n";
    std::cout<<"OBSERVED source successor recording drops="<<recording_drops<<"; subsequent genuine source step and repaired native validation required\n";
    if(inventory_mode=="freeze"){
        stage="genuine retained inventory Rust freeze and fixed-survivor selection";
        std::array<std::optional<MultiplayerRuntimeAuthority::CommonInventoryLease>,16> leases;
        for(auto p:clients){auto &runtime=*owners[p];leases[p]=runtime.retain_common_inventory(0xc080+p);
            check(leases[p]&&leases[p]->current(),"Genuine released inventory could not retain its exact owners");
            const auto inventory=runtime.common_inventory(*leases[p]);
            check(inventory&&inventory->lease==0xc080+p,"Opaque inventory did not preserve its actual lease identity");
            unsigned current{},historical{};
            for(const auto &entry:inventory->cuts)if(entry){
                if(entry->cut==repaired->seal){++current;check(entry->irreversible_common,"Rust lost the actual current CommonRelease witness");}
                else{++historical;check(p==candidate&&entry->cut==first->seal&&!entry->irreversible_common,
                    "Historical import minted an unobserved current CommonCommit witness or leaked a foreign cut");}}
            check(current==1&&historical==unsigned(p==candidate),"Inventory omitted or duplicated a genuinely retained native/foreground pair");}
        const auto first_peer=clients.front(),last_peer=clients.back();
        const auto silence_started=std::chrono::steady_clock::now();
        wait([&]{healthy();if(std::chrono::steady_clock::now()-silence_started<510ms)return false;
            for(auto p:clients)if(!sessions[p]->peer_migration()->suspected)return false;return true;},
            "Actual host gameplay silence did not authorize the fresh native inventory freeze");
        check(owners[first_peer]->freeze_common_inventory(*leases[first_peer]),"Actual retained inventory did not enter Rust selection");
        wait([&]{healthy();return sessions[last_peer]->survivor_selection()->inventory_peers&(1u<<first_peer);},
            "Authenticated native inventory did not reach the other physical survivor");
        check(sessions[first_peer]->phase()==OnlineMatchSession::Phase::AwaitingWorld&&!owners[first_peer]->begin_frame()&&
            sessions[last_peer]->phase()==OnlineMatchSession::Phase::Running&&
            !(sessions[last_peer]->survivor_selection()->inventory_peers&(1u<<last_peer))&&
            !sessions[last_peer]->survivor_selection()->selection_complete,
            "A remote inventory manufactured the missing local freeze, native frame or complete selection");
        check(owners[last_peer]->freeze_common_inventory(*leases[last_peer]),"Last real native inventory did not freeze");
        wait([&]{healthy();for(auto p:clients){const auto state=sessions[p]->survivor_selection();
            if(!state||!state->selection_complete)return false;}return true;},"Genuine native inventories failed the fixed all-survivor transcript");
        for(auto p:clients){const auto state=*sessions[p]->survivor_selection();
            check(state.selected==repaired->seal&&state.inventory_peers==recipient_mask&&state.accepted==recipient_mask&&!state.failed&&
                leases[p]->current()&&owners[p]->common_inventory(*leases[p])&&
                sessions[p]->phase()==OnlineMatchSession::Phase::AwaitingWorld&&!owners[p]->begin_frame()&&
                !sessions[p]->peer_migration()->decision&&std::ranges::none_of(sessions[p]->peer_migration()->ready,[](const auto &cut){return bool(cut);}),
                "Actual inventory selection lost fixed physical owners or fabricated election/full Ready");}
        std::cout<<"PASS genuine retained inventory Rust freeze: actual released and historical imported pairs after history eviction; current witness derived in Rust, all fixed physical inventories independently frozen, selected exact common cut, no full Ready or activation\n";
        std::cout<<"LIMIT retained native/foreground inventory and authenticated local transport; no production scheduling, full renderer/HUD/input/playing-audio readiness, host-loss adoption, WAN or hardware acceptance\n";
        return 0;
    }
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
        // Separately declared later local storage must not survive historical
        // publication. These controls never reach a native weapon/unit owner.
        auto &later_camera=f.poses.state();later_camera.view_zoom.fill(2);
        for(unsigned view=0;view<saved.origin().local_slots().size();++view){
            const auto &player=saved.current().players[saved.origin().local_slots()[view]];
            later_camera.view_zoom_unit[view]=player.source_unit^0x01000000u;
            later_camera.view_zoom_reset[view]=player.zoom_reset_serial^1u;
            later_camera.view_was_dead[view]=!player.dead;
        }
        const auto later_zoom=later_camera.view_zoom;
        const auto later_zoom_unit=later_camera.view_zoom_unit;
        const auto later_zoom_reset=later_camera.view_zoom_reset;
        const auto later_dead=later_camera.view_was_dead;
        const auto later=f.display_origin;
        check(!later.same(saved.origin())&&f.display_current.tick>saved.current().tick,
            "Receipt restore did not retain a genuinely later live state control");
        const auto unchanged=[&]{return f.display_origin.same(later)&&f.display_current.tick==later.tick()&&
            std::as_const(f.poses).state().client_fp_reported_units[0]==later.sequence()&&
            same_peer_envelopes(f.values.peer_sound_classes,later_peer_classes)&&
            std::as_const(f.poses).state().view_zoom==later_zoom&&
            std::as_const(f.poses).state().view_zoom_unit==later_zoom_unit&&
            std::as_const(f.poses).state().view_zoom_reset==later_zoom_reset&&
            std::as_const(f.poses).state().view_was_dead==later_dead&&
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
        for(unsigned view=0;view<saved.origin().local_slots().size();++view){
            const auto &camera=std::as_const(f.poses).state();
            const auto &player=saved.current().players[saved.origin().local_slots()[view]];
            check(camera.view_zoom[view]==std::int16_t((*saved.origin().sequence()+view)%3)-1&&
                camera.view_zoom_unit[view]==player.source_unit&&camera.view_zoom_reset[view]==player.zoom_reset_serial&&
                camera.view_was_dead[view]==player.dead,"Composed receipt publication lost its exact four-view zoom lifetimes");
        }
        auto expected=saved.frontier().peer_sound_classes;expected.advance(.03125f);
        f.values.peer_sound_classes.advance(.03125f);
        check(same_peer_envelopes(f.values.peer_sound_classes,expected),
            "Restored peer envelope did not continue the original fade from its exact historical phase");
    }
    require_live();
    std::cout<<"PASS native common-cut composed receipt adoption: exact effects/pose/frontiers/deferred cues; cancellation, foreign owner, stale mutation, wrong seal and executor controls\n";
    std::cout<<"PASS native common-cut peer audio envelopes: all54 target/current/remaining triples and game-over latch; continued fade suffix\n";
    if(selected_mcc)std::cout<<"PASS selected-MCC-native common-cut receipt route: three physical sessions and twelve couch players\n";
    const auto other_client=clients[1];
    std::shared_ptr<CountingClock> other_import_clock;
    std::uint64_t other_import_calls{};
    std::optional<SourceNativePublication::Boundary> other_host_boundary;
    std::unique_ptr<halo2::test::CommonClientPresentation> client_presentation;
    std::optional<halo2::test::CommonClientRoleRetirement> host_loser,client_loser;
    std::unique_ptr<MultiplayerRuntimeAuthority::OuterRetirementGuard> host_loser_guard,client_loser_guard;
    LanClient *retained_client{};
    std::unique_ptr<LanClient::PreparedRebind> old_client_endpoint;
    const CollisionWorld *later_client_collision{};
    MultiplayerSnapshotOrigin later_client_origin;
    if(client_mode){
        stage="genuine prospective client alternatives before election";
        // The second physical survivor owns a separately reconstructed host
        // alternative. Its live display world remains the original resident
        // replica; neither alternative is selected by this preparation.
        other_import_clock=std::make_shared<CountingClock>();
        check(owners[other_client]->prepare_common_prospective(first_cut_b,other_import_clock)&&
            other_import_clock->calls>0&&owners[other_client]->seal_common_prospective(first_cut_b),
            "Historical lease did not prepare and seal the other survivor's actual private native host");
        other_host_boundary=owners[other_client]->common_prospective_boundary(first_cut_b);
        check(other_host_boundary&&other_host_boundary->current()&&
            !other_host_boundary->same_scope(*prospective_boundary),
            "Two physical survivors did not retain different actual private host scopes");
        other_import_calls=other_import_clock->calls.load();
        auto &f=*foreground[other_client];
        client_presentation=std::make_unique<halo2::test::CommonClientPresentation>(*owners[other_client],first_frontiers[1],
            f.values,f.display_current,f.display_previous,f.display_origin,f.effects,f.sounds,f.poses);
        check(client_presentation->ready(),"Actual surviving display receipt did not prepare its headless client routing");
        retained_client=owners[other_client]->client();later_client_origin=retained_client->state_origin();
        later_client_collision=&owners[other_client]->match().simulation().collision();
        check(later_client_origin.tick()>first_origin_b.tick(),"Client rollback lacks a genuinely later live receipt control");
        for(unsigned n=0;n<clients.size();++n){const auto p=clients[n];auto &runtime=*owners[p];const auto &saved=first_frontiers[n];
            const auto role=derive_peer_prospective_client_role(generation,first->seal,p);
            check(runtime.prepare_common_client(saved.cut())&&runtime.prepare_common_client(saved.cut())&&
                runtime.prepared_common_client()==std::optional{role}&&!runtime.begin_frame()&&
                &runtime.world()==worlds[p].get()&&runtime.client()&&!runtime.host(),
                "Actual pre-election client preparation changed or failed to retain the same resident owner");
            rejects([&]{(void)runtime.match().prepare_replica(saved.frontier().replica);});
            rejects([&]{runtime.match().replicate_devices(saved.current().devices);});
            auto wrong=saved.cut();++wrong.seal.id;
            check(!runtime.prepare_common_client(wrong)&&!runtime.prepare_client_rebind()&&
                runtime.prepared_common_client()==std::optional{role},
                "Wrong cut or absent learned certificate replaced the staged actual client role");
            wrong=saved.cut();wrong.peer=clients[1-n];
            check(!runtime.prepare_common_client(wrong),"Foreign physical cut borrowed a prospective resident replica");
            bool foreign_refused{};std::thread foreign([&]{try{(void)runtime.prepare_common_client(saved.cut());}
                catch(const FormatError &){foreign_refused=true;}});foreign.join();
            const auto agreement=sessions[p]->peer_migration();
            check(foreign_refused&&runtime.prepared_common_client()==std::optional{role}&&agreement&&!agreement->decision&&
                std::ranges::none_of(agreement->ready,[](const auto &cut){return bool(cut);})&&
                sessions[p]->phase()==OnlineMatchSession::Phase::Running,
                "Actual client alternatives crossed executor or manufactured election/readiness metadata");
        }
        check(imported_clock->calls==prospective_clock_calls&&other_import_clock->calls==other_import_calls&&
            prospective_boundary->current()&&other_host_boundary->current()&&client_presentation->ready(),
            "Resident client preparation rebuilt native state or changed either sealed host scope");
        host_loser.emplace(halo2::test::CommonClientRoleRetirement{
            MultiplayerRuntimeAuthority::OuterRetirement::Reason::ProspectiveHostRetired,generation,first->seal,*other_host_boundary});
        client_loser.emplace(halo2::test::CommonClientRoleRetirement{
            MultiplayerRuntimeAuthority::OuterRetirement::Reason::ProspectiveClientRetired,generation,first->seal,*prospective_boundary});
        host_loser_guard=owners[other_client]->hold_outer_retirement_gate(halo2::test::CommonClientRoleRetirement::retire,&*host_loser);
        client_loser_guard=owners[candidate]->hold_outer_retirement_gate(halo2::test::CommonClientRoleRetirement::retire,&*client_loser);
        // Ordinary foreground receive/capture is deliberately finished here:
        // both resident replica mutation boundaries are now genuinely staged.
    }
    stage="exact private import consumed after actual host loss certificate";
    // End ordinary foreground servicing before closing its real host. The
    // private import has survived worker/history advancement without changing
    // any live world, Ready metadata or native endpoint above.
    diagnostic={};owners[host].reset();worlds[host].reset();options[host].online_match.reset();sessions[host].reset();
    check(sessions[candidate]->prepare_peer_migration(preimported_cut),"Explicit retained candidate metadata could not start the final certificate control");
    wait([&]{healthy();for(auto p:clients){const auto agreement=sessions[p]->peer_migration();
        if(!agreement||!agreement->decision)return false;}return true;},"Actual surviving sessions did not learn the exact retained candidate decision");
    const auto decision=*sessions[candidate]->peer_migration()->decision;
    check(decision.previous==generation&&decision.value.host==candidate&&decision.value.cut==preimported_cut.seal&&prepared_names(),
        "Host-loss certificate did not name the exact already imported native cut");
    for(unsigned field=0;field<7;++field){auto wrong=decision;
        if(field==0)wrong.previous.session.front()^=std::byte{1};
        if(field==1)++wrong.previous.epoch;
        if(field==2)++wrong.value.cut.tick;
        if(field==3)wrong.value.host=clients[1];
        if(field==4)++wrong.next.membership_revision;
        if(field==5)++wrong.ballot.round;
        if(field==6)wrong.voters^=std::uint16_t(1u<<candidate);
        check(!owners[candidate]->prepare_takeover(wrong,unused_clock)&&prepared_names()&&unused_clock->calls==0&&owners[candidate]->shadow_state(),
            "Wrong certificate consumed private native preparation or retired its independent worker");
    }
    if(client_mode){
        // Independent allocation-free-commit endpoint control. It carries no
        // historical state copy and is never published; the real prepared
        // role must leave this predecessor witness current until activation.
        old_client_endpoint=retained_client->prepare_rebind_host(generation,decision.next,first->seal);
        for(unsigned field=0;field<7;++field){auto wrong=decision;
            if(field==0)wrong.previous.session.front()^=std::byte{1};
            if(field==1)++wrong.previous.epoch;
            if(field==2)++wrong.value.cut.tick;
            if(field==3)wrong.value.host=other_client;
            if(field==4)++wrong.next.membership_revision;
            if(field==5)++wrong.ballot.round;
            if(field==6)wrong.voters^=std::uint16_t(1u<<other_client);
            check(!owners[other_client]->prepare_client_rebind(wrong)&&other_host_boundary->current()&&
                owners[other_client]->prepared_common_client()&&!owners[other_client]->pending_client_rebind()&&
                host_loser->refused==0&&!host_loser->retired&&owners[other_client]->client()==retained_client&&
                retained_client->can_commit_rebind_host(*old_client_endpoint)&&
                retained_client->state_origin().same(later_client_origin)&&
                &owners[other_client]->match().simulation().collision()==later_client_collision,
                "Wrong full client certificate retired a losing scope or consumed the actual prepared resident payload");
        }
        rejects([&]{(void)owners[other_client]->match().prepare_replica(first_frontiers[1].frontier().replica);});
    }
    std::shared_ptr<const SourceAuthorityRetainedWorld> retained_import_pair;
    check(owners[candidate]->recovery_eligibility()==MultiplayerRuntimeAuthority::RecoveryEligibility::LearnedCommonCut,
        "Actual learned certificate and retained exact common pair failed disconnect recovery eligibility");
    wait([&]{retained_import_pair=owners[candidate]->recover();return bool(retained_import_pair);},
        "Exact imported common pair did not retire its replay executor");
    auto wrong_admissions=*retained_import_pair;wrong_admissions.admissions.players.front().player^=1u;
    rejects([&]{SourcePeerHostTakeover::prepare(options[candidate],wrong_admissions,decision,unused_clock);});
    check(prepared_names()&&unused_clock->calls==0,"Wrong independent admissions consumed or reconstructed the private import");
    if(prospective_mode){
        // Certification must transfer the exact already derived native world
        // and its actual retained scope. No renderer/input/playing-audio Ready
        // is inferred by this deliberately bounded native-owner control.
        check(prospective_binding&&prospective_binding->current()&&prospective_boundary&&prospective_boundary->current()&&
            prospective_foreground->ready(*prospective_foreground_prepared)&&imported_clock->calls==prospective_clock_calls,
            "Private prospective scope did not survive history eviction, Release and unrelated Abort repair");
        const auto role=owners[candidate]->prepared_common_prospective();
        sessions[candidate]->loaded(fingerprint^1u,variant);bool fingerprint_refused{};
        try{fingerprint_refused=!owners[candidate]->prepare_takeover(decision,unused_clock);}
        catch(const FormatError &){fingerprint_refused=true;}
        sessions[candidate]->loaded(fingerprint,variant);
        check(fingerprint_refused&&prepared_names()&&owners[candidate]->prepared_common_prospective()==role&&
            prospective_binding->current()&&!owners[candidate]->pending_takeover()&&unused_clock->calls==0,
            "Fingerprint refusal consumed the sealed prospective target or its genuine scope");
        if(client_mode){
            check(!owners[candidate]->prepare_takeover(decision,unused_clock)&&owners[candidate]->pending_takeover()&&
                client_loser->refused==1&&!client_loser->unexpected&&!client_loser->retired&&
                prospective_boundary->current()&&owners[candidate]->prepared_common_client()&&
                prospective_foreground->ready(*prospective_foreground_prepared)&&!owners[candidate]->begin_frame(),
                "Losing-client outer refusal lost the winning actual host scope or admitted the staged resident replica");
            auto *held=owners[candidate]->pending_takeover();
            check(!owners[candidate]->prepare_takeover(decision,unused_clock)&&owners[candidate]->pending_takeover()==held&&
                client_loser->refused==2&&prospective_boundary->current()&&unused_clock->calls==0,
                "Repeated losing-client refusal replaced the exact already prepared winning host");
            rejects([&]{(void)owners[candidate]->match().prepare_replica(first_frontiers[0].frontier().replica);});
            client_loser->allow=true;
        }
        check(owners[candidate]->prepare_takeover(decision,unused_clock)&&unused_clock->calls==0&&
            !owners[candidate]->prepared_common_native()&&!owners[candidate]->prepared_common_prospective()&&
            imported_clock->calls==prospective_clock_calls&&prospective_binding->current(),
            "Full learned certificate reconstructed native state or replaced the retained prospective scope");
        if(client_mode){
            check(client_loser->retired==1&&!client_loser->unexpected&&!owners[candidate]->prepared_common_client()&&
                prospective_foreground->ready(*prospective_foreground_prepared)&&prospective_boundary->current(),
                "Permitted losing-client retirement consumed the winning host's foreground/scope");
            client_loser_guard.reset();
            auto &remote=*owners[other_client];
            check(!remote.prepare_client_rebind(decision)&&host_loser->refused==1&&!host_loser->unexpected&&
                other_host_boundary->current()&&remote.prepared_common_client()&&!remote.pending_client_rebind()&&
                remote.client()==retained_client&&&remote.world()==worlds[other_client].get()&&
                retained_client->can_commit_rebind_host(*old_client_endpoint)&&
                retained_client->state_origin().same(later_client_origin)&&
                &remote.match().simulation().collision()==later_client_collision&&!remote.begin_frame()&&client_presentation->ready(),
                "Losing-host outer refusal changed either actual alternative or its resident live receipt");
            rejects([&]{(void)remote.match().prepare_replica(first_frontiers[1].frontier().replica);});
            host_loser->allow=true;
            check(remote.prepare_client_rebind(decision)&&remote.pending_client_rebind()==std::optional{decision}&&
                !remote.prepared_common_client()&&!remote.prepared_common_native()&&!remote.prepared_common_prospective()&&
                !other_host_boundary->current()&&host_loser->retired==1&&!host_loser->unexpected&&
                remote.client()==retained_client&&&remote.world()==worlds[other_client].get()&&
                retained_client->can_commit_rebind_host(*old_client_endpoint)&&
                retained_client->state_origin().same(later_client_origin)&&
                &remote.match().simulation().collision()==later_client_collision&&other_import_clock->calls==other_import_calls&&
                client_presentation->ready(),
                "Actual client late binding rebuilt/restaged state or retained its losing private host scope");
            host_loser_guard.reset();
            check(remote.prepare_client_rebind(decision)&&remote.pending_client_rebind()==std::optional{decision},
                "Repeated exact client certificate replaced the transferred resident prepared owner");
            auto wrong=decision;++wrong.ballot.round;
            check(!remote.prepare_client_rebind(wrong)&&remote.pending_client_rebind()==std::optional{decision},
                "Wrong full certificate replaced an already bound actual client preparation");
            rejects([&]{(void)remote.match().prepare_replica(first_frontiers[1].frontier().replica);});
        }
        auto *takeover=owners[candidate]->pending_takeover();check(takeover!=nullptr,"Prospective certificate lost its pending owner");
        rejects([&]{(void)takeover->world();});rejects([&]{(void)takeover->decision();});
        rejects([&]{(void)takeover->camera();});rejects([&]{(void)takeover->hold_native_publication();});
        auto wrong=decision;++wrong.next.epoch;
        check(!owners[candidate]->prepare_takeover(wrong,unused_clock)&&
            owners[candidate]->prepare_takeover(decision,unused_clock)&&owners[candidate]->pending_takeover()==takeover&&
            prospective_binding->current()&&unused_clock->calls==0&&&owners[candidate]->world()==worlds[candidate].get()&&
            owners[candidate]->client()&&!owners[candidate]->host()&&sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,
            "Wrong/repeated certificate consumed the held target or changed the live native role");
        struct Publication {
            SourceForegroundWorldHandoff &owner;SourceForegroundWorldHandoff::Prepared &prepared;
            unsigned calls{},published{};bool allow{};
        } publication{*prospective_foreground,*prospective_foreground_prepared};
        const auto publish=[](void *opaque)noexcept{auto &p=*static_cast<Publication *>(opaque);++p.calls;
            if(!p.allow||!p.owner.ready(p.prepared))return false;
            if(!p.owner.commit(p.prepared))std::terminate();++p.published;return true;};
        check(!owners[candidate]->activate_takeover(publish,&publication)&&publication.calls==0&&prospective_binding->current(),
            "Prospective native adoption bypassed the missing survivor's exact-cut Ready");
        // Deliberately declared native-only readiness control, matching the
        // original certificate fixture. This is not the missing full runtime
        // foreground/input/playing-audio preparation or a playable migration.
        check(sessions[clients[1]]->prepare_peer_migration(first_cut_b),
            "Independent surviving receiver could not declare its genuine retained exact cut");
        wait([&]{healthy();const auto state=sessions[candidate]->peer_migration();
            return state&&state->ready[candidate]==std::optional{preimported_cut.seal}&&
                state->ready[clients[1]]==std::optional{preimported_cut.seal};},"Native-only activation control did not receive both survivor declarations");
        check(!owners[candidate]->activate_takeover(publish,&publication)&&publication.calls==1&&
            !publication.published&&prospective_foreground->ready(*prospective_foreground_prepared)&&
            owners[candidate]->pending_takeover()==takeover&&prospective_binding->current()&&owners[candidate]->client()&&
            !owners[candidate]->host()&&sessions[candidate]->authority_context()==generation,
            "Rejected prospective native callback changed ownership or consumed the retained scope");
        publication.allow=true;
        check(owners[candidate]->activate_takeover(publish,&publication)&&publication.calls==2&&
            publication.published==1&&!prospective_foreground->ready(*prospective_foreground_prepared)&&
            !owners[candidate]->pending_takeover()&&prospective_binding->current()&&!owners[candidate]->begin_frame()&&
            imported_clock->calls==prospective_clock_calls&&sessions[candidate]->authority_context()==decision.next,
            "Prospective native activation did not retain exclusion through explicit post-commit release");
        rejects([&]{(void)owners[candidate]->world();});rejects([&]{(void)owners[candidate]->host();});
        owners[candidate]->service();check(prospective_binding->current(),"Ordinary service released the active prospective native scope");
        owners[candidate]->release_prospective_publication();
        check(!prospective_binding->current()&&!prospective_boundary->current()&&&owners[candidate]->world()!=worlds[candidate].get()&&
            owners[candidate]->host()&&!owners[candidate]->client()&&
            owners[candidate]->match().simulation().ticks()==preimported_cut.seal.tick&&
            owners[candidate]->world().local_slots==sessions[candidate]->local_slots(),
            "Explicit prospective release lost the adopted exact native world or retained its temporary binding");
        check(!owners[candidate]->common_inventory(*historical_leases[candidate])&&!historical_leases[candidate]->current()&&
            historical_leases[clients[1]]->current(),"Actual context retirement retained an obsolete lease or revoked another runtime's owner");
        rejects([&]{owners[candidate]->release_prospective_publication();});
        if(client_mode){
            stage="genuine prospective resident client publication";
            auto &remote=*owners[other_client];
            check(remote.pending_client_rebind()==std::optional{decision}&&client_presentation->ready()&&
                !remote.activate_client_rebind(halo2::test::CommonClientPresentation::publish,client_presentation.get())&&
                client_presentation->calls==1&&!client_presentation->published&&client_presentation->ready()&&
                remote.pending_client_rebind()==std::optional{decision}&&remote.client()==retained_client&&
                &remote.world()==worlds[other_client].get()&&retained_client->can_commit_rebind_host(*old_client_endpoint)&&
                retained_client->state_origin().same(later_client_origin)&&
                &remote.match().simulation().collision()==later_client_collision&&
                sessions[other_client]->authority_context()==generation&&
                sessions[other_client]->phase()==OnlineMatchSession::Phase::Rebinding&&
                other_import_clock->calls==other_import_calls,
                "Rejected actual client callback changed endpoint, live replica, receipt or transferred preparation");
            rejects([&]{(void)remote.match().prepare_replica(first_frontiers[1].frontier().replica);});
            client_presentation->allow=true;
            check(remote.activate_client_rebind(halo2::test::CommonClientPresentation::publish,client_presentation.get())&&
                client_presentation->calls==2&&client_presentation->published==1&&!client_presentation->ready()&&
                !remote.pending_client_rebind()&&!remote.activate_client_rebind(halo2::test::CommonClientPresentation::publish,client_presentation.get())&&
                client_presentation->calls==2&&remote.client()==retained_client&&&remote.world()==worlds[other_client].get()&&
                sessions[other_client]->authority_context()==decision.next&&
                retained_client->state().tick==first->seal.tick&&!retained_client->state_origin().context()&&
                retained_client->received_states().empty()&&retained_client->received_origins().empty()&&
                retained_client->snapshot_buffered_bytes()==0&&!retained_client->can_commit_rebind_host(*old_client_endpoint)&&
                remote.match().replica_checkpoint(first_origin_b).matches(first_origin_b)&&
                foreground[other_client]->display_origin.same(first_origin_b)&&
                foreground[other_client]->display_current.tick==first->seal.tick&&
                foreground[other_client]->values.previous.same(first_frontiers[1].frontier().previous)&&
                other_import_clock->calls==other_import_calls&&imported_clock->calls==prospective_clock_calls,
                "Actual client publication failed exact historical receipt/replica rollback and same endpoint-owner transfer");
            check(!remote.common_inventory(*historical_leases[other_client])&&!historical_leases[other_client]->current(),
                "Successful exact client context publication retained an obsolete inventory lease");
            auto frame=remote.begin_frame();check(frame&&frame->finish(),
                "Successful actual client publication did not release its resident replica mutation barrier");
            std::cout<<"PASS genuine prospective client alternatives: both actual private host scopes and resident client replicas prepared before election without Ready; wrong cut/executor/full certificate refusals preserve owners; role-specific outer vetoes preserve both alternatives, then retire only the loser; same resident/native client payload publishes once with callback veto, exact historical replica/receipt rollback, old endpoint witness invalidation and unchanged native clock calls\n";
        }
        std::cout<<"PASS Rust prospective native role: actual departure/camera/lens preparation before election; same native scope retained across Release/history eviction/unrelated Abort; exact certificate transfers same sealed owner without reconstruction; fingerprint/certificate/executor refusals preserve eligibility; missing-survivor/callback veto; native-only adoption keeps public access fenced until explicit release\n";
        std::cout<<"PASS prospective held foreground boundary: typed private preparation before election; exact scope retained through repair/certificate binding; default and routing refusal; coupled native publication consumes the bound headless routing owner once\n";
        std::cout<<"LIMIT native-only activation with declared survivor readiness; no complete foreground/renderer/lens/input/playing-audio Ready, production scheduling/freeze driver, playable host migration, played match, WAN or microphone acceptance\n";
        return 0;
    }
    // loaded() changes the current native fingerprint without changing the
    // independently learned decision, predecessor or accepted admissions.
    // LanHost's genuine continuation validation rejects it after consuming
    // the imported world. The wrapper must clear rather than remain poisoned
    // with an empty target; the exact authentic CommonPair still owns retry.
    sessions[candidate]->loaded(fingerprint^1u,variant);bool fingerprint_refused{};
    try{(void)owners[candidate]->prepare_takeover(decision,unused_clock);}catch(const FormatError &){fingerprint_refused=true;}
    sessions[candidate]->loaded(fingerprint,variant);
    const auto retry_pair=owners[candidate]->common_cut_checkpoint(generation,preimported_cut.seal);
    check(fingerprint_refused&&!owners[candidate]->prepared_common_native()&&!owners[candidate]->pending_takeover()&&
        retry_pair&&retry_pair->proof().matches(retry_pair->origin())&&unused_clock->calls==0&&
        owners[candidate]->client()&&!owners[candidate]->host()&&&owners[candidate]->world()==worlds[candidate].get(),
        "Post-consumption constructor failure poisoned private preparation, lost authentic retry ownership or changed the live role");
    auto retry_clock=std::make_shared<CountingClock>();
    check(owners[candidate]->prepare_common_native(preimported_cut,retry_clock)&&retry_clock->calls>0&&prepared_names(),
        "Authentic retained pair could not rebuild a consumed failed native target");
    check(owners[candidate]->prepare_takeover(decision,unused_clock)&&unused_clock->calls==0&&!owners[candidate]->prepared_common_native(),
        "Certificate preparation reconstructed instead of consuming the same private native import");
    auto *takeover=owners[candidate]->pending_takeover();
    check(takeover&&takeover->decision()==decision&&takeover->world().match.simulation().ticks()==preimported_cut.seal.tick&&
        takeover->world().local_slots==sessions[candidate]->local_slots()&&&owners[candidate]->world()==worlds[candidate].get()&&
        owners[candidate]->client()&&!owners[candidate]->host()&&sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,
        "Consuming the private import changed the still-live world or failed its exact native role finalization");
    {auto publication=takeover->hold_native_publication();check(publication.current(),"Imported target escaped without its owning native publication capability");}
    auto wrong_decision=decision;++wrong_decision.next.epoch;
    check(!owners[candidate]->prepare_takeover(wrong_decision,unused_clock)&&owners[candidate]->pending_takeover()==takeover&&
        owners[candidate]->prepare_takeover(decision,unused_clock)&&unused_clock->calls==0,
        "A wrong or repeated staged certificate replaced the same prepared native owner");
    std::cout<<"PASS decision-free exact native import: genuine first pending pair, cold clock consumers verified; same selected first owner survives Release/history eviction, unrelated replay Abort and repaired-pair rotation; full certificate/admission refusals preserve preparation; consumed fingerprint failure clears invalid target and authentic pair retries; same opaque retry owner finalizes without reconstruction\n";
    std::cout<<"LIMIT headless CPU/native owner-pin and certificate-finalization integration; selected pending NativeImport Abort retirement is source-audited and session-veto tested separately, not directly run here; no production pre-election scheduling/freeze driver, full renderer/HUD/input/playing-audio adopter, host-loss activation, WAN or physical controller/headset acceptance\n";
    return 0;
}catch(const std::exception &error){std::cerr<<stage<<": "<<error.what()<<'\n';return 1;}
