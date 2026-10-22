#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/effect_presentation_history.hpp"
#include "halo2/material_presentation_continuity.hpp"
#include "halo2/multiplayer_scene_continuity.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "mcc_native_prompt_hud_fixture.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <set>
#include <sstream>
#include <string_view>
#include <thread>
#if defined(__linux__)
#include <sys/resource.h>
#endif

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::int64_t observation_time(std::chrono::steady_clock::time_point at){
    return std::chrono::duration_cast<std::chrono::nanoseconds>(at.time_since_epoch()).count();}
std::int64_t observation_time(){return observation_time(std::chrono::steady_clock::now());}
using SceneCaptureCosts=MultiplayerSceneContinuity::CaptureCosts;
constexpr std::array scene_leaf_names{"grenade","values","scenery","machine","prop","dynamic",
    "projectile","point_light","material","total"};
auto scene_works(SceneCaptureCosts &cost){
    return std::array{&cost.grenade,&cost.values,&cost.scenery,&cost.machine,&cost.prop,&cost.dynamic,
        &cost.projectile,&cost.point_light,&cost.material,&cost.total};
}
auto scene_works(const SceneCaptureCosts &cost){
    return std::array{&cost.grenade,&cost.values,&cost.scenery,&cost.machine,&cost.prop,&cost.dynamic,
        &cost.projectile,&cost.point_light,&cost.material,&cost.total};
}
bool scene_delta(const SceneCaptureCosts &now,const SceneCaptureCosts &previous,SceneCaptureCosts &out){
    if(now.attempts<previous.attempts||now.completed<previous.completed)return false;
    SceneCaptureCosts result{};result.attempts=now.attempts-previous.attempts;
    result.completed=now.completed-previous.completed;
    const auto n=scene_works(now),p=scene_works(previous);auto d=scene_works(result);
    for(unsigned i=0;i<n.size();++i){
        if(n[i]->wall_ms<p[i]->wall_ms||n[i]->thread_cpu_ms<p[i]->thread_cpu_ms||
            n[i]->samples<p[i]->samples||n[i]->thread_cpu_samples<p[i]->thread_cpu_samples)return false;
        *d[i]={n[i]->wall_ms-p[i]->wall_ms,n[i]->thread_cpu_ms-p[i]->thread_cpu_ms,
            n[i]->samples-p[i]->samples,n[i]->thread_cpu_samples-p[i]->thread_cpu_samples};
    }
    out=result;return true;
}
void add_scene_costs(SceneCaptureCosts &sum,const SceneCaptureCosts &cost){
    sum.attempts+=cost.attempts;sum.completed+=cost.completed;
    auto s=scene_works(sum);const auto c=scene_works(cost);
    for(unsigned i=0;i<s.size();++i){s[i]->wall_ms+=c[i]->wall_ms;s[i]->thread_cpu_ms+=c[i]->thread_cpu_ms;
        s[i]->samples+=c[i]->samples;s[i]->thread_cpu_samples+=c[i]->thread_cpu_samples;}
}
void scene_payload(unsigned rank,const char *side,const SceneCaptureCosts &cost,bool valid){
    std::cout<<"OBSERVED paired-scene-leaf rank="<<rank<<" side="<<side<<" valid="<<valid
        <<" scope="<<(rank?"whole_delivered_frame_window":"named_cumulative_window")
        <<" attempts="<<cost.attempts<<" completed="<<cost.completed;
    const auto work=scene_works(cost);
    for(unsigned i=0;i<work.size();++i)std::cout<<' '<<scene_leaf_names[i]<<"_ms="<<work[i]->wall_ms
        <<' '<<scene_leaf_names[i]<<"_cpu_ms="<<work[i]->thread_cpu_ms
        <<' '<<scene_leaf_names[i]<<"_samples="<<work[i]->samples
        <<' '<<scene_leaf_names[i]<<"_cpu_samples="<<work[i]->thread_cpu_samples;
    std::cout<<'\n';
}
// Fixed observation storage only. Missing frame callbacks leave endpoints
// unpaired; they never create completed frames or change acceptance gates.
struct PairedInputGaps {
    struct Between {std::uint64_t zero_frames{},other_frames{},missing_loops{};
        double wall_ms{},capture_ms{},receive_ms{},projection_ms{};
        SceneCaptureCosts scene{};bool scene_valid{true};};
    struct Gap {
        bool used{},previous_paired{},current_paired{},same_frame{};
        std::uint64_t before_ordinal{},after_ordinal{};
        std::int64_t before_ns{},after_ns{};
        MultiplayerFrameTiming previous,current;
        SceneCaptureCosts previous_scene{},current_scene{};
        bool previous_scene_valid{},current_scene_valid{};
        Between between;
    };
    std::array<Gap,8> largest{};
    std::optional<MultiplayerFrameTiming> last_command_frame;
    SceneCaptureCosts last_command_scene{};bool last_command_scene_valid{};
    Between between;
    std::int64_t last_input{};
    std::uint64_t last_loop{},frames{},zero_frames{},missing_loops{},unmatched_frames{};
    std::uint64_t command_tails{},send_calls{},observer_calls{},receive_calls{};
    void input(std::int64_t at,std::uint64_t ordinal){
        if(last_input){
            auto slot=std::find_if(largest.begin(),largest.end(),[](const auto &g){return !g.used;});
            if(slot==largest.end())slot=std::min_element(largest.begin(),largest.end(),[](const auto &a,const auto &b){
                return a.after_ns-a.before_ns<b.after_ns-b.before_ns;});
            if(!slot->used||at-last_input>slot->after_ns-slot->before_ns){
                *slot=Gap{};slot->used=true;slot->before_ns=last_input;slot->after_ns=at;
                slot->before_ordinal=ordinal-1;slot->after_ordinal=ordinal;
            }
        }
        last_input=at;
    }
    void frame(const MultiplayerFrameTiming &frame,std::uint64_t actual_callbacks,
        const SceneCaptureCosts &scene,bool scene_valid){
        const auto &s=frame.schedule;++frames;
        command_tails+=s.command_tails;send_calls+=s.client_send_calls;
        observer_calls+=s.input_observer_calls;receive_calls+=s.receive_calls;
        const bool ordered=s.measured&&s.loop>last_loop;
        const auto holes=ordered?s.loop-last_loop-1:1;
        missing_loops+=holes;between.missing_loops+=holes;
        last_loop=s.loop;
        const bool range=ordered&&s.input_observer_calls&&s.first_observer&&
            s.last_observer>=s.first_observer&&
            s.last_observer-s.first_observer+1==s.input_observer_calls&&
            s.last_observer==actual_callbacks;
        for(auto &g:largest)if(g.used&&!g.current_paired&&range&&
            g.after_ordinal>=s.first_observer&&g.after_ordinal<=s.last_observer){
            g.current=frame;g.current_paired=true;
            g.current_scene=scene;g.current_scene_valid=scene_valid;
            if(g.before_ordinal>=s.first_observer&&g.before_ordinal<=s.last_observer){
                g.previous=frame;g.previous_paired=true;g.same_frame=true;g.between={};
                g.previous_scene=scene;g.previous_scene_valid=scene_valid;
            }else{
                g.between=between;
                if(g.after_ordinal==s.first_observer&&last_command_frame&&
                    last_command_frame->schedule.last_observer==g.before_ordinal&&
                    last_command_frame->schedule.loop<s.loop&&
                    !between.missing_loops&&!between.other_frames){
                    g.previous=*last_command_frame;g.previous_paired=true;
                    g.previous_scene=last_command_scene;g.previous_scene_valid=last_command_scene_valid;
                }
            }
        }
        if(s.input_observer_calls){
            if(range){last_command_frame=frame;last_command_scene=scene;last_command_scene_valid=scene_valid;}
            else{last_command_frame.reset();last_command_scene_valid=false;++unmatched_frames;}
            between={};
        }else{
            if(!s.command_tails){++zero_frames;++between.zero_frames;}
            else{++between.other_frames;++unmatched_frames;}
            between.wall_ms+=frame.wall_ms;between.capture_ms+=frame.receipt_capture.total.wall_ms;
            between.receive_ms+=s.receive.wall_ms;between.projection_ms+=s.projection.wall_ms;
            add_scene_costs(between.scene,scene);between.scene_valid=between.scene_valid&&scene_valid;
        }
    }
    static double cpu(const MultiplayerFrameTiming::Stamp &begin,const MultiplayerFrameTiming::Stamp &end){
        return begin.thread_cpu_available&&end.thread_cpu_available&&begin.thread_cpu_ms>=0&&
            end.thread_cpu_ms>=begin.thread_cpu_ms?end.thread_cpu_ms-begin.thread_cpu_ms:-1.;
    }
    static void payload(unsigned rank,const char *side,const MultiplayerFrameTiming &f){
        const auto &s=f.schedule;
        std::cout<<"OBSERVED paired-input-frame rank="<<rank<<" side="<<side<<" loop="<<s.loop
            <<" first_observer="<<s.first_observer<<" last_observer="<<s.last_observer
            <<" command_tails="<<s.command_tails<<" returned_client_send_calls="<<s.client_send_calls
            <<" input_observer_calls="<<s.input_observer_calls<<" receive_calls="<<s.receive_calls
            <<" bank="<<s.bank_size<<" cursor_begin="<<s.cursor_begin<<" cursor_end="<<s.cursor_end
            <<" projected="<<s.projected_states<<" had_pending="<<s.had_pending<<" has_pending="<<s.has_pending
            <<" terminal_drain="<<s.terminal_drain<<" frame_wall_ms="<<f.wall_ms
            <<" input_ms="<<f.input_ms<<" common_ms="<<f.common_ms<<" renderer_ms="<<f.renderer_ms
            <<" capture_ms="<<f.receipt_capture.total.wall_ms<<" capture_cpu_ms="<<f.receipt_capture.total.thread_cpu_ms
            <<" capture_samples="<<f.receipt_capture.total.samples<<" capture_cpu_samples="<<f.receipt_capture.total.thread_cpu_samples
            <<" scene_ms="<<f.receipt_capture.scene.wall_ms<<" scene_cpu_ms="<<f.receipt_capture.scene.thread_cpu_ms
            <<" scene_samples="<<f.receipt_capture.scene.samples<<" scene_cpu_samples="<<f.receipt_capture.scene.thread_cpu_samples
            <<" locomotion_ms="<<f.receipt_capture.locomotion.wall_ms<<" locomotion_cpu_ms="<<f.receipt_capture.locomotion.thread_cpu_ms
            <<" locomotion_samples="<<f.receipt_capture.locomotion.samples<<" locomotion_cpu_samples="<<f.receipt_capture.locomotion.thread_cpu_samples
            <<" receive_ms="<<s.receive.wall_ms<<" receive_cpu_ms="<<s.receive.thread_cpu_ms
            <<" receive_samples="<<s.receive.samples<<" receive_cpu_samples="<<s.receive.thread_cpu_samples
            <<" projection_ms="<<s.projection.wall_ms<<" projection_cpu_ms="<<s.projection.thread_cpu_ms
            <<" projection_samples="<<s.projection.samples<<" projection_cpu_samples="<<s.projection.thread_cpu_samples
            <<" prefix_cpu_ms="<<cpu(s.iteration_begin,s.timed_start)
            <<" timed_to_first_tail_cpu_ms="<<(s.command_tails?cpu(s.timed_start,s.first_command_tail):-1.)
            <<" last_tail_to_render_cpu_ms="<<(s.command_tails?cpu(s.last_command_tail,s.renderer_end):-1.)
            <<" fence_ms="<<f.renderer.fence_ms<<" acquire_ms="<<f.renderer.acquire_ms
            <<" submit_ms="<<f.renderer.submit_ms<<" present_ms="<<f.renderer.present_ms<<'\n';
    }
    void report()const{
        auto sorted=largest;
        std::sort(sorted.begin(),sorted.end(),[](const auto &a,const auto &b){
            return (a.used?a.after_ns-a.before_ns:-1)>(b.used?b.after_ns-b.before_ns:-1);});
        std::cout<<"OBSERVED paired-input-summary frames="<<frames<<" zero_completed_command_frames="<<zero_frames
            <<" missing_loop_callbacks="<<missing_loops<<" unmatched_frames="<<unmatched_frames
            <<" command_tails="<<command_tails<<" returned_client_send_calls="<<send_calls
            <<" input_observer_calls="<<observer_calls<<" receive_calls="<<receive_calls
            <<" scope=local_existing_input_observer_not_host_arrival stamp_cpu_unavailable=-1 work_cpu_sample_coverage_explicit=1\n";
        unsigned rank{};for(const auto &g:sorted)if(g.used){++rank;
            const auto &p=g.previous.schedule,&c=g.current.schedule;
            const bool chronology=g.before_ns<=g.after_ns&&(g.same_frame||
                (g.before_ns<=p.renderer_end.wall_ns&&p.renderer_end.wall_ns<=c.iteration_begin.wall_ns&&
                 c.iteration_begin.wall_ns<=c.timed_start.wall_ns&&c.timed_start.wall_ns<=g.after_ns));
            const bool paired=g.previous_paired&&g.current_paired&&chronology;
            std::cout<<"OBSERVED paired-input-gap rank="<<rank<<" paired="<<paired<<" same_frame="<<g.same_frame
                <<" previous_frame_paired="<<g.previous_paired<<" current_frame_paired="<<g.current_paired
                <<" before_observer="<<g.before_ordinal<<" after_observer="<<g.after_ordinal
                <<" before_ns="<<g.before_ns<<" after_ns="<<g.after_ns
                <<" gap_ns="<<g.after_ns-g.before_ns<<" zero_frames="<<g.between.zero_frames
                <<" missing_loops="<<g.between.missing_loops<<" other_frames="<<g.between.other_frames
                <<" intervening_frame_wall_ms="<<g.between.wall_ms<<" intervening_capture_ms="<<g.between.capture_ms
                <<" intervening_receive_ms="<<g.between.receive_ms<<" intervening_projection_ms="<<g.between.projection_ms;
            if(paired&&!g.same_frame){
                const auto post=p.renderer_end.wall_ns-g.before_ns;
                const auto boundary=c.iteration_begin.wall_ns-p.renderer_end.wall_ns;
                const auto prefix=c.timed_start.wall_ns-c.iteration_begin.wall_ns;
                const auto before=g.after_ns-c.timed_start.wall_ns;
                std::cout<<" previous_post_command_ns="<<post<<" boundary_and_intervening_ns="<<boundary
                    <<" current_prefix_ns="<<prefix<<" current_pre_command_ns="<<before
                    <<" reconciliation_residual_ns="<<(g.after_ns-g.before_ns)-(post+boundary+prefix+before);
            }
            std::cout<<'\n';
            if(g.previous_paired){payload(rank,"previous",g.previous);
                scene_payload(rank,"previous",g.previous_scene,g.previous_scene_valid);}
            if(g.current_paired){payload(rank,"current",g.current);
                scene_payload(rank,"current",g.current_scene,g.current_scene_valid);}
            if(g.between.zero_frames||g.between.other_frames||g.between.missing_loops)
                scene_payload(rank,"intervening",g.between.scene,g.between.scene_valid&&!g.between.missing_loops);
        }
    }
};
template<class F>void wait(F ready,const char *why){const auto until=std::chrono::steady_clock::now()+120s;
    while(!ready()){check(std::chrono::steady_clock::now()<until,why);std::this_thread::sleep_for(5ms);}}
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();
    std::array<std::uint8_t,9> colors;std::copy_n(defaults.begin()+0x40,colors.size(),colors.begin());
    return native_selected_player_configuration(name,colors,-1);}
