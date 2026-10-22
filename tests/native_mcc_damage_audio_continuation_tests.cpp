#include "halo2/unit_damage_audio.hpp"
#include "halo2/cache.hpp"
#include "halo2/effect_presentation_continuity.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_authority_codec.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>

namespace {
using namespace halo2;
constexpr auto none=0xffffffffu;
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
// These are declared adapter operands, not native player/object admission.
constexpr std::array<std::uint32_t,4> units{0x6d040000u,0x6d040001u,0x6d040002u,0x6d040003u};
void need(bool condition,const char* message){if(!condition)throw FormatError(message);}
template<class F>void rejects(F&& call,const char* message){
  bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}need(rejected,message);
}
template<class F>void worker(F&& call){
  std::exception_ptr failure;std::thread thread([&]{try{call();}catch(...){failure=std::current_exception();}});
  thread.join();if(failure)std::rethrow_exception(failure);
}
double peak(std::span<const float> pcm){
  double value=0;for(const auto sample:pcm){need(std::isfinite(sample),"Damage audio emitted nonfinite actual MCC PCM");
    value=std::max(value,std::abs(double(sample)));}return value;
}
struct DamageOwner {std::unique_ptr<NativeUnitDamageAudio> damage;};
struct Fixture {
  std::shared_ptr<const Cache> cache;
  const LevelContent& content;
  std::uint32_t auxiliary=0x5821b36du;
  std::vector<MultiplayerAudioSubmission> submitted;
  bool destroy_on_packet=false;
  MultiplayerAudioKind destroy_kind=MultiplayerAudioKind::Shield;
  unsigned destroyed_packets=0;
  std::weak_ptr<DamageOwner> destruction_target;
  MultiplayerAudio audio;
  // Destruction precedes Audio teardown; Cache and auxiliary outlive both.
  std::shared_ptr<DamageOwner> owner=std::make_shared<DamageOwner>();
  Fixture(std::shared_ptr<const Cache> c,const LevelContent& world,bool shared=true)
      :cache(std::move(c)),content(world),audio(cache,0x5821b36d,manual,[this](const auto& packet){
        submitted.push_back(packet);
        if(destroy_on_packet && packet.kind==destroy_kind){
          destroy_on_packet=false;++destroyed_packets;
          if(const auto target=destruction_target.lock())target->damage.reset();
        }
      },shared?&auxiliary:nullptr){
    destruction_target=owner;audio.enroll_continuation();
    const std::array<Vec3,4> listeners{{{0,0,0},{1,0,0},{2,0,0},{3,0,0}}};audio.listeners(listeners);
    owner->damage=std::make_unique<NativeUnitDamageAudio>(*cache,audio,auxiliary);
    owner->damage->enroll_continuation();
  }
  NativeUnitDamageAudio& damage(){need(bool(owner->damage),"Damage wrapper unexpectedly closed");return *owner->damage;}
  const BipedDefinition& biped(unsigned view)const{return content.player_representations.at(2+(view&1)).biped;}
  UnitDamageVoiceEvent report(unsigned view,std::uint64_t tick,bool hard=false)const {
    return {tick*4+view,tick,units.at(view),content.weapons.at(cache->slayer_starting_weapon(5)).damage,
        hard?1u:0u,{float(view),0,0},1,0,1};
  }
  void birth(std::uint64_t tick=1){
    damage().advance(tick-1);
    for(unsigned view=0;view<4;++view)damage().hud(view,none,true,0,0,0,false,view&1);
    damage().advance(tick);
    for(unsigned view=0;view<4;++view)damage().hud(view,units[view],false,1,1,70,false,view&1);
    need(audio.voices()==0,"Four local births fabricated authored damage/shield audio");
  }
  void active(std::uint64_t tick=2){
    birth(tick-1);damage().advance(tick);
    for(unsigned view=0;view<4;++view){
      damage().hud(view,units[view],false,1,0,70,false,view&1);
      // The verified hard-report request has an authored cue for both actual
      // species. The separate ordinary-report control preserves authored NONE.
      damage().damage(biped(view),report(view,tick,true));
    }
    for(unsigned view=0;view<4;++view){
      need(std::any_of(submitted.begin(),submitted.end(),[&](const auto& s){
        return s.kind==MultiplayerAudioKind::Shield && s.owner==units[view] && s.view==view;}),
        "Actual authored shield requests did not cover all four local views");
      need(std::any_of(submitted.begin(),submitted.end(),[&](const auto& s){
        return s.kind==MultiplayerAudioKind::DamageVoice && s.owner==units[view] && s.position &&
            s.position->x==float(view);}),"Actual authored damage requests did not cover both species/four local units");
    }
    need(audio.voices()>0,"Actual four-view damage/shield cut contains no playing PCM owner");
  }
  double drain(){
    double mixed=0;
    for(unsigned i=0;i<512 && audio.voices();++i){mixed=std::max(mixed,peak(audio.render(4096)));audio.update();}
    need(audio.voices()==0,"Damage-owned PCM failed bounded genuine completion/fade retirement");return mixed;
  }
};
void stale_hud_handle(const std::shared_ptr<const Cache>& cache,const LevelContent& content){
  Fixture f(cache,content);f.birth();f.damage().advance(2);
  f.damage().hud(0,units[0],false,1,.75f,70,false,false);
  need(f.audio.voices()>0,"Actual HUD damage impulse was absent");
  need(f.drain()>0,"Actual HUD damage impulse had no finite/nonzero PCM");
  const auto previous=f.submitted.size();
  // The adapter still retains the completed HUD handle. A second pulse must
  // query that exact retired receipt and start its next real cue without
  // passing a stale handle to the strict typed stop mutator.
  f.damage().hud(0,units[0],false,1,.75f,70,false,false);
  need(f.submitted.size()>previous && f.audio.voices()>0,"Completed HUD handle prevented its next authored pulse");
  need(f.drain()>0,"Repeated authored HUD pulse lost its actual PCM");
}
void packet_owner_destruction(const std::shared_ptr<const Cache>& cache,const LevelContent& content){
  for(unsigned path=0;path<3;++path){
    Fixture f(cache,content);f.birth();f.damage().advance(2);
    f.destroy_kind=path==1?MultiplayerAudioKind::DamageVoice:MultiplayerAudioKind::Shield;
    f.destroy_on_packet=true;
    try{
      rejects([&]{
        if(path==1)f.damage().damage(f.biped(0),f.report(0,2,true));
        else if(path==2)f.damage().hud(1,units[1],false,1,1,70,true,true);
        else f.damage().hud(0,units[0],false,1,.75f,70,false,false);
      },"Actual packet observer destruction failed to close the active Damage operation");
    }catch(const std::exception& error){
      throw FormatError("Damage observer path="+std::to_string(path)+
          " observed_packets="+std::to_string(f.destroyed_packets)+
          " wrapper_alive="+std::to_string(bool(f.owner->damage))+": "+error.what());
    }
    need(f.destroyed_packets==1 && !f.owner->damage,
        "Destruction control never reached its actual authored Core packet observer");
    (void)f.drain();
    need(peak(f.audio.render(4096))==0,"Destroyed Damage wrapper leaked new PCM into following output");
  }
}
void absent_soft_damage_cue(const std::shared_ptr<const Cache>& cache,const LevelContent& content){
  for(unsigned view=0;view<2;++view){
    Fixture f(cache,content);f.birth();f.damage().advance(2);
    f.damage().damage(f.biped(view),f.report(view,2));
    need(std::none_of(f.submitted.begin(),f.submitted.end(),[](const auto& packet){
      return packet.kind==MultiplayerAudioKind::DamageVoice;
    }) && f.audio.voices()==0,
        "The observed ordinary-report authored silence was replaced with a fabricated voice");
  }
}
void auxiliary_refusals(const std::shared_ptr<const Cache>& cache,const LevelContent& content){
  Fixture unbound(cache,content,false);
  const auto damage=unbound.damage().checkpoint();const auto core=unbound.audio.checkpoint();
  auto prepared=unbound.audio.prepare_restore(core);
  rejects([&]{(void)unbound.damage().prepare_restore(damage,prepared);},
      "Unbound Core masqueraded as the Damage shared-auxiliary owner");
  Fixture shared(cache,content);const auto shared_core=shared.audio.checkpoint();
  rejects([&]{(void)shared.audio.prepare_restore(shared_core);},
      "Shared auxiliary restoration accepted a caller without its genuine effects writer agreement");
  rejects([&]{worker([&]{(void)shared.damage().checkpoint();});},"Damage capture crossed its enrolled producer executor");
  rejects([&]{worker([&]{shared.damage().advance(1);});},"Damage mutation crossed its enrolled producer executor");
}
struct NativeEffects {
  MultiplayerPreparedWorld& native;
  Fixture& fixture;
  std::uint32_t primary=0x12345678u;
  SourceEffectRandom random{primary,fixture.auxiliary};
  Mesh mesh;
  NativeWorldEffects world;
  NativeMuzzleEffects muzzle;
  SourceMapEffects map;
  SourceCombatAttachments combat;
  EffectPresentationContinuity effects;
  SourceMatchAuthority authority;
  AuthorityTransferContext context;
  NativeEffects(MultiplayerPreparedWorld& n,Fixture& f)
      :native(n),fixture(f),world(*f.cache,mesh,n.match.simulation().collision(),random),
       muzzle(*f.cache,random),map(*f.cache,n.content.objects,world),combat(*f.cache,world,{}),
       effects(muzzle,world,map,combat,random),authority(n.match,*n.source_objects){
    // Local context metadata binds an actually encoded native checkpoint. It
    // supplies no peer authentication, election, transport or host readiness.
    context.host=0;context.peers=1;context.epoch=2;context.membership_revision=1;
    context.session[0]=std::byte{0x6d};
  }
  struct Cut {
    SourceMatchAuthority::Checkpoint native;
    ValidatedAuthorityCut seal;
    EffectPresentationContinuity::Checkpoint effects;
    MultiplayerAudio::Checkpoint audio;
    NativeUnitDamageAudio::Checkpoint damage;
    std::uint32_t auxiliary{};
  };
  Cut capture(){
    const auto tick=native.match.simulation().ticks();
    world.advance(tick,tick_seconds);muzzle.advance(std::uint32_t(tick),tick_seconds);
    const auto binding=source_authority_binding(context);
    auto body=authority.checkpoint(binding);const auto encoded=encode_source_authority(body);
    ValidatedAuthorityCut seal;seal.peer=0;seal.context=context;seal.seal.id=1;
    seal.seal.tick=tick;seal.seal.bytes=encoded.size();
    SHA256(reinterpret_cast<const unsigned char*>(encoded.data()),encoded.size(),
        reinterpret_cast<unsigned char*>(seal.seal.digest.data()));
    auto e=effects.checkpoint(seal);auto a=fixture.audio.checkpoint();auto d=fixture.damage().checkpoint();
    return {std::move(body),seal,std::move(e),std::move(a),std::move(d),fixture.auxiliary};
  }
  void publish(const Cut& cut){
    auto body=authority.prepare(cut.native,source_authority_binding(context));
    auto e=effects.prepare_restore(cut.effects,cut.seal,{},{});
    const auto agreement=effects.audio_auxiliary(e,cut.seal);
    auto a=fixture.audio.prepare_restore(cut.audio,agreement);
    auto d=fixture.damage().prepare_restore(cut.damage,a);
    need(effects.restore_ready(e,cut.seal) && agreement.ready(&fixture.auxiliary,cut.auxiliary) &&
        !agreement.committed(&fixture.auxiliary,cut.auxiliary) && fixture.damage().ready(d,a),
        "Actual Effects/Core/Damage preparations failed their complete preflight");
    // This local fixture installs the actual encoded native body first. The
    // following publication qualifies Effects+playing Audio+Damage only; it
    // does not claim a complete foreground/native atomic runtime transaction.
    need(authority.commit(std::move(body)),"Actual MCC native cut failed its genuine prepared restore");
    bool core_ready=false,effects_ready=false,damage_ready=false,effects_committed=false;
    bool intent_reserved=false,core_committed=false,damage_committed=false;
    {
      auto held=fixture.audio.hold_publication(a);
      core_ready=fixture.audio.ready(a,held);effects_ready=effects.restore_ready(e,cut.seal);
      damage_ready=fixture.damage().ready(d,a);
      if(core_ready && effects_ready && damage_ready){
        // Reserve both script-cancellation domains before the first owner
        // swaps. Foreign wrapper cancellation can then only queue a request
        // for the installed producer, without changing the sealed image.
        intent_reserved=fixture.audio.begin_commit(a,held);
        if(intent_reserved){
          effects_committed=effects.commit_restore(std::move(e),cut.seal);
          if(effects_committed)core_committed=fixture.audio.commit(a,held);
          if(core_committed)damage_committed=fixture.damage().commit(d,a);
        }
      }
    }
    need(core_ready && effects_ready && damage_ready && intent_reserved && effects_committed && core_committed && damage_committed,
        "Actual Effects writer/Core/Damage publication failed after preflight");
    need(agreement.committed(&fixture.auxiliary,cut.auxiliary) && fixture.auxiliary==cut.auxiliary,
        "Playing Audio publication lacked the real Effects auxiliary write");
    // Retire plain outgoing images now, while target voices reuse saved IDs.
    // Their destruction must not call stop_voice on the freshly installed bank.
    d=NativeUnitDamageAudio::PreparedReplacement{};a=MultiplayerAudio::PreparedReplacement{};
  }
};
struct DamageTrace {
  std::vector<float> pcm;
  std::vector<std::size_t> voices;
  std::vector<MultiplayerAudioSubmission> submitted;
  std::uint32_t auxiliary{};
};
DamageTrace suffix(Fixture& f,std::uint64_t tick){
  DamageTrace trace;const auto first=f.submitted.size();
  for(unsigned step=0;step<96;++step){
    f.damage().advance(tick+step+1);
    for(unsigned view=0;view<4;++view){
      if(step<4)f.damage().hud(view,units[view],false,1,0,70,false,view&1);
      else if(step<8)f.damage().hud(view,units[view],false,1,.5f,70,true,view&1);
      else if(step<32)f.damage().hud(view,units[view],false,1,1,70,false,view&1);
      else f.damage().hud(view,none,true,0,0,0,false,view&1);
      if(step==8)f.damage().damage(f.biped(view),f.report(view,tick+step+1,true));
      if(step==24 || step==32)f.damage().remove(units[view]);
    }
    auto pcm=f.audio.render(4096);trace.pcm.insert(trace.pcm.end(),pcm.begin(),pcm.end());
    f.audio.update();trace.voices.push_back(f.audio.voices());
  }
  need(f.audio.voices()==0,"Actual suffix left HUD/damage PCM after absent-view retirement and fades");
  trace.submitted.assign(f.submitted.begin()+std::ptrdiff_t(first),f.submitted.end());
  trace.auxiliary=f.auxiliary;return trace;
}
void equal(const DamageTrace& a,const DamageTrace& b){
  need(a.pcm==b.pcm && a.voices==b.voices && a.auxiliary==b.auxiliary && a.submitted.size()==b.submitted.size(),
      "Four-view Damage/Core/Effects restoration changed actual PCM, voice timing or shared RNG");
  for(std::size_t i=0;i<a.submitted.size();++i){const auto& x=a.submitted[i];const auto& y=b.submitted[i];
    need(x.kind==y.kind && x.sound==y.sound && x.owner==y.owner && x.view==y.view &&
        x.pitch_range==y.pitch_range && x.permutation==y.permutation && x.voice==y.voice && x.frames==y.frames &&
        x.rng_before==y.rng_before && x.rng_after==y.rng_after && x.source_pitch_bits==y.source_pitch_bits &&
        bool(x.position)==bool(y.position),"Actual post-cut damage/shield request selection differs");
    if(x.position)need(x.position->x==y.position->x && x.position->y==y.position->y && x.position->z==y.position->z,
        "Damage restoration changed its actual spatial report operand");
  }
}
void paired_restore(MultiplayerPreparedWorld& native){
  while(native.match.simulation().ticks()<2)native.match.step({});
  Fixture f(native.cache,native.content);const auto tick=native.match.simulation().ticks();f.active(tick);
  (void)f.audio.render(64);NativeEffects effects(native,f);const auto cut=effects.capture();
  const auto active=f.audio.voices();
  const auto expected=suffix(f,tick);need(peak(expected.pcm)>0,"Actual four-view active damage suffix contains no nonzero PCM");
  effects.publish(cut);need(f.audio.voices()==active,"Restored active Damage voice census differs");
  // Publish the same saved active image again before any producer progress.
  // This outgoing Damage image retains active old-branch handles with the
  // very same numeric IDs as the new target. Its plain retirement must not
  // stop the target voices through an outgoing whole-adapter destructor.
  effects.publish(cut);need(f.audio.voices()==active,"Outgoing Damage image retirement stopped freshly restored reused voice IDs");
  equal(expected,suffix(f,tick));effects.publish(cut);equal(expected,suffix(f,tick));
  std::cout<<"MCC damage native_tick="<<cut.seal.seal.tick<<" encoded_bytes="<<cut.seal.seal.bytes
      <<" active_voices="<<active<<" suffix_samples="<<expected.pcm.size()
      <<" suffix_requests="<<expected.submitted.size()<<" peak="<<peak(expected.pcm)<<'\n';
}
void preparation_guards(MultiplayerPreparedWorld& native){
  for(const bool expired:{false,true}){
    Fixture f(native.cache,native.content);
    if(expired){
      f.birth();f.damage().advance(2);f.damage().hud(0,units[0],false,1,.75f,70,false,false);
      need(f.drain()>0,"Expired-handle preparation control never played genuine HUD PCM");
    }
    NativeEffects effects(native,f);const auto cut=effects.capture();
    auto e=effects.effects.prepare_restore(cut.effects,cut.seal,{},{});
    const auto agreement=effects.effects.audio_auxiliary(e,cut.seal);
    auto a=f.audio.prepare_restore(cut.audio,agreement);auto sibling=f.audio.prepare_restore(cut.audio,agreement);
    auto d=f.damage().prepare_restore(cut.damage,a);
    need(f.damage().ready(d,a) && !f.damage().ready(d,sibling) && !f.damage().commit(d,a),
        "Empty/expired Damage image accepted sibling preparation identity or unpublished Audio");
    auto discarded=f.damage().prepare_restore(cut.damage,a);discarded=NativeUnitDamageAudio::PreparedReplacement{};
    need(!f.damage().ready(discarded,a),"Discarded Damage preparation retained publication authority");
    NativeUnitDamageAudio foreign(*f.cache,f.audio,f.auxiliary);foreign.enroll_continuation();
    rejects([&]{(void)foreign.prepare_restore(cut.damage,a);},"Foreign Damage wrapper accepted another owner image");
    bool foreign_ready=true;worker([&]{foreign_ready=f.damage().ready(d,a);});
    need(!foreign_ready,"Damage preparation readiness crossed its producer executor");
    f.damage().advance(expired?3:1);
    need(!f.damage().ready(d,a),"Damage producer mutation preserved an earlier preparation");
    auto current=f.damage().prepare_restore(cut.damage,a);
    {
      auto held=f.audio.hold_publication(a);
      worker([&]{a=MultiplayerAudio::PreparedReplacement{};});
      need(!f.damage().ready(current,a) && !f.damage().commit(current,a),
          "Cancelled exact Audio preparation left Damage publication authorized");
    }
    current=NativeUnitDamageAudio::PreparedReplacement{};
    // No target was published by the refusal controls. A fresh genuine
    // Effects/Core/Damage preparation remains usable afterward.
    effects.publish(cut);
    if(expired){
      const auto first=f.submitted.size();
      f.damage().hud(0,units[0],false,1,.75f,70,false,false);
      need(f.submitted.size()>first && f.audio.voices()>0,
          "Restored expired HUD receipt stopped a future voice with its reused numeric ID");
      need(f.drain()>0,"Restored expired HUD handle failed its next real pulse");
    }
  }
}
void genuine_auxiliary_mismatch(MultiplayerPreparedWorld& native){
  Fixture f(native.cache,native.content);f.birth();f.damage().advance(2);
  f.damage().hud(0,units[0],false,1,.75f,70,false,false);need(f.drain()>0,"Auxiliary mismatch control lacked authored PCM");
  NativeEffects effects(native,f);const auto earlier=effects.capture();
  f.damage().hud(0,units[0],false,1,.75f,70,false,false);
  need(f.auxiliary!=earlier.auxiliary,"Actual authored gain/pitch draws did not change the shared auxiliary word");
  const auto later=effects.capture();auto e=effects.effects.prepare_restore(later.effects,later.seal,{},{});
  const auto agreement=effects.effects.audio_auxiliary(e,later.seal);
  auto a=f.audio.prepare_restore(later.audio,agreement);
  rejects([&]{(void)f.damage().prepare_restore(earlier.damage,a);},
      "Damage accepted another actual Effects/Core cut's auxiliary value");
  auto wrong_e=effects.effects.prepare_restore(earlier.effects,earlier.seal,{},{});
  const auto wrong_agreement=effects.effects.audio_auxiliary(wrong_e,earlier.seal);
  need(agreement.ready(&f.auxiliary,later.auxiliary) && wrong_agreement.ready(&f.auxiliary,earlier.auxiliary),
      "Auxiliary mismatch control did not retain two genuine prepared Effects targets");
  rejects([&]{(void)f.audio.prepare_restore(later.audio,wrong_agreement);},
      "Core accepted a genuine Effects capability for the wrong captured auxiliary value");
}
}
int main(int argc,char** argv){try{
  need(argc==3 && std::string_view(argv[1])=="--mcc","Supply --mcc ABSOLUTE_IMPORTED_LOCKOUT_MAP");
  const std::filesystem::path map=argv[2];
  need(map.is_absolute() && std::filesystem::is_regular_file(map),"Explicit imported MCC map must exist");
  {
    const Cache preflight(map);
    need(preflight.is_mcc(),"Damage runtime assets require actual MCC import before native world construction");
  }
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch && *scratch,"Native damage fixture requires its managed test scratch owner");
  MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();options.local_players=4;
  options.variant={"Slayer",0,{{0x59,5},{10,0},{2,0}}};options.mute=true;options.source_object_world=true;
  options.session_random_seed=0x6d040015u;
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"damage-native";
  MultiplayerPreparedWorld native(options);
  need(native.cache->is_mcc() && native.source_objects && native.source_objects->owns_physics() && native.local_slots.size()==4,
      "Actual MCC four-player native world did not initialize its source owner");
  native.bind_source_local_views(native.local_slots);
  const auto& cache=native.cache;const auto& content=native.content;
  need(content.player_representations.size()>3 && content.player_representations[2].biped.datum!=none &&
      content.player_representations[3].biped.datum!=none,"Actual MCC multiplayer species representations are absent");
  stale_hud_handle(cache,content);absent_soft_damage_cue(cache,content);
  packet_owner_destruction(cache,content);auxiliary_refusals(cache,content);
  paired_restore(native);preparation_guards(native);genuine_auxiliary_mismatch(native);
  std::cout<<"PASS actual MCC damage continuation: four-view authored shield/damage active cut and exact PCM/request/RNG suffix, real native encoded body, genuine four-owner Effects writer, stale HUD next pulse, packet-observer wrapper destruction, empty/expired sibling identity, mutation/foreign/cancel guards and plain outgoing retirement\n";
  std::cout<<"LIMIT Damage unit IDs are declared adapter inputs; native world has actual four local births and local encoded cut. No network authentication, WAN, complete foreground atomic adoption, device audibility or full migration readiness is claimed\n";
  std::cout<<"LIMIT All four Effects owners are cold: no active particle, map attachment or combat graph coverage is claimed\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
