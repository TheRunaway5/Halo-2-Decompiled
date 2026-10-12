#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
struct Host {
    Cache cache;LevelContent content;MultiplayerMatch match;LanHost host;
    std::uint32_t control=null_datum,gate=null_datum;
    std::atomic<bool> activate{},advance{};
    std::exception_ptr error;std::mutex mutex;std::jthread worker;
    static SlayerRules rules(const Cache &c){SlayerRules r;r.weapon=c.slayer_starting_weapon(5);r.time_ticks=r.score_limit=0;return r;}
    Host(const MultiplayerOptions &o,bool late):cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe)),
        content(cache.load_content(0,true)),match(content,0,cache.spawns(),rules(cache)),host(match,0,
        multiplayer_fingerprint(o.map,cache.shared_directory(),0,cache.slayer_starting_weapon(5),o.gameplay_xbe)){
        match.join("Device audio authority");
        for(const auto &[handle,owner]:match.simulation().devices().owners()){
            const auto &d=*content.devices[owner.placement];
            if(d.control)control=handle;
            if(d.machine && d.placement.position_group==0)gate=handle;
        }
        check(control!=null_datum && gate!=null_datum,"Authored Zanzibar device owners missing");
        std::array<Controls,multiplayer_capacity> commands{};
        if(late){check(match.activate_admitted_control(control).changed,"Prepare actual moving gate");
            for(unsigned i=0;i<45;++i)match.step(commands);}
        worker=std::jthread([this](std::stop_token stop){try{
            std::array<Controls,multiplayer_capacity> commands{};auto next=std::chrono::steady_clock::now();
            while(!stop.stop_requested()){
                host.receive(commands);
                // This probe supplies already-admitted host actions. Local
                // SDL selection/hold admission is covered by the paired test.
                if(activate.exchange(false))check(match.activate_admitted_control(control).changed,"Host control admission failed");
                if(advance)match.step(commands);
                host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
            }
        }catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
    }
    void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv){try{
    check(argc==4,"Supply ZANZIBAR_MAP SHARED XBE");
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
    SDL_SetMainReady();MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];
    o.starting_weapon=5;o.width=960;o.height=540;
    if(const auto *views=std::getenv("HALO2_TEST_CONTROL_VIEWS"))o.local_players=unsigned(std::stoul(views));
    check(o.local_players==1 || o.local_players==4,"Supply one or four actual LAN views");
    if(const auto *artwork=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))o.ui_assets=artwork;
    o.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
    const bool late=std::getenv("HALO2_TEST_DEVICE_LATE");Host host(o,late);
    o.join="127.0.0.1:"+std::to_string(host.host.port());
    o.desktop=std::make_shared<DesktopSession>();o.desktop->window(o.width,o.height,false);
    using Identity=std::pair<std::uint32_t,std::uint32_t>;
    std::map<Identity,std::set<std::uint64_t>> voices;
    const auto identity=[&](std::uint32_t owner,unsigned index){
        return Identity{owner,host.content.devices[host.match.simulation().devices().find(owner)->placement]->motion.effects[index]};
    };
    const auto button=identity(host.control,0),opening=identity(host.gate,0),opened=identity(host.gate,2);
    voices[button];voices[opening];voices[opened];bool joined{},settled{};unsigned settled_at{};
    o.input_observer=[&](unsigned tick,std::span<const Controls>,const MultiplayerState &s){
        if(!joined && std::count_if(s.players.begin(),s.players.end(),[](const auto&p){return p.connected;})>1){
            joined=true;host.activate=!late;host.advance=true;
        }
        for(const auto &owner:s.devices.owners)if(owner.state.handle==host.gate && owner.state.motion.position==1){
            if(!settled)settled_at=tick;settled=true;
        }
        if(settled && tick>settled_at+15){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Quit LAN device audio probe");}
        check(tick<500,"LAN device audio probe timed out");
    };
    o.audio_observer=[&](const MultiplayerAudioSubmission &s){
        const auto found=voices.find({s.owner,s.sound});if(found==voices.end())return;
        check(s.kind==MultiplayerAudioKind::Impulse && s.frames && s.voice,"Device request lost actual SDL PCM/owner");
        if(found->second.insert(s.voice).second)
            std::cout<<"LAN authored device PCM owner="<<s.owner<<" tag="<<s.sound<<" voice="<<s.voice<<" frames="<<s.frames<<'\n';
    };
    const auto result=run_multiplayer(o);host.finish();
    check(result==0 && joined && settled,"Native LAN device sequence incomplete");
    check(voices[opened].size()==1,"LAN gate endpoint PCM absent or duplicated");
    check(voices[button].size()==(late?0u:1u) && voices[opening].size()==(late?0u:1u),
          "LAN button/opening PCM absent, duplicated or replayed on late join");
    std::cout<<"PASS native LAN device audio views="<<o.local_players<<" moving_late_join="<<late
             <<" button="<<voices[button].size()<<" opening="<<voices[opening].size()<<" opened="<<voices[opened].size()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
