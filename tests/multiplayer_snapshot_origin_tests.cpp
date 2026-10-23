#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/effect_presentation_continuity.hpp"
#include "halo2/multiplayer_pose_continuity.hpp"
#include "halo2/fp_replication.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>
#include "multiplayer_scene_continuity_fixture.hpp"
#include "multiplayer_hud_group_fixture.hpp"
#define HALO2_LOCOMOTION_GROUP_STORAGE_AUDIT
#include "multiplayer_locomotion_group_fixture.hpp"
#include "helpers/multiplayer_scope_continuity.hpp"

using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
template<class F>void rejects(F run){bool rejected=false;try{run();}catch(const FormatError&){rejected=true;}
    check(rejected,"Unmatched accepted snapshot was admitted");}
struct FixedClock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
// Sequential logical-endpoint adapter, not encryption or WAN evidence.
struct Bus {struct Packet {unsigned from{};std::vector<std::byte> bytes;};std::array<std::deque<Packet>,16> queues;};
struct Endpoint final:MultiplayerDatagrams {
    std::shared_ptr<Bus> bus;unsigned self;
    Endpoint(std::shared_ptr<Bus> b,unsigned s):bus(std::move(b)),self(s){}
    void send(std::span<const std::byte> bytes,unsigned to)override{check(to<16,"Bad fixture endpoint");bus->queues[to].push_back({self,{bytes.begin(),bytes.end()}});}
    bool receive(std::vector<std::byte> &bytes,unsigned &from)override{auto &q=bus->queues[self];if(q.empty())return false;
        from=q.front().from;bytes=std::move(q.front().bytes);q.pop_front();return true;}
};
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> colors;
    std::copy_n(defaults.begin()+0x40,colors.size(),colors.begin());return native_selected_player_configuration(name,colors,-1);}
AuthorityTransferDigest hash(std::span<const std::byte> bytes){AuthorityTransferDigest out{};
    check(SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char*>(out.data())),"Cannot hash native cut");return out;}
std::uint32_t sequence(const Bus::Packet &p){check(p.bytes.size()>=27,"Snapshot header is truncated");return Bytes(p.bytes).u32(23);}
bool same_nodes(std::span<const Transform> a,std::span<const Transform> b){return a.size()==b.size()&&
    (a.empty()||!std::memcmp(a.data(),b.data(),a.size_bytes()));}
bool same_hand(const FirstPersonHandBinding &a,const FirstPersonHandBinding &b){return
    a.flags==b.flags&&a.weapon==b.weapon&&a.weapon_node_count==b.weapon_node_count&&a.arms_node_count==b.arms_node_count&&
    a.weapon_nodes==b.weapon_nodes&&a.arms_nodes==b.arms_nodes&&a.graph==b.graph&&a.animation_identifier==b.animation_identifier&&
    a.channel_flags==b.channel_flags&&a.channel_options==b.channel_options&&std::bit_cast<std::uint32_t>(a.rate)==std::bit_cast<std::uint32_t>(b.rate)&&
    std::bit_cast<std::uint32_t>(a.frame)==std::bit_cast<std::uint32_t>(b.frame)&&a.activity==b.activity&&a.channel_delay==b.channel_delay&&
    a.events==b.events&&a.future_events==b.future_events&&a.dual_wield==b.dual_wield&&a.secondary_hand==b.secondary_hand&&a.fire_ticks==b.fire_ticks;}
bool same_view(const FirstPersonViewBinding &a,const FirstPersonViewBinding &b){return a.unit==b.unit&&a.representation==b.representation&&
    same_hand(a.hands[0],b.hands[0])&&same_hand(a.hands[1],b.hands[1]);}
