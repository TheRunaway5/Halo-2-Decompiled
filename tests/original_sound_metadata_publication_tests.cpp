#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
template<class F>void rejects(F&& f){bool refused{};try{f();}catch(const FormatError&){refused=true;}
 need(refused,"Immutable sound publication accepted a foreign resource operand");}
void same_encoded(const EncodedSound& a,const EncodedSound& b){
 need(a.data==b.data&&a.codec==b.codec&&a.sample_rate==b.sample_rate&&a.channels==b.channels&&
 a.samples==b.samples&&a.sound_class==b.sound_class&&a.packet_chunks.size()==b.packet_chunks.size(),
 "Immutable encoded sound differs from its admitted resource owner");
 for(unsigned n=0;n<a.packet_chunks.size();++n){const auto& x=a.packet_chunks[n];const auto& y=b.packet_chunks[n];
 need(x.bytes==y.bytes&&x.pcm_bytes==y.pcm_bytes&&x.reset_after==y.reset_after,"Immutable sound decoder boundaries differ");}
}
void same(const SoundVariationDefinition& a,const SoundVariationDefinition& b){
 need(a.datum==b.datum&&a.sound==b.sound&&a.playback==b.playback&&a.sound_class==b.sound_class&&
 a.range==b.range&&a.scale==b.scale&&a.permutations==b.permutations&&a.used==b.used&&
 a.sample_rate_index==b.sample_rate_index&&a.pitch_ranges.size()==b.pitch_ranges.size(),
 "Immutable playback metadata differs from the admitted original canonical bank");
 for(unsigned n=0;n<a.pitch_ranges.size();++n)need(a.pitch_ranges[n].interval==b.pitch_ranges[n].interval&&
 a.pitch_ranges[n].permutations==b.pitch_ranges[n].permutations,"Immutable original pitch metadata differs");
}
}
int main(int argc,char**argv)try{
 if(argc!=3)throw FormatError("Supply produced original-sound map and JSON report");
 if(!*argv[1])return 77;
 auto cache=std::make_shared<const Cache>(argv[1]);
 const auto catalog=cache->original_sound_catalog();need(bool(catalog),"Immutable original playback needs normal catalog admission");
 const auto input=catalog->input(*cache);
 MccSoundRuntimePlan expected;
 const auto delivery=MccRuntimeCacheDelivery::plan(*cache,0x90000000u,0x60000000u,
 [&](MccSoundRuntimePlan&& p){expected=std::move(p);},input);
 need(expected.original&&expected.original==delivery.original_sound_owner(),"Expected original canonical publication is incomplete");
 const auto publication=MccSoundMetadataPublication::create(cache);
 nlohmann::json report{{"passed",false},{"original_context",catalog->original_context()},{"original_bindings",input.bindings.size()},
 {"cases",nlohmann::json::array()},{"scope","Immutable client metadata and actual manual SDL PCM; no native World epoch, source voice admission, network timing or physical device"},
 {"native_world_or_factory_verified",false},{"physical_device_or_xbox_waveform_verified",false}};
 unsigned failures{},pitches{},voices{},routes{},controls{};
 for(const auto& binding:input.bindings){
  nlohmann::json row{{"sound",binding.sound.value},{"name",binding.bank->metadata().name},{"passed",false}};
  try{
   const auto canonical=expected.original->bank();const auto first=canonical->load(binding.sound);
   for(unsigned pitch=0;pitch<first.pitch_ranges.size();++pitch){const auto definition=publication.load(*cache,binding.sound,pitch);
    same(definition,canonical->load(binding.sound,pitch));++pitches;
    for(unsigned p=0;p<definition.permutations.size();++p){same_encoded(publication.encoded(*cache,binding.sound,pitch,p),canonical->encoded(binding.sound,pitch,p));++routes;}}
   std::vector<MultiplayerAudioSubmission> packets;
   MultiplayerAudio audio(cache,0x5821b36du,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
       [&](const auto& p){packets.push_back(p);});
   audio.enroll_continuation();audio.bind_native_loop_metadata(publication);
   const auto voice=audio.play_impulse_receipt(binding.sound.value);
   need(voice&&packets.size()==1,"Immutable original playback did not create exactly one queued voice");
   const auto packet=packets.front();const auto decoded=decode_sound(canonical->encoded(binding.sound,packet.pitch_range,packet.permutation));
   need(packet.frames==decoded.frames(),"Immutable Audio still selected MCC payload frames for original metadata");
   double peak{};std::uint64_t frames{},hash=0xcbf29ce484222325ull;
   const auto collect=[&](const auto& pcm){frames+=pcm.size()/2;for(float p:pcm){need(std::isfinite(p),"Immutable original Audio produced nonfinite PCM");
    peak=std::max(peak,std::abs(double(p)));hash=(hash^std::bit_cast<std::uint32_t>(p))*0x100000001b3ull;}};
   collect(audio.render(std::min<std::size_t>(137,packet.frames/2)));audio.update();
   const auto checkpoint=audio.checkpoint();need(checkpoint.voices()==1,"Immutable original cut omitted a playing voice");
   const auto position=audio.playback_position(*voice);const auto expected_pcm=audio.render(512);audio.update();
   const auto physical=audio.mixed_frames();auto replacement=audio.prepare_restore(checkpoint);
   const auto restored=replacement.voice(*voice);need(bool(restored),"Immutable original restore lost its voice mapping");
   {auto held=audio.hold_publication(replacement);need(audio.ready(replacement,held)&&audio.begin_commit(replacement,held)&&audio.commit(replacement,held),
     "Immutable original playing Audio publication failed");}
   need(audio.mixed_frames()==physical&&!audio.voice_active(*voice)&&audio.voice_active(*restored),"Immutable original restore rewound output or revived stale receipts");
   const auto resumed=audio.playback_position(*restored);need(resumed.source_frames==position.source_frames&&resumed.output_frames==position.output_frames,
     "Immutable original restore changed its saved playing cursor");
   const auto actual_pcm=audio.render(512);audio.update();need(actual_pcm.size()==expected_pcm.size(),"Immutable original replay extent changed");
   for(unsigned n=0;n<actual_pcm.size();++n)need(std::bit_cast<std::uint32_t>(actual_pcm[n])==std::bit_cast<std::uint32_t>(expected_pcm[n]),"Immutable original PCM replay changed");
   collect(actual_pcm);
   for(unsigned n=0;n<4096&&audio.voices();++n){const auto pcm=audio.render(4096);frames+=pcm.size()/2;
    for(float p:pcm){need(std::isfinite(p),"Immutable original Audio produced nonfinite PCM");peak=std::max(peak,std::abs(double(p)));
      hash=(hash^std::bit_cast<std::uint32_t>(p))*0x100000001b3ull;}audio.update();}
   need(!audio.voices()&&peak>0&&packets.size()==1,"Immutable original voice was silent, duplicated or failed finite completion");
   ++voices;row.update({{"passed",true},{"selected_pitch",packet.pitch_range},{"selected_permutation",packet.permutation},
    {"original_pcm_frames",packet.frames},{"mixed_frames",frames},{"mixed_fnv64",hash},{"peak",peak},{"playing_original_pcm_replay_verified",true}});
  }catch(const std::exception& e){row["error"]=e.what();++failures;}
  report["cases"].push_back(row);
 }
 const auto sound=input.bindings.front().sound;
 rejects([&]{(void)publication.encoded(*cache,{sound.value^0x10000u},0,0);});++controls;
 rejects([&]{(void)publication.encoded(*cache,sound,UINT32_MAX,0);});++controls;
 rejects([&]{(void)publication.encoded(*cache,sound,0,UINT32_MAX);});++controls;
 auto other=std::make_shared<const Cache>(argv[1]);
 rejects([&]{(void)publication.encoded(*other,sound,0,0);});++controls;
 const std::shared_ptr<const Cache> alias(cache.get(),[](const Cache*){});
 rejects([&]{publication.require_owner(alias);});++controls;
 const auto prefix=std::ranges::find_if(cache->tags(),[&](const auto& t){return t.group=="snd!"&&!expected.original->contains(t.handle());});
 need(prefix!=cache->tags().end(),"Original publication lost its retained MCC prefix");
 same_encoded(publication.encoded(*cache,prefix->handle(),0,0),cache->load_sound(prefix->datum,0,0));
 report.update({{"passed",failures==0},{"failures",failures},{"pitch_definitions",pitches},{"original_pcm_voices",voices},
 {"original_encoded_routes",routes},{"negative_controls",controls},{"retained_mcc_route_unchanged",true}});
 std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<"Immutable original playback: "<<catalog->original_context()<<" bindings="<<input.bindings.size()<<" failures="<<failures<<'\n';
 return failures?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

