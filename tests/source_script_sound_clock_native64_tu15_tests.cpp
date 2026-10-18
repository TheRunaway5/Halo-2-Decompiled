#include "halo2/source_script_sound_clock_native64_tu15.hpp"
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace {
using namespace halo2;
static_assert(!std::is_default_constructible_v<SourceScriptSoundClockVoice64Tu15>);
static_assert(!std::is_constructible_v<SourceScriptSoundClockVoice64Tu15,std::uint64_t>);
static_assert(!std::is_default_constructible_v<SourceScriptSoundClockAdmission64Tu15>);
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void fails(F&&f,const char*message){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}require(threw,message);}
const SoundDriverFloatingPointContext fp{0x027f,0x1f80};
struct Providers {
    unsigned reads=0,selections=0,durations=0,stops=0;
    SourceScriptSoundClockTimeTu15 now{100,60,0x3c888889};
    SourceScriptSoundClockPolicyTu15 policy(){
        SourceScriptSoundClockPolicyTu15 p;
        p.time=[this]{++reads;return now;};
        p.selected_seconds=[this](unsigned,std::uint32_t,const auto&){++durations;return 0x3f800000u;};
        p.select=[this](std::uint32_t,auto&s){++selections;s={0,2};};
        return p;
    }
    auto stop(){return [this](const auto&voice){require(!voice,"Metadata fixture acquired a fake voice");++stops;};}
};
void admissions(){
    Providers p;auto policy=p.policy();SourceScriptSoundClockNative64Tu15 clock,foreign;
    require(!clock.begin(0xffffffff,{}, {},{}),"Outer NONE admission called providers");
    const auto a=clock.begin(42,policy,p.stop(),fp);require(bool(a),"First admission unavailable");
    require(a->sound()==42 && a->selection()==SourceScriptSoundClockSelectionTu15{0,2},"Admission did not capture immutable selector");
    const auto snapshot=clock.rows();
    require(!foreign.finish_failure(*a),"Foreign owner accepted admission");
    fails([&]{clock.begin(43,policy,p.stop(),fp);},"Pending admission allowed interleaving begin");
    fails([&]{clock.stop_first(42,p.stop());},"Pending admission allowed stop interleaving");
    require(clock.finish_failure(*a),"Failed native admission result rejected");
    require(clock.rows()==snapshot,"Failure erased committed row");
    require(!clock.finish_failure(*a),"Duplicate finish accepted");
    const auto b=clock.begin(42,policy,p.stop(),fp);require(b && p.stops==1,"Restart did not issue exact ordered stop intent");
    require(!clock.finish_failure(*a),"Replaced generation accepted old finish");
    require(clock.finish_failure(*b),"Restart failure could not finish");
    clock.reset();require(!clock.finish_failure(*b),"Reset epoch accepted old admission");
    require(a->sound()==42 && a->selection()==SourceScriptSoundClockSelectionTu15{0,2},"Reset changed captured admission metadata");
    require(clock.rows()==SourceScriptSoundClockRowsTu15{},"Reset did not restore literal original metadata defaults");
    const auto c=clock.begin(42,policy,p.stop(),fp);clock.reset();
    require(!clock.finish_failure(*c),"Pending reset did not invalidate ticket");
    const auto d=clock.begin(42,policy,p.stop(),fp);require(clock.finish_failure(*d),"Same tuple fresh epoch failed");
    require(!clock.finish_failure(*c),"Identical tuple admitted stale epoch");
}
void time_and_stop(){
    Providers p;auto policy=p.policy();SourceScriptSoundClockNative64Tu15 clock;
    auto a=clock.begin(42,policy,p.stop(),fp);clock.finish_failure(*a);
    const SourceScriptSoundGameTicksTu15 game=clock.explicit_remaining(42,30,p.now,fp);
    const SourceScriptSoundTicks30Tu15 stored=clock.stored_remaining(42,1000,p.now,p.now,fp);
    const SourceScriptSoundTicks30Tu15 language=clock.language_remaining(42,policy,fp);
    require(game.value==60 && stored.value==30 && language.value==30,"Game/HS frame units collapsed");
    p.now.tick_bits=160;
    require(clock.explicit_remaining(42,30,p.now,fp).value==0 && clock.language_remaining(42,policy,fp).value==0,"Expired query did not clamp");
    auto before=clock.rows();require(clock.stop_first(42,p.stop()) && p.stops==1,"First stop request failed");
    before[0].sound=before[0].start_tick_bits=0xffffffff;
    require(clock.rows()==before,"Stop changed retained selector/predicate");
    require(!clock.stop_first(42,{}) && !clock.stop_first(0xffffffff,{}),"Missing/NONE stop requested playback");
    require(!clock.finish_failure(*a),"Stopped generation still admitted");
    const auto reads=p.reads,durations=p.durations;
    require(clock.language_remaining(42,{},fp).value==0 && p.reads==reads && p.durations==durations,"Missing language row called providers");
}
void capacity_and_failures(){
    Providers p;auto policy=p.policy();SourceScriptSoundClockNative64Tu15 clock;
    for(unsigned n=1;n<=16;++n){const auto a=clock.begin(n,policy,p.stop(),fp);require(a && clock.finish_failure(*a),"Clock lost capacity before16 rows");}
    const auto snapshot=clock.rows();const auto selections=p.selections;
    require(!clock.begin(17,policy,p.stop(),fp) && p.selections==selections && clock.rows()==snapshot,"Full clock selected or changed a row");
    p.now.tick_bits+=60;const auto expired=clock.begin(17,policy,p.stop(),fp);
    require(expired && clock.rows()[0].sound==17 && p.stops==0,"Expired replacement stopped old playback or chose wrong row");
    clock.finish_failure(*expired);clock.reset();
    policy.select=[](std::uint32_t,auto&s){s.pitch_range=9;throw std::runtime_error("Selection failed after store");};
    fails([&]{clock.begin(42,policy,p.stop(),fp);},"Selection failure swallowed");
    require(clock.rows()[0].selection==SourceScriptSoundClockSelectionTu15{9,0xff} && clock.rows()[0].sound==0xffffffff,"Failure rolled back partial selector or published row");
    policy=p.policy();auto a=clock.begin(42,policy,p.stop(),fp);clock.finish_failure(*a);
    const auto before=clock.rows();
    fails([&]{clock.begin(42,policy,{},fp);},"Missing actual stop owner accepted");
    require(clock.rows()==before,"Rejected stop changed clock row");
}
void gates(){
    Providers p;auto policy=p.policy();SourceScriptSoundClockNative64Tu15 clock;
    auto a=clock.begin(42,policy,p.stop(),fp);clock.finish_failure(*a);
    const auto reads=p.reads,durations=p.durations,selections=p.selections;
    std::atomic<unsigned> rejected=0;
    std::thread wrong([&]{
        const auto reject=[&](auto&&f){try{f();}catch(const std::runtime_error&){++rejected;}};
        reject([&]{clock.begin(43,policy,p.stop(),fp);});
        reject([&]{clock.language_remaining(42,policy,fp);});
        reject([&]{clock.finish_failure(*a);});
        reject([&]{clock.rows();});
        reject([&]{clock.reset();});
    });wrong.join();
    require(rejected==5 && p.reads==reads && p.durations==durations && p.selections==selections,"Wrong-thread operation read providers");
    clock.reset();unsigned nested=0;
    policy.select=[&](std::uint32_t,auto&s){
        fails([&]{clock.rows();},"Provider reentered clock read");++nested;s={1,2};
    };
    auto b=clock.begin(42,policy,p.stop(),fp);require(b && nested==1 && clock.finish_failure(*b),"Reentrant rejection poisoned outer admission");
    policy.selected_seconds=[&](unsigned,std::uint32_t,const auto&){clock.reset();return 0u;};
    fails([&]{clock.language_remaining(42,policy,fp);},"Duration callback reentered reset");
    require(clock.rows()[0].sound==42,"Reentrant failure changed existing row");
}
void callback_lifetimes(){
    Providers p;auto policy=p.policy();
    auto clock=std::make_unique<SourceScriptSoundClockNative64Tu15>();auto*raw=clock.get();
    policy.select=[&](std::uint32_t,auto&s){s={3,4};clock.reset();policy={};};
    const auto admission=raw->begin(42,policy,{},fp);
    require(!clock && admission && admission->sound()==42 && admission->selection()==SourceScriptSoundClockSelectionTu15{3,4},"Selection callback destroyed an unretained owner/callback");

    clock=std::make_unique<SourceScriptSoundClockNative64Tu15>();raw=clock.get();policy=p.policy();
    policy.time=[&]{clock.reset();policy={};return p.now;};
    const auto timed=raw->begin(43,policy,{},fp);
    require(!clock && timed && timed->sound()==43,"Time callback destroyed an unretained owner/callback");

    clock=std::make_unique<SourceScriptSoundClockNative64Tu15>();raw=clock.get();policy=p.policy();
    auto a=clock->begin(42,policy,p.stop(),fp);clock->finish_failure(*a);
    policy.selected_seconds=[&](unsigned,std::uint32_t,const auto&){clock.reset();policy={};return 0x3f800000u;};
    require(raw->language_remaining(42,policy,fp).value==30 && !clock,"Duration callback destroyed an unretained query owner");

    clock=std::make_unique<SourceScriptSoundClockNative64Tu15>();raw=clock.get();policy=p.policy();
    a=clock->begin(42,policy,p.stop(),fp);clock->finish_failure(*a);
    SourceScriptSoundClockNative64Tu15::StopRequest stop=[&](const auto&voice){require(!voice,"Metadata clock acquired a fake voice");clock.reset();stop={};};
    require(raw->stop_first(42,stop) && !clock,"Stop callback destroyed an unretained owner/callback");
}
}
int main(){try{admissions();time_and_stop();capacity_and_failures();gates();callback_lifetimes();std::cout<<"Native64 script sound owner metadata/ticket/thread/lifetime controls PASS; actual Audio voice receipt tests separate\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
