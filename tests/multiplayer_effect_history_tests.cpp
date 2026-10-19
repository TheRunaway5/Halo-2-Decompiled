#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/effect_presentation_history.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/desktop_session.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <set>
#include <thread>
#if defined(__linux__)
#include <sys/resource.h>
#endif

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void wait(F ready,const char *why){const auto until=std::chrono::steady_clock::now()+120s;
    while(!ready()){check(std::chrono::steady_clock::now()<until,why);std::this_thread::sleep_for(5ms);}}
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();
    std::array<std::uint8_t,9> colors;std::copy_n(defaults.begin()+0x40,colors.size(),colors.begin());
    return native_selected_player_configuration(name,colors,-1);}
// A real native authority on its own executor, with four host couch seats.
// Simulation runs at30Hz; this fixture broadcasts at10Hz. No master gameplay
// endpoint, fabricated native cut, clock or accepted snapshot is supplied.
class Host {
    std::jthread worker_;
    std::exception_ptr error_;
public:
    std::atomic<unsigned> validated{};
    explicit Host(MultiplayerOptions options,std::uint64_t fingerprint) {
        std::promise<void> ready;auto future=ready.get_future();
        worker_=std::jthread([this,options=std::move(options),fingerprint,ready=std::move(ready)](std::stop_token stop)mutable {
            bool announced{};try {
                const auto session=options.online_match;const auto &roster=session->roster();
                MultiplayerPreparedWorld world(options);
                const auto locals=session->local_slots();
                for(unsigned view=0;view<locals.size();++view){const auto slot=locals[view];
                    check(world.match.join(roster[slot].name,-1,options.selected_profiles[view],slot)==slot,"Host source profile changed its admitted slot");}
                world.bind_source_local_views(locals);world.match.debug_settings({});
                // Match the normal runtime: starting_weapon is a later setter
                // write, so advertise the resolved world's exact ordered list.
                // The original fixture's pre-override list encoded differently
                // even though its final304 native bytes were equal.
                const auto &variant=world.match.session_configuration();check(variant.has_value(),"Native host lost its resolved configuration");
                LanHost host(world.match,session,fingerprint,roster,*variant);
                SourceMatchAuthority authority(world.match,*world.source_objects);
                session->loaded(fingerprint,*variant);ready.set_value();announced=true;
                while(!stop.stop_requested()&&session->phase()!=OnlineMatchSession::Phase::Running){
                    check(session->phase()!=OnlineMatchSession::Phase::Failed,session->error().c_str());std::this_thread::sleep_for(5ms);}
                bool admitted{};unsigned frame{};std::unique_ptr<SourceAuthorityPublisher> publisher;
                const unsigned target=session->self()==0?4:0;
                std::array<Controls,multiplayer_capacity> controls{};
                auto next=std::chrono::steady_clock::now();
                while(!stop.stop_requested()) {
                    check(session->phase()==OnlineMatchSession::Phase::Running,session->error().c_str());
                    if(publisher){publisher->update();const auto state=publisher->state();check(state.error.empty(),state.error.c_str());
                        validated=state.validations;}
                    const auto phase=[&]{host.receive(controls);
                        for(const auto slot:locals){controls[slot]={};controls[slot].yaw=world.match.unit(slot).yaw;}
                        if(host.peers()==1)world.match.step(controls);if(frame%3==0)host.broadcast();};
                    if(publisher)check(publisher->execute(phase),"Native publisher refused its complete fixture frame");else phase();
                    if(!admitted&&host.peers()==1){std::vector<unsigned> slots;for(const auto &row:roster)slots.push_back(row.slot);
                        const auto context=session->authority_context();check(context.has_value(),"Host has no independent authority context");
                        admitted=session->publish_source_players(*context,authority.player_admissions(slots));}
                    if(admitted&&!publisher&&session->source_players_confirmed(target)&&session->authority_host_continuation_supported(target)){
                        publisher=std::make_unique<SourceAuthorityPublisher>(authority,host,session,target,90s);
                        check(publisher->capture(),"Native host could not capture its paired baseline");}
                    ++frame;next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
                }
            }catch(...){if(!announced)ready.set_exception(std::current_exception());else error_=std::current_exception();}
        });future.get();
    }
    ~Host(){worker_.request_stop();if(worker_.joinable())worker_.join();}
    void finish(){worker_.request_stop();worker_.join();if(error_)std::rethrow_exception(error_);}
};
}
int main(int argc,char **argv)try {
    check(argc==5,"Supply ORIGIN DESCRIPTORS TU15_XBE MAP");
    if(!std::getenv("DISPLAY")||!std::filesystem::is_regular_file(argv[3])||!std::filesystem::is_regular_file(argv[4]))return 77;
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
        sessions[i]=std::make_shared<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),d.at("slot"),roster,true,false,false,transport);}
    wait([&]{for(const auto &s:sessions){check(s->phase()!=OnlineMatchSession::Phase::Failed,s->error().c_str());
        if(s->phase()!=OnlineMatchSession::Phase::Loading)return false;}return true;},"Direct measured host loading timeout");
    check(sessions[0]->host()==sessions[1]->host(),"Measured physical host differs");
    const auto host_session=sessions[sessions[0]->host()==sessions[0]->self()?0:1];
    const auto client_session=sessions[host_session==sessions[0]?1:0];
    MultiplayerOptions options;options.map=argv[4];options.shared_maps=options.map.parent_path();options.gameplay_xbe=argv[3];
    options.local_players=4;options.starting_weapon=5;options.session_random_seed=0x713719;options.source_object_world=true;
    options.variant={"Slayer",0,{{0x59,5},{10,0},{2,0}}};options.width=960;options.height=540;options.mute=true;
    options.validation=std::getenv("HALO2_REPLICA_VALIDATION");
    Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(options.map,options.shared_maps,0,
        cache.slayer_starting_weapon(5),options.gameplay_xbe),options.variant);
    auto server=options;server.online_match=host_session;
    for(auto slot:host_session->local_slots())server.selected_profiles.push_back(profile(roster[slot].name));
    Host host(server,fingerprint);
    options.online_match=client_session;for(auto slot:client_session->local_slots())options.selected_profiles.push_back(profile(roster[slot].name));
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    options.desktop=std::make_shared<DesktopSession>();options.desktop->window(options.width,options.height,false);
    unsigned accepted{},captured{},draws{},awaiting_previous{},retained_receipts{};std::size_t maximum_retained{};
    std::set<std::uint64_t> bound_cuts;std::optional<ValidatedAuthorityCut> last_bound;bool quit{};
    const auto deadline=std::chrono::steady_clock::now()+180s;
    options.effect_history_observer=[&](const MultiplayerSnapshotOrigin &origin,const EffectPresentationObservation &effect){
        ++accepted;check(origin.context()==client_session->authority_context(),"Runtime receipt has a different independent authority context");
        check(effect.world_tick==origin.tick()&&effect.muzzle_tick==origin.tick(),"Runtime replaced an incomplete CPU effect tick");
        check(effect.history.context==origin.context()&&effect.history.retained<=effect.history.capacity&&effect.history.capacity==128,
            "Runtime history exceeded or bypassed its gated retention policy");
        if(effect.runtime_frontier_retained){
            ++retained_receipts;
            check(effect.history.newest_sequence==origin.sequence()&&effect.history.captured==retained_receipts,
                "Runtime receipt/frontier/effects were not captured atomically");
        }else{
            // Continuity can bind after an already-decoded admission body.
            // That unbound previous endpoint must not acquire proof retroactively.
            check(retained_receipts==0&&effect.history.captured==0,
                "Runtime dropped a complete receipt after frontier history became available");
            ++awaiting_previous;
        }
        captured=effect.history.captured;maximum_retained=std::max(maximum_retained,effect.history.retained);
        check(effect.runtime_frontier_bound==effect.bound.has_value(),"Native cut bound only effects without its runtime frontier");
        if(effect.bound){check(effect.bound->peer==client_session->self()&&effect.bound->context==origin.context(),
            "Runtime effect association bypassed native recipient/context validation");
            bound_cuts.insert(effect.bound->seal.id);last_bound=effect.bound;}
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame){
        check(std::chrono::steady_clock::now()<deadline,"Rendered ONLINE effect history did not bind two actual native cuts");
        if(frame.submitted)++draws;
        if(frame.submitted&&bound_cuts.size()>=2&&captured>=256&&!quit){SDL_Event event{};event.type=SDL_QUIT;
            check(SDL_PushEvent(&event)==1,"Cannot finish private rendered ONLINE fixture");quit=true;}
    };
    const auto result=run_multiplayer(options);host.finish();
    check(result==0&&quit&&draws>0&&last_bound&&bound_cuts.size()>=2&&host.validated>=2,
        "Rendered native ONLINE history never associated two natively validated paired cuts");
    check(accepted==captured+awaiting_previous&&captured==retained_receipts&&captured>=256&&maximum_retained==128,
        "Runtime omitted complete receipt frontiers or never exercised two full history turnovers");
    std::cout<<"PASS real local direct-DTLS ONLINE four-view runtime: accepted="<<accepted<<" captured="<<captured
        <<" awaiting_proven_previous="<<awaiting_previous<<" retained_peak="<<maximum_retained
        <<" bound_native_cuts="<<bound_cuts.size()<<" rendered_frames="<<draws<<'\n';
    std::cout<<"PASS actual windowed receipt/runtime/effects history: authentic state/slots and deferred queues retained in one entry; no migration activation\n";
#if defined(__linux__)
    rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"Cannot measure rendered native fixture process peak RSS");
    std::cout<<"OBSERVED whole fixture process peak_rss_kib="<<usage.ru_maxrss<<"; includes native worlds/renderer and is not isolated history memory\n";
#endif
    std::cout<<"LIMIT fixture identity,30Hz native authority/10Hz snapshots, count-bound CPU history and two local physical session owners; no default migration activation, byte-budget/catch-up qualification, GPU/audio handoff, physical controllers/headsets or WAN acceptance\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
