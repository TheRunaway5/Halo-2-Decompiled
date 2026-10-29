#define SDL_MAIN_HANDLED
#include "player_shadows_runtime_capture.hpp"
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <syncstream>
#include <thread>

using namespace halo2;
namespace {
using Clock=std::chrono::steady_clock;
nlohmann::json report;
void need(bool value,const char *why){if(!value)throw FormatError(why);}
void save(const std::filesystem::path &path) {
    const auto bytes=report.dump(2)+"\n";
    need(bytes.size()<=1024*1024,"Joined shadow receipt exceeds its storage bound");
    std::ofstream out(path);out<<bytes;need(bool(out),"Could not save joined shadow receipt");
}
// All native construction, advancement, capture and destruction stay on this
// executor. The client receives ordinary authenticated UDP snapshots; neither
// its renderer nor its native presentation owner borrows the live host.
class Host {
    std::promise<std::string> address_;
    std::mutex mutex_;
    std::exception_ptr error_;
public:
    std::atomic<bool> advance{};
    std::atomic<unsigned> tick{};
    std::string address;
private:
    // Stop/join before every field captured by this executor, including on
    // constructor readiness timeout where the class destructor does not run.
    std::jthread worker_;
public:
    explicit Host(MultiplayerOptions options) {
        auto ready=address_.get_future();
        worker_=std::jthread([this,options=std::move(options)](std::stop_token stop) {
            bool announced=false;
            try {
                auto world=std::make_shared<MultiplayerPreparedWorld>(options);
                need(world->source_objects&&world->source_objects->owns_physics(),
                    "Joined shadow host lacks the actual native gameplay world");
                std::array<Controls,multiplayer_capacity> controls{};
                for(unsigned n=0;n<30;++n)world->match.step(controls);
                const auto fingerprint=multiplayer_fingerprint(options.map,options.shared_maps,
                    options.bsp,world->network_starting_weapon_identity());
                auto host=std::make_unique<LanHost>(world->match,0,fingerprint,options.variant);
                const auto endpoint=host->join_address();
                MultiplayerRuntimeAuthority authority(options,world,std::move(host),{});
                address_.set_value(endpoint);announced=true;
                auto next=Clock::now(),diagnostic_at=next+std::chrono::seconds(5);
                while(!stop.stop_requested()) {
                    authority.service();
                    auto frame=authority.begin_frame();
                    need(bool(frame),"Actual raw-LAN host unexpectedly fenced its native frame");
                    authority.host()->receive(controls);
                    if(advance&&world->match.simulation().ticks()<180)world->match.step(controls);
                    tick=world->match.simulation().ticks();
                    authority.broadcast_candidate_frame(frame);
                    frame.reset();
                    const auto stats=authority.host()->shadow_transport_statistics();
                    if(!stats.error.empty())
                        throw FormatError("Actual native presentation producer rejected encoding: "+stats.error);
                    if(Clock::now()>=diagnostic_at) {
                        std::osyncstream(std::cerr)<<"Joined shadow producer progress: tick="<<tick.load()
                            <<" peers="<<authority.host()->peers()<<" captures="<<stats.captures
                            <<" encoded="<<stats.encoded<<" published="<<stats.published
                            <<" acknowledged="<<stats.acknowledged<<" rejected="<<stats.rejected
                            <<" encoding="<<stats.encoding<<" awaiting_native="<<stats.awaiting_native
                            <<" error="<<stats.error<<'\n';
                        diagnostic_at=Clock::now()+std::chrono::seconds(5);
                    }
                    next+=std::chrono::microseconds(33333);
                    std::this_thread::sleep_until(next);
                    if(next<Clock::now()-std::chrono::milliseconds(100))next=Clock::now();
                }
            }catch(...) {
                const auto error=std::current_exception();
                {std::lock_guard lock(mutex_);error_=error;}
                try{std::rethrow_exception(error);}
                catch(const std::exception& failure){std::osyncstream(std::cerr)<<"Joined shadow producer failed: "<<failure.what()<<'\n';}
                catch(...){std::osyncstream(std::cerr)<<"Joined shadow producer failed with a nonstandard exception\n";}
                if(!announced)address_.set_exception(error);
            }
        });
        need(ready.wait_for(std::chrono::seconds(45))==std::future_status::ready,
            "Native shadow host construction timed out");
        address=ready.get();
    }
    ~Host(){worker_.request_stop();if(worker_.joinable())worker_.join();}
    void check_error(){std::lock_guard lock(mutex_);if(error_)std::rethrow_exception(error_);}
    void finish(){worker_.request_stop();worker_.join();check_error();}
};
}

int main(int argc,char **argv) {
    std::filesystem::path output;
    try {
        need(argc==5,"Supply prepared MCC map, shared maps, graphics mode and report");
        output=argv[4];
        const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
        need(scratch&&*scratch,"Joined shadow check requires its owned test scratch");
        const std::string mode=argv[3];
        need(mode=="Original"||mode=="pStencil"||mode=="PathTracing","Invalid joined shadow graphics mode");
        report={{"passed",false},{"mode",mode},{"scope","Actual UDP joining player, native host and submitted GPU shadow frame"},
            {"retained_large_artifacts",nlohmann::json::array()}};
        SDL_SetMainReady();
        MultiplayerOptions options;
        options.map=argv[1];options.shared_maps=argv[2];options.width=640;options.height=480;
        options.local_players=1;options.hidden=true;options.mute=true;options.validation=true;
        options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};options.starting_weapon=5;
        options.session_random_seed=0x12345678u;
        options.source_object_savegame_directory=std::filesystem::path(scratch)/"host-world";
        Host host(options);
        options.source_object_savegame_directory.clear();options.join=host.address;
        options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
        need(!options.prepared_world->source_objects,"Joining shadow test silently created gameplay authority");
        shadow_test::Capture capture;
        capture.mode=mode;capture.picture=std::filesystem::path(scratch)/"joined-player-shadow.ppm";
        capture.advance=[&]{host.check_error();host.advance=true;};capture.finish=[&]{host.finish();};
        report=shadow_test::capture(options,capture);
        report["scope"]="Actual UDP joining player, native host and submitted GPU shadow frame";
        report["host_tick"]=host.tick.load();
        report["passed"]=true;save(output);
        std::cout<<"PASS actual joined player shadow mode="<<mode<<" tick="<<report["tick"]<<" local="<<report["local_unit"]<<'\n';
        return 0;
    }catch(const std::exception &error) {
        report["error"]=error.what();report["passed"]=false;
        if(!output.empty())try{save(output);}catch(...){}
        std::cerr<<error.what()<<'\n';return 1;
    }
}
