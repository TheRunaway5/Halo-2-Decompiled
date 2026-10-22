#include "halo2/multiplayer_audio.hpp"
#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <new>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <exception>
#include <iostream>
#include <thread>

namespace {
thread_local bool watch_commit=false;
thread_local std::size_t allocations=0,releases=0;
void* allocate(std::size_t n){if(watch_commit)++allocations;if(auto*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void release(void*p)noexcept{if(p&&watch_commit)++releases;std::free(p);}
}
void* operator new(std::size_t n){return allocate(n);}
void* operator new[](std::size_t n){return allocate(n);}
void operator delete(void*p)noexcept{release(p);}
void operator delete[](void*p)noexcept{release(p);}
void operator delete(void*p,std::size_t)noexcept{release(p);}
void operator delete[](void*p,std::size_t)noexcept{release(p);}
void* operator new(std::size_t n,std::align_val_t a){
  if(watch_commit)++allocations;const auto alignment=std::size_t(a);
  if(n>std::size_t(-1)-(alignment-1))throw std::bad_alloc();
#ifdef _MSC_VER
  if(auto*p=_aligned_malloc(n?n:1,alignment))return p;
#else
  if(auto*p=std::aligned_alloc(alignment,((n?n:1)+alignment-1)/alignment*alignment))return p;
#endif
  throw std::bad_alloc();
}
void* operator new[](std::size_t n,std::align_val_t a){return ::operator new(n,a);}
namespace {
void release_aligned(void*p)noexcept{
#ifdef _MSC_VER
  if(p&&watch_commit)++releases;_aligned_free(p);
#else
  release(p);
#endif
}
}
void operator delete(void*p,std::align_val_t)noexcept{release_aligned(p);}
void operator delete[](void*p,std::align_val_t)noexcept{release_aligned(p);}
void operator delete(void*p,std::size_t,std::align_val_t)noexcept{release_aligned(p);}
void operator delete[](void*p,std::size_t,std::align_val_t)noexcept{release_aligned(p);}


namespace {
using namespace halo2;
constexpr std::uint32_t none=0xffffffffu;
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
const SoundDriverFloatingPointContext fp{0x027f,0x1f80};
void need(bool value,const char*message){if(!value)throw FormatError(message);}
template<class F>void rejects(F&&call,std::string_view reason,const char*message){
  bool rejected=false;
  try{call();}catch(const std::exception&error){rejected=std::string_view(error.what()).find(reason)!=std::string_view::npos;}
  need(rejected,message);
}
template<class F>decltype(auto) stage(std::string_view name,F&&call){
  try{return call();}catch(const std::exception&error){throw FormatError(std::string(name)+": "+error.what());}
}
template<class F>void worker(F&&call){
  std::exception_ptr failure;std::thread thread([&]{try{call();}catch(...){failure=std::current_exception();}});
  thread.join();if(failure)std::rethrow_exception(failure);
}
void preflight(){
  const auto before=SDL_WasInit(SDL_INIT_AUDIO);
  const SdlSoundPlatformConfiguration unavailable{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,256,"invalid-owner-must-not-open-this-device"};
  rejects([&]{MultiplayerAudio audio(std::shared_ptr<const Cache>{},1,unavailable);},
      "retaining Cache owner","Null Cache owner reached SDL construction");
  need(SDL_WasInit(SDL_INIT_AUDIO)==before,"Rejected Cache owner changed SDL initialization");
}
std::uint32_t impulse(const Cache&cache){
  const auto tag=cache.query_tag({.handle=TagHandle{0xe268435bu},.group=TagGroup{"snd!"},.require_address=true});
  need(tag && tag->name.ends_with("mc_warthog_p_exit"),"Explicit MCC Lockout sound fixture differs");
  return tag->datum;
}
double peak(std::span<const float>pcm){
  double value=0;for(const float sample:pcm){need(std::isfinite(sample),"Actual PCM produced a nonfinite sample");value=std::max(value,std::abs(double(sample)));}return value;
}
double consume(MultiplayerAudio&audio){
  double mixed_peak=0;
  for(unsigned i=0;i<512 && audio.voices();++i){mixed_peak=std::max(mixed_peak,peak(audio.render(4096)));audio.update();}
  need(audio.voices()==0,"Actual PCM did not complete within the bounded manual witness");
  return mixed_peak;
}
void owners(const std::filesystem::path&map,const std::shared_ptr<const Cache>&cache){
  const auto before=SDL_WasInit(SDL_INIT_AUDIO);
  const SdlSoundPlatformConfiguration unavailable{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,256,"invalid-owner-must-not-open-this-device"};
  const std::shared_ptr<const Cache> nonowning(std::shared_ptr<const Cache>{},cache.get());
  const std::shared_ptr<const Cache> null_alias(cache,nullptr);
  need(nonowning && !nonowning.use_count() && !null_alias && null_alias.use_count(),"Invalid owner controls lost their premises");
  for(const auto&invalid:{nonowning,null_alias})rejects([&]{MultiplayerAudio audio(invalid,1,unavailable);},
      "retaining Cache owner","Invalid Cache control block reached SDL construction");
  need(SDL_WasInit(SDL_INIT_AUDIO)==before,"Invalid alias changed SDL initialization");
  std::weak_ptr<const Cache> weak;
  {
    auto actual=std::make_shared<const Cache>(map);weak=actual;
    auto audio=std::make_unique<MultiplayerAudio>(actual,17,manual);
    const auto sound=impulse(*actual);actual.reset();
    need(!weak.expired(),"Audio did not retain its actual Cache");
    audio->enroll_continuation();audio->prepare(sound);
    need(audio->play_impulse(sound).has_value(),"Retained actual Cache failed playback");consume(*audio);
    audio.reset();
  }
  need(weak.expired(),"Audio retained Cache after owner retirement");
  MultiplayerAudio borrowed(*cache,19,manual);
  rejects([&]{borrowed.enroll_continuation();},"retaining Cache owner","Borrowed Cache acquired continuation ownership");
}
void legacy_and_enrollment(const std::shared_ptr<const Cache>&cache,std::uint32_t sound){
  for(const bool owning:{false,true}){
    auto audio=owning?std::make_unique<MultiplayerAudio>(cache,23,manual):
        std::make_unique<MultiplayerAudio>(*cache,23,manual);
    worker([&]{need(audio->play_impulse(sound).has_value(),"Legacy foreign producer failed real playback");(void)audio->render(64);});
    audio->stop();
    if(owning)rejects([&]{audio->enroll_continuation();},"exclusive creating producer","Foreign producer history acquired enrollment");
    worker([&]{need(audio->play_impulse(sound).has_value(),"Rejected enrollment changed legacy producer behavior");});
    audio->stop();
  }
  MultiplayerAudio active(cache,29,manual);
  rejects([&]{worker([&]{active.enroll_continuation();});},"foreign thread","Foreign caller enrolled Audio");
  need(active.play_impulse(sound).has_value(),"Live enrollment guard control failed");
  rejects([&]{active.enroll_continuation();},"quiescent","Live playback acquired enrollment");active.stop();
  active.enroll_continuation();active.enroll_continuation();
  rejects([&]{worker([&]{active.prepare(sound);});},"foreign thread","Enrolled producer accepted foreign mutation");
  need(active.play_impulse(sound).has_value(),"Enrolled producer failed after foreign refusal");consume(active);
}
struct Result {
  std::vector<float> prefix;
  std::vector<SdlSoundCompletion> completed;
  std::vector<MultiplayerAudioSubmission> submitted;
  double mixed_peak{};
  std::uint64_t physical{},presentation{};
};
Result ordinary(const std::shared_ptr<const Cache>&cache,std::uint32_t sound,bool enrolled){
  Result result;MultiplayerAudio*current=nullptr;bool reject_observer=false,reject_completion=false;
  MultiplayerAudio audio(cache,0x5821b36d,manual,[&](const auto&packet){
    // This runs after actual SDL submit but before the Core voice map entry.
    const auto position=current->playback_position(packet.voice);
    need(!position.first_device_frame && position.source_frames==0,"Submission observer lost the real pre-emplace stream");
    rejects([&]{current->enroll_continuation();},"quiescent","Packet observer reentered enrollment");reject_observer=true;
    result.submitted.push_back(packet);
  });current=&audio;
  if(enrolled)audio.enroll_continuation();
  const std::array<Vec3,1> listener{{{0,0,0}}};audio.listeners(listener);
  unsigned callbacks=0;
  std::function<void(const SdlSoundCompletion&)> completed=[&](const auto&packet){
    result.completed.push_back(packet);++callbacks;
    rejects([&]{audio.enroll_continuation();},"quiescent","Completion callback reentered enrollment");reject_completion=true;
    if(callbacks==1)need(audio.play_impulse(sound).has_value(),"Completion callback failed to chain actual playback");
  };
  const auto first=audio.play_impulse(sound,1,MultiplayerAudioKind::Impulse,0x6d04,0,completed);
  need(bool(first),"Actual impulse was not admitted");
  audio.master_volume(.75f);std::array<float,sound_class_count> gains{};gains.fill(-3.f);audio.class_gains(gains);
  audio.paused(true);const auto silent=audio.render(96);
  need(std::all_of(silent.begin(),silent.end(),[](float x){return x==0;}),"Paused Audio consumed PCM");audio.paused(false);
  result.prefix=audio.render(1024);
  result.mixed_peak=std::max(peak(result.prefix),consume(audio));
  need(result.mixed_peak>0,"Actual complete MCC cue produced no nonzero mixed PCM");
  need(callbacks==1 && reject_observer && reject_completion,"Accounted callback/reentry controls were not exercised");
  const auto scheduled=audio.schedule_impulse(sound,audio.presentation_frames()+48000,1,MultiplayerAudioKind::Impulse,0x6d05,0,completed);
  need(bool(scheduled),"Actual scheduled impulse was not admitted");audio.stop();
  need(callbacks==2 && result.completed.back().status==SdlSoundCompletionStatus::Flushed,"Scheduled stop lost real flushed accounting");
  result.physical=audio.mixed_frames();result.presentation=audio.presentation_frames();return result;
}
void equal(const Result&a,const Result&b){
  need(a.prefix==b.prefix && a.mixed_peak==b.mixed_peak && a.physical==b.physical && a.presentation==b.presentation,"Enrollment changed actual PCM or clock behavior");
  need(a.completed.size()==b.completed.size() && a.submitted.size()==b.submitted.size(),"Enrollment changed packet/callback counts");
  for(unsigned i=0;i<a.completed.size();++i){const auto&x=a.completed[i];const auto&y=b.completed[i];
    need(x.voice==y.voice && x.channel_context==y.channel_context && x.packet_context==y.packet_context &&
        x.submission==y.submission && x.status==y.status && x.pcm_frames==y.pcm_frames,"Enrollment changed exact completion identity");}
  for(unsigned i=0;i<a.submitted.size();++i){const auto&x=a.submitted[i];const auto&y=b.submitted[i];
    need(x.sound==y.sound && x.voice==y.voice && x.pitch_range==y.pitch_range && x.permutation==y.permutation &&
        x.frames==y.frames && x.rng_before==y.rng_before && x.rng_after==y.rng_after &&
        x.source_pitch_bits==y.source_pitch_bits,"Enrollment changed actual authored selection");}
}
void failed_observer(const std::shared_ptr<const Cache>&cache,std::uint32_t sound){
  for(const bool enrolled:{false,true}){
    bool fail=true;std::uint64_t failed{};MultiplayerAudio*current=nullptr;
    MultiplayerAudio audio(cache,31,manual,[&](const auto&packet){
      (void)current->playback_position(packet.voice);
      if(fail){failed=packet.voice;throw FormatError("intentional packet observer failure");}
    });current=&audio;if(enrolled)audio.enroll_continuation();
    rejects([&]{audio.play_impulse(sound);},"intentional packet observer failure","Packet observer control did not fail");
    need(audio.voices()==0,"Failed pre-emplace packet leaked a Core voice");
    bool retired=false;try{audio.playback_position(failed);}catch(const std::exception&){retired=true;}
    need(retired,"Failed packet retained an unaccounted platform stream");
    fail=false;need(audio.play_impulse(sound).has_value(),"Observer failure poisoned following playback");consume(audio);
  }
}
SourceScriptSoundClockPolicyTu15 policy(){
  SourceScriptSoundClockPolicyTu15 p;
  // Explicit fixed policy controls isolate actual Core/PCM lifetime. They do
  // not claim authentic World timing or a restored native script caller.
  p.time=[]{return SourceScriptSoundClockTimeTu15{100,30,0x3d088889};};
  p.selected_seconds=[](unsigned,std::uint32_t,const auto&){return 0x3f800000u;};
  p.select=[](std::uint32_t,auto&selection){selection={0xff,0xff};};return p;
}
std::vector<std::int32_t> script_fade(const std::shared_ptr<const Cache>&cache,std::uint32_t sound,bool enrolled){
  MultiplayerAudio audio(cache,37,manual);if(enrolled)audio.enroll_continuation();auto clock=audio.make_script_sound_clock();
  auto prepared=audio.prepare_script_impulse(clock,sound,policy(),{},fp);need(bool(prepared),"Script preparation control unavailable");
  const auto rows=clock->rows();
  rejects([&]{audio.enroll_continuation();},"quiescent","Pending script admission acquired enrollment");
  need(clock->rows()==rows,"Rejected enrollment changed script metadata");unsigned completions=0;
  const auto id=audio.play_prepared_script_impulse(*prepared,1,MultiplayerAudioKind::Impulse,none,0,{},
      audio.presentation_frames()+48000*2,[&](const auto&packet){
        need(packet.status==SdlSoundCompletionStatus::Flushed,"Timed script stop lost actual queued packet status");++completions;});
  need(bool(id),"Script PCM publication failed");
  need(rows[0].sound==sound,"Fresh single script admission did not occupy row zero");
  auto stopped_rows=rows;stopped_rows[0].sound=none;stopped_rows[0].start_tick_bits=none;
  need(clock->stop_first(sound,[&](const auto&receipt){need(receipt && audio.request_script_stop(*receipt),"Actual script receipt failed timed stop");}),"Script timing row unavailable");
  need(clock->rows()==stopped_rows,"Original stop did not clear only sound/start while preserving all other row fields");
  std::vector<std::int32_t> volumes;
  audio.paused(true);(void)audio.render(48);audio.update();volumes.push_back(audio.voice_parameters(*id).volume_hundredths_db);
  audio.paused(false);(void)audio.render(4800);audio.update();need(audio.voice_active(*id),"Script fade retired early");volumes.push_back(audio.voice_parameters(*id).volume_hundredths_db);
  (void)audio.render(48000);audio.update();
  need(!audio.voice_active(*id),"Script fade retained its native voice after retirement");
  need(completions==1,"Script fade did not report exactly one accounted completion");
  need(clock->rows()==stopped_rows,"PCM retirement changed the exact post-stop script metadata");return volumes;
}
std::uint32_t loop_sound(const Cache&cache){
  for(const auto&tag:cache.select_tags({.group=TagGroup{"lsnd"},.require_address=true})){
    const auto d=cache.load_looping_sound(tag.datum);
    if((d.flags&2) || d.tracks.size()!=1)continue;const auto&t=d.tracks.front();
    if(t.start!=none || t.end!=none || t.loop==none || t.fade_in!=0)continue;
    const auto variation=cache.load_sound_variation(t.loop);
    if(variation.pitch_ranges.empty() || variation.permutations.empty() || variation.permutations.size()>16)continue;
    const auto pcm=decode_sound(cache.load_sound(t.loop,0,0));
    if(pcm.frames() && pcm.frames()<=96000)return tag.datum;
  }
  throw FormatError("Explicit MCC fixture has no bounded ordinary authored loop");
}
std::vector<std::uint32_t> looping(const std::shared_ptr<const Cache>&cache,std::uint32_t tag,bool enrolled){
  std::vector<std::uint32_t> permutations;MultiplayerAudio audio(cache,41,manual,[&](const auto&packet){permutations.push_back(packet.permutation);});
  if(enrolled)audio.enroll_continuation();const auto ids=audio.interface_loop(tag,0x6d06,0);need(ids.size()==1,"Authored loop fixture changed membership");
  for(unsigned i=0;i<512 && permutations.size()<3;++i){(void)audio.render(4096);audio.update();}
  need(permutations.size()>=3 && audio.voice_active(ids[0]),"Actual loop failed completion/ACK/resubmission");
  audio.listeners(std::array<Vec3,1>{{{0,0,0}}});audio.master_volume(.5f);audio.paused(true);(void)audio.render(64);audio.paused(false);
  audio.stop();need(audio.voices()==0,"Actual loop stop leaked playback");return permutations;
}
bool publish(MultiplayerAudio&audio,MultiplayerAudio::PreparedReplacement&prepared,MultiplayerAudio::HeldPublication&held){
  allocations=releases=0;watch_commit=true;
  const auto began=audio.begin_commit(prepared,held);const auto result=began && audio.commit(prepared,held);
  watch_commit=false;need(allocations==0 && releases==0,"Core publication allocated or retired storage while held");
  return result;
}
void same_pcm(const std::vector<float>&a,const std::vector<float>&b){
  need(a.size()==b.size(),"Core restore changed actual mixed extent");
  for(std::size_t i=0;i<a.size();++i)need(std::bit_cast<std::uint32_t>(a[i])==std::bit_cast<std::uint32_t>(b[i]),
      "Core restore changed actual MCC mixed sample bits");
}
void same_parameters(const SdlSoundVoiceParameters&a,const SdlSoundVoiceParameters&b){
  need(a.relative_rate_bits==b.relative_rate_bits && a.volume_hundredths_db==b.volume_hundredths_db &&
      a.paused==b.paused && a.routes.size()==b.routes.size(),"Core restore lost voice parameters");
  for(unsigned i=0;i<a.routes.size();++i)need(a.routes[i].pcm_channel==b.routes[i].pcm_channel &&
      a.routes[i].output_channel==b.routes[i].output_channel &&
      std::bit_cast<std::uint32_t>(a.routes[i].linear_gain)==std::bit_cast<std::uint32_t>(b.routes[i].linear_gain),
      "Core restore changed route bits");
}
void continuation(const std::shared_ptr<const Cache>&cache,std::uint32_t sound,std::uint32_t loop){
  std::vector<MultiplayerAudioSubmission> submitted;
  MultiplayerAudio audio(cache,0x651d83f1,manual,[&](const auto&p){submitted.push_back(p);});
  audio.enroll_continuation();audio.listeners(std::array<Vec3,1>{{{0,0,0}}});audio.master_volume(.625f);
  std::array<float,sound_class_count> gains{};gains.fill(-4.f);audio.class_gains(gains);
  const auto spatial=audio.play_impulse_at_receipt(sound,{0,0,0});need(bool(spatial),"Actual spatial continuation cue unavailable");
  const auto scheduled=audio.schedule_impulse_receipt(sound,audio.presentation_frames()+3000);
  need(bool(scheduled),"Actual scheduled continuation cue unavailable");
  const auto loops=audio.interface_loop_receipts(loop,0x6d07,0);need(loops.size()==1,"Actual continuation loop membership differs");
  const std::array<MultiplayerAmbientTarget,1> ambient{{{loop,0,.75f,-6.f,.25f}}};
  audio.ambient(ambient,.05f);
  (void)audio.render(137);audio.update();audio.paused(true);
  const auto before_position=audio.playback_position(*spatial);const auto before_parameters=audio.voice_parameters(*spatial);
  const auto cp=audio.checkpoint();need(cp.voices()==4,"Core checkpoint lost actual stream census");
  struct Branch {std::vector<float> pcm;std::vector<MultiplayerAudioSubmission> packets;MultiplayerAudio::VoiceReceipt created;};
  const auto branch=[&]{
    Branch result;const auto first=submitted.size();audio.ambient(ambient,.1f);
    auto samples=audio.render(257);need(peak(samples)==0,"Restored global pause did not retain actual silence");
    result.pcm.insert(result.pcm.end(),samples.begin(),samples.end());audio.paused(false);
    samples=audio.render(511);result.pcm.insert(result.pcm.end(),samples.begin(),samples.end());audio.update();
    const auto created=audio.play_impulse_receipt(sound,.8f);need(bool(created),"Post-cut genuine typed cue unavailable");result.created=*created;
    unsigned loop_packets=0;
    for(unsigned n=0;n<512 && loop_packets<3;++n){
      samples=audio.render(4096);(void)peak(samples);result.pcm.insert(result.pcm.end(),samples.begin(),samples.end());audio.update();
      loop_packets=0;for(std::size_t k=first;k<submitted.size();++k)if(submitted[k].kind==MultiplayerAudioKind::Shield)++loop_packets;
    }
    need(loop_packets>=3,"Actual restored loop did not resubmit bounded PCM");
    result.packets.assign(submitted.begin()+std::ptrdiff_t(first),submitted.end());audio.stop();return result;
  };
  const auto original=branch();need(peak(original.pcm)>0,"Core continuation reference branch was silent");
  audio.master_volume(.125f);gains.fill(-27.f);audio.class_gains(gains);audio.listeners(std::array<Vec3,1>{{{40,3,1}}});
  auto first=audio.prepare_restore(cp);auto sibling=audio.prepare_restore(cp);
  const auto first_identity=first.identity();const auto sibling_identity=sibling.identity();
  need(first.matches(first_identity) && !first.matches(sibling_identity),"Core preparations share authority");
  const auto doomed=first.voice(*spatial);const auto restored=sibling.voice(*spatial);
  need(doomed && restored && !doomed->same(*restored),"Sibling Core target record was reused");
  need(!audio.voice_active(*restored),"Prospective Core receipt mutated the old branch");
  first=MultiplayerAudio::PreparedReplacement{};
  const auto physical=audio.mixed_frames();
  {
    auto held=audio.hold_publication(sibling);
    need(audio.ready(sibling,held),"Core full target refused unchanged actual source");
    worker([&]{need(!audio.ready(sibling,held),"Foreign Core ready admitted callback ownership");});
    rejects([&]{audio.update();},"held","Ordinary Core method reentered held callback exclusion");
    need(publish(audio,sibling,held),"Core full target did not publish");
    need(sibling.published() && sibling.matches(sibling_identity),"Core published witness is not exact");
    need(!audio.commit(sibling,held),"Core target committed twice");
  }
  need(audio.mixed_frames()==physical,"Core publication rewound physical frames");
  need(!audio.voice_active(*spatial) && !audio.voice_active(*doomed) && !audio.voice_active(original.created),
      "Old/canceled Core receipt revived after reused logical IDs");
  need(audio.voice_active(*restored),"Actual restored Core receipt was not installed");
  rejects([&]{audio.stop_voice(spatial->id());},"raw voice mutation","Legacy raw ID regained restored mutation authority");
  rejects([&]{audio.stop_voice(*spatial);},"stale or foreign","Old Core receipt stopped restored playback");
  same_parameters(before_parameters,audio.voice_parameters(*restored));
  const auto position=audio.playback_position(*restored);
  need(position.source_frames==before_position.source_frames && position.output_frames==before_position.output_frames &&
      position.first_device_frame==before_position.first_device_frame && !position.first_resumed_device_frame &&
      position.publication_epoch>before_position.publication_epoch,"Core restore invented/omitted consumption history");
  const auto replay=branch();same_pcm(original.pcm,replay.pcm);
  need(!original.created.same(replay.created) && original.created.id()==replay.created.id(),"Restored Core reused fresh packet-owner record");
  need(original.packets.size()==replay.packets.size(),"Core restore changed selection event count");
  for(unsigned i=0;i<original.packets.size();++i){const auto&a=original.packets[i];const auto&b=replay.packets[i];
    need(a.sound==b.sound && a.voice==b.voice && a.permutation==b.permutation && a.pitch_range==b.pitch_range &&
        a.frames==b.frames && a.rng_before==b.rng_before && a.rng_after==b.rng_after && a.source_pitch_bits==b.source_pitch_bits,
        "Core restore changed authored RNG/permutation/packet counters");}
  auto stale=audio.prepare_restore(cp);const auto identity=stale.identity();audio.master_volume(.4f);
  need(!stale.matches(identity),"Core producer mutation retained preparation authority");
  rejects([&]{(void)audio.hold_publication(stale);},"stale","Core held a revision-stale target");
  auto canceled=audio.prepare_restore(cp);
  {auto held=audio.hold_publication(canceled);canceled=MultiplayerAudio::PreparedReplacement{};
    need(!audio.ready(canceled,held),"Canceled wrapper published its still-held image");}
}
void script_continuation(const std::shared_ptr<const Cache>&cache,std::uint32_t sound,bool retire_previous){
  MultiplayerAudio audio(cache,0x128c38,manual);audio.enroll_continuation();auto clock=audio.make_script_sound_clock();
  auto first=audio.prepare_script_impulse(clock,sound,policy(),{},fp);need(bool(first),"Initial script continuation admission unavailable");
  const auto old=audio.play_prepared_script_impulse(*first,1,MultiplayerAudioKind::Impulse,none,0,{},audio.presentation_frames()+96000);
  need(bool(old),"Initial genuine script continuation packet unavailable");
  std::optional<SourceScriptSoundClockVoice64Tu15> previous;
  auto pending=audio.prepare_script_impulse(clock,sound,policy(),[&](const auto&voice){
    need(bool(voice),"Repeated script admission lost actual previous voice");previous=voice;
    if(retire_previous)audio.stop();else need(audio.request_script_stop(*voice),"Active previous script voice lost its fade");
  },fp);
  need(pending && previous,"Actual mixed previous/pending script graph unavailable");
  const auto rows=clock->rows();const auto cp=audio.checkpoint();need(cp.voices()==(retire_previous?0:1),"Core invented or omitted prior script stream");
  need(audio.play_prepared_script_impulse(*pending).has_value(),"Future pending script branch failed genuine PCM");audio.stop();
  auto prepared=audio.prepare_restore(cp);auto target=prepared.script(*pending);need(bool(target),"Pending external script caller was not remapped");
  need(!prepared.script(*pending),"Pending script target was issued to two cancel-owning callers");
  {auto held=audio.hold_publication(prepared);need(audio.ready(prepared,held),"Mixed script target was not ready");
    rejects([&]{clock->reset();},"unavailable","Ordinary clock reset crossed held producer fence");
    rejects([&]{(void)clock->rows();},"unavailable","Ordinary clock query crossed held producer fence");
    need(audio.ready(prepared,held),"Rejected held clock calls changed the prepared witness");
    need(publish(audio,prepared,held),"Mixed script target did not publish");}
  need(clock->rows()==rows,"Core/clock publication changed complete16 metadata rows");
  rejects([&]{audio.play_prepared_script_impulse(*pending);},"another Audio owner","Old pending script wrapper regained authority");
  rejects([&]{audio.request_script_stop(*previous);},"actual Audio owner","Old previous script receipt regained authority");
  (void)audio.render(48000);audio.update();
  need(clock->rows()==rows,"Previous script retirement cleared replacement pending metadata");
  need(audio.play_prepared_script_impulse(*target).has_value(),"Fresh pending wrapper failed once-only actual PCM publication");
  rejects([&]{audio.play_prepared_script_impulse(*target);},"stale clock generation","Fresh script wrapper published twice");
  need(consume(audio)>0,"Restored script branch produced no genuine nonzero PCM");
}
void held_owner_retirement(const std::shared_ptr<const Cache>&cache,std::uint32_t sound){
  const auto initialized=SDL_WasInit(SDL_INIT_AUDIO);
  auto audio=std::make_unique<MultiplayerAudio>(cache,73,manual);audio->enroll_continuation();
  need(bool(audio->play_impulse_receipt(sound)),"Owner-retirement genuine packet unavailable");
  auto cp=audio->checkpoint();auto prepared=audio->prepare_restore(cp);const auto identity=prepared.identity();
  {auto held=audio->hold_publication(prepared);audio.reset();
    need(!prepared.matches(identity) && !prepared.published(),"Dead public Core retained publication authority");}
  need(SDL_WasInit(SDL_INIT_AUDIO)==initialized,"Held owner death retained native device initialization");
  worker([p=std::move(prepared),saved=std::move(cp)]()mutable{p=MultiplayerAudio::PreparedReplacement{};saved=MultiplayerAudio::Checkpoint{};});
}
void script_cancellation(const std::shared_ptr<const Cache>&cache,std::uint32_t sound){
  MultiplayerAudio audio(cache,0x528,manual);audio.enroll_continuation();auto clock=audio.make_script_sound_clock();
  unsigned empty_stops=0;
  const auto actual_stop=[&](const std::optional<SourceScriptSoundClockVoice64Tu15>&voice){
    if(voice)need(audio.request_script_stop(*voice),"Repeated cancellation admission lost its actual previous voice");
    else ++empty_stops;
  };
  auto pending=audio.prepare_script_impulse(clock,sound,policy(),{},fp);need(bool(pending),"Cancellation control lacks genuine pending admission");
  const auto saved=audio.checkpoint();
  auto canceled=audio.prepare_restore(saved);auto target=canceled.script(*pending);need(bool(target),"Prospective cancellation target unavailable");
  worker([p=std::move(target)]()mutable{p.reset();});
  {auto held=audio.hold_publication(canceled);need(!audio.ready(canceled,held) && !audio.begin_commit(canceled,held),
      "Canceled prospective wrapper retained a sealed pending image");}
  auto source_stale=audio.prepare_restore(saved);
  worker([p=std::move(pending)]()mutable{p.reset();});
  {auto held=audio.hold_publication(source_stale);need(!audio.ready(source_stale,held),
      "Actual current wrapper cancellation did not stale Core preparation");}
  // The retained checkpoint is still a genuine historical pending image.
  audio.update();
  pending=stage("script cancellation second admission",[&]{return audio.prepare_script_impulse(clock,sound,policy(),actual_stop,fp);});need(bool(pending),"Second cancellation admission unavailable");
  const auto rows=clock->rows();const auto second=audio.checkpoint();auto reserved=audio.prepare_restore(second);
  target=reserved.script(*pending);need(bool(target),"Reserved prospective wrapper unavailable");
  const auto identity=reserved.identity();
  {auto held=audio.hold_publication(reserved);need(audio.begin_commit(reserved,held),"Exact Core intent did not reserve");
    worker([p=std::move(target)]()mutable{p.reset();});
    need(audio.ready(reserved,held) && reserved.matches(identity),"Late cancellation changed the reserved target before publication");
    need(publish(audio,reserved,held) && reserved.published(),"Reserved target failed after prospective cancellation");}
  need(!reserved.published(),"Released cancellation reservation hid a genuine producer request");
  audio.update();need(audio.voices()==0,"Deferred pending cancellation invented playback");
  need(clock->rows()==rows,"Failure retirement changed the original clock metadata policy");
  auto again=stage("script cancellation abort admission",[&]{return audio.prepare_script_impulse(clock,sound,policy(),actual_stop,fp);});need(bool(again),"Abort-intent admission unavailable");
  need(empty_stops==2,"Repeated actual metadata rows did not exercise their empty-voice stop owner");
  const auto last=audio.checkpoint();auto aborted=audio.prepare_restore(last);const auto abort_identity=aborted.identity();
  {auto held=audio.hold_publication(aborted);need(audio.begin_commit(aborted,held),"Abort control did not reserve intent");}
  need(!aborted.matches(abort_identity),"Unpublished intent survived held scope retirement");
  need(audio.play_prepared_script_impulse(*again).has_value(),"Aborted target disturbed the actual current pending branch");
  need(consume(audio)>0,"Abort control lost actual current MCC PCM");
}
void callback_owner_retirement(const std::shared_ptr<const Cache>&cache,std::uint32_t sound,std::uint32_t loop,bool enrolled){
  const auto initialized=SDL_WasInit(SDL_INIT_AUDIO);bool armed=false,removed=false,pending_at_removal=false;unsigned completed=0;
  std::optional<MultiplayerAudio::Voice> pending;
  std::optional<SdlSoundCompletionStatus> completion_status;
  std::unique_ptr<MultiplayerAudio> audio;
  audio=std::make_unique<MultiplayerAudio>(cache,0x74,manual,[&](const auto&){
    if(armed){armed=false;removed=true;
      pending_at_removal=pending && audio->voice_active(*pending) &&
        !audio->playback_position(*pending).first_device_frame;
      audio.reset();}
  });
  if(enrolled)audio->enroll_continuation();
  pending=audio->schedule_impulse(sound,std::uint64_t(512)*4096+1,1,MultiplayerAudioKind::Impulse,none,0,
      [&](const auto&packet){++completed;completion_status=packet.status;});
  need(pending.has_value(),
      "Callback-owner control lacks genuine one-shot completion");
  need(audio->interface_loop(loop,0x6d08,0).size()==1,"Callback-owner control lacks genuine loop");armed=true;
  for(unsigned i=0;i<512 && audio;++i){(void)audio->render(4096);audio->update();}
  need(removed && pending_at_removal && !audio && completed==1 && completion_status==SdlSoundCompletionStatus::Flushed,
      "Observer owner retirement lost or repeated pending actual flushed completion");
  need(SDL_WasInit(SDL_INIT_AUDIO)==initialized,"Observer retirement leaked the actual SDL endpoint");
}

}
int main(int argc,char**argv){try{
  if(argc==2 && std::string_view(argv[1])=="--owner-preflight"){preflight();std::cout<<"PASS Core rejects null retaining owner before SDL\n";return 0;}
  need(argc==3 && std::string_view(argv[1])=="--mcc","Use --owner-preflight or --mcc ABSOLUTE_MCC_LOCKOUT_MAP");
  const std::filesystem::path map=argv[2];need(map.is_absolute() && std::filesystem::is_regular_file(map),"Explicit MCC map must be an existing absolute file");
  auto cache=std::make_shared<const Cache>(map);need(cache->is_mcc(),"Core fixture requires MCC-only runtime resources");
  preflight();const auto sound=impulse(*cache);owners(map,cache);legacy_and_enrollment(cache,sound);
  equal(ordinary(cache,sound,false),ordinary(cache,sound,true));failed_observer(cache,sound);
  need(script_fade(cache,sound,false)==script_fade(cache,sound,true),"Enrollment changed logical fade behavior before restoration");
  const auto loop=loop_sound(*cache);need(looping(cache,loop,false)==looping(cache,loop,true),"Enrollment changed actual loop permutation/packet behavior");
  stage("PCM suffix",[&]{continuation(cache,sound,loop);});stage("active previous script",[&]{script_continuation(cache,sound,false);});stage("retired previous script",[&]{script_continuation(cache,sound,true);});stage("held owner retirement",[&]{held_owner_retirement(cache,sound);});
  stage("script cancellation",[&]{script_cancellation(cache,sound);});stage("legacy callback retirement",[&]{callback_owner_retirement(cache,sound,loop,false);});stage("enrolled callback retirement",[&]{callback_owner_retirement(cache,sound,loop,true);});
  std::cout<<"PASS genuine MCC Core local continuation: retaining Cache, invalid aliases, legacy foreign producer, creator/reentry/pending guards, pre-emplace observation, exact PCM/RNG/completion, failure cleanup, actual loops, typed script and logical fade\n";
  std::cout<<"LIMIT local Core/clock continuation only; native target-World binding, aggregate companion adoption, physical audibility and complete M04 readiness remain open; fixed script policy is a lifetime control\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
