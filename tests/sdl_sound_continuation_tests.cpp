#include "halo2/sdl_sound_platform.hpp"
#include <SDL.h>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <stdexcept>
#include <thread>
#include <tuple>
#ifdef __unix__
#include <sys/wait.h>
#include <unistd.h>
#endif

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
using Platform=SdlSoundPlatform;
void need(bool b,const char*message){if(!b)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected,"Required refusal was absent");}
SdlSoundPlatformConfiguration config(bool callback=false){return {callback?SdlSoundPlatformConfiguration::Delivery::DeviceCallback:SdlSoundPlatformConfiguration::Delivery::Manual,64,{}};}
SdlSoundVoiceParameters parameters(float rate=1,bool paused=false){return {std::bit_cast<std::uint32_t>(rate),0,paused,{{0,0,1},{1,1,1}}};}
std::shared_ptr<const std::vector<float>> pcm(){return std::make_shared<const std::vector<float>>(std::initializer_list<float>{0,0,.2f,.3f,.4f,.5f,.6f,.7f,.8f,.9f});}
bool same(const SdlSoundCompletion&a,const SdlSoundCompletion&b){return std::tie(a.voice,a.channel_context,a.packet_context,a.submission,a.status,a.pcm_frames)==std::tie(b.voice,b.channel_context,b.packet_context,b.submission,b.status,b.pcm_frames);}
SdlSoundVoiceParameters authored_parameters(bool paused=false){return {std::bit_cast<std::uint32_t>(.5f),-600,paused,{{0,1,.75f},{1,0,.5f}}};}
struct Setup {Platform::VoiceReceipt a,b,paused;};
Setup setup(Platform& p){
  auto data=pcm();auto a=p.create_voice_receipt(101,authored_parameters());
  need(p.submit(a,{data,0,2,201})==Platform::Submission::Accepted,"First slot not accepted");
  need(p.submit(a,{data,2,3,202})==Platform::Submission::Accepted,"Second slot not accepted");
  auto b=p.create_voice_receipt(102,parameters(),8);
  p.submit(b,{data,0,2,203});
  auto paused=p.create_voice_receipt(103,authored_parameters(true));p.submit(paused,{data,0,2,204});
  p.render(5);p.paused(true);p.render(7);
  need(p.rendered_frames()==12&&p.elapsed_frames()==12&&p.presentation_frames()==5,"Three-clock capture premise differs");
  need(p.status(a).completed_packets==1&&p.status(a).pending_packets==1,"Capture lost completed unacknowledged first slot");
  need(!p.playback_position(b).first_device_frame,"Future voice consumed before deadline");
  return {a,b,paused};
}
struct Suffix {std::vector<float> samples;std::vector<SdlSoundCompletion> completions;SdlSoundPlaybackPosition a,b;std::uint64_t elapsed{},presentation{};};
Suffix suffix(Platform& p,const Setup& s){
  Suffix out;need(p.status(s.paused).paused&&p.status(s.paused).pending_packets==1&&p.playback_position(s.paused).source_frames==0,"Historical per-voice pause was not restored");
  p.paused(false);out.samples=p.render(4);
  auto first=p.completion(s.a);need(bool(first),"Saved completed slot disappeared");out.completions.push_back(first->value());
  p.acknowledge(*first);rejects([&]{p.acknowledge(*first);});
  auto rest=p.render(6);out.samples.insert(out.samples.end(),rest.begin(),rest.end());
  for(const auto& voice:{s.a,s.b}){auto c=p.completion(voice);need(bool(c),"Suffix completion absent");out.completions.push_back(c->value());p.acknowledge(*c);}
  out.a=p.playback_position(s.a);out.b=p.playback_position(s.b);
  out.elapsed=p.elapsed_frames();out.presentation=p.presentation_frames();return out;
}
Suffix witness(){Platform p(config());const auto s=setup(p);return suffix(p,s);}
void exact_restore(){
  const auto expected=witness();Platform p(config());const auto s=setup(p);auto saved=p.checkpoint();
  const auto old_completion=p.completion(s.a);
  p.parameters(s.paused,parameters(2));p.paused(false);p.render(37);
  p.acknowledge(*old_completion);p.acknowledge(*p.completion(s.a));
  p.submit(s.a,{pcm(),0,1,205}); // Same voice advanced next_submission from3 to4.
  auto changed=parameters(1.5f,true);changed.volume_hundredths_db=-1200;changed.routes={{0,0,.25f},{1,1,.125f}};
  p.parameters(s.a,changed);p.parameters(s.b,parameters(2,true));
  auto future=p.create_voice_receipt(500,parameters());auto future_pcm=pcm();std::weak_ptr<const std::vector<float>> future_weak=future_pcm;
  p.submit(future,{future_pcm,0,1,501});future_pcm.reset();p.render(1);const auto future_completion=p.completion(future);
  Setup restored;std::uint64_t physical{};
  {
    auto prepared=p.prepare_restore(saved);p.render(9); // Actual callback/manual progress is not producer staleness.
    physical=p.rendered_frames();
    {
      auto held=p.hold_publication(prepared);const auto before=*held.clocks();
      need(p.ready(prepared,held),"Exact held preparation not ready");
      rejects([&]{p.create_voice_receipt(1,parameters());});rejects([&]{p.rendered_frames();});rejects([&]{p.checkpoint();});
      allocations=releases=0;watch_commit=true;const auto committed=p.commit(prepared,held);watch_commit=false;
      need(committed&&allocations==0&&releases==0,"Publication allocated or retired storage");
      const auto now=*held.clocks();need(now.physical==before.physical&&now.elapsed==12&&now.presentation==5&&now.publication_epoch==2,"Publication rewound physical clock or lost logical clocks");
      need(!p.commit(prepared,held),"Preparation committed twice");
      restored={*prepared.voice(s.a.id()),*prepared.voice(s.b.id()),*prepared.voice(s.paused.id())};
      need(!held.status(s.a)&&held.status(restored.a)->completed_packets==1,"Held witnesses admitted stale receipt or lost slot state");
      bool foreign_ready=true,foreign_commit=true;
      std::thread foreign([&]{foreign_ready=p.ready(prepared,held);foreign_commit=p.commit(prepared,held);});foreign.join();
      need(!foreign_ready&&!foreign_commit,"Foreign noexcept preflight/publication accepted");
    }
    need(p.rendered_frames()==physical&&!p.playback_position(restored.a).first_resumed_device_frame,"Commit invented physical resumed observation");
    need(!future_weak.expired(),"Consumed preparation dropped outgoing PCM too early");
    rejects([&]{p.parameters(s.a,parameters());});rejects([&]{p.acknowledge(*old_completion);});rejects([&]{p.acknowledge(*future_completion);});
    rejects([&]{p.create_voice(1,parameters());});rejects([&]{p.submit(s.a.id(),{pcm(),0,1,1});});
    rejects([&]{p.parameters(s.a.id(),parameters());});rejects([&]{p.acknowledge(s.a.id(),1);});
    rejects([&]{p.flush(s.a.id());});rejects([&]{p.destroy_voice(s.a.id());});
  }
  need(future_weak.expired(),"Outgoing PCM did not retire with consumed token");
  const auto actual=suffix(p,restored);
  need(actual.samples==expected.samples&&actual.completions.size()==expected.completions.size(),"Exact resumed PCM suffix differs");
  for(std::size_t i=0;i<actual.completions.size();++i)need(same(actual.completions[i],expected.completions[i]),"Submission/context/status suffix differs");
  need(actual.a.source_frames==expected.a.source_frames&&actual.a.output_frames==expected.a.output_frames&&actual.a.first_device_frame==expected.a.first_device_frame&&actual.a.first_presentation_frame==expected.a.first_presentation_frame,"Saved observed prefix or fractional cursor differs");
  need(actual.b.source_frames==expected.b.source_frames&&actual.b.output_frames==expected.b.output_frames&&actual.b.first_presentation_frame==8,"Scheduled logical deadline differs");
  need(actual.a.first_resumed_device_frame==physical&&actual.b.first_resumed_device_frame==physical+3&&actual.b.first_device_frame==physical+3,"Resumed physical first frames were not actual consumption");
  need(actual.elapsed==expected.elapsed&&actual.presentation==expected.presentation&&p.rendered_frames()==physical+10,"Logical suffix clocks differ");
  p.submit(restored.a,{pcm(),0,1,604});p.render(2);
  auto next=p.completion(restored.a);need(next&&next->value().submission==3,"Historical per-voice next_submission was not restored");p.acknowledge(*next);
  auto replacement=p.create_voice_receipt(600,parameters());need(replacement.id()==future.id(),"Logical next voice ID was not restored");
  p.submit(replacement,{pcm(),0,1,602});rejects([&]{p.flush(future);});
  const auto new_start=p.rendered_frames();
  need(p.playback_position(replacement).publication_epoch==2&&!p.playback_position(replacement).first_resumed_device_frame,"Fresh post-publication voice had stale epoch or invented observation");
  p.render(1);need(p.playback_position(replacement).publication_epoch==2&&p.playback_position(replacement).first_resumed_device_frame==new_start,"Fresh post-publication voice did not observe its actual first sample");auto c=p.completion(replacement);need(bool(c),"New typed voice unavailable after publication");p.acknowledge(*c);p.destroy_voice(replacement);
}
void prospective_siblings(){
  Platform p(config());auto voice=p.create_voice_receipt(1,parameters());p.submit(voice,{pcm(),0,2,2});auto cp=p.checkpoint();
  auto canceled=p.prepare_restore(cp),winner=p.prepare_restore(cp),competing=p.prepare_restore(cp);
  auto a=*canceled.voice(voice.id()),b=*winner.voice(voice.id()),c=*competing.voice(voice.id());
  need(winner.voices()==1&&!winner.voice(999),"Preparation target census was not exact");
  rejects([&]{p.status(a);});rejects([&]{p.submit(b,{pcm(),0,1,3});});rejects([&]{p.flush(c);});
  canceled={};
  {auto held=p.hold_publication(winner);need(!held.status(b),"Live witness falsely admitted prospective target");need(p.commit(winner,held)&&held.status(b).has_value(),"Prospective receipt did not become exact installed authority");}
  rejects([&]{p.flush(a);});rejects([&]{p.flush(c);});rejects([&]{p.hold_publication(competing);});
  p.render(2);p.acknowledge(*p.completion(b));p.destroy_voice(b);
}
void guards_and_retirement(){
  Platform::Checkpoint dead;
  {Platform owner(config());auto voice=owner.create_voice_receipt(1,parameters());owner.submit(voice,{pcm(),0,2,2});dead=owner.checkpoint();}
  Platform p(config());rejects([&]{p.prepare_restore(dead);});
  auto voice=p.create_voice_receipt(1,parameters());p.submit(voice,{pcm(),0,2,2});auto cp=p.checkpoint();
  auto stale=p.prepare_restore(cp);p.parameters(voice,parameters(.75f));rejects([&]{p.hold_publication(stale);});
  auto empty=Platform::PreparedReplacement{};rejects([&]{p.hold_publication(empty);});
  bool foreign=false;std::thread t([&]{try{p.flush(voice);}catch(const std::logic_error&){foreign=true;}});t.join();need(foreign,"Enrolled foreign mutation accepted");
  {auto prepared=p.prepare_restore(cp);auto held=p.hold_publication(prepared);std::thread cancel([q=std::move(prepared)]{});cancel.join();need(!p.ready(prepared,held)&&!p.commit(prepared,held),"Canceled wrapper still published");}
  auto layout=p.prepare_restore(cp);need(p.speaker_layout(0).empty(),"Same-channel layout selection failed");rejects([&]{p.hold_publication(layout);});rejects([&]{p.prepare_restore(cp);});
  cp=p.checkpoint();auto removed=p.prepare_restore(cp);need(!p.device_removed(p.device_id()+1),"Foreign device event accepted");need(p.device_removed(p.device_id()),"Actual removal not observed");rejects([&]{p.hold_publication(removed);});rejects([&]{p.prepare_restore(cp);});need(p.completion(voice)->value().status==SdlSoundCompletionStatus::Failure,"Removal invented successful completion");
}
void cancel_and_owner_death(){
  {
    Platform p(config());auto data=pcm();std::weak_ptr<const std::vector<float>> weak=data;
    auto voice=p.create_voice_receipt(1,parameters());p.submit(voice,{data,0,1,2});data.reset();auto cp=p.checkpoint();p.flush(voice);p.acknowledge(*p.completion(voice));p.destroy_voice(voice);
    {
      auto prepared=p.prepare_restore(cp);cp={};
      {auto held=p.hold_publication(prepared);std::thread cancel([q=std::move(prepared)]{});cancel.join();need(!weak.expired(),"Held cancellation released private target PCM under lock");}
    }
    need(weak.expired(),"Canceled private target PCM leaked");
  }
  {
    auto p=std::make_unique<Platform>(config());auto data=pcm();std::weak_ptr<const std::vector<float>> weak=data;
    auto voice=p->create_voice_receipt(1,parameters());p->submit(voice,{data,0,1,2});data.reset();auto cp=p->checkpoint();auto prepared=p->prepare_restore(cp);cp={};
    {auto held=p->hold_publication(prepared);p.reset();need(!held.clocks()&&!held.status(voice)&&!weak.expired(),"Owner death freed endpoint or preserved live readiness");prepared={};}
    need(weak.expired(),"Last held endpoint/PCM pin leaked after unlock");
  }
}
void pcm_owners(){
  Platform p(config());auto voice=p.create_voice_receipt(1,parameters());auto data=pcm();
  std::shared_ptr<const std::vector<float>> nonowning(std::shared_ptr<int>{},data.get());
  rejects([&]{p.submit(voice,{nonowning,0,1,1});});
  std::shared_ptr<const std::vector<float>> null_alias(data,nullptr);rejects([&]{p.submit(voice,{null_alias,0,1,1});});
  rejects([&]{p.submit(voice,{data,5,1,1});});
  struct Owner {std::shared_ptr<const std::vector<float>> pcm;};
  auto first=std::make_shared<Owner>(Owner{data}),second=std::make_shared<Owner>(Owner{data});
  std::weak_ptr<Owner> first_weak=first,second_weak=second;
  std::shared_ptr<const std::vector<float>> a(first,data.get()),b(second,data.get());
  need(a.get()==b.get()&&(a.owner_before(b)||b.owner_before(a)),"Distinct real PCM owners premise failed");
  p.submit(voice,{a,0,1,1});p.submit(voice,{b,1,1,2});auto cp=p.checkpoint();a.reset();b.reset();first.reset();second.reset();data.reset();
  p.render(3);p.acknowledge(*p.completion(voice));p.acknowledge(*p.completion(voice));p.destroy_voice(voice);
  need(!first_weak.expired()&&!second_weak.expired(),"Checkpoint interned distinct PCM owner identities");
  {auto prepared=p.prepare_restore(cp);cp={};{auto held=p.hold_publication(prepared);need(p.commit(prepared,held),"Distinct-owner restore failed");}auto restored=*prepared.voice(voice.id());p.render(2);p.acknowledge(*p.completion(restored));p.acknowledge(*p.completion(restored));p.destroy_voice(restored);}
  need(first_weak.expired()&&second_weak.expired(),"Final distinct PCM owners leaked");
}
void explicit_enrollment(){
  {
    Platform p(config());
    allocations=releases=0;watch_commit=true;
    p.enroll_continuation();p.enroll_continuation();
    watch_commit=false;
    need(allocations==0&&releases==0,"Explicit enrollment allocated or retired storage");
    bool refused=false;
    std::thread foreign([&]{try{(void)p.rendered_frames();}catch(const std::logic_error&){refused=true;}});
    foreign.join();need(refused,"Explicit enrollment did not bind the creating executor");
  }
  {
    Platform p(config());Platform::Voice voice{};bool refused=false;
    std::thread foreign([&]{
      try{p.enroll_continuation();}catch(const std::logic_error&){refused=true;}
      // Failed foreign enrollment must leave the real legacy producer usable.
      voice=p.create_voice(1,parameters());p.submit(voice,{pcm(),0,1,2});
    });foreign.join();need(refused,"Foreign explicit enrollment was admitted");
    rejects([&]{p.enroll_continuation();});
    p.render(1);const auto done=p.completion(voice);
    need(bool(done),"Failed enrollment damaged legacy packet playback");
    p.acknowledge(voice,done->submission);p.destroy_voice(voice);
  }
  {
    Platform p(config());need(p.device_removed(p.device_id()),"Device removal premise failed");
    rejects([&]{p.enroll_continuation();});
  }
}
void legacy_compatibility(){
  {
    Platform p(config());Platform::Voice voice{};std::thread worker([&]{voice=p.create_voice(1,parameters());p.submit(voice,{pcm(),0,1,2});});worker.join();
    p.render(1);auto c=p.completion(voice);need(bool(c),"Legacy worker submission lost");p.acknowledge(voice,c->submission);p.destroy_voice(voice);rejects([&]{p.checkpoint();});
  }
  {
    Platform p(config());auto data=pcm();std::shared_ptr<const std::vector<float>> unowned(std::shared_ptr<int>{},data.get());
    auto voice=p.create_voice(1,parameters());p.submit(voice,{unowned,0,1,2});rejects([&]{p.checkpoint();});p.flush(voice);p.acknowledge(voice,p.completion(voice)->submission);p.destroy_voice(voice);
  }
}
void legacy_removal_refusal(){
  Platform p(config(true));const auto device=p.device_id();bool refused=false;
  std::thread foreign([&]{try{p.device_removed(device);}catch(const std::logic_error&){refused=true;}});foreign.join();
  need(refused,"Pre-enrollment foreign endpoint removal was admitted");
  const auto before=p.rendered_frames();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(p.rendered_frames()==before&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(2));
  need(p.rendered_frames()>before,"Rejected foreign removal paused real callback");
  auto cp=p.checkpoint();auto prepared=p.prepare_restore(cp);auto held=p.hold_publication(prepared);
  need(p.ready(prepared,held),"Rejected foreign removal poisoned endpoint enrollment");
}
void callback_exclusion(){
  Platform p(config(true));p.paused(true);auto voice=p.create_voice_receipt(1,parameters());p.submit(voice,{pcm(),0,2,2});auto cp=p.checkpoint();auto prepared=p.prepare_restore(cp);
  const auto start=p.rendered_frames();auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(p.rendered_frames()==start&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(2));
  need(p.rendered_frames()>start,"Real dummy callback did not progress during prepare");
  {auto held=p.hold_publication(prepared);const auto before=*held.clocks();std::this_thread::sleep_for(std::chrono::milliseconds(25));const auto after=*held.clocks();need(after.physical==before.physical&&after.elapsed==before.elapsed&&after.presentation==before.presentation,"Held token did not exclude actual callback");need(p.ready(prepared,held)&&p.commit(prepared,held),"Callback progress made preparation stale");}
  auto restored=*prepared.voice(voice.id());need(!p.playback_position(restored).first_resumed_device_frame,"Paused callback invented resumed consumption");p.paused(false);
  deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(!p.completion(restored)&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(2));
  need(bool(p.completion(restored))&&bool(p.playback_position(restored).first_resumed_device_frame),"Real resumed callback did not observe PCM");
}
#ifdef __unix__
int foreign_held_parent(const char*program){
  const auto child=fork();need(child>=0,"fork failed");if(!child){execl(program,program,"--foreign-held-child",nullptr);_Exit(74);}
  int status{};need(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==73,"Foreign Held destruction did not fail fast at contract guard");return 0;
}
#endif
}
int main(int argc,char**argv)try{
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
#ifdef __unix__
  if(argc==2&&std::string(argv[1])=="--foreign-held-parent")return foreign_held_parent(argv[0]);
  if(argc==2&&std::string(argv[1])=="--foreign-held-child"){
    std::set_terminate([]{_Exit(73);});Platform p(config());auto cp=p.checkpoint();auto prepared=p.prepare_restore(cp);
    auto*held=new Platform::HeldPublication(p.hold_publication(prepared));std::thread foreign([&]{delete held;});foreign.join();return 75;
  }
#endif
  need(argc==1,"Unknown continuation test option");explicit_enrollment();exact_restore();prospective_siblings();guards_and_retirement();cancel_and_owner_death();pcm_owners();legacy_compatibility();legacy_removal_refusal();callback_exclusion();
  std::cout<<"SDL continuation: exact fractional/two-slot/deadline suffix, three clocks, epoch+record authority, held callback exclusion, once/cancel/stale/foreign/owner/device/PCM retirement PASS; no runtime adoption/physical audibility claim\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