void pose_roundtrip(const Cache &cache,std::span<const SceneObjects::Object> early_scene,
    std::span<const SceneObjects::Object> later_scene,const MultiplayerSnapshotOrigin &early,const MultiplayerSnapshotProof &early_proof,
    const MultiplayerSnapshotProof &alternate_early_proof,const MultiplayerSnapshotOrigin &later,const MultiplayerSnapshotProof &later_proof){
    check(later.tick()>early.tick()&&early.same_connection(later),"Pose restore requires two real received native ticks");
    const auto &a=early.accepted_state(),&b=later.accepted_state();
    std::optional<RenderModel> model;const SourceWorldPose *early_pose{},*later_pose{};std::uint32_t render_model{};
    for(const auto &root:a.source_scene_roots){
        const auto one=std::ranges::find_if(a.source_body_poses,[&](const auto &p){return p.object==root.object&&!p.nodes.empty();});
        const auto two=std::ranges::find_if(b.source_body_poses,[&](const auto &p){return p.object==root.object&&!p.nodes.empty();});
        if(one==a.source_body_poses.end()||two==b.source_body_poses.end())continue;
        try{auto candidate=cache.load_model_hierarchy(root.definition);
            if(candidate.bones.size()!=one->nodes.size()||candidate.bones.size()!=two->nodes.size())continue;
            render_model=cache.load_initial_model_definition(root.definition).render_model;
            model=std::move(candidate);early_pose=&*one;later_pose=&*two;break;
        }catch(const FormatError &){} // Genuine non-model/unsupported authored owner; require a real supported bank below.
    }
    check(model&&early_pose&&later_pose&&render_model!=null_datum,"Real native receipts lack a supported authored completed WORLD hierarchy");
    const auto key=std::pair{early_pose->object,render_model};
    const MultiplayerPoseContinuity::FpPoseKey fp_key{early_pose->object,render_model,0};
    check(cache.original_data()&&cache.original_data()->unit_constructor_tail_constants,"Pose transition control lacks original operands");
    const auto &transition_constants=*cache.original_data()->unit_constructor_tail_constants;
    check(cache.original_data()->unit_scope_camera_constants.has_value(),"Scope continuation lacks authenticated original operands");
    const auto &scope_constants=*cache.original_data()->unit_scope_camera_constants;
    std::vector<BonePose> bind_locals;for(const auto &bone:model->bones)bind_locals.push_back(bone.bind);
    auto transition_locals=bind_locals;transition_locals.front().translation.x+=1;
    MultiplayerPoseContinuity poses,foreign;
    const auto populate=[&](MultiplayerPoseContinuity::Values &v,const MultiplayerSnapshotOrigin &origin,
        std::span<const SceneObjects::Object> scene,bool first){
        const auto &state=origin.accepted_state();
        if(first){v.scene_interpolation.reset(*origin.tick(),scene);v.source_body_interpolation.reset(*origin.tick(),state.source_body_poses);}
        else {v.scene_interpolation.capture(*origin.tick(),scene);v.source_body_interpolation.capture(*origin.tick(),state.source_body_poses);}
        const auto &palette=first?early_pose->nodes:later_pose->nodes;
        v.source_render_interpolation[key].capture(*origin.tick(),palette);
        for(unsigned view=0;view<origin.local_slots().size();++view){const auto &p=state.players[origin.local_slots()[view]];
            const auto representation=source_session_player_representation(p.source_configuration,state.source_game_type);
            auto definition=cache.load_first_person_binding(p.weapon,representation);
            observe_first_person_binding(v.client_fp_views[view],{p.source_player,p.source_unit,int(representation),{p.source_weapon,p.secondary_weapon.datum}},
                [&](std::uint32_t weapon,std::int32_t)->const FirstPersonBindingDefinition *{return weapon==p.source_weapon?&definition:nullptr;});
            v.client_fp_clock_available[view]=apply_replicated_first_person_clock(v.client_fp_origins[view],v.client_fp_views[view],p.first_person_clock,definition);
            v.client_fp_reported_units[view]=p.source_unit;
            // Explicit container roundtrip controls use a real completed
            // WORLD palette in all four banks. This does not certify the
            // native first-person producer or invent an authored FP pose.
            v.source_fp_interpolation[view][fp_key].capture(*origin.tick(),palette);
            v.source_fp_lifetimes[view]={p.source_unit,p.source_weapon,p.secondary_weapon.datum,p.source_parent,std::uint32_t(p.dead)};
            const auto slot=origin.local_slots()[view];v.aim_alive[slot]=p.connected&&!p.dead;
            if(first){
                // Declared CPU-controller storage controls execute existing
                // source arithmetic with imported operands and authored local
                // bones; they are not a claim about this unit's live gait.
                source_aim_blend_select(v.aim_blends[slot],true,30);source_aim_blend_select(v.body_aim_blends[slot],true,30);
                v.body_transitions[slot].tick(0x0400000cu,false,bind_locals,transition_constants);
                v.body_transitions[slot].tick(0x0a000014u,false,transition_locals,transition_constants);
            }else v.body_transitions[slot].tick(0x0a000014u,false,transition_locals,transition_constants);
            source_aim_blend_advance(v.aim_blends[slot]);source_aim_blend_advance(v.body_aim_blends[slot]);
        }
        if(first)test::prime_scope_continuity(v,scope_constants,origin);
        else for(unsigned view=0;view<origin.local_slots().size();++view){
            v.view_zoom[view]=-1;
            // Declared local storage controls, never native unit admissions.
            // Every lifetime differs so omitting any field from restore fails.
            v.view_zoom_unit[view]^=0x01000000u;
            v.view_zoom_reset[view]^=1u;
            v.view_was_dead[view]=!v.view_was_dead[view];
            for(unsigned frame=0;frame<12;++frame)test::scope_continuity_step(v,scope_constants,view,false);
        }
    };
    auto &aliased_render=poses.state().source_render_interpolation;
    populate(poses.state(),early,early_scene,true);
    const auto initial=std::as_const(poses).state();
    const auto snapshot=poses.capture(early);
    check(snapshot.valid()&&snapshot.matches(early)&&!snapshot.matches(later)&&poses.owns_snapshot(snapshot)&&!foreign.owns_snapshot(snapshot),
        "Pose snapshot lost exact receipt or stable local ownership");
    rejects([&]{poses.capture(MultiplayerSnapshotOrigin{});});
    rejects([&]{poses.prepare_restore(MultiplayerPoseContinuity::Snapshot{},early_proof);});
    rejects([&]{poses.prepare_restore(snapshot,MultiplayerSnapshotProof{});});
    rejects([&]{poses.prepare_restore(snapshot,later_proof);});
    rejects([&]{foreign.prepare_restore(snapshot,early_proof);});
    populate(poses.state(),later,later_scene,false);
    check(test::scope_continuity_words(std::as_const(poses).state())!=test::scope_continuity_words(initial),
        "Scope rollback control did not advance past its retained polynomial");
    const auto later_scope=test::scope_continuity_words(std::as_const(poses).state());
    const auto scope_unchanged=[&]{return test::scope_continuity_words(std::as_const(poses).state())==later_scope;};
    check(poses.owns_snapshot(snapshot)&&std::as_const(poses).state().source_render_interpolation.at(key).paired(),
        "A later real native tick expired the historical owner or failed to populate paired pose storage");
    {auto cancelled=poses.prepare_restore(snapshot,early_proof);check(poses.ready(cancelled,early_proof),"Valid pose preparation refused");}
    check(std::as_const(poses).state().source_render_interpolation.at(key).paired()&&scope_unchanged(),
        "Cancelling a pose preparation changed live history");
    auto stale=poses.prepare_restore(snapshot,early_proof);
    poses.mutating();aliased_render.at(key).capture_render(*later.tick(),later_pose->nodes);
    check(!poses.ready(stale,early_proof)&&!poses.commit(stale),"Retained-alias mutation did not invalidate preparation");
    auto accessed=poses.prepare_restore(snapshot,early_proof);(void)poses.state();
    check(!poses.ready(accessed,early_proof)&&!poses.commit(accessed),"Mutable state admission left a stale pose token eligible");
    auto prepared=poses.prepare_restore(snapshot,early_proof),competing=poses.prepare_restore(snapshot,early_proof);
    check(alternate_early_proof.matches(early)&&alternate_early_proof.cut().seal!=early_proof.cut().seal&&
        !poses.ready(prepared,alternate_early_proof)&&!poses.ready(prepared,later_proof)&&!foreign.ready(prepared,early_proof)&&!foreign.commit(prepared),
        "Foreign owner or different exact native proof committed pose history");
    std::exception_ptr error;std::thread other([&]{try{
        check(!poses.owns_snapshot(snapshot)&&!poses.ready(prepared,early_proof)&&!poses.commit(prepared),"Foreign executor published pose history");
        rejects([&]{poses.capture(early);});rejects([&]{poses.prepare_restore(snapshot,early_proof);});rejects([&]{(void)std::as_const(poses).state();});
    }catch(...){error=std::current_exception();}});other.join();if(error)std::rethrow_exception(error);
    check(scope_unchanged(),"Rejected pose preparations changed the later scope polynomial or unit/reset/death lifetimes");
    const auto *retired=&std::as_const(poses).state().source_render_interpolation.at(key);
    const auto old_live_nodes=retired->sample(*model,1);
    check(poses.ready(prepared,early_proof)&&poses.commit(prepared),"Exact pose restoration failed after controls");
    const auto &restored=std::as_const(poses).state();
    check(test::scope_continuity_words(restored)==test::scope_continuity_words(initial),
        "Pose restoration lost local scope derivatives, polynomial, zoom selectors or full unit lifetimes");
    check(&aliased_render==&restored.source_render_interpolation&&!restored.source_render_interpolation.at(key).paired()&&
        same_nodes(restored.source_render_interpolation.at(key).sample(*model,1),early_pose->nodes)&&
        same_nodes(restored.source_body_interpolation.sample(early_pose->object,1),early_pose->nodes)&&
        same_nodes(retired->sample(*model,1),old_live_nodes),
        "Pose commit reset/reconstructed endpoints, invalidated aliases or retired old containers inside publication");
    check(!poses.ready(prepared,early_proof)&&!poses.commit(prepared)&&!poses.ready(competing,early_proof)&&!poses.commit(competing),
        "Consumed or competing same-revision pose token committed twice");
    for(unsigned view=0;view<early.local_slots().size();++view){check(
        same_view(restored.client_fp_views[view],initial.client_fp_views[view])&&same_view(restored.client_fp_origins[view],initial.client_fp_origins[view])&&
        restored.client_fp_clock_available[view]==initial.client_fp_clock_available[view]&&
        restored.client_secondary_clock_available[view]==initial.client_secondary_clock_available[view]&&
        restored.client_fp_reported_units[view]==initial.client_fp_reported_units[view]&&restored.source_fp_lifetimes[view]==initial.source_fp_lifetimes[view]&&
        !restored.source_fp_interpolation[view].at(fp_key).paired()&&same_nodes(restored.source_fp_interpolation[view].at(fp_key).sample(*model,1),early_pose->nodes),
        "Four-view FP restore lost salted bindings, clock/node/event words, lifetimes or owning palette storage");
        const auto slot=early.local_slots()[view];
        check(!std::memcmp(&restored.aim_blends[slot],&initial.aim_blends[slot],sizeof(FirstPersonAimBlend))&&
            !std::memcmp(&restored.body_aim_blends[slot],&initial.body_aim_blends[slot],sizeof(FirstPersonAimBlend))&&
            restored.aim_alive[slot]==initial.aim_alive[slot],"Pose restore lost live aim-controller fields");
        auto expected=bind_locals,actual=bind_locals;initial.body_transitions[slot].apply(expected,.5f);restored.body_transitions[slot].apply(actual,.5f);
        check(!std::memcmp(expected.data(),actual.data(),actual.size()*sizeof(BonePose)),"Pose restore lost original transition clock/current/saved local banks");
    }
    check(!early_scene.empty(),"Actual native scene interpolation control is empty");
    const auto scene_owner=std::ranges::find_if(early_scene,[&](const auto &o){return o.handle!=null_datum&&o.visible&&!o.bsp_hidden&&
        std::ranges::any_of(later_scene,[&](const auto &next){return next.handle==o.handle&&next.placement==o.placement&&next.created_tick==o.created_tick&&next.visible&&!next.bsp_hidden;});});
    check(scene_owner!=early_scene.end(),"Actual scene endpoint has no surviving source lifetime");
    const auto next_owner=std::ranges::find_if(later_scene,[&](const auto &o){return o.handle==scene_owner->handle;});
    auto continued=restored.scene_interpolation;continued.capture(*later.tick(),later_scene);
    auto query=*next_owner;query.position.x+=3;if(query.solved_transform)query.solved_transform->position.x+=3;
    const auto before=scene_object_transform(*scene_owner),after=continued.sample(query,0);
    check(!std::memcmp(&before,&after,sizeof(Transform)),"Restored scene history did not supply its owning previous endpoint to the next real native tick");
    auto expected_scope=initial;
    for(unsigned frame=0;frame<28;++frame){
        auto &actual_scope=poses.state();
        for(unsigned view=0;view<early.local_slots().size();++view){
            const bool zoomed=frame<8 || frame>=20;
            test::scope_continuity_step(expected_scope,scope_constants,view,zoomed);
            test::scope_continuity_step(actual_scope,scope_constants,view,zoomed);
        }
        check(test::scope_continuity_words(actual_scope)==test::scope_continuity_words(expected_scope),
            "Restored scope history changed its exact future FOV, derivatives or transition expiry");
    }
    std::cout<<"PASS exact proof-bound local pose/FP history rollback: actual two native ticks, four couch views, owning WORLD/node/scene endpoints, salted clocks, aliases, cancellation/staleness/foreign executor, consumed and deferred retirement\n";
    std::cout<<"PASS four-view local scope history: unfinished source polynomials, persistent targets, zoom/unit/reset lifetimes, exact 28-frame suffix after native-proof-bound rollback\n";
    std::cout<<"LIMIT pose fixture uses real received native cuts and authored metadata; declared FP storage/CPU transition controls do not qualify native FP/locomotion, HUD, renderer or playing-audio adoption\n";
}
}
int main(int argc,char **argv)try {
    const bool mcc=argc>=2&&std::string_view(argv[1])=="--mcc";
    if(mcc)check(argc==3,"Supply --mcc MAP for the requested MCC snapshot/group route");
    if(!mcc&&(argc!=3 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2])))return 77;
    MultiplayerOptions options;options.map=argv[2];options.starting_weapon=5;
    if(mcc){check(std::filesystem::is_regular_file(options.map),"Requested MCC snapshot/group map is missing");
        options.shared_maps=options.map.parent_path();
        // Reject reference caches before creating a native world. The real
        // Cache decoder still validates the complete MCC input below.
        std::ifstream input(options.map,std::ios::binary);std::array<std::byte,8> header{};
        check(bool(input.read(reinterpret_cast<char*>(header.data()),header.size())),"Requested MCC map header is truncated");
        check(Bytes(header).u32(0)==0x68656164u&&Bytes(header).u32(4)==13,
            "Requested MCC snapshot/group route rejects non-MCC map inputs");}
    else {options.gameplay_xbe=argv[1];
        std::cout<<"REFERENCE authentic snapshot/group component driver uses the explicit offline XBE/map inputs; not MCC runtime qualification\n";}
    options.local_players=1;options.source_object_world=true;
    MultiplayerPreparedWorld world(options,std::nullopt,std::make_shared<FixedClock>());world.bind_source_local_views(world.local_slots);
    if(mcc){check(options.gameplay_xbe.empty()&&world.cache->is_mcc()&&!world.cache->is_vista()&&
        world.cache->original_data()==OriginalData::native_tu15(),"MCC snapshot/group route requires imported MCC content and compiled native gameplay");
        std::cout<<"MCC authentic receipt/full16 gait component route: external_xbe=0; complete fixtures unchanged; logical transport, no runtime role activation\n";}
    SourceMatchAuthority authority(world.match,*world.source_objects);
    auto bus=std::make_shared<Bus>();constexpr std::uint64_t fingerprint=0x15c071900;
    std::vector<OnlineAdmission> roster;
    for(unsigned slot=0;slot<6;++slot)roster.push_back({slot,"Receipt "+std::to_string(slot),-1,{},slot==0?0u:slot<5?1u:5u,slot>1&&slot<5});
    LanHost host(world.match,std::make_shared<Endpoint>(bus,0),fingerprint,roster);
    const std::array profiles{profile("One"),profile("Two"),profile("Three"),profile("Four")};
    LanClient couch(std::make_shared<Endpoint>(bus,1),0,fingerprint,{},profiles);
    check(!couch.state_origin().tick(),"Unadmitted client invented a state receipt");
    const MultiplayerSnapshotOrigin missing;
    rejects([&]{(void)missing.accepted_state();});
    check(missing.local_slots().empty()&&!missing.same_connection(missing),"Missing receipt invented owned admission");
    std::array<Controls,4> local{};std::array<Controls,16> controls{};
    for(unsigned i=0;i<32&&!couch.ready();++i){couch.send(local);host.receive(controls);host.broadcast();couch.receive();}
    check(couch.ready() && couch.slots().size()==4 && !couch.received_origins().empty(),"Four-seat receipt admission failed");
    const auto unbound=couch.received_origins().back();check(!unbound.context(),"Pre-admission receipt invented a context");
    check(couch.state_origin().same(unbound)&&unbound.local_slots().empty()&&!unbound.same_connection(unbound),
        "Pre-admission receipt invented retained decoded state or a bound connection");
    rejects([&]{(void)unbound.accepted_state();});
    AuthorityTransferContext context;context.host=0;context.peers=(1u<<0)|(1u<<1)|(1u<<5);
    context.epoch=3;context.membership_revision=7;context.session[0]=std::byte{0x71};
    couch.bind_continuity(context,1,roster);const auto binding=source_authority_binding(context);
    check(!couch.state_origin().tick()&&couch.received_origins().back().same(unbound)&&
        !couch.received_origins().back().context(),
        "Continuity binding retroactively certified a pre-admission state");
    world.match.step(controls);host.broadcast();const auto first_packets=bus->queues[1];couch.receive();
    check(couch.received_states().size()==couch.received_origins().size(),"Accepted state/receipt order differs");
    const auto early=couch.received_origins().back();const auto tick=couch.state().tick;
    check(early.tick()==tick && early.context()==context && !couch.state().players[5].connected,"Initial receipt has wrong state");
    const std::array couch_slots{1u,2u,3u,4u};
    const auto *retained_early=&early.accepted_state();
    check(couch.state_origin().same(early)&&early.same_connection(early)&&
        std::ranges::equal(early.local_slots(),couch_slots)&&retained_early!=&couch.state()&&
        retained_early->tick==tick&&!retained_early->players[5].connected&&
        retained_early->players[1].source_configuration==couch.state().players[1].source_configuration&&
        retained_early->players[4].source_configuration==couch.state().players[4].source_configuration,
        "Bound receipt did not retain its actual decoded state and four couch slots");
    const auto native_cut=[&](std::uint64_t ordinal){
        const auto native=authority.checkpoint(binding);const auto bytes=encode_source_authority(native);
        return ValidatedAuthorityCut{1,context,{ordinal,authority.execution_tick(),bytes.size(),hash(bytes)}};
    };
    const auto early_cut=native_cut(1);const auto early_host=host.checkpoint(binding);
    const auto early_proof=couch.prove_snapshot(early_host,early_cut,early);
    NativeSceneContinuityFixture scene_fixture(world);scene_fixture.capture(early);
    NativeHudGroupFixture hud_fixture(world);hud_fixture.capture(early);
    NativeLocomotionGroupFixture locomotion_fixture(world);locomotion_fixture.capture(early);
    const auto alternate_early_cut=native_cut(5);
    const auto alternate_early_proof=couch.prove_snapshot(early_host,alternate_early_cut,early);
    const std::vector<SceneObjects::Object> early_scene(world.match.simulation().scene().objects().begin(),world.match.simulation().scene().objects().end());
    rejects([&]{(void)couch.prove_snapshot(early_host,early_cut,unbound);});
    rejects([&]{(void)couch.prove_snapshot(early_host,early_cut,MultiplayerSnapshotOrigin{});});
    const auto encode_host=LanHost::encode_continuation(early_host,{context,early_cut.seal});
    const auto admitted=authority.player_admissions(std::array{0u,1u,2u,3u,4u});
    const auto decoded=LanHost::decode_continuation(encode_host,{context,early_cut.seal},fingerprint,roster,admitted);
    check(couch.prove_snapshot(decoded,early_cut,early).matches(early),"Codec lost exact snapshot origin");
    auto obsolete=encode_host;obsolete[7]=std::byte{'1'};
    rejects([&]{(void)LanHost::decode_continuation(obsolete,{context,early_cut.seal},fingerprint,roster,admitted);});

    std::uint32_t primary=0x10203040,auxiliary=0x12345678;const SourceEffectRandom random{primary,auxiliary};Mesh mesh;
    NativeWorldEffects effects(*world.cache,mesh,world.match.simulation().collision(),random);
    NativeMuzzleEffects muzzle(*world.cache,random);SourceMapEffects map(*world.cache,world.content.objects,effects);
    SourceCombatAttachments combat(*world.cache,effects,{});EffectPresentationContinuity presentation(muzzle,effects,map,combat,random);
    effects.advance(tick,tick_seconds);muzzle.advance(tick,tick_seconds);
    rejects([&]{(void)presentation.capture(unbound);});
    const auto early_presentation=presentation.capture(early);
    rejects([&]{(void)presentation.bind(early_presentation,early_cut);});
    const auto early_bound=presentation.bind(early_presentation,early_proof);
    check(early_bound.cut().seal==early_cut.seal,"Origin proof did not retain its exact native cut");

    auto other=std::make_unique<LanClient>(std::make_shared<Endpoint>(bus,5),0,fingerprint);Controls neutral{};
    for(unsigned i=0;i<32&&!other->ready();++i){other->send(std::span(&neutral,1));host.receive(controls);other->receive();}
    check(other->ready(),"Same-tick late join failed");other->bind_continuity(context,5,roster);
    host.broadcast();const auto genuine=bus->queues[1];bus->queues[1].clear();
    check(!genuine.empty() && !first_packets.empty(),"Missing body/sequence mismatch controls");
    const auto late_cut=native_cut(2);const auto late_host=host.checkpoint(binding);
    // Reuse an older valid body under the newer sequence in this fixture's
    // logical transport. DTLS would reject network tampering; this exercises
    // accidental/malicious body association after a caller claims acceptance.
    for(auto packet:first_packets){const auto next=sequence(genuine.front());
        for(unsigned i=0;i<4;++i)packet.bytes[23+i]=std::byte(next>>(8*i));bus->queues[1].push_back(std::move(packet));}
    couch.receive();check(couch.state().tick==tick && !couch.state().players[5].connected && !couch.received_origins().empty(),
        "Reused valid-body control did not reach the ordinary decoder");
    const auto wrong_body=couch.received_origins().back();
    rejects([&]{(void)couch.prove_snapshot(late_host,late_cut,wrong_body);});
    bus->queues[1]=genuine;couch.receive();check(couch.received_origins().empty(),"Duplicate sequence created another origin receipt");
    host.broadcast();const auto replacement_packets=bus->queues[5];couch.receive();other->receive();
    check(couch.state().tick==tick && couch.state().players[5].connected && couch.received_states().size()==couch.received_origins().size(),
        "Newer same-tick roster was not accepted with its receipt");
    const auto latest=couch.received_origins().back(),other_origin=other->received_origins().back();
    check(latest.sequence()!=early.sequence(),"Same-tick states lost their distinct sequence");
    check(early.same_connection(latest)&&!early.same(latest)&&!early.same_connection(other_origin)&&
        !early.same_connection(unbound)&& &early.accepted_state()==retained_early&&
        !retained_early->players[5].connected&&latest.accepted_state().players[5].connected,
        "Later same-tick adoption mutated an earlier owning state or mixed connections");
    const auto latest_cut=native_cut(3);const auto latest_host=host.checkpoint(binding);
    const auto proof=couch.prove_snapshot(latest_host,latest_cut,latest);
    rejects([&]{(void)couch.prove_snapshot(latest_host,latest_cut,early);});
    rejects([&]{(void)couch.prove_snapshot(latest_host,latest_cut,other_origin);});
    rejects([&]{(void)presentation.bind(early_presentation,proof);});
    check(couch.prove_snapshot(early_host,early_cut,early).matches(early),"Later receives destroyed an older owning receipt");
    const auto latest_presentation=presentation.capture(latest);const auto bound=presentation.bind(latest_presentation,proof);
    check(bound.cut().seal==latest_cut.seal,"Same-tick receipt bound the wrong native cut");
    host.broadcast();host.broadcast();couch.receive();
    check(couch.received_states().size()==2&&couch.received_origins().size()==2&&
        couch.state_origin().same(couch.received_origins().back())&&
        early.same_connection(couch.state_origin())&& &early.accepted_state()==retained_early&&
        std::ranges::equal(early.local_slots(),couch_slots)&&!retained_early->players[5].connected,
        "A later receive batch discarded or changed the retained decoded receipt");
    const auto last_adopted=couch.state_origin();
    couch.receive();check(couch.received_states().empty() && couch.received_origins().empty()&&couch.state_origin().same(last_adopted),
        "Empty receive changed the last adopted receipt or retained per-poll rows");
    auto next=context;++next.epoch;++next.membership_revision;next.peers&=~1u;next.host=1;
    const auto other_proof=other->prove_snapshot(latest_host,latest_cut,other_origin);
    rejects([&]{(void)other->prepare_rebind_host(context,next,latest_cut.seal,MultiplayerSnapshotProof{});});
    rejects([&]{(void)other->prepare_rebind_host(context,next,latest_cut.seal,proof);});
    auto wrong_cut=latest_cut.seal;++wrong_cut.id;
    rejects([&]{(void)other->prepare_rebind_host(context,next,wrong_cut,other_proof);});
    // A later real source tick must be rolled back to the exact proven body,
    // while the receipt itself keeps its original historical generation.
    world.match.step(controls);host.broadcast();other->receive();
    couch.receive();const auto later_pose_origin=couch.state_origin();
    const auto later_pose_cut=native_cut(4);const auto later_pose_host=host.checkpoint(binding);
    const auto later_pose_proof=couch.prove_snapshot(later_pose_host,later_pose_cut,later_pose_origin);
    scene_fixture.restore(early,early_proof,alternate_early_proof,later_pose_origin,later_pose_proof);
    hud_fixture.restore(early,early_proof,alternate_early_proof,later_pose_origin,later_pose_proof);
    locomotion_fixture.restore(early,early_proof,alternate_early_proof,later_pose_origin,later_pose_proof);
    pose_roundtrip(*world.cache,early_scene,world.match.simulation().scene().objects(),early,early_proof,alternate_early_proof,later_pose_origin,later_pose_proof);
    check(other->state().tick>tick&&other->state_origin().tick()==other->state().tick,
        "Exact rebind rollback control never consumed a later native snapshot");
    const auto later_tick=other->state().tick;
    auto prepared=other->prepare_rebind_host(context,next,latest_cut.seal,other_proof);
    check(other->state().tick==later_tick&&other->can_commit_rebind_host(*prepared),
        "Preparing an exact client rollback changed the live decoded state");
    check(other->commit_rebind_host(std::move(prepared)),"Cannot exercise source-generation retirement");
    check(prepared&&!other->can_commit_rebind_host(*prepared)&&
        !other->commit_rebind_host(std::move(prepared))&&prepared,
        "Successful rebind discarded retired storage or a consumed token committed twice");
    check(other->state().tick==tick&&other->state().players[5].source_configuration==other_origin.accepted_state().players[5].source_configuration&&
        other->state().source_body_poses.size()==other_origin.accepted_state().source_body_poses.size()&&
        !other->state_origin().tick()&&other->received_origins().empty()&&other->received_states().empty(),
        "Exact rebind retained future decoded state or relabeled a predecessor receipt as a successor");
    rejects([&]{(void)other->prove_snapshot(latest_host,latest_cut,other_origin);});
    // Reuse real decoded snapshot bytes through the new logical endpoint.
    // This checks generation ownership, not replacement-host native proof.
    check(!replacement_packets.empty(),"Missing real snapshot for the new-generation decoder control");
    const auto replacement_sequence=sequence(replacement_packets.back());
    bus->queues[5].clear();
    for(auto packet:replacement_packets)if(sequence(packet)==replacement_sequence){
        packet.from=1;bus->queues[5].push_back(std::move(packet));}
    other->receive();
    const auto rebound=other->state_origin();const std::array other_slots{5u};
    check(rebound.context()==next&&rebound.tick()==tick&&rebound.same_connection(rebound)&&
        !other_origin.same_connection(rebound)&&!rebound.same_connection(other_origin)&&
        std::ranges::equal(rebound.local_slots(),other_slots)&&rebound.accepted_state().players[5].connected,
        "New-generation receipt inherited the retired connection identity");
    const auto *retained_rebound=&rebound.accepted_state();
    other.reset();
    check(&rebound.accepted_state()==retained_rebound&&retained_rebound->tick==tick&&
        retained_rebound->players[5].connected&&std::ranges::equal(rebound.local_slots(),other_slots)&&
        other_origin.accepted_state().players[5].connected&&std::ranges::equal(other_origin.local_slots(),other_slots),
        "Client destruction discarded the receipt's owned decoded state or slots");
    std::cout<<"PASS exact native snapshot receipt/sidecar proof, owned decoded state/four couch slots through later batches and client destruction, same-tick late join, wrong body under matching sequence, stale/duplicate/foreign/unbound receipts, codec roundtrip/version and epoch retirement\n";
    std::cout<<"PASS proof-bound decoded client rollback from a later native tick; rejected missing/foreign/wrong-cut proofs, retained consumed retirement token, unchanged couch ownership and authentic new-epoch first receive\n";
    std::cout<<"LIMIT logical endpoint adapter and locally validated native cuts; no DTLS/WAN/quorum or runtime/GPU migration; effect presenters are empty\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
