#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/effect_presentation_history.hpp"
#include <SDL.h>
#include <atomic>
#include <bit>
#include <iostream>
#include <map>
#include <set>
#include <future>
#include <mutex>
#include <thread>

using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Host {
    std::string join_address;unsigned port{};
    std::atomic<bool> step{},join{},leave{};
    std::exception_ptr error;std::mutex mutex;std::jthread worker;
    explicit Host(const MultiplayerOptions &o) {
        MultiplayerOptions server;server.map=o.map;server.shared_maps=o.shared_maps;server.gameplay_xbe=o.gameplay_xbe;
        server.starting_weapon=o.starting_weapon;server.local_players=1;server.source_object_world=true;
        std::promise<unsigned> ready;auto future=ready.get_future();
        worker=std::jthread([this,server,ready=std::move(ready)](std::stop_token stop)mutable {bool published=false;try{
            MultiplayerPreparedWorld world(server);world.bind_source_local_views(world.local_slots);
            auto &match=world.match;const auto &cache=*world.cache;
            match.configure_replicated_first_person(cache.load_first_person_binding(world.rules.weapon,2));
            const auto fingerprint=multiplayer_fingerprint(server.map,cache.shared_directory(),0,world.network_starting_weapon_identity(),server.gameplay_xbe);
            LanHost host(match,0,fingerprint);join_address=host.join_address();ready.set_value(host.port());published=true;
            std::array<Controls,multiplayer_capacity> controls{};std::unique_ptr<LanClient> late;
            std::array<std::optional<unsigned>,multiplayer_capacity> initial_ammunition;
            auto next=std::chrono::steady_clock::now();
            while(!stop.stop_requested()) {
                if(join.exchange(false))late=std::make_unique<LanClient>(host.join_address(),fingerprint,1);
                if(late){Controls neutral{};late->send(std::span(&neutral,1));}
                if(leave.exchange(false))late.reset();
                host.receive(controls);
                if(step.exchange(false)) {
                    const auto state=match.state();
                    for(unsigned id=0;id<multiplayer_capacity;++id)if(state.players[id].connected) {
                        controls[id]={};
                        if(!initial_ammunition[id] && !state.players[id].dead)initial_ammunition[id]=state.players[id].loaded;
                        const bool fired=initial_ammunition[id] && state.players[id].loaded<*initial_ammunition[id];
                        controls[id].fire=!fired;controls[id].reload=fired;
                    }
                    match.step(controls);(void)match.take_sounds();
                }
                host.broadcast();if(late)late->receive();
                next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
            }
        }catch(...){if(!published)ready.set_exception(std::current_exception());std::lock_guard lock(mutex);error=std::current_exception();}});
        port=future.get();
    }
    void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply MAP SHARED_MAPS TU15_XBE");
    if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[3]))return 77;
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=4;options.starting_weapon=5;options.width=960;options.height=540;
    options.validation=std::getenv("HALO2_REPLICA_VALIDATION");
    Host host(options);options.join=host.join_address;
    options.desktop=std::make_shared<DesktopSession>();options.desktop->window(options.width,options.height,false);
    enum class Phase {Warm,Settle,Join,Leave,Done};Phase phase=Phase::Warm;
    std::uint32_t tick{},frozen{},last_requested=~0u;unsigned accepted{},refreshes{},stage_refreshes{},draws{};
    unsigned local_cues{},first_person_sounds{},clock_packets{},sound_baseline{},visible_refreshes{};
    unsigned completed_effect_receipts{};
    std::set<std::uint32_t> local_weapons;
    std::optional<MultiplayerSnapshotOrigin> prior_origin;
    std::map<std::pair<unsigned,std::uint32_t>,std::pair<std::uint32_t,std::uint32_t>> completed_gaits;
    bool joined_drawn{},left_drawn{},quit{};
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(180);
    options.audio_observer=[&](const MultiplayerAudioSubmission &s){
        if(s.kind==MultiplayerAudioKind::FirstPerson){++first_person_sounds;++local_cues;}
        else if(s.kind==MultiplayerAudioKind::Impulse && local_weapons.contains(s.owner))++local_cues;
    };
    options.replica_observer=[&](const MultiplayerState &state,const MultiplayerSnapshotOrigin &origin,bool advanced) {
        check(origin.tick()==state.tick && origin.sequence().has_value(),"Runtime lost its exact accepted receipt");
        if(prior_origin)check(!origin.same(*prior_origin),"Runtime processed an accepted receipt twice");
        prior_origin=origin;tick=state.tick;++accepted;
        local_weapons.clear();for(unsigned id=1;id<=4;++id) {
            if(state.players[id].source_weapon!=null_datum)local_weapons.insert(state.players[id].source_weapon);
            clock_packets+=state.players[id].first_person_clock.present;
        }
        if(!advanced){++refreshes;++stage_refreshes;}
        const auto count=std::count_if(state.players.begin(),state.players.end(),[](const auto &p){return p.connected && !(p.source_flags&2u);});
        check(count==5 || count==6,"Four couch seats/host roster changed unexpectedly");
        if(phase!=Phase::Warm) {
            check(state.tick==frozen && !advanced,"Same-tick refresh advanced the host/native presentation timeline");
            if(phase==Phase::Join && count==6){phase=Phase::Leave;stage_refreshes=0;}
            else if(phase==Phase::Leave && (state.players[5].source_flags&2u) && left_drawn && stage_refreshes>=12) {
                check(local_cues==sound_baseline,"Same-tick snapshots replayed retained local weapon audio");
                phase=Phase::Done;
            }
        }
    };
    options.effect_history_observer=[&](const MultiplayerSnapshotOrigin &origin,const EffectPresentationObservation &effect) {
        check(effect.world_tick==origin.tick()&&effect.muzzle_tick==origin.tick(),
            "Replica receipt replaced a tick before both CPU effect owners completed it");
        check(!origin.context()&&!effect.bound&&effect.history.retained==0&&effect.history.captured==0,
            "Raw LAN fixture manufactured independently admitted ONLINE effect history");
        ++completed_effect_receipts;
    };
    options.view_state_observer=[&](unsigned,const MultiplayerState &state,const Camera &){
        if(phase==Phase::Leave && state.players[5].connected && !(state.players[5].source_flags&2u))joined_drawn=true;
        if(phase==Phase::Leave && joined_drawn && state.players[5].connected && (state.players[5].source_flags&2u))left_drawn=true;
    };
    options.animation_observer=[&](unsigned,unsigned player,const AnimationRenderObservation &draw) {
        if(draw.first_person || !draw.completed_locomotion_frame || phase==Phase::Warm || phase==Phase::Settle || player==5)return;
        const auto key=std::pair{player,draw.unit},value=std::pair{draw.body_activity,std::bit_cast<std::uint32_t>(*draw.completed_locomotion_frame)};
        const auto [it,inserted]=completed_gaits.emplace(key,value);
        check(inserted || it->second==value,"Same-tick replica advanced an existing gait channel");++visible_refreshes;
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
        if(!frame.submitted)return;++draws;
        check(std::chrono::steady_clock::now()<deadline,"Rendered replica refresh scenario timed out");
        if(phase==Phase::Warm && accepted) {
            check(tick<180,"No actual local weapon cue established the repeated-clock control");
            if(local_cues){phase=Phase::Settle;frozen=tick;stage_refreshes=0;}
            else if(last_requested!=tick){last_requested=tick;host.step=true;}
        }else if(phase==Phase::Settle && stage_refreshes>=12) {
            sound_baseline=local_cues;phase=Phase::Join;stage_refreshes=0;host.join=true;
        }else if(phase==Phase::Leave && joined_drawn && stage_refreshes>=8)host.leave=true;
        else if(phase==Phase::Done && !quit) {
            SDL_Event event{};event.type=SDL_QUIT;check(SDL_PushEvent(&event)==1,"Cannot exit rendered refresh test");quit=true;
        }
    };
    const auto result=run_multiplayer(options);host.finish();
    check(result==0 && quit && phase==Phase::Done && joined_drawn && left_drawn,"Rendered same-tick roster lifetime incomplete");
    check(refreshes>=32 && draws>0 && visible_refreshes>4 && !completed_gaits.empty() && local_cues>0,
        "Rendered same-tick clock/audio/gait controls were not exercised");
    check(completed_effect_receipts==accepted,"Not every accepted replica completed the CPU effect barrier");
    std::cout<<"PASS four-view runtime same-tick join/departure tick="<<frozen<<" receipts="<<accepted<<" refreshes="<<refreshes
        <<" rendered_frames="<<draws<<" unchanged_gait_draws="<<visible_refreshes<<" real_local_weapon_cues="<<local_cues
        <<" fp_channel_audio="<<first_person_sounds<<" authentic_remote_fp_clocks="<<clock_packets<<'\n';
    std::cout<<"PASS completed CPU effect barriers="<<completed_effect_receipts<<" without fabricated raw-LAN ONLINE admission\n";
    std::cout<<"LIMIT localhost UDP actual TU1.5 native authority and virtual display; no host election, live DTLS/WAN or rendered migration/headset acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
