#include "halo2/effect_sound.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <array>
#include <bit>
#include <iostream>
#include <optional>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F &&run){
  bool rejected{};try{run();}catch(const FormatError&){rejected=true;}
  check(rejected,"Invalid pending effect sound operation accepted");
}
struct Submitted {
  EffectSoundCue cue;
  std::uint32_t owner;
  Vec3 position;
};
bool same(const Submitted &a,const Submitted &b){
  const auto bits=[](float value){return std::bit_cast<std::uint32_t>(value);};
  return bits(a.cue.elapsed)==bits(b.cue.elapsed)&&bits(a.cue.gain)==bits(b.cue.gain)&&
      a.cue.sound==b.cue.sound&&a.cue.event==b.cue.event&&
      a.cue.scale_a==b.cue.scale_a&&a.cue.scale_b==b.cue.scale_b&&a.owner==b.owner&&
      bits(a.position.x)==bits(b.position.x)&&bits(a.position.y)==bits(b.position.y)&&
      bits(a.position.z)==bits(b.position.z);
}
}
int main(int argc,char **argv)try {
  if(argc!=3)throw FormatError("Supply MAP SHARED");
  if(!std::filesystem::exists(argv[1]))return 77;
  Cache cache(argv[1],argv[2]);
  // Literal Lockout upper/lower and brute upper/lower firing references.
  // All four event0 sound parts point to the same actual upper-fire bank.
  const std::array effects{0xe4e0036bu,0xe4e4036fu,0xec380ab3u,0xec390ab4u};
  unsigned forwarded{};
  for(auto datum:effects) {
    const auto tag=cache.load_source_tag_metadata(datum);
    const Bytes root(tag.bytes_at(tag.source_address(),0x1c));
    const Bytes events(tag.bytes_at(root.u32(0x18),root.u32(0x14)*0x38));
    if(events.u32(0x1c)<tag.source_address())++forwarded;
    const auto cues=fixed_effect_sound_cues(cache,datum);
    if(cues.size()!=1 || cues[0].sound!=0xe4e1036cu || cues[0].elapsed!=0 ||
        cues[0].gain!=1 || cues[0].event!=0)
      throw FormatError("Actual shared plasma firing sound metadata differs");
    const auto muzzle=cache.load_muzzle_effect(datum);
    if(muzzle.sounds.size()!=1 || muzzle.sounds[0].sound!=cues[0].sound || muzzle.systems.empty())
      throw FormatError("Shared firing sound prevented actual authored muzzle admission");
  }
  if(forwarded!=3)throw FormatError("Actual cross-tag firing part controls missing");
  {
    NativeEffectSounds sounds(cache),other(cache);
    NativeEffectSounds::Checkpoint absent;
    check(!absent.valid()&&!sounds.owns_checkpoint(absent),"Empty cue checkpoint acquired an owner");
    rejects([&]{(void)absent.pending_count();});
    rejects([&]{(void)sounds.prepare_restore(absent);});
    // Literal Lockout fusion-coil cue has a nonzero authored delay. Capture
    // its partly elapsed host record beside a time-zero plasma firing part.
    constexpr std::uint32_t fusion=0xe70a0595u;
    constexpr std::uint32_t first_owner=0x12340001u,second_owner=0xabcd0001u,third_owner=0x87650002u;
    const Vec3 first_position{1.25f,-2.f,3.5f},second_position{-4.f,5.25f,-6.f},third_position{7.f,8.f,-9.5f};
    auto first_cues=fixed_effect_sound_cues(cache,fusion,.25f,.75f);
    auto second_cues=fixed_effect_sound_cues(cache,fusion,.5f,.125f);
    check(first_cues.size()==1&&second_cues.size()==1&&first_cues[0].sound==0xe70f059au&&
        std::bit_cast<std::uint32_t>(first_cues[0].elapsed)==0x3d088889u,
        "Actual delayed Lockout fusion-coil cue changed");
    sounds.spawn(fusion,first_owner,first_position,.25f,.75f);
    sounds.spawn(fusion,second_owner,second_position,.5f,.125f);
    std::vector<Submitted> submitted;
    const auto submit=[&](const EffectSoundCue &cue,std::uint32_t owner,Vec3 position){
      submitted.push_back({cue,owner,position});
    };
    sounds.advance(.01f,submit);check(submitted.empty(),"Partly elapsed authored cues fired early");
    sounds.spawn(effects[0],third_owner,third_position);
    const auto checkpoint=sounds.checkpoint();
    const auto copied=checkpoint;
    check(checkpoint.valid()&&checkpoint.pending_count()==3&&sounds.owns_checkpoint(checkpoint),
        "Cue checkpoint lost its actual pending records");
    {
      auto cancelled=sounds.prepare_restore(checkpoint);
      check(sounds.restore_ready(cancelled),"Fresh pending cue restore is not ready");
      (void)sounds.checkpoint();
      check(sounds.restore_ready(cancelled),"Read-only cue capture invalidated preparation");
    }
    check(submitted.empty()&&sounds.checkpoint().pending_count()==3,
        "Cancelling cue restore changed current pending cues or invoked submission");
    sounds.advance(.125f,submit);
    check(submitted.size()==3&&sounds.checkpoint().pending_count()==0,
        "Current authored cues failed ordinary submission before restore");
    first_cues[0].elapsed-=.01f;first_cues[0].elapsed-=.125f;
    second_cues[0].elapsed-=.01f;second_cues[0].elapsed-=.125f;
    auto immediate=fixed_effect_sound_cues(cache,effects[0]);immediate[0].elapsed-=.125f;
    const std::array expected{Submitted{first_cues[0],first_owner,first_position},
        Submitted{second_cues[0],second_owner,second_position},Submitted{immediate[0],third_owner,third_position}};
    for(unsigned n=0;n<expected.size();++n)
      check(same(submitted[n],expected[n]),"Ordinary authored cue lost exact elapsed/gain/event/salt/position");
    const auto first_submission=submitted;
    sounds.spawn(effects[1],0x55550002u,{10,11,12});
    std::optional<NativeEffectSounds::PreparedRestore> admitted{sounds.prepare_restore(copied)};
    check(!other.owns_checkpoint(copied),"Cue checkpoint crossed owners sharing one cache");
    rejects([&]{(void)other.prepare_restore(copied);});
    auto foreign_owner=other.prepare_restore(other.checkpoint());
    check(!sounds.restore_ready(foreign_owner)&&!sounds.commit_restore(std::move(foreign_owner)),
        "Cue preparation crossed its original owner");
    bool foreign_checkpoint_rejected{},foreign_prepare_rejected{},foreign_publication_rejected{};
    std::thread foreign([&]{
      try{(void)sounds.checkpoint();}catch(const FormatError&){foreign_checkpoint_rejected=true;}
      try{(void)sounds.prepare_restore(copied);}catch(const FormatError&){foreign_prepare_rejected=true;}
      foreign_publication_rejected=!sounds.owns_checkpoint(copied)&&!sounds.restore_ready(*admitted)&&
          !sounds.commit_restore(std::move(*admitted));
    });foreign.join();
    check(foreign_checkpoint_rejected&&foreign_prepare_rejected&&foreign_publication_rejected&&
        sounds.restore_ready(*admitted),"Foreign executor consumed or admitted pending cue state");
    const auto callbacks_before_commit=submitted.size();
    check(sounds.commit_restore(std::move(*admitted)),"Pending cue publication failed");
    check(submitted.size()==callbacks_before_commit&&sounds.checkpoint().pending_count()==3,
        "Pending cue publication submitted audio or lost restored records");
    check(!sounds.restore_ready(*admitted)&&!sounds.commit_restore(std::move(*admitted)),
        "Consumed pending cue preparation replayed");
    admitted.reset();
    check(submitted.size()==callbacks_before_commit,"Retired pending storage invoked a sound callback");
    sounds.advance(.125f,submit);
    check(submitted.size()==6,"Restored authored pending cues did not submit exactly once");
    for(unsigned n=0;n<first_submission.size();++n)
      check(same(submitted[n+3],first_submission[n]),
          "Pending cue checkpoint failed exact elapsed/gain/event/full-salt/position round trip");
    check(checkpoint.pending_count()==3&&copied.pending_count()==3&&sounds.owns_checkpoint(copied),
        "Later publication or advance mutated an older immutable cue checkpoint");
    const auto stale=[&](auto &prepared){
      check(!sounds.restore_ready(prepared)&&!sounds.commit_restore(std::move(prepared)),
          "Pending cue restore accepted an intervening mutation");
    };
    auto spawned=sounds.prepare_restore(copied);
    sounds.spawn(fusion,0x99990003u,{-1,-2,-3});stale(spawned);
    auto advanced=sounds.prepare_restore(copied);
    sounds.advance(.001f,submit);stale(advanced);
    auto rejected_advance=sounds.prepare_restore(copied);
    rejects([&]{sounds.advance(-1,submit);});stale(rejected_advance);
    auto rejected_spawn=sounds.prepare_restore(copied);
    sounds.spawn(0xffffffffu,0xffffffffu,{});stale(rejected_spawn);
    sounds.spawn(effects[2],0xaaaa0003u,{-4,-5,-6});
    auto failed_advance=sounds.prepare_restore(copied);
    unsigned callback_attempts{};
    rejects([&]{sounds.advance(0,[&](const auto&,auto,auto){
      ++callback_attempts;throw FormatError("Declared failing pending sound submit control");
    });});
    stale(failed_advance);
    check(callback_attempts==1&&sounds.checkpoint().pending_count()==2,
        "Failed advance changed its existing pending-cue retry semantics");
    auto retired=sounds.prepare_restore(copied);sounds.retire_pending();stale(retired);
    auto empty_retired=sounds.prepare_restore(copied);sounds.retire_pending();stale(empty_retired);
    check(sounds.checkpoint().pending_count()==0&&submitted.size()==6&&checkpoint.pending_count()==3,
        "Rejected/stale cue restoration changed callbacks or immutable pending state");
  }
  std::cout<<"PASS four literal plasma firing effects, three cross-tag part banks, exact sound/event/gain and authored FP systems\n";
  std::cout<<"PASS exact pending effect sound checkpoint: authored delayed/time-zero cues, complete salted owners/positions, cancellation, stale mutations, foreign executor/owner, single-use callback-free publication; playing voices remain separate\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
