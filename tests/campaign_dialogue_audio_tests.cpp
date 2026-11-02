#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/original_data.hpp"
#include "halo2/sound_direct_route_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
double peak(std::span<const float> pcm){double value{};for(auto sample:pcm){need(std::isfinite(sample),"Nonfinite dialogue output");value=std::max(value,double(std::abs(sample)));}return value;}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value){for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(8*n));}
}
int main(int argc,char**argv)try {
  if(argc!=4 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[3]))return 77;
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  const auto owner=std::make_shared<const Cache>(argv[1],argv[2],OriginalData::load_xbe(argv[3]));const auto& cache=*owner;
  need(cache.is_mcc(),"Dialogue delivery regression requires actual prepared MCC content");
  const auto publication=MccSoundMetadataPublication::create(owner);
  const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
  const std::array<Vec3,1> listener{{{0,0,0}}};unsigned sounds{},raw_controls{},authored_mutes{},class_gain_controls{};bool arbiter{},prophet{};
  for(const auto& tag:cache.tags()) {
    if(tag.group!="snd!" || !tag.address || !tag.name.starts_with("sound\\dialog\\levels\\01_spacestation\\cinematic\\"))continue;
    const auto raw=cache.read_tag_bytes(tag.address,28);if(Bytes(raw).u16(0)&0x800u)continue;
    const auto definition=publication.load(cache,tag.handle());const Bytes snd(definition.sound),cls(definition.sound_class);
    if(!source_sound_direct_location_flags_tu15(snd.u16(0),cls.u16(10)))continue;
    need(cls.f32(0x14)==-3.f,"Opening fixture class direct gain changed");
    const Bytes playback(definition.playback);
    if(playback.f32(0x10)==-64.f && playback.f32(0x14)==0.f) {
      MultiplayerAudio audio(owner,0x5821b36du,manual);audio.bind_native_loop_metadata(publication);audio.listeners(listener);
      need(!audio.play_impulse_at(tag.datum,{1,0,0}),"Authored mute was overridden by direct routing");
      ++authored_mutes;continue;
    }
    const auto frames=decode_sound(cache.load_sound(tag.datum,0,0)).frames();
    const auto measure=[&](float distance,bool request,bool direct) {
      MultiplayerAudio audio(owner,0x5821b36du,manual);audio.bind_native_loop_metadata(publication);audio.listeners(listener);
      std::optional<MultiplayerAudio::Voice> voice;
      if(request) {
        std::array<std::byte,160> bytes{};put(bytes,0,13u);put(bytes,12,0x11000000u|(direct?0x800u:0u));
        put(bytes,16,std::bit_cast<std::uint32_t>(1.f));put(bytes,24,std::bit_cast<std::uint32_t>(distance));put(bytes,80,0xe1234567u);
        voice=audio.source_impulse(tag.datum,bytes,MultiplayerAudioKind::Impulse,0);
      }else voice=audio.play_impulse_at(tag.datum,{distance,0,0},1,MultiplayerAudioKind::Impulse,0xe1234567u);
      if(!voice)throw FormatError("Authored opening dialogue did not create its actual PCM voice: "+tag.name+(request?" source":" semantic"));
      const auto volume=audio.voice_parameters(*voice).volume_hundredths_db;
      return std::pair{volume,audio.render(frames)};
    };
    const auto near=measure(1,false,true);const auto distant=measure(25,false,true);
    need(peak(near.second)>0.0001 && near==distant,"Direct dialogue path was distance attenuated or muted");
    // Zero authored playback/control/permutation gain isolates the original
    // class's -3dB direct gain at the real driver output. Simply dropping all
    // attenuation must fail even though it would make the line audible.
    if(playback.f32(0x10)==0.f && playback.f32(0x14)==0.f &&
        Bytes(definition.scale).f32(4)+cls.f32(0x24)==0.f &&
        definition.permutations.size()==1 && definition.permutations[0][4]==std::byte{}) {
      need(near.first==-300,"Actual direct output lost the authored class gain");++class_gain_controls;
    }
    const auto native_request=measure(25,true,true);const auto located_request=measure(25,true,false);
    need(native_request==near,"Actual source request lost its 0800 direct path");
    need(peak(located_request.second)==0 && located_request.first==-6400,"Explicit source located-only request gained a direct route");
    ++raw_controls;++sounds;arbiter|=tag.name.ends_with("x01_0030_der");prophet|=tag.name.ends_with("x01_0050_pom");
  }
  need(arbiter&&prophet&&sounds>=2&&class_gain_controls,"Actual missing dialogue or isolated native class gain fixture absent");
  need(source_sound_direct_location_flags_tu15(0x100u,0x80u)==0 && source_sound_direct_location_flags_tu15(0,0)==0,
      "Sound opt-out or ordinary located class acquired the direct route");
  std::cout<<"PASS "<<sounds<<" actual opening dialogue PCM voices retain authored direct gain at 1 and 25 units; "<<raw_controls<<" explicit source direct/located controls; "<<authored_mutes<<" authored mutes retained; "<<class_gain_controls<<" actual -3dB class gain controls; actual Arbiter and Prophet lines included\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
