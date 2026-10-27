#include "halo2/multiplayer_audio.hpp"
#include "halo2/audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
template<class F>void rejects(F&& operation,const char* reason){bool rejected=false;
  try{operation();}catch(const FormatError&){rejected=true;}check(rejected,reason);}
std::uint32_t sound(const Cache& cache,const std::string& name){
  for(const auto& tag:cache.tags())if(tag.group=="snd!" && tag.name==name)return tag.datum;
  throw std::runtime_error("Actual Banshee phase resource is absent: "+name);
}
void same_pcm(const std::vector<float>& a,const std::vector<float>& b){
  check(a.size()==b.size(),"Phase continuation changed mixed extent");
  for(unsigned n=0;n<a.size();++n)check(std::bit_cast<std::uint32_t>(a[n])==std::bit_cast<std::uint32_t>(b[n]),
      "Phase continuation changed consumed PCM bits");
}
}
int main(int argc,char** argv)try{
  if(argc==2 && !*argv[1])return 77;
  if(argc!=2)throw std::runtime_error("Supply actual prepared MCC Coagulation map");
  auto cache=std::make_shared<const Cache>(argv[1]);check(cache->is_mcc(),"Phase fixture requires actual MCC resources");
  const auto a=sound(*cache,"sound\\vehicles\\banshee\\banshee_boost_left\\track1\\in");
  const auto b=sound(*cache,"sound\\vehicles\\banshee\\banshee_boost_left\\track1\\loop");
  const auto different=sound(*cache,"sound\\vehicles\\banshee\\banshee_multi_eng_left\\b_engine_left\\loop");
  const auto first=decode_sound(cache->load_sound(a,0,0)),next=decode_sound(cache->load_sound(b,0,0));
  unsigned packets{};std::uint64_t id{};
  MultiplayerAudio audio(cache,0x3a918c2bu,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
      [&](const auto& event){
        check(event.permutation==0 && event.pitch_range==0 && event.rng_before==event.rng_after,
            "Selected phase output reran its controller selection");
        if(!id)id=event.voice;check(event.voice==id,"Phase output recreated its actual native stream");
        check(event.sound==(packets%2?b:a),"Phase output selected a different authored sound");++packets;
      });
  audio.enroll_continuation();
  const auto metadata=MccSoundMetadataPublication::create(cache);
  audio.bind_native_loop_metadata(metadata);
  const MultiplayerLoopPacketTu15 input{a,0xffffffffu,0,0,0,{300,-3,1},{}};
  const auto voice=audio.play_loop_packet(input,audio.presentation_frames()+1024);
  check(voice && packets==1,"Initial actual phase PCM was not submitted");
  rejects([&]{audio.bind_native_loop_metadata(metadata);},"Active loop voice admitted immutable metadata rebind");
  const auto parameters=audio.voice_parameters(*voice);
  rejects([&]{audio.queue_loop_packet_phase(*voice,different,0,0);},
      "Differing phase controls were silently substituted");
  check(audio.voice_active(*voice) && audio.voices()==1 && packets==1,
      "Rejected phase changed its actual playback owner");
  check(audio.queue_loop_packet_phase(*voice,b,0,0) && packets==2,
      "Following phase did not reach the same actual stream");
  check(!audio.queue_loop_packet_phase(*voice,b,0,0) && packets==2,
      "Full phase queue consumed a third selected packet");
  (void)audio.render(2048);
  check(audio.playback_position(*voice).output_frames>0,"Phase prefix consumed no actual PCM");
  const auto checkpoint=audio.checkpoint();
  const auto rate=std::bit_cast<float>(parameters.relative_rate_bits);
  const auto frames=first.frames()+next.frames();
  const auto remaining=std::size_t(std::ceil(double(frames)/rate))+2048;
  const auto expected=audio.render(remaining);
  const auto position=audio.playback_position(*voice);
  check(position.output_frames==std::uint64_t(std::ceil(double(frames)/rate)) &&
      std::abs(position.source_frames-double(frames))<.00001,
      "Phase queue inserted an underrun or lost authored decoded frames");
  check(std::ranges::any_of(expected,[](float v){return v!=0;}),"Phase suffix contains only silence");
  audio.update();check(!audio.voice_active(*voice),"Completed phase stream did not retire");
  auto replacement=audio.prepare_restore(checkpoint);const auto target=replacement.voice(*voice);
  check(bool(target),"Phase checkpoint lost its retained stream");
  {auto held=audio.hold_publication(replacement);check(audio.begin_commit(replacement,held) &&
      audio.commit(replacement,held),"Phase publication failed");}
  rejects([&]{audio.queue_loop_packet_phase(*voice,b,0,0);},"Retired phase receipt survived publication");
  same_pcm(expected,audio.render(remaining));audio.update();audio.stop();
  std::cout<<"PASS actual Banshee phase PCM: same native stream, two-slot backpressure, no underrun, exact restored suffix\n"
      <<"LIMIT native output-port control with actual immutable converted metadata; source phase/synchronization ownership and launcher gameplay are separate\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
