#include "halo2/native_vista_announcer.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>
#include <thread>
#include <tuple>
#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace {
thread_local bool watch_publication=false;
thread_local std::size_t allocations=0,releases=0;
void* allocate(std::size_t size){
  if(watch_publication)++allocations;
  if(auto* pointer=std::malloc(size?size:1))return pointer;throw std::bad_alloc();
}
void release(void* pointer)noexcept {
  if(pointer && watch_publication)++releases;std::free(pointer);
}
}
void* operator new(std::size_t size){return allocate(size);}
void* operator new[](std::size_t size){return allocate(size);}
void operator delete(void* pointer)noexcept{release(pointer);}
void operator delete[](void* pointer)noexcept{release(pointer);}
void operator delete(void* pointer,std::size_t)noexcept{release(pointer);}
void operator delete[](void* pointer,std::size_t)noexcept{release(pointer);}
void* operator new(std::size_t size,std::align_val_t align){
  if(watch_publication)++allocations;const auto alignment=std::size_t(align);
  if(size>std::size_t(-1)-(alignment-1))throw std::bad_alloc();
#ifdef _MSC_VER
  if(auto* pointer=_aligned_malloc(size?size:1,alignment))return pointer;
#else
  if(auto* pointer=std::aligned_alloc(alignment,((size?size:1)+alignment-1)/alignment*alignment))return pointer;
#endif
  throw std::bad_alloc();
}
void* operator new[](std::size_t size,std::align_val_t align){return ::operator new(size,align);}
namespace {
void release_aligned(void* pointer)noexcept {
#ifdef _MSC_VER
  if(pointer && watch_publication)++releases;_aligned_free(pointer);
#else
  release(pointer);
#endif
}
}
void operator delete(void* pointer,std::align_val_t)noexcept{release_aligned(pointer);}
void operator delete[](void* pointer,std::align_val_t)noexcept{release_aligned(pointer);}
void operator delete(void* pointer,std::size_t,std::align_val_t)noexcept{release_aligned(pointer);}
void operator delete[](void* pointer,std::size_t,std::align_val_t)noexcept{release_aligned(pointer);}

