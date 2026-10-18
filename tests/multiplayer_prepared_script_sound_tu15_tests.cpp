#include "halo2/multiplayer_audio.hpp"
#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
#include <type_traits>

namespace {std::atomic_bool fail_publication_allocation{false};}
void*operator new(std::size_t n){if(fail_publication_allocation.exchange(false))throw std::bad_alloc();if(auto*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}

namespace {
using namespace halo2;
void need(bool value,const char*message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F&&f,const char*message){bool bad=false;try{f();}catch(const std::exception&){bad=true;}need(bad,message);}
constexpr auto none=0xffffffffu;
const SoundDriverFloatingPointContext fp{0x027f,0x1f80};
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
std::uint32_t named(const Cache&cache,std::uint32_t full_salt,std::string_view suffix){
    const auto tag=cache.query_tag({.handle=TagHandle{full_salt},.group=TagGroup::from_string("snd!"),.require_address=true});
    need(tag && tag->datum==full_salt && tag->name.ends_with(suffix),"Actual full-salt sound fixture differs");return full_salt;
}
struct Policy {
    const Cache&cache;std::uint32_t sound;SoundVariationDefinition definition;
    std::uint32_t&random;std::vector<std::uint32_t>prefix;std::vector<std::uint16_t>chances;
    SourceScriptSoundClockTimeTu15 now{100,30,0x3d088889};unsigned reads=0,selections=0;
    Policy(const Cache&c,std::uint32_t s,std::uint32_t&r):cache(c),sound(s),definition(c.load_sound_variation(s)),random(r){
        for(const auto&p:definition.permutations)chances.push_back(Bytes(p).u16(2));
        const auto base=Bytes(definition.range).u16(6);
        prefix.resize((std::size_t(base)+chances.size()+32+31)/32);
    }
    SourceScriptSoundClockPolicyTu15 callbacks(){
        SourceScriptSoundClockPolicyTu15 p;
        p.time=[this]{++reads;return now;};
        // Owned one-second controls isolate admission/lifetime. These are not
        // a claim that this actual sound's original TU1.5 timing is one second.
        p.selected_seconds=[](unsigned,std::uint32_t,const auto&){return 0x3f800000u;};
        p.select=[this](std::uint32_t id,auto&selection){
            need(id==sound,"Clock selection lost actual full sound identity");++selections;
            SourceScriptSoundClockSelectionInputTu15 in;
            in.sound=sound;in.pitch_range_count=std::uint8_t(definition.pitch_ranges.size());
            in.permutation_count=std::int16_t(definition.permutations.size());
            const auto base=Bytes(definition.range).u16(6);
            if((std::to_integer<unsigned>(definition.sound_class[10])&0x20) && base!=0xffff)in.prefix_bit=base;
            in.skip_fractions=chances;
            // Prefix zero is an owned test control, already covered by the
            // genuine213410 CPU matrix, not an inferred live World bank.
            source_script_sound_clock_select_tu15(in,prefix,random,selection,fp);
        };
        return p;
    }
};
void consume(MultiplayerAudio&audio){
    for(unsigned i=0;i<512 && audio.voices();++i){(void)audio.render(4096);audio.update();}
    need(audio.voices()==0,"Actual PCM packet failed to complete within bounded native mixing");
}
void forced_and_lifetime(const Cache&cache,std::uint32_t sound){
    std::uint32_t random=0x9147a23b;Policy policy(cache,sound,random);std::vector<MultiplayerAudioSubmission>events;
    MultiplayerAudio audio(cache,0x12345678,manual,[&](const auto&s){events.push_back(s);},&random);
    auto clock=audio.make_script_sound_clock();
    auto prepared=audio.prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);need(bool(prepared),"Actual preparation unavailable");
    const auto pair=prepared->selection();need(pair.pitch_range==0 && pair.permutation!=0xff,"One-range fixture did not produce exact forced selector");
    SoundLocalRandom expected(random);source_sound_gain(policy.definition,expected);source_sound_pitch(policy.definition,expected);
    const auto snapshot=clock->rows();unsigned completions=0;
    const auto voice=audio.play_prepared_script_impulse(*prepared,1,MultiplayerAudioKind::Impulse,none,0,{},{},[&](const auto&p){need(p.status==SdlSoundCompletionStatus::Success,"Actual completed packet failed");++completions;});
    need(voice && events.size()==1 && events[0].voice==*voice && events[0].pitch_range==pair.pitch_range && events[0].permutation==pair.permutation,"Actual packet did not preserve prepared selector/64bit identity");
    need(random==expected.seed && events[0].rng_before==expected.seed && events[0].rng_after==expected.seed,"Forced playback changed original selection/gain/pitch RNG phase");
    need(events[0].frames==decode_sound(cache.load_sound(sound,pair.pitch_range,pair.permutation)).frames(),"Actual PCM frame owner differs from submitted packet");
    rejects([&]{audio.play_prepared_script_impulse(*prepared);},"Preparation consumed twice");
    consume(audio);need(completions==1 && clock->rows()==snapshot,"Natural PCM completion erased source clock metadata");
    bool detached=false;need(clock->stop_first(sound,[&](const auto&receipt){detached=!receipt;}),"Retained timing row disappeared");
    need(detached,"Natural completion retained stale native voice link");
    std::cout<<"PASS actual forced packet, selection-before-variation RNG, PCM completion and detached clock lifetime\n";
}
void cancellation_and_threads(const Cache&cache,std::uint32_t sound){
    std::uint32_t random=7;Policy policy(cache,sound,random);MultiplayerAudio audio(cache,7,manual,{},&random);auto clock=audio.make_script_sound_clock();
    auto prepared=audio.prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);need(bool(prepared),"Cancellation preparation unavailable");
    const auto snapshot=clock->rows();const auto reads=policy.reads,selections=policy.selections;const auto rng=random;
    std::thread foreign([p=std::move(prepared)]()mutable{p.reset();});foreign.join();
    need(policy.reads==reads && policy.selections==selections && random==rng && clock->rows()==snapshot,"Foreign destruction executed a Clock/provider or changed RNG");
    bool still_pending=false;
    try{(void)clock->begin(sound,policy.callbacks(),[](const auto&){},fp);}
    catch(const std::runtime_error&e){still_pending=std::string_view(e.what())=="Native script sound admission still pending";}
    need(still_pending && policy.reads==reads && policy.selections==selections && random==rng,
        "Foreign destruction fabricated immediate producer finish or read policy before producer drain");
    audio.update();auto next=audio.prepare_script_impulse(clock,sound,policy.callbacks(),[](const auto&v){need(!v,"Canceled admission acquired a voice");},fp);need(bool(next),"Producer failed to drain exact canceled ticket");
    const auto next_snapshot=clock->rows();const auto next_rng=random;std::atomic<unsigned>rejected=0;
    std::thread wrong([&]{
        const auto reject=[&](auto&&f){try{f();}catch(const std::exception&){++rejected;}};
        reject([&]{audio.play_prepared_script_impulse(*next);});reject([&]{audio.update();});
        reject([&]{audio.stop();});reject([&]{audio.play_impulse(sound);});reject([&]{audio.render(16);});
    });wrong.join();need(rejected==5 && random==next_rng && clock->rows()==next_snapshot,"Foreign producer mutated script-backed Audio");
    audio.cancel_prepared_script_impulse(*next);need(audio.voices()==0,"Cancellation created actual playback");
    std::cout<<"PASS foreign Prepared destruction queues truthful producer cancellation; all ordinary producers reject foreign thread\n";
}
void failed_publication(const Cache&cache,std::uint32_t sound){
    for(unsigned mode=0;mode<2;++mode){
        std::uint32_t random=17;Policy policy(cache,sound,random);std::uint64_t attempted=0;
        MultiplayerAudio audio(cache,17,manual,[&](const auto&s){attempted=s.voice;if(mode==0)throw std::runtime_error("Real packet observer failure");fail_publication_allocation.store(true);},&random);
        auto clock=audio.make_script_sound_clock();auto prepared=audio.prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);const auto snapshot=clock->rows();
        rejects([&]{audio.play_prepared_script_impulse(*prepared);},"Real packet/publication failure was swallowed");
        fail_publication_allocation.store(false);
        need(attempted && audio.voices()==0 && clock->rows()==snapshot,"Failed actual admission lost row or retained published voice");
        rejects([&]{audio.playback_position(attempted);},"Failed unowned platform voice was not accounted/destroyed");
        bool absent=false;clock->stop_first(sound,[&](const auto&v){absent=!v;});need(absent,"Failed admission forged an actual voice receipt");
    }
    std::cout<<"PASS real Accepted-packet observer failure and map-allocation failure cleanup, retained source row/no fake voice\n";
}
void stale_and_shutdown(const Cache&cache,std::uint32_t sound){
    std::uint32_t random=23;Policy policy(cache,sound,random);auto audio=std::make_unique<MultiplayerAudio>(cache,23,manual,MultiplayerAudioObserver{},&random);
    auto clock=audio->make_script_sound_clock();
    {
        MultiplayerAudio foreign(cache,23,manual,{},&random);const auto reads=policy.reads,selections=policy.selections;const auto rng=random;
        rejects([&]{foreign.prepare_script_impulse(clock,sound,policy.callbacks(),[](const auto&){},fp);},"Another actual Audio accepted a live foreign epoch");
        need(policy.reads==reads && policy.selections==selections && random==rng && foreign.voices()==0,
            "Live foreign Audio read policy/RNG or admitted a voice");
    }
    auto stale=audio->prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);clock->reset();
    const auto rng=random;rejects([&]{audio->play_prepared_script_impulse(*stale);},"Reset generation decoded/played stale preparation");need(random==rng && audio->voices()==0,"Stale admission changed RNG/backend");
    auto fresh=audio->prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);stale.reset();audio->update();
    need(audio->play_prepared_script_impulse(*fresh).has_value(),"Canceled stale ticket retired replacement row");
    audio->stop();audio.reset();rejects([&]{clock->rows();},"Closed actual Audio epoch remains admitted");
    auto other=std::make_unique<MultiplayerAudio>(cache,23,manual,MultiplayerAudioObserver{},&random);
    rejects([&]{other->prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);},"Another actual Audio accepted foreign epoch");
    std::cout<<"PASS stale/reset ticket before RNG/backend, replacement cancellation isolation and foreign Audio epoch rejection\n";
}
void observer_shutdown(const Cache&cache,std::uint32_t sound){
    std::uint32_t random=31;Policy policy(cache,sound,random);std::unique_ptr<MultiplayerAudio>audio;
    audio=std::make_unique<MultiplayerAudio>(cache,31,manual,[&](const auto&){audio.reset();},&random);
    auto clock=audio->make_script_sound_clock();auto prepared=audio->prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);auto*raw=audio.get();
    rejects([&]{raw->play_prepared_script_impulse(*prepared);},"Audio destruction inside observer published a voice");
    need(!audio,"Observer failed to destroy actual public owner");rejects([&]{clock->rows();},"Observer shutdown left admitted Audio epoch");
    prepared.reset();
    std::cout<<"PASS actual owner destruction before emplace retains real Core for packet cleanup and invalidates clock\n";
}
void late_completion(const Cache&cache,std::uint32_t sound){
    std::uint32_t random=41;Policy policy(cache,sound,random);MultiplayerAudio audio(cache,41,manual,{},&random);auto clock=audio.make_script_sound_clock();
    auto first=audio.prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);const auto old=audio.play_prepared_script_impulse(*first);need(bool(old),"First real lifetime voice absent");
    policy.now.tick_bits=101;bool stopped_old=false;
    auto next=audio.prepare_script_impulse(clock,sound,policy.callbacks(),[&](const auto&v){need(v && v->id()==*old,"Restart stop intent lost old actual identity");stopped_old=true;},fp);
    const auto current=audio.play_prepared_script_impulse(*next,1,MultiplayerAudioKind::Impulse,none,0,{},audio.presentation_frames()+48000u*60u);
    need(stopped_old && current && *current!=*old,"Actual replacement voice identities alias");
    for(unsigned n=0;n<128 && audio.voice_active(*old);++n){audio.render(4096);audio.update();}
    need(!audio.voice_active(*old) && audio.voice_active(*current) && audio.playback_position(*current).source_frames==0,"Late-completion fixture did not preserve actual future voice");
    bool retained=false;clock->stop_first(sound,[&](const auto&v){retained=v && v->id()==*current;});
    need(retained,"Old actual completion detached replacement clock voice");
    audio.stop();
    std::cout<<"PASS old real PCM completion cannot detach reused-generation future voice; source stop intent remains separate\n";
}
std::uint32_t multi_range_fixture(const Cache&cache){
    for(const auto&t:cache.tags())if(t.group=="snd!" && t.address){
        const auto active=cache.query_tag({.handle=TagHandle{t.datum},.group=TagGroup::from_string("snd!"),.require_address=true});
        if(!active || active->address!=t.address)continue;
        const Bytes raw(cache.read_tag_bytes(t.address,20));const auto ranges=std::to_integer<unsigned>(raw.slice(10,1)[0]);const auto maximum_ms=raw.u32(16);
        if(ranges>1 && ranges<=8 && maximum_ms && maximum_ms<=10000){
            const auto definition=cache.load_sound_variation(t.datum);
            need(definition.pitch_ranges.size()==ranges,"Actual multi-range fixture ownership differs");
            std::cout<<"FF fixture="<<t.name<<" full="<<std::hex<<t.datum<<std::dec<<" ranges="<<unsigned(ranges)<<'\n';return t.datum;
        }
    }
    throw std::runtime_error("Actual MCC fixture has no bounded multi-range sound");
}
void ordinary_ff(const Cache&cache){
    const auto sound=multi_range_fixture(cache);std::vector<float>baseline;std::optional<MultiplayerAudioSubmission>ordinary;
    std::uint32_t expected_seed;
    {
        std::uint32_t random=0x1872c4d1;MultiplayerAudio audio(cache,29,manual,[&](const auto&s){ordinary=s;},&random);
        need(audio.play_impulse(sound).has_value(),"Actual ordinary multi-range voice unavailable");baseline=audio.render(4096);expected_seed=random;audio.stop();
    }
    std::uint32_t random=0x1872c4d1;Policy policy(cache,sound,random);std::optional<MultiplayerAudioSubmission>submitted;
    MultiplayerAudio audio(cache,29,manual,[&](const auto&s){submitted=s;},&random);auto clock=audio.make_script_sound_clock();
    auto prepared=audio.prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);
    need(prepared && prepared->selection()==SourceScriptSoundClockSelectionTu15{0xff,0xff} && random==0x1872c4d1,"Genuine multi-range clock selector lost FF/no-draw branch");
    need(audio.play_prepared_script_impulse(*prepared).has_value(),"Actual FF multi-range voice unavailable");const auto actual=audio.render(4096);
    need(submitted && ordinary && submitted->pitch_range==ordinary->pitch_range && submitted->permutation==ordinary->permutation &&
        submitted->rng_before==ordinary->rng_before && submitted->rng_after==ordinary->rng_after &&
        submitted->source_pitch_bits==ordinary->source_pitch_bits && random==expected_seed && actual==baseline,
        "FF handoff changed existing ordinary selection/RNG/pitch/PCM output");
    need(clock->rows()[0].selection==SourceScriptSoundClockSelectionTu15{0xff,0xff},"Ordinary packet observer overwrote FF clock selector");
    audio.stop();
    std::cout<<"PASS genuine FF selector retains unchanged ordinary range/permutation/RNG/pitch and4096 PCM frames\n";
}
void timed_stop(const Cache&cache,std::uint32_t sound){
    std::uint32_t random=53;Policy policy(cache,sound,random);
    MultiplayerAudio audio(cache,53,manual,{},&random);auto clock=audio.make_script_sound_clock();
    auto prepared=audio.prepare_script_impulse(clock,sound,policy.callbacks(),{},fp);
    unsigned completions=0;
    // Retain a real accepted packet beyond the fade interval, so natural
    // completion cannot conceal an incorrect immediate stop or lost request.
    const auto voice=audio.play_prepared_script_impulse(*prepared,1,MultiplayerAudioKind::Impulse,none,0,{},
        audio.presentation_frames()+48000u*2u,[&](const auto&p){
            need(p.status==SdlSoundCompletionStatus::Flushed,"Timed stop did not account actual queued packet");++completions;});
    need(bool(voice),"Timed-stop actual voice unavailable");
    const auto original_volume=audio.voice_parameters(*voice).volume_hundredths_db;const auto seed=random;
    std::optional<SourceScriptSoundClockVoice64Tu15> receipt;
    need(clock->stop_first(sound,[&](const auto&v){receipt=v;need(v && audio.request_script_stop(*v),"Timed request lost actual voice receipt");}),"Clock failed first-match timed stop");
    const auto rows=clock->rows();
    need(audio.voice_active(*voice) && completions==0,"Timed request immediately flushed actual packet");
    MultiplayerAudio other(cache,59,manual);
    rejects([&]{other.request_script_stop(*receipt);},"Foreign Audio accepted another live epoch receipt");
    bool rejected=false;std::thread wrong([&]{try{audio.request_script_stop(*receipt);}catch(const std::exception&){rejected=true;}});wrong.join();
    need(rejected && audio.voice_active(*voice) && other.voices()==0,"Foreign stop mutated actual playback");
    audio.render(48);audio.update();audio.render(4800);audio.update();
    need(audio.voice_active(*voice) && audio.voice_parameters(*voice).volume_hundredths_db<original_volume && completions==0,
        "Timed request failed to advance existing native fade without flushing");
    need(audio.request_script_stop(*receipt),"Repeated timed stop lost live actual record");
    audio.render(48);audio.update();audio.render(4800);audio.update();
    need(audio.voice_active(*voice) && completions==0,"Repeated timed request retired before its fade");
    audio.render(48000);audio.update();
    need(!audio.voice_active(*voice) && completions==1 && !audio.request_script_stop(*receipt),"Timed retirement duplicated completion or revived stale receipt");
    need(random==seed && clock->rows()==rows,"Timed playback request altered RNG or cleared clock row again");
    std::cout<<"PASS actual300ms timed-stop request, resampled restart, foreign ownership/thread rejection and single accounted retirement\n";
}
}
int main(int argc,char**argv){try{
    static_assert(sizeof(MultiplayerAudio)==sizeof(void*));
    static_assert(std::is_same_v<MultiplayerAudio::Voice,std::uint64_t>);
    static_assert(!std::is_copy_constructible_v<MultiplayerPreparedScriptImpulseTu15>);
    static_assert(std::is_nothrow_destructible_v<MultiplayerPreparedScriptImpulseTu15>);
    if(argc!=2 || !*argv[1])return 77;
    const Cache cache(argv[1]);need(cache.is_mcc(),"Actual fixture must use MCC-only Cache");
    const auto sound=named(cache,0xe268435bu,"mc_warthog_p_exit");
    forced_and_lifetime(cache,sound);cancellation_and_threads(cache,sound);failed_publication(cache,sound);stale_and_shutdown(cache,sound);observer_shutdown(cache,sound);late_completion(cache,sound);ordinary_ff(cache);timed_stop(cache,sound);
    std::cout<<"LIMIT fixed clock/prefix controls test ownership/selection handoff; actual TU1.5 sound timing/gain policy and Sim/Main wiring remain separate; timed stop reuses native Audio cadence with no original hardware-cadence, >UINT32 allocator crossing or physical audibility claim\n";
    return 0;
}catch(const std::exception&e){fail_publication_allocation.store(false);std::cerr<<e.what()<<'\n';return 1;}}
