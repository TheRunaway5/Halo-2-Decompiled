#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <type_traits>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
double peak(std::span<const float> samples){double result=0;for(const auto s:samples){
  check(std::isfinite(s),"Native audio produced nonfinite PCM");result=std::max(result,std::abs(double(s)));}return result;}
void world_distance_handoff(const Cache &cache,std::uint32_t sound){
  const auto measure=[&](Vec3 point,std::span<const Vec3> listeners){
    MultiplayerAudio audio(cache,0x5821b36d,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
    audio.listeners(listeners);audio.impulse_at(sound,point);
    return peak(audio.render(4096));
  };
  const std::array<Vec3,1> listener{{{0,0,0}}};
  check(measure({0,0,0},listener)>0,"Located authored sound became silent at its listener");
  check(measure({1000000,0,0},listener)<measure({0,0,0},listener)*0.001,
      "World emitter position was discarded: distant sound stays at full volume");
  check(measure({0,0,0},{})==0,"World sound without a listener became a centered sound");
  const std::array<Vec3,2> split{{{1000000,0,0},{0,0,0}}};
  check(measure({0,0,0},split)==measure({0,0,0},listener),"Nearest local audio listener was not selected");
  MultiplayerAudio moving(cache,0x5821b36d,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
  moving.listeners(listener);moving.impulse_at(sound,{0,0,0});
  check(peak(moving.render(4096))>0,"Moving-listener sound failed to start");
  const std::array<Vec3,1> far{{{1000000,0,0}}};moving.listeners(far);
  check(peak(moving.render(4096))<0.001,"Active world voice retained an obsolete listener position");
  std::cout<<"PASS actual PCM world distance handoff: authored near/far gain, no listener, split views and moving listeners; HRTF/PVS separate\n";
}
void shared_variation_ownership(const Cache &cache,std::uint32_t sound) {
  std::uint32_t auxiliary=0x9147a23b;
  SoundLocalRandom expected(auxiliary);
  const auto definition=cache.load_sound_variation(sound);
  source_sound_gain(definition,expected);source_sound_pitch(definition,expected);
  unsigned packets{};
  MultiplayerAudio audio(cache,0x5821b36d,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
      [&](const auto &submission) {
        check(auxiliary==expected.seed,"Shared gain/pitch draws were not published before packet delivery");
        check(submission.rng_before==expected.seed,"Impulse permutation did not retain its post-pitch private seed");
        auxiliary=0x31415926; // Another consumer at the packet callback boundary.
        ++packets;
      },&auxiliary);
  check(audio.play_impulse(sound).has_value() && packets==1 && auxiliary==0x31415926,
      "Shared audio variation lost its actual owner or overwrote a subsequent consumer");
  audio.stop();
  std::cout<<"PASS shared auxiliary gain/pitch, private post-pitch impulse seed and callback publication\n";
}
void completion_ownership(const Cache &cache,std::uint32_t sound){
  static_assert(std::is_same_v<MultiplayerAudio::Voice,std::uint64_t>);
  std::vector<MultiplayerAudioSubmission> submitted;
  std::vector<SdlSoundCompletion> completed;
  MultiplayerAudio audio(cache,0x5821b36d,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
      [&](const auto &submission){submitted.push_back(submission);});
  const auto request=[&](MultiplayerAudio::Completion owner,std::uint32_t context=0x28a13b04){
    const auto voice=audio.play_impulse(sound,1,MultiplayerAudioKind::Impulse,context,0,std::move(owner));
    check(voice.has_value() && *voice==submitted.back().voice,
        "Native impulse did not return its actual submitted voice handle");
    return *voice;
  };
  const auto owner=[&](const SdlSoundCompletion &packet){
    check(audio.voices()==0,"Completion ran before the retired voice bank was released");
    completed.push_back(packet);
  };
  const auto absent=audio.play_impulse(null_datum,1,MultiplayerAudioKind::Impulse,null_datum,0,owner);
  check(!absent && completed.empty() && audio.voices()==0,"NONE invented a voice or completion");
  const auto first=request(owner),second=request(owner,0x728aa091);
  check(first!=second && audio.voices()==2,"Simultaneous impulse handles alias");
  audio.master_volume(0);
  // Actual authored PCM and driver pitch run through the Manual SDL mixer.
  // A generous bounded sample count completes both packets without a guessed
  // completion timer. Callbacks must wait for the explicit update owner.
  for(unsigned i=0;i<360;++i)check(peak(audio.render(4096))==0,"Muted completion control became audible");
  check(completed.empty() && audio.voices()==2,"Mixer invoked a completion owner directly");
  audio.update();
  check(completed.size()==2 && audio.voices()==0,"Finished impulses did not retire exactly once");
  for(unsigned i=0;i<2;++i){const auto &packet=completed[i];const auto &submission=submitted[i];
    check(packet.voice==submission.voice && packet.channel_context==submission.owner &&
        packet.status==SdlSoundCompletionStatus::Success && packet.pcm_frames==submission.frames &&
        packet.submission==1 && packet.packet_context!=0,
        "Completion lost actual SDL packet/voice/status ownership");}
  audio.update();audio.stop();
  check(completed.size()==2,"A terminal impulse completed twice");

  audio.master_volume(1);submitted.clear();completed.clear();
  const auto consumed=request(owner);
  for(unsigned i=0;i<360;++i)(void)audio.render(4096);
  const auto pending=request(owner);
  audio.stop();
  check(completed.size()==2 && completed[0].voice==consumed &&
      completed[0].status==SdlSoundCompletionStatus::Success && completed[1].voice==pending &&
      completed[1].status==SdlSoundCompletionStatus::Flushed && audio.voices()==0,
      "Stop fabricated a successful pending completion or overwrote an actual success");
  audio.update();audio.stop();check(completed.size()==2,"Flushed impulse completed twice");

  auto lifetime=std::make_shared<int>(7);std::weak_ptr<int> retained=lifetime;
  unsigned lifetime_calls=0;
  request([keep=lifetime,&lifetime_calls](const auto &packet){
    check(*keep==7 && packet.status==SdlSoundCompletionStatus::Flushed,
        "Pending callback did not retain its owner until actual cancellation");++lifetime_calls;});
  lifetime.reset();check(!retained.expired(),"Pending impulse released its callback prematurely");
  audio.stop();check(lifetime_calls==1 && retained.expired(),"Retired impulse retained its callback owner");

  unsigned throwing_calls=0;
  request([&](const auto &){++throwing_calls;throw std::runtime_error("completion control");});
  request([&](const auto &){++throwing_calls;});
  bool threw=false;
  try{audio.stop();}catch(const std::runtime_error &error){threw=std::string(error.what())=="completion control";}
  check(threw && throwing_calls==2 && audio.voices()==0,
      "Throwing completion skipped another owner or retained native voices");
  audio.update();check(throwing_calls==2,"Throwing completion was delivered twice");

  std::optional<MultiplayerAudio::Voice> chained;
  unsigned chained_calls=0;
  const auto predecessor=request([&](const auto &packet){
    check(packet.status==SdlSoundCompletionStatus::Success && audio.voices()==0,
        "Chained completion ran before actual predecessor retirement");
    chained=request([&](const auto &next){
      check(chained && next.voice==*chained && next.status==SdlSoundCompletionStatus::Flushed,
          "Chained request lost its separate native completion");++chained_calls;});
  });
  for(unsigned i=0;i<360;++i)(void)audio.render(4096);
  audio.update();
  check(chained && *chained!=predecessor && audio.voices()==1 && chained_calls==0,
      "Completion could not safely submit a subsequent native voice");
  audio.stop();check(chained_calls==1 && audio.voices()==0,"Chained native voice did not retire");

  unsigned destruction_calls=0;
  {
    MultiplayerAudio scoped(cache,0x27b8ca61,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
    const auto voice=scoped.play_impulse(sound,1,MultiplayerAudioKind::Impulse,0x38a14299,0,
        [&](const auto &packet){
          check(packet.voice!=0 && packet.channel_context==0x38a14299 &&
              packet.status==SdlSoundCompletionStatus::Flushed,
              "Destruction did not retain actual flushed packet identity");
          ++destruction_calls;throw std::runtime_error("destructor completion control");});
    check(voice.has_value(),"Scoped native playback did not create an actual voice");
  }
  check(destruction_calls==1,"Destruction did not complete its pending owner exactly once");
  std::cout<<"PASS actual native impulse handles/completions: simultaneous success, muted mixer, "
      "consumed/pending stop, callback lifetime, exception draining, chained playback and destruction\n";
}
}
int main(int argc,char **argv){
  try{
    check(argc==4,"Supply MAP SHARED_MAPS UPDATE_XBE");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    auto content=cache.load_content(0,true);
    const auto weapon=cache.slayer_starting_weapon(5);
    const auto definition=cache.load_first_person_binding(weapon,2);
    std::set<std::uint32_t> reload_tags;
    for(const auto &clip:definition.clock_clips)if(clip.activity==0x0b000065 || clip.activity==0x0c000064)
      for(const auto &event:clip.events)if(event.type==1&&event.tag!=null_datum)reload_tags.insert(event.tag);
    check(!reload_tags.empty(),"Original reload clips have no sound event");
    world_distance_handoff(cache,*reload_tags.begin());
    shared_variation_ownership(cache,*reload_tags.begin());
    completion_ownership(cache,*reload_tags.begin());
    unsigned reloads=0,callbacks=0,ambient_submissions=0;double reload_peak=0,ambient_peak=0;
    for(const auto count:{1u,4u}){
      SlayerRules rules;rules.weapon=weapon;rules.time_ticks=rules.score_limit=0;
      MultiplayerMatch match(content,0,cache.spawns(),rules);std::vector<unsigned> ids;
      for(unsigned i=0;i<count;++i)ids.push_back(match.join("Audio regression"));
      match.configure_local_views(ids,definition);
      std::vector<MultiplayerAudioSubmission> observed;
      // Deterministic native audio provider input, not an assigned guest RNG.
      MultiplayerAudio audio(cache,0x87e28a31,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
          [&](const auto &s){observed.push_back(s);});
      for(const auto tag:reload_tags)audio.prepare(tag);
      match.configure_first_person_event_owner([&](unsigned view,unsigned hand,
          const FirstPersonViewBinding &binding,const FirstPersonAnimationEvent &event){
        check(hand==0 && view<count,"First-person event owner escaped its view");
        check(binding.unit==match.unit(ids[view]).source_datum &&
            binding.hands[0].weapon==match.unit(ids[view]).weapon.source_datum,
            "Sound event used a stale salted owner");
        ++callbacks;audio.first_person(view,binding,event);
      });
      for(unsigned tick=0;tick<130;++tick){
        std::array<Controls,multiplayer_capacity> controls{};
        controls[ids[0]].fire=tick>=6&&tick<20;controls[ids[0]].reload=tick==28;
        controls[ids[0]].yaw=match.unit(ids[0]).yaw;
        match.step(controls);match.take_sounds();audio.update();
        reload_peak=std::max(reload_peak,peak(audio.render(1600)));
      }
      unsigned run_reload=0;
      for(const auto &s:observed)if(s.kind==MultiplayerAudioKind::FirstPerson && reload_tags.contains(s.sound)){
        check(s.view==0 && s.frames>0 && s.owner==match.unit(ids[0]).weapon.source_datum,
            "Reload submission lost local view/weapon/resource ownership");
        check(s.permutation<cache.load_sound_variation(s.sound).permutations.size(),"Reload selected outside authored permutations");
        ++run_reload;
      }
      check(run_reload==1,"Real fire/release/reload did not submit exactly one authored reload event");reloads+=run_reload;
      audio.stop();check(audio.voices()==0,"Stopping multiplayer retained native voices");
    }
    check(reload_peak>0,"Actual authored reload events mixed to silence");
    const auto bank=cache.load_multiplayer_audio_bsp(0);const auto geometry=cache.load_bsp_atmosphere(0);
    check(bank.clusters.size()==geometry.clusters.size() && !bank.palette.empty(),"Literal BSP ambient palette/cluster mapping missing");
    std::vector<MultiplayerAmbientTarget> targets;
    for(const auto &spawn:cache.spawns())if(spawn.bsp==0){
      const auto eye=spawn.position+Vec3{0,0,content.player.camera_height};
      targets=multiplayer_ambient_targets(bank,geometry,eye,geometry.cluster_at(eye),0,false);
      if(!targets.empty())break;
    }
    check(!targets.empty(),"Actual map spawn never selected its authored ambient tag");
    std::set<std::uint32_t> expected_tracks;
    std::set<std::uint32_t> inactive_alternates;
    for(const auto &target:targets)for(const auto &track:cache.load_looping_sound(target.sound).tracks){
      if(track.loop!=null_datum)expected_tracks.insert(track.loop);
      else if(track.alternate!=null_datum)inactive_alternates.insert(track.alternate);
    }
    std::set<std::uint32_t> actual_tracks;
    MultiplayerAudio ambient(cache,0x42398b13,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
        [&](const auto &s){check(s.kind==MultiplayerAudioKind::Ambient,"Ambient submitted an unrelated event");
          actual_tracks.insert(s.sound);++ambient_submissions;});
    for(unsigned tick=0;tick<150;++tick){ambient.ambient(targets,1.f/30);ambient.update();
      ambient_peak=std::max(ambient_peak,peak(ambient.render(1600)));}
    check(actual_tracks==expected_tracks && ambient_peak>0,"Authored ambient tracks did not reach native PCM output");
    ambient.master_volume(0);
    check(peak(ambient.render(1600))==0 && ambient.voices()>0,"native mute left audible game PCM or stopped ambient voices");
    ambient.master_volume(1);
    check(peak(ambient.render(1600))>0,"native unmute failed to restore ongoing game voices");
    for(const auto tag:inactive_alternates)check(!actual_tracks.contains(tag),"Normal ambient lane invented alternate/weather activation");
    for(unsigned tick=0;tick<150 && ambient.voices();++tick){ambient.ambient({},1.f/30);ambient.update();ambient.render(1600);}
    check(ambient.voices()==0,"Departed ambient target did not fade/release its tracks");
    std::cout<<"PASS native authored audio: "<<reloads<<" reload events via real one/four-view source clocks, "<<callbacks
      <<" callbacks, reload_peak="<<reload_peak<<", "<<ambient_submissions<<" ambient packets/"<<actual_tracks.size()
      <<" authored tracks, ambient_peak="<<ambient_peak<<"; all native voice lifetimes released\n";
    std::cout<<"LIMIT native stereo playback and normal ambient lane; original global scheduler/PVS/DSP/weather/detail scheduling remain separate\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