void require_unadvertised_migration(const std::shared_ptr<OnlineMatchSession> &session){
    const auto agreement=session->peer_migration();
    check(agreement&&!agreement->decision&&
        std::none_of(agreement->ready.begin(),agreement->ready.end(),[](const auto &cut){return cut.has_value();}),
        "Recovery-pin coordination advertised full migration readiness or elected a replacement host");
}
SourceWorldParticleDrawsTu15::ResourceState authored_weapon_particle_resources(MultiplayerPreparedWorld &world){
    // Independently census real cache definitions before the host admits any
    // player. This bank never prepares storage, publishes observers or draws.
    SourceMatchAuthority authority(world.match,*world.source_objects);
    const auto boundary=authority.execution_boundary();
    SourceWorldParticleDrawsTu15 bank(*world.cache,world.content.objects);
    std::set<std::uint32_t> effects;
    const auto collect=[&](const WeaponDefinition &weapon){
        for(const auto &row:weapon.firing_effects)effects.insert(row.effects.begin(),row.effects.end());
        if(weapon.ready_effect!=null_datum)effects.insert(weapon.ready_effect);
    };
    for(const auto &[datum,weapon]:world.content.weapons){
        collect(weapon);
        for(const auto &barrel:weapon.barrel_views){
            check(bool(barrel),"Independent weapon resource census lost its authored barrel view");collect(*barrel);
        }
        const auto metadata=world.cache->load_source_tag_metadata(datum);
        const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));const auto count=root.u32(0x94);
        std::optional<SourceTagMetadata> owner;
        if(count){
            const auto tag=world.cache->query_tag({.contains=TagAddressRange{root.u32(0x98),std::size_t(count)*0x18},.require_address=true});
            check(tag,"Independent weapon attachment census has no resident metadata owner");
            owner.emplace(world.cache->load_source_tag_metadata(tag->datum));
        }
        const Bytes attachments(count?owner->bytes_at(root.u32(0x98),std::size_t(count)*0x18):std::span<const std::byte>{});
        for(unsigned row=0;row<count;++row){const auto at=row*0x18;
            if(attachments.u32(at)==0x65666665u&&attachments.u32(at+4)!=null_datum)effects.insert(attachments.u32(at+4));}
    }
    unsigned fp_supported{},world_supported{},boundary_count{};std::vector<std::string> diagnostics;
    const auto diagnose=[&](std::uint32_t effect,const char *pass,const FormatError &error){
        ++boundary_count;
        if(diagnostics.size()<16){std::ostringstream text;text<<std::hex<<effect<<std::dec<<' '<<pass<<": "<<error.what();
            diagnostics.push_back(text.str().substr(0,240));}
    };
    for(const auto effect:effects){
        if(effect==null_datum)continue;
        const auto tag=world.cache->query_tag({.handle=TagHandle{effect},.require_address=true});
        if(!tag||tag->group!="effe")continue;
        try{fp_supported+=bank.preload_first_person(effect);}catch(const FormatError &error){diagnose(effect,"FP",error);}
        try{world_supported+=bank.preload_world(effect);}catch(const FormatError &error){diagnose(effect,"WORLD",error);}
    }
    const auto resources=bank.resource_state();
    check(fp_supported&&world_supported&&resources.materials&&resources.first_person_systems&&resources.world_systems,
        "Independent authored census established no supported FP/WORLD weapon particle resources");
    check(authority.execution_boundary()==boundary,"Independent passive weapon census changed the native host execution boundary");
    std::cout<<"Independent authored weapon particle census: effects="<<effects.size()<<" fp_supported="<<fp_supported
        <<" world_supported="<<world_supported<<" unsupported_boundary_calls="<<boundary_count<<'\n';
    for(const auto &diagnostic:diagnostics)std::cout<<"RESOURCE BOUNDARY independent weapon "<<diagnostic<<'\n';
    return resources;
}
// A real native authority on its own executor, with four host couch seats.
// Use the production authority/common-cut driver and broadcast every actual
//30Hz step. No master gameplay endpoint, fabricated native cut, clock,
// accepted snapshot or complete-owner readiness is supplied.
class Host {
    std::jthread worker_;
    std::exception_ptr error_;
    // Written once before the ready promise, immutable for all client reads.
    SourceWorldParticleDrawsTu15::ResourceState expected_particle_resources_{};
public:
    std::atomic<std::uint64_t> offered_cuts{},native_validated_cuts{},committed_cuts{},released_cuts{},aborted_cuts{};
    std::atomic<std::uint64_t> recording_drops{},broadcast_calls{},source_tick{};
    std::atomic<std::int64_t> admitted_at{},first_drop_at{};
    std::atomic<std::uint64_t> first_drop_tick{};
    // Written only by the native host executor; inspected after finish joins.
    std::optional<LanHost::PeerRemoval> first_removal;
    explicit Host(MultiplayerOptions options,bool selected_mcc) {
        std::promise<void> ready;auto future=ready.get_future();
        worker_=std::jthread([this,options=std::move(options),selected_mcc,ready=std::move(ready)](std::stop_token stop)mutable {
            bool announced{};try {
                const auto session=options.online_match;const auto &roster=session->roster();
                auto world=std::make_shared<MultiplayerPreparedWorld>(options);
                if(selected_mcc)check(options.gameplay_xbe.empty()&&world->cache->is_mcc()&&!world->cache->is_vista()&&
                    world->cache->original_data()==OriginalData::native_tu15()&&world->source_objects&&world->source_objects->owns_physics(),
                    "Selected MCC host did not retain compiled native gameplay ownership without an external executable");
                expected_particle_resources_=authored_weapon_particle_resources(*world);
                const auto locals=session->local_slots();
                check(locals.size()==4,"Native host lost its four actual couch seats");
                for(unsigned view=0;view<locals.size();++view){const auto slot=locals[view];
                    check(world->match.join(roster[slot].name,-1,options.selected_profiles[view],slot)==slot,"Host source profile changed its admitted slot");}
                world->bind_source_local_views(locals);
                // Match the normal runtime: starting_weapon is a later setter
                // write, so advertise the resolved world's exact ordered list.
                // The original fixture's pre-override list encoded differently
                // even though its final304 native bytes were equal.
                const auto &variant=world->match.session_configuration();check(variant.has_value(),"Native host lost its resolved configuration");
                // Match the production runtime's prepared-world operands.
                // A source-owned engine selects its real loadout at birth;
                // the cache's legacy starting-weapon adapter is not this
                // world's pre-admission rules.weapon identity.
                const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(options.map,
                    world->cache->shared_directory(),std::uint32_t(options.bsp),world->rules.weapon,
                    options.gameplay_xbe),*variant);
                std::cout<<"OBSERVED native host loading identity: fingerprint="<<fingerprint
                    <<" prepared_weapon="<<world->rules.weapon
                    <<" legacy_adapter_weapon="<<world->cache->slayer_starting_weapon(options.starting_weapon)
                    <<" source_engine_required="<<world->rules.requires_source_engine
                    <<" native_protocol="<<native_multiplayer_protocol<<'\n';
                auto host=std::make_unique<LanHost>(world->match,session,fingerprint,roster,*variant);
                MultiplayerRuntimeAuthority runtime(options,world,std::move(host),nullptr);
                session->loaded(fingerprint,*variant);ready.set_value();announced=true;
                while(!stop.stop_requested()&&session->phase()!=OnlineMatchSession::Phase::Running){
                    check(session->phase()!=OnlineMatchSession::Phase::Failed,session->error().c_str());std::this_thread::sleep_for(5ms);}
                std::array<Controls,multiplayer_capacity> controls{};
                const auto observe_peers=[&]{
                    const auto peers=runtime.host()->peers();
                    if(peers==1&&!admitted_at.load())admitted_at=observation_time();
                    if(peers!=1&&admitted_at.load()&&!first_drop_at.load()){
                        first_removal=runtime.host()->last_peer_removal();
                        first_drop_tick=runtime.match().simulation().ticks();first_drop_at=observation_time();}
                };
                // Match loaded/all-player admission in the windowed runtime:
                // actual hello/input processing and snapshots, no world tick
                // or publication service before every real seat is admitted.
                const auto admission_deadline=std::chrono::steady_clock::now()+15s;
                bool admitted{};
                while(!stop.stop_requested()&&!admitted){
                    check(session->phase()==OnlineMatchSession::Phase::Running,session->error().c_str());
                    runtime.host()->receive(controls);runtime.admit_players();runtime.host()->broadcast();++broadcast_calls;
                    observe_peers();
                    admitted=std::all_of(roster.begin(),roster.end(),[&](const auto &row){return runtime.match().player(row.slot).connected;});
                    if(!admitted){check(std::chrono::steady_clock::now()<admission_deadline,"Host real all-player admission timeout");
                        std::this_thread::sleep_for(10ms);}
                }
                std::set<std::uint64_t> offered,native_validated,committed,released,aborted;
                const auto observe_common=[&]{
                    require_unadvertised_migration(session);
                    if(const auto common=session->common_cut()){
                        const auto context=session->authority_context();
                        check(context&&common->context==*context&&
                            common->recipients==std::uint16_t(context->peers&~(std::uint16_t(1)<<session->self())),
                            "Production common-cut driver changed the fixed physical recipient membership");
                        if(common->pending){
                            const auto id=common->pending->seal.id;offered.insert(id);
                            if(common->native_all_validated)native_validated.insert(id);
                            if(common->release&&common->released==common->recipients)released.insert(id);
                            if(common->abort&&common->aborted==common->recipients)aborted.insert(id);
                        }
                        if(common->committed)committed.insert(common->committed->seal.id);
                    }
                    offered_cuts=offered.size();native_validated_cuts=native_validated.size();committed_cuts=committed.size();
                    released_cuts=released.size();aborted_cuts=aborted.size();
                    check(&runtime.world()==world.get()&&runtime.host()&&!runtime.client()&&session->host()==session->self(),
                        "Common recovery-pin publication changed the actual native host/world role");
                };
                auto next=std::chrono::steady_clock::now();
                while(!stop.stop_requested()) {
                    check(session->phase()==OnlineMatchSession::Phase::Running,session->error().c_str());
                    runtime.service();observe_common();
                    auto frame=runtime.begin_frame();check(bool(frame),"Healthy native host refused its real runtime frame");
                    runtime.host()->receive(controls);runtime.admit_players();observe_peers();
                    for(const auto slot:locals){controls[slot]={};controls[slot].yaw=runtime.match().unit(slot).yaw;}
                    runtime.match().step(controls);
                    // Every advanced native cut retains a genuinely emitted
                    // same-tick snapshot, as in the actual windowed host.
                    runtime.broadcast_candidate_frame(frame);++broadcast_calls;
                    check(bool(frame),"Healthy native candidate broadcast fenced its replacement frame");
                    // Losing a bounded successor journal does not invalidate
                    // completed live work or the separately held common pin.
                    if(!frame->finish())++recording_drops;
                    // Clock-only observation: state() executes native journal
                    // work and must never be called outside a recorded phase.
                    source_tick=runtime.match().simulation().ticks();observe_common();
                    next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
                }
            }catch(...){if(!announced)ready.set_exception(std::current_exception());else error_=std::current_exception();}
        });future.get();
    }
    SourceWorldParticleDrawsTu15::ResourceState expected_particle_resources()const noexcept{return expected_particle_resources_;}
    ~Host(){worker_.request_stop();if(worker_.joinable())worker_.join();}
    void finish(){worker_.request_stop();worker_.join();if(error_)std::rethrow_exception(error_);}
};
}
int main(int argc,char **argv)try {
    check(argc==5,"Supply ORIGIN DESCRIPTORS (TU15_XBE | --mcc) MAP");
    const bool selected_mcc=std::string_view(argv[3])=="--mcc";
    if(!std::getenv("DISPLAY")||(!selected_mcc&&!std::filesystem::is_regular_file(argv[3]))||
        !std::filesystem::is_regular_file(argv[4]))return 77;
    nlohmann::json descriptors;{std::ifstream in(argv[2]);in>>descriptors;}
    check(descriptors.size()==2,"Expected two physical four-seat peers");
    std::vector<OnlineAdmission> roster;
    for(const auto &row:descriptors.at(0).at("players")){
        OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");p.peer=row.at("peer");p.guest=row.at("guest");
        const std::string identity=row.at("identity");check(identity.size()==16,"Bad independent fixture identity");
        for(unsigned i=0;i<8;++i)p.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));roster.push_back(p);
    }
    check(roster.size()==8,"Expected eight independently admitted players");
    std::array<std::shared_ptr<OnlineMatchSession>,2> sessions;
    for(unsigned i=0;i<2;++i){const auto &d=descriptors[i];OnlineMatchTransportOptions transport;
        transport.direct_udp=true;transport.peer_host_migration=true;
        transport.snapshot_candidates=selected_mcc;
        sessions[i]=std::make_shared<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),d.at("slot"),roster,true,false,false,transport);}
    wait([&]{for(const auto &s:sessions){check(s->phase()!=OnlineMatchSession::Phase::Failed,s->error().c_str());
        if(s->phase()!=OnlineMatchSession::Phase::Loading)return false;}return true;},"Direct measured host loading timeout");
    check(sessions[0]->host()==sessions[1]->host(),"Measured physical host differs");
    const auto host_session=sessions[sessions[0]->host()==sessions[0]->self()?0:1];
    const auto client_session=sessions[host_session==sessions[0]?1:0];
    MultiplayerOptions options;options.map=argv[4];options.shared_maps=options.map.parent_path();
    if(!selected_mcc)options.gameplay_xbe=argv[3];
    options.local_players=4;options.starting_weapon=5;options.session_random_seed=0x713719;options.source_object_world=true;
    options.variant={"Slayer",0,{{0x59,5},{10,0},{2,0}}};options.width=960;options.height=540;options.mute=true;
    options.validation=std::getenv("HALO2_REPLICA_VALIDATION");
    Cache cache(options.map,options.shared_maps,options.gameplay_xbe.empty()?std::shared_ptr<const OriginalData>{}:
        OriginalData::load_xbe(options.gameplay_xbe));
    if(selected_mcc){
        check(options.gameplay_xbe.empty()&&cache.is_mcc()&&!cache.is_vista()&&cache.original_data()==OriginalData::native_tu15(),
            "Selected MCC ONLINE fixture requires actual MCC content and compiled native gameplay without an external executable");
        std::cout<<"OBSERVED selected-MCC-native ONLINE input: actual MCC cache and compiled native TU1.5 gameplay; external_xbe=0\n";
        mcc_native_prompt_test::verify_native_mcc_prompt_hud(options.shared_maps.parent_path()/"h2_fonts");
    }else std::cout<<"REFERENCE offline Xbox TU1.5 ONLINE history route; not MCC runtime acceptance\n";
    auto server=options;server.online_match=host_session;
    for(auto slot:host_session->local_slots())server.selected_profiles.push_back(profile(roster[slot].name));
    Host host(server,selected_mcc);
    const auto expected_particles=host.expected_particle_resources();
    options.online_match=client_session;for(auto slot:client_session->local_slots())options.selected_profiles.push_back(profile(roster[slot].name));
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    options.desktop=std::make_shared<DesktopSession>();options.desktop->window(options.width,options.height,false);
    unsigned accepted{},resource_observations{},captured{},draws{},awaiting_previous{},retained_receipts{};std::size_t maximum_retained{};
    unsigned scene_receipts{};std::size_t maximum_scene_material_bytes{},maximum_scene_unique_material_bytes{},maximum_scene_material_nodes{},scene_new_material_bytes{},scene_shared_material_bytes{},scene_generated_image_bytes{};
    std::optional<std::array<std::size_t,6>> source_resource_counts;
    std::optional<MultiplayerHudContinuity::Statistics> hud_census;unsigned hud_receipts{};
    unsigned locomotion_receipts{};std::size_t maximum_occupied_gaits{};std::uint16_t observed_unavailable_gaits{};
    std::set<std::uint64_t> bound_cuts;std::optional<ValidatedAuthorityCut> last_bound;bool quit{};
    std::int64_t first_input_at{},last_input_at{},maximum_input_gap{},completion_requested_at{};std::uint64_t input_deliveries{};
    std::int64_t first_frame_at{},first_full_capture_at{},deadline_branch_at{},runtime_caught_at{};
    SceneCaptureCosts scene_capture_costs{},scene_at_last_frame{};
    unsigned scene_delta_failures{},startup_pending_candidate_observations{};
    double maximum_frame_input_ms{},maximum_frame_wall_ms{};
    struct CaptureStage {
        double wall_ms{},thread_cpu_ms{},maximum_frame_wall_ms{};
        std::uint64_t samples{},thread_cpu_samples{};
        void add(const MultiplayerFrameTiming::Work &work){
            wall_ms+=work.wall_ms;thread_cpu_ms+=work.thread_cpu_ms;
            maximum_frame_wall_ms=std::max(maximum_frame_wall_ms,work.wall_ms);
            samples+=work.samples;thread_cpu_samples+=work.thread_cpu_samples;
        }
    };
    std::array<CaptureStage,9> capture_stages{};
    std::uint64_t capture_attempts{},capture_completed{},capture_retained{};
    PairedInputGaps paired_input_gaps;
    options.input_observer=[&](std::uint32_t,std::span<const Controls>,const MultiplayerState &){
        const auto at=observation_time();if(!first_input_at)first_input_at=at;
        if(last_input_at)maximum_input_gap=std::max(maximum_input_gap,at-last_input_at);
        last_input_at=at;++input_deliveries;
        paired_input_gaps.input(at,input_deliveries);
    };
    const auto deadline_started=std::chrono::steady_clock::now();
    const auto deadline=deadline_started+180s;
    options.effect_history_observer=[&](const MultiplayerSnapshotOrigin &origin,const EffectPresentationObservation &effect){
        ++accepted;check(origin.context()==client_session->authority_context(),"Runtime receipt has a different independent authority context");
        const auto &resources=effect.source_resources;
        check(resources.migration_candidate,"Actual four-view ONLINE client did not admit passive migration-candidate resources");
        check(!resources.particle_producer&&!resources.model_producer,
            "Passive migration-candidate admission activated a native particle or model producer");
        check(resources.particle_materials>0&&resources.first_person_systems>0&&resources.world_systems>0&&resources.model_light_sections>0,
            "Passive migration-candidate resource admission omitted particle, first-person, world or model-light resources");
        check(resources.particle_materials==expected_particles.materials&&
            resources.first_person_systems==expected_particles.first_person_systems&&
            resources.world_systems==expected_particles.world_systems,
            "ONLINE client skipped supported authored weapon particle resources from the independent complete cache census");
        check(resources.materials_now==resources.materials_at_upload,
            "Migration-candidate material residency changed after renderer upload");
        const std::array<std::size_t,6> counts{resources.particle_materials,resources.first_person_systems,resources.world_systems,
            resources.model_light_sections,resources.materials_at_upload,resources.materials_now};
        if(source_resource_counts)check(*source_resource_counts==counts,
            "Passive migration-candidate resource counts changed across actual accepted receipts");
        else source_resource_counts=counts;
        ++resource_observations;
        check(effect.world_tick==origin.tick()&&effect.muzzle_tick==origin.tick(),"Runtime replaced an incomplete CPU effect tick");
        check(effect.history.context==origin.context()&&effect.history.retained<=effect.history.capacity&&effect.history.capacity==128,
            "Runtime history exceeded or bypassed its gated retention policy");
        if(!retained_receipts)if(const auto delivery=client_session->snapshot_candidates()){
            const auto candidate=delivery->state();
            if(candidate.pending&&candidate.pending->context==effect.history.context){
                ++startup_pending_candidate_observations;
                check(!(candidate.rejected&(std::uint16_t(1)<<client_session->self())),
                    "Startup candidate was rejected before the first completed runtime history");
            }
        }
        if(effect.runtime_frontier_retained){
            if(!first_full_capture_at)first_full_capture_at=observation_time();
            ++retained_receipts;
            check(effect.history.newest_sequence==origin.sequence()&&effect.history.captured==retained_receipts,
                "Runtime receipt/frontier/effects were not captured atomically");
            check(effect.scene_frontier_retained&&effect.scene_statistics.has_value(),
                "Actual windowed history omitted its complete scene/callback/material owner capture");
            const auto &scene=*effect.scene_statistics;
            check(scene.material_roots==resources.materials_now&&scene.material_nodes>=scene.material_roots&&
                scene.material_logical_bytes>0&&scene.new_material_value_bytes+scene.shared_material_value_bytes>0,
                "Actual scene history skipped resident material roots or omitted mutable graph accounting");
            check(scene.generated_material_image_bytes>0,"Actual scene history omitted its admitted CPU radar image outputs");
            check(scene.material_unique_bytes>0&&scene.material_unique_bytes<=MaterialPresentationContinuity::Limits{}.checkpoint_bytes,
                "Actual scene history exceeded its unchanged unique material payload cap");
            ++scene_receipts;
            check(effect.hud_frontier_retained&&effect.hud_statistics.has_value(),
                "Actual windowed history omitted its complete CPU HUD/scoreboard registry capture");
            const auto &huds=*effect.hud_statistics;
            check(huds.huds[0]>0&&huds.huds[3]>0,"Actual HUD history omitted primary or unit HUD owners");
            if(hud_census)check(huds.huds==hud_census->huds&&huds.scoreboard==hud_census->scoreboard,
                "Actual HUD history changed its full fixed resident registry census");
            else hud_census=huds;
            ++hud_receipts;
            check(effect.locomotion_frontier_retained&&effect.locomotion_statistics.has_value(),
                "Actual windowed history omitted its full16 native locomotion group capture");
            check(effect.locomotion_statistics->occupied<=multiplayer_capacity,
                "Actual native locomotion group exceeded its fixed slot census");
            ++locomotion_receipts;
            maximum_occupied_gaits=std::max(maximum_occupied_gaits,effect.locomotion_statistics->occupied);
            observed_unavailable_gaits|=effect.current_locomotion_unavailable;
            maximum_scene_material_bytes=std::max(maximum_scene_material_bytes,scene.material_logical_bytes);
            maximum_scene_unique_material_bytes=std::max(maximum_scene_unique_material_bytes,scene.material_unique_bytes);
            maximum_scene_material_nodes=std::max(maximum_scene_material_nodes,scene.material_nodes);
            scene_new_material_bytes+=scene.new_material_value_bytes;scene_shared_material_bytes+=scene.shared_material_value_bytes;
            scene_generated_image_bytes=std::max(scene_generated_image_bytes,scene.generated_material_image_bytes);
        }else{
            // Continuity can bind after an already-decoded admission body.
            // That unbound previous endpoint must not acquire proof retroactively.
            check(retained_receipts==0&&effect.history.captured==0,
                "Runtime dropped a complete receipt after frontier history became available");
            ++awaiting_previous;
        }
        captured=effect.history.captured;maximum_retained=std::max(maximum_retained,effect.history.retained);
        check(effect.runtime_frontier_bound==effect.bound.has_value(),"Native cut bound only effects without its runtime frontier");
        check(effect.scene_frontier_bound==effect.bound.has_value(),"Native cut bound a receipt without its exact owning scene/material snapshot");
        check(effect.hud_frontier_bound==effect.bound.has_value(),"Native cut bound a receipt without its complete CPU HUD/scoreboard snapshot");
        check(effect.locomotion_frontier_bound==effect.bound.has_value(),"Native cut bound a receipt without its full16 native locomotion snapshot");
        if(effect.bound){check(effect.bound->peer==client_session->self()&&effect.bound->context==origin.context(),
            "Runtime effect association bypassed native recipient/context validation");
            bound_cuts.insert(effect.bound->seal.id);last_bound=effect.bound;}
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame){
        SceneCaptureCosts scene_frame{};
        const bool scene_valid=scene_delta(scene_capture_costs,scene_at_last_frame,scene_frame);
        if(!scene_valid)++scene_delta_failures;
        scene_at_last_frame=scene_capture_costs;
        paired_input_gaps.frame(frame,input_deliveries,scene_frame,scene_valid);
        maximum_frame_input_ms=std::max(maximum_frame_input_ms,frame.input_ms);
        maximum_frame_wall_ms=std::max(maximum_frame_wall_ms,frame.wall_ms);
        const auto &cost=frame.receipt_capture;
        capture_attempts+=cost.attempts;capture_completed+=cost.completed;capture_retained+=cost.retained;
        const std::array<const MultiplayerFrameTiming::Work *,9> work{&cost.values,&cost.replica,&cost.pose,
            &cost.scene,&cost.hud,&cost.locomotion,&cost.sounds,&cost.retention,&cost.total};
        for(unsigned i=0;i<work.size();++i)capture_stages[i].add(*work[i]);
        require_unadvertised_migration(client_session);
        const auto observed=std::chrono::steady_clock::now();
        if(!first_frame_at)first_frame_at=observation_time(observed);
        if(!(observed<deadline)){
            deadline_branch_at=observation_time(observed);
            std::ostringstream error;
            error<<"Rendered ONLINE effect history deadline: accepted="<<accepted<<" captured="<<captured
                <<" bound_cuts="<<bound_cuts.size()<<" retained_peak="<<maximum_retained
                <<" awaiting_previous="<<awaiting_previous<<" submitted_frames="<<draws
                <<" capture_attempts="<<capture_attempts<<" capture_completed="<<capture_completed
                <<" capture_retained="<<capture_retained<<" deadline_branch_ns="<<deadline_branch_at;
            throw FormatError(error.str());
        }
        if(frame.submitted)++draws;
        if(frame.submitted&&bound_cuts.size()>=2&&captured>=256&&!quit){SDL_Event event{};event.type=SDL_QUIT;
            const auto requested_at=observation_time();
            check(SDL_PushEvent(&event)==1,"Cannot finish private rendered ONLINE fixture");
            completion_requested_at=requested_at;quit=true;}
    };
    MultiplayerRuntimeOutcome outcome;
    std::optional<int> result;std::exception_ptr runtime_error;
    const auto runtime_invoked_at=observation_time();
    try{
        MultiplayerSceneContinuity::CaptureCostScope scene_cost_scope(scene_capture_costs);
        result=run_multiplayer(options,outcome);
    }catch(...){
        runtime_caught_at=observation_time();runtime_error=std::current_exception();}
    const auto runtime_returned_at=observation_time();
    std::exception_ptr host_error;try{host.finish();}catch(...){host_error=std::current_exception();}
    const auto host_loaded=host_session->loaded_fingerprint(),client_loaded=client_session->loaded_fingerprint();
    std::cout<<"OBSERVED native loading identities: host="<<(host_loaded?std::to_string(*host_loaded):"unobserved")
        <<" client="<<(client_loaded?std::to_string(*client_loaded):"unobserved")
        <<" both_observed="<<bool(host_loaded&&client_loaded)
        <<" equal="<<bool(host_loaded&&client_loaded&&*host_loaded==*client_loaded)<<'\n';
    paired_input_gaps.report();
    std::cout<<"OBSERVED startup candidate pending windows="<<startup_pending_candidate_observations
        <<" scope=before_first_full_history_callback no_fabricated_arm_or_receipt=1\n";
    SceneCaptureCosts unreported_scene{};
    const bool unreported_scene_valid=scene_delta(scene_capture_costs,scene_at_last_frame,unreported_scene);
    std::cout<<"OBSERVED scene-leaf-summary scope=caller_thread_all_scoped_captures frame_delta_failures="
        <<scene_delta_failures<<" default_path_unscoped=1 cpu_sample_coverage_explicit=1\n";
    scene_payload(0,"all_scoped",scene_capture_costs,true);
    scene_payload(0,"delivered_frame_windows",scene_at_last_frame,!scene_delta_failures);
    scene_payload(0,"unreported_after_last_frame",unreported_scene,unreported_scene_valid);
    // Always retain the actual progress boundary before the acceptance check;
    // an early exit must distinguish history, native validation and the room.
    std::cout<<"OBSERVED rendered receipt progress: result="<<(result?std::to_string(*result):"not_returned")
        <<" exit_reason="<<(runtime_error?"unretained":std::to_string(unsigned(outcome.reason)))
        <<" quit_requested="<<quit<<" accepted="<<accepted<<" captured="<<captured<<" retained_peak="<<maximum_retained
        <<" awaiting_previous="<<awaiting_previous<<" scene_receipts="<<scene_receipts<<" bound_cuts="<<bound_cuts.size()
        <<" hud_receipts="<<hud_receipts
        <<" locomotion_receipts="<<locomotion_receipts<<" maximum_occupied_gaits="<<maximum_occupied_gaits
        <<" observed_unavailable_gaits="<<observed_unavailable_gaits
        <<" rendered_frames="<<draws<<" host_tick="<<host.source_tick<<" host_offered_cuts="<<host.offered_cuts
        <<" host_native_all_cuts="<<host.native_validated_cuts<<" host_committed_cuts="<<host.committed_cuts
        <<" host_released_cuts="<<host.released_cuts<<" host_aborted_cuts="<<host.aborted_cuts
        <<" host_recording_drops="<<host.recording_drops<<" host_broadcast_calls="<<host.broadcast_calls
        <<" client_phase="<<unsigned(client_session->phase())<<'\n';
    if(hud_census)std::cout<<"OBSERVED complete CPU HUD census: primary="<<hud_census->huds[0]
        <<" offhand="<<hud_census->huds[1]<<" vehicle="<<hud_census->huds[2]<<" unit="<<hud_census->huds[3]
        <<" parent="<<hud_census->huds[4]<<" scoreboard="<<hud_census->scoreboard<<'\n';
    const auto admitted_at=host.admitted_at.load(),drop_at=host.first_drop_at.load();
    std::cout<<"OBSERVED actual input liveness: deliveries="<<input_deliveries
        <<" admitted="<<bool(admitted_at)<<" first_input="<<bool(first_input_at)
        <<" admission_to_first_input_ms="<<(admitted_at&&first_input_at?double(first_input_at-admitted_at)/1e6:-1.)
        <<" maximum_input_gap_ms="<<double(maximum_input_gap)/1e6
        <<" first_peer_drop="<<bool(drop_at)<<" first_drop_tick="<<host.first_drop_tick
        <<" admission_to_first_drop_ms="<<(admitted_at&&drop_at?double(drop_at-admitted_at)/1e6:-1.)
        <<" admission_to_completion_request_ms="<<(admitted_at&&completion_requested_at?double(completion_requested_at-admitted_at)/1e6:-1.)
        <<" peer_dropped_before_completion="<<bool(drop_at&&(!completion_requested_at||drop_at<completion_requested_at))
        <<" completion_to_runtime_return_ms="<<(completion_requested_at?double(runtime_returned_at-completion_requested_at)/1e6:-1.)
        <<" maximum_frame_input_ms="<<maximum_frame_input_ms<<" maximum_frame_wall_ms="<<maximum_frame_wall_ms
        <<" runtime_error="<<bool(runtime_error)<<" host_error="<<bool(host_error)
        <<" observation=local_command_submission_not_host_acceptance\n";
    const auto relative_ms=[](std::int64_t at,std::int64_t base){return at&&base?double(at-base)/1e6:-1.;};
    const auto relation=[](std::int64_t at,std::int64_t boundary){
        return !at||!boundary?"unobserved":at<boundary?"before":at==boundary?"equal":"after";};
    std::cout<<"OBSERVED history-cost anchors deadline_start_ns="<<observation_time(deadline_started)
        <<" runtime_invoked_ns="<<runtime_invoked_at<<" first_completed_frame_ns="<<first_frame_at
        <<" first_full_capture_ns="<<first_full_capture_at<<" deadline_branch_ns="<<deadline_branch_at
        <<" runtime_catch_ns="<<runtime_caught_at<<" runtime_returned_ns="<<runtime_returned_at
        <<" invocation_to_first_frame_ms="<<relative_ms(first_frame_at,runtime_invoked_at)
        <<" invocation_to_first_capture_ms="<<relative_ms(first_full_capture_at,runtime_invoked_at)
        <<" invocation_to_deadline_branch_ms="<<relative_ms(deadline_branch_at,runtime_invoked_at)
        <<" invocation_to_catch_ms="<<relative_ms(runtime_caught_at,runtime_invoked_at)
        <<" invocation_to_return_ms="<<relative_ms(runtime_returned_at,runtime_invoked_at)<<'\n';
    if(const auto &removed=host.first_removal){
        const auto input_at=removed->last_accepted_input_at_ns.value_or(0);
        std::cout<<"OBSERVED history-cost first_host_removal reason="
            <<(removed->reason==LanHost::PeerRemovalReason::authenticated_leave?"authenticated_leave":"idle_timeout")
            <<" ordinal="<<removed->ordinal<<" source_tick="<<removed->source_tick
            <<" slot_mask="<<removed->slot_mask<<" views="<<removed->local_players
            <<" input_seen="<<removed->input_seen<<" input_sequence="<<removed->input_sequence
            <<" input_acceptances_current_owner="<<removed->input_acceptances
            <<" last_accepted_input_ns="<<(removed->last_accepted_input_at_ns?std::to_string(input_at):"unknown")
            <<" last_accepted_input_age_ns="<<(removed->last_accepted_input_age_ns?
                std::to_string(*removed->last_accepted_input_age_ns):"unknown")
            <<" observed_ns="<<removed->observed_at_ns<<" completed_ns="<<removed->removed_at_ns
            <<" idle_age_ns="<<removed->idle_age_ns
            <<" admission_to_removed_ms="<<relative_ms(removed->removed_at_ns,admitted_at)
            <<" removal_vs_deadline="<<relation(removed->removed_at_ns,observation_time(deadline))
            <<" removal_vs_deadline_branch="<<relation(removed->removed_at_ns,deadline_branch_at)
            <<" removal_vs_catch="<<relation(removed->removed_at_ns,runtime_caught_at)
            <<" removal_vs_runtime_return="<<relation(removed->removed_at_ns,runtime_returned_at)<<'\n';
    }else std::cout<<"OBSERVED history-cost first_host_removal reason=unobserved\n";
    std::cout<<"OBSERVED history-cost capture_totals attempts="<<capture_attempts<<" completed="<<capture_completed
        <<" retained="<<capture_retained
        <<" scope=delivered_frame_observers cpu_scope=current_thread_linux_or_unavailable\n";
    constexpr std::array<const char *,9> stage_names{"values","replica","pose","scene","hud","locomotion","sounds","retention","total"};
    for(unsigned i=0;i<capture_stages.size();++i){const auto &stage=capture_stages[i];
        std::cout<<"OBSERVED history-cost capture_stage name="<<stage_names[i]<<" wall_ms="<<stage.wall_ms
            <<" thread_cpu_ms="<<stage.thread_cpu_ms<<" samples="<<stage.samples
            <<" thread_cpu_samples="<<stage.thread_cpu_samples<<" maximum_frame_wall_ms="<<stage.maximum_frame_wall_ms<<'\n';}
    if(outcome.state)
        std::cout<<"OBSERVED client retained outcome: tick="<<outcome.state->tick
            <<" game_over="<<outcome.state->game_over_complete()<<" results_finalized="<<outcome.state->results_finalized<<'\n';
    if(const auto common=client_session->common_cut())
        std::cout<<"OBSERVED client common-cut progress: pending="<<bool(common->pending)
            <<" pending_tick="<<(common->pending?std::to_string(common->pending->seal.tick):"none")
            <<" committed="<<bool(common->committed)<<" committed_tick="<<(common->committed?std::to_string(common->committed->seal.tick):"none")
            <<" native_all="<<common->native_all_validated<<" associated="<<common->associated
            <<" applied="<<common->applied<<" released="<<common->released<<" commit="<<common->commit
            <<" release="<<common->release<<" rejected="<<common->rejected<<'\n';
    if(runtime_error)std::rethrow_exception(runtime_error);
    if(host_error)std::rethrow_exception(host_error);
    check(result==0&&quit&&draws>0&&last_bound&&bound_cuts.size()>=2&&host.native_validated_cuts>=2,
        "Rendered native ONLINE history never associated two natively validated paired cuts");
    // Native world/renderer teardown can outlive a normal client Leave. Only
    // an observed peer loss before our real completion request is a live-room
    // failure; timestamps never drive input, session timeouts or source ticks.
    check(completion_requested_at&&(!drop_at||drop_at>=completion_requested_at),
        "Rendered ONLINE client lost its physical peer before completing the actual cut/history checks");
    check(accepted==captured+awaiting_previous&&captured==retained_receipts&&captured>=256&&maximum_retained==128,
        "Runtime omitted complete receipt frontiers or never exercised two full history turnovers");
    check(resource_observations==accepted&&source_resource_counts.has_value(),
        "Passive migration-candidate resources were not verified for every actual accepted receipt");
    check(scene_receipts==retained_receipts&&scene_shared_material_bytes>0,
        "Windowed receipt history omitted scene owners or never shared unchanged actual material values");
    check(hud_receipts==retained_receipts&&hud_census.has_value(),
        "Windowed receipt history omitted complete CPU HUD/scoreboard registries");
    check(locomotion_receipts==retained_receipts,
        "Windowed receipt history omitted its full16 native locomotion owner");
    std::cout<<"PASS real local direct-DTLS ONLINE four-view runtime: accepted="<<accepted<<" captured="<<captured
        <<" awaiting_proven_previous="<<awaiting_previous<<" retained_peak="<<maximum_retained
        <<" bound_native_cuts="<<bound_cuts.size()<<" rendered_frames="<<draws<<'\n';
    std::cout<<"PASS actual windowed receipt/runtime/effects history: authentic state/slots and deferred queues retained in one entry; no migration activation\n";
    std::cout<<"PASS actual proof-bound CPU HUD registry history: captures="<<hud_receipts
        <<" primary="<<hud_census->huds[0]<<" offhand="<<hud_census->huds[1]<<" vehicle="<<hud_census->huds[2]
        <<" unit="<<hud_census->huds[3]<<" parent="<<hud_census->huds[4]<<" scoreboard="<<hud_census->scoreboard
        <<"; exact then-current preceding-render values and bindings; no animation advance, GPU or migration acceptance\n";
    std::cout<<"PASS production common-cut host driver: actual same-tick snapshot broadcasts and fixed physical membership; native_all_cuts="
        <<host.native_validated_cuts<<" committed_cuts="<<host.committed_cuts<<" released_cuts="<<host.released_cuts
        <<"; recovery-pin ownership only, no full Ready/election/role activation\n";
    std::cout<<"PASS actual proof-bound full16 locomotion receipt history: captures="<<locomotion_receipts
        <<" maximum_occupied="<<maximum_occupied_gaits<<" observed_current_unavailable_mask="<<observed_unavailable_gaits
        <<"; exact occupied/empty/dormant slots and metadata retained; no physical input/audio or migration activation\n";
    if(!maximum_occupied_gaits)std::cout<<"LIMIT actual runtime locomotion history had no occupied native gait; empty/unavailable census only\n";
    std::cout<<"PASS actual proof-bound scene receipt history: captures="<<scene_receipts<<" peak_material_nodes="<<maximum_scene_material_nodes
        <<" peak_material_logical_bytes="<<maximum_scene_material_bytes<<" peak_material_unique_bytes="<<maximum_scene_unique_material_bytes
        <<" cumulative_new_material_value_bytes="<<scene_new_material_bytes
        <<" cumulative_shared_material_value_bytes="<<scene_shared_material_bytes
        <<" peak_generated_image_bytes="<<scene_generated_image_bytes
        <<"; all six scene adapters, actual callback/portal/light state and complete resident material graph retained at the CPU receipt boundary; preceding-render caches remain their exact captured values; no migration activation\n";
    const auto &resources=*source_resource_counts;
    std::cout<<"PASS passive migration-candidate resources: observations="<<resource_observations
        <<" particle_materials="<<resources[0]<<" first_person_systems="<<resources[1]<<" world_systems="<<resources[2]
        <<" model_light_sections="<<resources[3]<<" materials_at_upload="<<resources[4]<<" materials_now="<<resources[5]
        <<"; counts stable across all accepted receipts; native particle/model producers remained dormant; no migration activation\n";
    std::cout<<"PASS complete authored weapon particle census matches ONLINE client: particle_materials="<<expected_particles.materials
        <<" first_person_systems="<<expected_particles.first_person_systems<<" world_systems="<<expected_particles.world_systems<<'\n';
    if(selected_mcc)std::cout<<"PASS selected-MCC-native ONLINE receipt route: actual MCC cache, compiled native gameplay, empty external XBE; unchanged capture/history/proof/census gates; payload provenance and release acceptance remain separate\n";
#if defined(__linux__)
    rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"Cannot measure rendered native fixture process peak RSS");
    std::cout<<"OBSERVED whole fixture process peak_rss_kib="<<usage.ru_maxrss<<"; includes native worlds/renderer and is not isolated history memory\n";
#endif
    std::cout<<"LIMIT fixture identity,30Hz native authority with actual per-step snapshots, count-bound CPU history and two local physical session owners; no default migration activation, byte-budget/catch-up qualification, GPU/audio handoff, physical controllers/headsets or WAN acceptance\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