namespace {
using namespace halo2;
using Announcer=NativeVistaAnnouncer;
const std::array<float,sound_class_count> gains{};
const VistaAnnouncerLocalPlayer local{0x12340000u,9,0,0};
void need(bool condition,const char* message){if(!condition)throw FormatError(message);}
template<class F>void rejects(F&& call,const char* message){
  bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}need(rejected,message);
}
template<class F>void worker(F&& call){
  std::exception_ptr failure;std::thread thread([&]{try{call();}catch(...){failure=std::current_exception();}});
  thread.join();if(failure)std::rethrow_exception(failure);
}
bool same(const SdlSoundCompletion& a,const SdlSoundCompletion& b){
  return std::tie(a.voice,a.channel_context,a.packet_context,a.submission,a.status,a.pcm_frames)==
      std::tie(b.voice,b.channel_context,b.packet_context,b.submission,b.status,b.pcm_frames);
}
void equal(const NativeVistaAnnouncerSnapshot& a,const NativeVistaAnnouncerSnapshot& b){
  need(a.source.queue==b.source.queue && a.source.recent==b.source.recent &&
      a.game_over_published==b.game_over_published && a.pending_start_boundary==b.pending_start_boundary &&
      a.muted==b.muted && a.next_token==b.next_token && a.published_byte==b.published_byte,
      "Announcer restored incomplete source values, cursor or publication flags");
  need(a.voices.size()==b.voices.size() && a.completions.size()==b.completions.size(),
      "Announcer restored a different voice/report census");
  for(std::size_t i=0;i<a.voices.size();++i)
    need(a.voices[i].queue_token==b.voices[i].queue_token && a.voices[i].native_voice==b.voices[i].native_voice,
        "Announcer changed its queue-token/native-voice mapping");
  for(std::size_t i=0;i<a.completions.size();++i)
    need(a.completions[i].queue_token==b.completions[i].queue_token &&
        same(a.completions[i].packet,b.completions[i].packet),"Announcer changed historical completion reports");
}
double peak(std::span<const float> pcm){
  double result=0;for(const auto sample:pcm){need(std::isfinite(sample),"MCC announcer emitted nonfinite PCM");
    result=std::max(result,std::abs(double(sample)));}return result;
}
void drain_sentinel(Announcer& owner){
  for(unsigned i=0;i<62;++i)owner.pre_world_tick();
  need(owner.snapshot().source.queue.count==0,"Actual source sentinel did not retire by native ticks");
}
void activation(Announcer& owner,std::uint32_t tick){
  auto event=vista_announcer_event_construct(2,0);event.words[2]=local.datum;
  const auto selected=owner.publish_player_activation(event,1,false,tick,{&local,1});
  need(selected.size()==1 && selected[0].announcer && selected[0].requested &&
      selected[0].sound!=vista_announcer_none,"Genuine MCC activation did not queue its authored start cue");
}
void start_activation(Announcer& owner,std::uint32_t tick){
  activation(owner,tick);
  for(unsigned i=0;i<256 && owner.snapshot().voices.empty();++i)owner.pre_world_tick();
  need(owner.snapshot().voices.size()==1,"Actual authored start delay did not reach genuine playback");
}
void end(Announcer& owner,std::uint32_t tick){
  const auto selected=owner.post_world_tick(true,1,true,tick,{&local,1});
  need(selected.size()==1 && selected[0].announcer && selected[0].requested &&
      selected[0].sound!=vista_announcer_none,"Genuine MCC end event did not queue its authored cue");
}
struct Trace {
  std::vector<float> pcm;
  std::vector<NativeVistaAnnouncerSnapshot> states;
};
Trace finish(Announcer& owner){
  Trace trace;
  for(unsigned i=0;i<512;++i){
    auto pcm=owner.render_manual(4096);trace.pcm.insert(trace.pcm.end(),pcm.begin(),pcm.end());
    owner.audio_update(1,gains);owner.pre_world_tick();trace.states.push_back(owner.snapshot());
    const auto& state=trace.states.back();
    if(state.voices.empty() && !state.source.queue.count)return trace;
  }
  throw FormatError("Actual MCC announcer cues did not finish in bounded source/PCM trace");
}
void equal(const Trace& a,const Trace& b){
  need(a.pcm==b.pcm && a.states.size()==b.states.size(),"Restored MCC announcer PCM suffix or native-tick timing differs");
  for(std::size_t i=0;i<a.states.size();++i)equal(a.states[i],b.states[i]);
}
void publish(Announcer& owner,Announcer::PreparedReplacement& prepared){
  bool ready=false,committed=false;std::size_t allocated=0,released=0;
  {
    auto held=owner.hold_publication(prepared);
    watch_publication=true;allocations=releases=0;
    ready=owner.ready(prepared,held);committed=owner.commit(prepared,held);
    allocated=allocations;released=releases;watch_publication=false;
    need(!owner.ready(prepared,held) && !owner.commit(prepared,held),"Announcer published one preparation twice");
    rejects([&]{(void)owner.snapshot();},"Ordinary announcer observation entered a held publication");
  }
  need(ready && committed,"Authenticated MCC announcer preparation failed publication");
  need(!allocated && !released,"Announcer publication allocated or retired ownership under exclusion");
}
void active_restore(const NativeVistaAnnouncerConfiguration& config){
  auto owner=Announcer::open(config);owner->enroll_continuation();drain_sentinel(*owner);
  owner->audio_update(1,gains);start_activation(*owner,62);
  (void)owner->render_manual(64);end(*owner,63);
  const auto saved_state=owner->snapshot();
  need(saved_state.source.queue.count==2 && saved_state.source.queue.slots[0].active &&
      saved_state.voices.size()==1 && saved_state.completions.empty() && saved_state.next_token==1 &&
      saved_state.published_byte==1 && !saved_state.pending_start_boundary,
      "Active-plus-queued MCC cue checkpoint premise differs");
  const auto saved=owner->checkpoint();const auto expected=finish(*owner);
  need(peak(expected.pcm)>0,"Actual complete MCC cue suffix contains no nonzero mixed PCM");
  const auto first_retirement=std::find_if(expected.states.begin(),expected.states.end(),
      [](const auto& state){return !state.completions.empty();});
  need(first_retirement!=expected.states.end(),"Actual active MCC cue never retired");
  const auto active_samples=std::size_t(first_retirement-expected.states.begin()+1)*4096*2;
  need(peak(std::span<const float>(expected.pcm).first(active_samples))>0,
      "Captured active MCC cue had no actual nonzero PCM left before its own retirement");
  const auto completed=owner->snapshot();
  need(completed.completions.size()==2 && completed.next_token==2,"Actual source queue failed exact-once completion/cursor progression");
  for(const auto& completion:completed.completions)
    need(completion.packet.status==SdlSoundCompletionStatus::Success && completion.packet.pcm_frames,
        "MCC cue failed genuine successful PCM retirement");
  auto target=owner->prepare_restore(saved);auto sibling=owner->prepare_restore(saved);
  publish(*owner,target);equal(saved_state,owner->snapshot());
  bool stale=false;
  try{auto held=owner->hold_publication(sibling);stale=!owner->ready(sibling,held) && !owner->commit(sibling,held);}
  catch(const std::exception&){stale=true;}
  need(stale,"Sibling announcer preparation remained publishable in a new branch");
  equal(expected,finish(*owner));
  const auto before=owner->snapshot();owner->audio_update(1,gains);owner->pre_world_tick();owner->stop();
  equal(before,owner->snapshot());
  const auto historical=owner->checkpoint();
  start_activation(*owner,1000);(void)finish(*owner);
  need(owner->snapshot().completions.size()==3 && owner->snapshot().next_token==3,
      "Fresh postpublication playback failed exact cursor/report progression");
  auto historical_target=owner->prepare_restore(historical);publish(*owner,historical_target);
  equal(before,owner->snapshot());
  auto again=owner->prepare_restore(saved);publish(*owner,again);equal(expected,finish(*owner));
}
void refusal_and_legacy(const NativeVistaAnnouncerConfiguration& config){
  {
    auto legacy=Announcer::open(config);
    worker([&]{legacy->audio_update(1,gains);legacy->pre_world_tick();});
    rejects([&]{legacy->enroll_continuation();},"Foreign legacy producer history acquired continuation enrollment");
    worker([&]{legacy->audio_update(1,gains);});
  }
  auto owner=Announcer::open(config);owner->enroll_continuation();const auto before=owner->snapshot();
  rejects([&]{worker([&]{owner->audio_update(.1f,gains);});},"Enrolled announcer accepted a foreign audio producer");
  equal(before,owner->snapshot());const auto saved=owner->checkpoint();auto prepared=owner->prepare_restore(saved);
  owner->pre_world_tick();const auto advanced=owner->snapshot();bool refused=false;
  try{auto held=owner->hold_publication(prepared);refused=!owner->ready(prepared,held) && !owner->commit(prepared,held);}
  catch(const std::exception&){refused=true;}
  need(refused,"A changed announcer producer retained its old preparation");equal(advanced,owner->snapshot());
  auto cancelled=owner->prepare_restore(saved);
  {
    auto held=owner->hold_publication(cancelled);
    worker([&]{cancelled=Announcer::PreparedReplacement{};});
    need(!owner->ready(cancelled,held) && !owner->commit(cancelled,held),"Foreign cancelled wrapper published an announcer branch");
  }
  equal(advanced,owner->snapshot());
  auto foreign=Announcer::open(config);foreign->enroll_continuation();
  rejects([&]{(void)foreign->prepare_restore(saved);},"Foreign announcer accepted a numeric-looking owner image");
}
void muted(const NativeVistaAnnouncerConfiguration& original){
  auto config=original;config.mute=true;config.platform.device_name="must-not-open-muted-device";
  auto owner=Announcer::open(config);owner->enroll_continuation();drain_sentinel(*owner);end(*owner,62);
  const auto saved_state=owner->snapshot();const auto saved=owner->checkpoint();const auto expected=finish(*owner);
  need(peak(expected.pcm)==0 && owner->snapshot().completions.empty(),"Muted MCC announcer fabricated playback or reports");
  auto target=owner->prepare_restore(saved);publish(*owner,target);equal(saved_state,owner->snapshot());equal(expected,finish(*owner));
}
void orphan(const NativeVistaAnnouncerConfiguration& config){
  auto owner=Announcer::open(config);owner->enroll_continuation();drain_sentinel(*owner);
  start_activation(*owner,62);const auto saved=owner->checkpoint();
  auto prepared=owner->prepare_restore(saved);owner.reset();
  worker([&]{prepared=Announcer::PreparedReplacement{};});
}
}
int main(int argc,char** argv){try{
  need(argc==3 && std::string_view(argv[1])=="--mcc","Supply --mcc ABSOLUTE_IMPORTED_LOCKOUT_MAP");
  const std::filesystem::path map=argv[2];
  need(map.is_absolute() && std::filesystem::is_regular_file(map),"Explicit MCC map must exist at its absolute imported path");
  const auto cache=std::make_shared<const Cache>(map);need(cache->is_mcc(),"Announcer runtime assets must come from MCC import");
  NativeVistaAnnouncerConfiguration config{map.parent_path(),cache->scenario(),cache->original_data(),
      0,30,{0x027f,0x1f80},false,0x5821b36d,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}}};
  {
    auto owner=Announcer::open(config);
    need(owner->assets().cache->is_mcc() && &owner->audio_cache()==owner->assets().cache.get() &&
        owner->audio_cache().scenario()==cache->scenario(),"MCC announcer changed its actual scenario sound namespace");
  }
  active_restore(config);refusal_and_legacy(config);muted(config);orphan(config);
  std::cout<<"PASS actual MCC announcer continuation: active and queued source cues, exact PCM suffix, full source/maps/cursor/flags/reports, fresh completion branches, exact-once retirement, no-allocation publication, cancellation/sibling/foreign/legacy/mute/lifetime controls\n";
  return 0;
}catch(const std::exception& error){watch_publication=false;std::cerr<<error.what()<<'\n';return 1;}}
