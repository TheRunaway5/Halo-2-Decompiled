#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/simulation_authority.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <iostream>
#include <tuple>
#include <SDL.h>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F fn,const char *why){bool no=false;try{fn();}catch(const std::exception &){no=true;}check(no,why);}
struct Program {
 ScriptProgram p;
 std::uint32_t node(ScriptNode n){auto i=std::uint32_t(p.nodes.size());n.salt=std::uint16_t(0xe000+i);p.nodes.push_back(n);return (std::uint32_t(n.salt)<<16)|i;}
 std::uint32_t lit(std::uint32_t b,unsigned t=8){return node({0,std::uint16_t(t),std::uint16_t(t),9,null_datum,0,b});}
 std::uint32_t real(float f){return lit(std::bit_cast<std::uint32_t>(f),6);}
 std::uint32_t global(){return node({0,8,8,13,null_datum,0,0});}
 std::uint32_t call(std::string n,std::vector<std::uint32_t> a={},unsigned t=4){auto at=std::uint32_t(p.strings.size());p.strings+=n;p.strings+='\0';for(unsigned i=0;i<a.size();++i)p.nodes[a[i]&65535u].next=i+1<a.size()?a[i+1]:null_datum;auto fn=node({0,0,2,9,a.empty()?null_datum:a[0],at,0});return node({0,0,std::uint16_t(t),8,null_datum,at,fn});}
 Program(){
  p.globals.push_back({"phase",8,lit(0)});
  auto root=call("begin",{
   call("sound_impulse_start",{lit(0xea12089d,32),lit(null_datum),real(1)}),
   call("set",{global(),lit(1)},8),
   // Declared native host player-alias input; marker comes from the actual
   // source player in source-backed mode, not a fabricated source object.
   call("sound_impulse_start",{lit(0xea13089e,32),lit(0),real(1)}),
   call("set",{global(),lit(2)},8),
   call("sound_impulse_stop",{lit(0xea12089d,32)}),
   call("sound_looping_start",{lit(0x12345678,35),lit(null_datum),real(1)}),
   call("sound_looping_set_alternate",{lit(0x12345678,35),lit(1,5)}),
   call("sound_looping_stop",{lit(0x12345678,35)}),
   call("set",{global(),lit(3)},8)});
  p.scripts.push_back({"main",0,4,root});
 }
};
using Seen=std::tuple<SoundEvent::Action,std::uint32_t,int>;
void run(Cache &cache,bool source_mode,const std::filesystem::path &xbe){
 auto c=cache.load_content(0,false);c.scripts=Program{}.p;
 // This is a bounded host-phase/guard fixture: real player/assets with no
 // automatically authored scene placements, not a full opening-scene boot.
 c.objects.clear();
 std::shared_ptr<CampaignSourceRuntime> source;
 if(source_mode)source=std::make_shared<CampaignSourceRuntime>(cache,xbe,0,1,0x12345678);
 Simulation sim(c,0,cache.spawns().front(),{},{},{},{},{},{},source);
 sim.cinematic_text_services([](auto){return true;},[](auto){return 1.f;});
 sim.request_checkpoint();sim.step({});check(sim.has_checkpoint(),"checkpoint was not captured");
 sim.request_revert();sim.step({});
 // This older queued Reset must precede the new impulse.
 std::vector<Seen> seen;unsigned diagnostics{},rng_writes{},nested_checks{},capture_checks{};
 std::uint32_t random=0x12345678;
 std::unique_ptr<MultiplayerAudio> audio;
 std::vector<MultiplayerAudioSubmission> packets;
 if(source_mode)audio=std::make_unique<MultiplayerAudio>(cache,random,
     SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}},
     [&](const auto &p){packets.push_back(p);},&random);
 std::vector<std::uint32_t> rng_before,rng_after;
 const Simulation::AuthorityDigest identity{std::byte{0x53},std::byte{0x15}};
 bool restored=false;
 {
  SourceScriptSoundAdmissionBindingTu15 bound(sim,[&](const SoundEvent &event){
   try {
    const int phase=int(sim.scripts().globals().at(0).number());
    seen.emplace_back(event.action,event.datum,phase);
    // Campaign/debug idle authority checkpoints are outside the supported
    // ONLINE scope. Require the active-call boundary's first-gate reason,
    // rather than accepting an unrelated qualification/saved-state failure.
    bool active_capture_rejected=false;
    try {(void)sim.authority_checkpoint(identity);}
    catch(const std::exception &e) {
      check(std::string(e.what())=="Native continuation requires a completed unfaulted authority boundary",
            "callback authority rejection did not come from active-call boundary");
      active_capture_rejected=true;
    }
    check(active_capture_rejected,"callback captured active Simulation authority");++capture_checks;
    if(event.action==SoundEvent::Reset){if(audio)audio->stop();if(restored)check(sim.checkpoint_restores()==2 && phase==0,"restore delivered before complete Simulation publication");return;}
    if(!source_mode || (event.action!=SoundEvent::Impulse && event.action!=SoundEvent::NonSpatialImpulse))return;
    auto &world=source->objects().tu15_world();auto &m=world.memory();
    const auto auxiliary=Bytes(m.view(0x4f3cd8u,4)).u32(0)+4u;
    random=Bytes(m.view(auxiliary,4)).u32(0);rng_before.push_back(random);
    // A real read-only render borrow must reject this same public RNG writer.
    world.inspect_render_memory([&](const auto &,const auto &){
      rejects([&]{world.write_source_call_input(auxiliary,std::as_bytes(std::span(&random,1)));},"render borrow admitted audio RNG publication");
    });++nested_checks;
    bool observed=false;const int guard_owner=1;
    source_runtime_register_write_observer(m,&guard_owner,[&](auto at,auto n){if(at==auxiliary&&n==4)observed=true;});
    struct Unbind {SourceRuntimeMemory &m;const void *id;~Unbind(){source_runtime_unregister_write_observer(m,id);}}unregister{m,&guard_owner};
    // Exact existing host-adapter request/RNG handoff, now inside script host.
    if(event.action==SoundEvent::Impulse)(void)audio->play_impulse_at(event.datum,event.position,event.gain);
    else (void)audio->play_impulse(event.datum,event.gain);
    world.write_source_call_input(auxiliary,std::as_bytes(std::span(&random,1)));
    check(observed && Bytes(world.memory().view(auxiliary,4)).u32(0)==random,"audio RNG bypassed live bank observer/public writer");
    ++rng_writes;rng_after.push_back(random);
   } catch(const std::exception &e){++diagnostics;std::cerr<<"Audio fixture diagnostic: "<<e.what()<<'\n';}
  });
  rejects([&]{SourceScriptSoundAdmissionBindingTu15 duplicate(sim,[](const auto &){});},"duplicate callback replaced live owner");
  sim.start_level();sim.step({});
  check(sim.scripts().diagnostics().empty(),"source host phase faulted script");
  check(seen.size()==7,"audio was deferred beyond host call or queue event omitted");
  check(seen[0]==Seen{SoundEvent::Reset,null_datum,0} &&
        seen[1]==Seen{SoundEvent::NonSpatialImpulse,0xea12089d,0} &&
        seen[2]==Seen{SoundEvent::Impulse,0xea13089e,1} &&
        seen[3]==Seen{SoundEvent::ImpulseStop,0xea12089d,2} &&
        seen[4]==Seen{SoundEvent::LoopStart,0x12345678,2} &&
        seen[5]==Seen{SoundEvent::LoopAlternate,0x12345678,2} &&
        seen[6]==Seen{SoundEvent::LoopStop,0x12345678,2},"script admission overtook pending reset/phase/FIFO order");
  check(sim.take_sounds().empty(),"immediate event also queued for duplicate delivery");
  check(sim.scripts().globals().at(0).number()==3,"script did not resume after synchronous sound host return");
  if(source_mode){check(rng_writes==2&&nested_checks==2&&packets.size()==2,"actual audio/RNG/public guard controls incomplete");check(rng_before[1]==rng_after[0],"later source request did not observe prior immediate auxiliary RNG publication");}
  restored=true;sim.request_revert();sim.step({});
  check(seen.size()==8 && std::get<0>(seen.back())==SoundEvent::Reset,"restored reset was delayed or omitted");
 }
 check(diagnostics==0,"host audio/guard diagnostic occurred");
 const auto count=seen.size();sim.start_level();sim.step({});
 check(seen.size()==count,"destroyed binding retained its callback");
 const auto queue=sim.take_sounds();check(queue.size()==6 && queue[0].action==SoundEvent::NonSpatialImpulse && queue[1].action==SoundEvent::Impulse && queue[2].action==SoundEvent::ImpulseStop,"headless/unbound queue contract changed");
 // Reentrant FIFO and exceptional callback lifetime use the real owner identity
 // and the same production queue-drain helper without invoking game code twice.
 std::vector<SoundEvent> pending{{1,{},1,SoundEvent::Impulse},{2,{},1,SoundEvent::Impulse}};
 std::vector<unsigned> order;
 {SourceScriptSoundAdmissionBindingTu15 bound(sim,[&](const auto &e){order.push_back(e.datum);if(e.datum==1){pending.push_back({3,{}});source_admit_queued_script_sounds_tu15(sim,pending);}});source_admit_queued_script_sounds_tu15(sim,pending);}
 check(order==std::vector<unsigned>{1,2,3},"reentrant source admission reordered the existing queue");
 pending={{4,{}},{5,{}}};bool once=true;
 {SourceScriptSoundAdmissionBindingTu15 bound(sim,[&](const auto &e){if(once){once=false;throw FormatError("declared host interruption");}order.push_back(e.datum);});rejects([&]{source_admit_queued_script_sounds_tu15(sim,pending);},"declared callback interruption absent");source_admit_queued_script_sounds_tu15(sim,pending);}
 check(pending.empty()&&order.back()==5,"callback unwind retained draining state or lost later queued event");
 pending={{6,{}},{7,{}}};unsigned removed_calls{};
 std::unique_ptr<SourceScriptSoundAdmissionBindingTu15> removable;
 removable=std::make_unique<SourceScriptSoundAdmissionBindingTu15>(sim,[&](const auto &){++removed_calls;removable.reset();});
 source_admit_queued_script_sounds_tu15(sim,pending);
 check(removed_calls==1 && pending.size()==1 && pending[0].datum==7,"removed binding consumed a later queued event");
 {SourceScriptSoundAdmissionBindingTu15 replacement(sim,[&](const auto &e){check(e.datum==7,"replacement received wrong queued event");++removed_calls;});source_admit_queued_script_sounds_tu15(sim,pending);}
 check(removed_calls==2 && pending.empty(),"replacement binding did not retain the remaining queue contract");
 std::cout<<"PASS "<<(source_mode?"source-backed":"native")<<" Simulation host FIFO/start/stop/reset/restore/absent binding/reentrancy/unwind; capture_checks="<<capture_checks<<" rng_writes="<<rng_writes<<" real_render_guards="<<nested_checks<<'\n';
}
}
int main(int argc,char **argv)try {
 if(argc!=4)throw FormatError("TU15_XBE OPENING_MAP native|source");
 if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
 SDL_setenv("SDL_AUDIODRIVER","dummy",1);
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
 const std::string mode=argv[3];check(mode=="native"||mode=="source","unknown mode");run(cache,mode=="source",argv[1]);
 std::cout<<"LIMIT Native adapter request phase only; original16-row/voice pool/language-time semantics and listener/Xbox DSP parity remain open.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
