#include "halo2/multiplayer_audio.hpp"
#include "halo2/audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using J=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
void same(const std::vector<float>& a,const std::vector<float>& b){need(a.size()==b.size(),"Chunk output extent differs");
 for(unsigned n=0;n<a.size();++n)need(std::bit_cast<std::uint32_t>(a[n])==std::bit_cast<std::uint32_t>(b[n]),"Chunk queue changed uninterrupted PCM bits");}
}
int main(int argc,char**argv)try{
 if(argc!=3)throw FormatError("Supply original-catalog map and bounded report");if(!*argv[1])return 77;
 auto cache=std::make_shared<const Cache>(argv[1]);const auto catalog=cache->original_sound_catalog();need(bool(catalog),"Require normal original catalog admission");
 const auto input=catalog->input(*cache);const auto metadata=MccSoundMetadataPublication::create(cache);
 J report{{"passed",false},{"cases",J::array()},{"scope","Real immutable original metadata, production MultiplayerAudio and manual SDL chunk consumption; no source voice admission, native World, Xbox device or wet feedback"}};
 unsigned cases{},restores{},backpressure{},selected{},expected{},cancellations{};
 for(const auto& binding:input.bindings)for(const auto& range:binding.bank->metadata().pitch_ranges)
  for(const auto& p:range.permutations)if(p.chunks.size()>1)++expected;
 for(const auto& binding:input.bindings){const auto& m=binding.bank->metadata();
  for(unsigned pitch=0;pitch<m.pitch_ranges.size();++pitch)for(unsigned p=0;p<m.pitch_ranges[pitch].permutations.size();++p){
   const auto encoded=metadata.encoded(*cache,binding.sound,pitch,p);if(encoded.packet_chunks.size()<2)continue;
   const auto decoded=decode_sound(encoded);need(decoded.chunk_boundaries.size()==encoded.packet_chunks.size(),"Missing decoder-owned boundaries");
   unsigned observed{};MultiplayerAudio audio(cache,0x5821b36du,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},[&](const auto& event){
    need(event.sound==binding.sound.value&&event.pitch_range==pitch&&event.permutation==p&&event.frames==decoded.frames()&&event.rng_before==event.rng_after,"Chunk delivery reselected a controller permutation");++observed;});
   audio.enroll_continuation();audio.bind_native_loop_metadata(metadata);
   const auto definition=metadata.load(*cache,binding.sound,pitch);
   const auto center=std::bit_cast<std::int16_t>(Bytes(definition.pitch_ranges[pitch].interval).u16(0));
   const MultiplayerLoopPacketTu15 request{binding.sound.value,0xffffffffu,0,pitch,p,{float(center)+137.f,-3,1},{}};
   const auto voice=audio.play_loop_packet(request);need(voice&&observed==1,"Selected multi-chunk sound was not delivered");
   need(!audio.loop_packet_accepts_input(*voice),"Multi-chunk permutation occupied only one native packet slot");
   need(!audio.queue_loop_packet(*voice,p)&&observed==1,"Full chunk queue consumed another controller selection");++backpressure;
   const auto parameters=audio.voice_parameters(*voice);const auto data=std::make_shared<const std::vector<float>>(decoded.stereo);
   SdlSoundPlatform::Voice reference_voice{};
   const auto make_reference=[&]{auto device=std::make_unique<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
    reference_voice=device->create_voice(0x1234,parameters);need(device->submit(reference_voice,{data,0,decoded.frames(),0x888})==SdlSoundPlatform::Submission::Accepted,"Reference PCM was not accepted");return device;};
   const auto rate=std::bit_cast<float>(parameters.relative_rate_bits);
   const auto crossing=std::size_t(std::ceil(double(decoded.chunk_boundaries.front().end_frame)/rate))+17;
   need(crossing>137&&crossing<std::size_t(std::ceil(double(decoded.frames())/rate)),"Chunk control lacks its first-boundary witness");
   auto reference=make_reference();same(audio.render(137),reference->render(137));const auto checkpoint=audio.checkpoint();
   auto current=*voice;SdlSoundPlaybackPosition position{};unsigned blocks{};
   const auto finish=[&]{while(audio.voice_active(current)){
    const auto frames=blocks?std::array<std::size_t,4>{1600,311,1024,733}[blocks%4]:crossing-137;++blocks;
    same(audio.render(frames),reference->render(frames));
    if(blocks==1){auto changed=request.parameters;changed.gain_db=-9;
     audio.loop_packet_parameters(current,changed);reference->parameters(reference_voice,audio.voice_parameters(current));}
    position=audio.playback_position(current);audio.update();need(blocks<10000,"Chunk completion did not make finite progress");}};
   finish();need(std::abs(position.source_frames-double(decoded.frames()))<.00001,"Chunk queue lost or invented consumed source frames");
   const auto physical=audio.mixed_frames();auto replacement=audio.prepare_restore(checkpoint);const auto target=replacement.voice(*voice);need(bool(target),"Chunk publication lost its retained voice");
   {auto held=audio.hold_publication(replacement);need(audio.ready(replacement,held)&&audio.begin_commit(replacement,held)&&audio.commit(replacement,held),"Playing chunk publication failed");}
   need(audio.mixed_frames()==physical&&!audio.voice_active(*voice),"Chunk restore rewound output or revived a stale receipt");
   current=*target;reference=make_reference();(void)reference->render(137);blocks=0;finish();need(observed==1,"Chunk restore duplicated its permutation selection");
   need(std::abs(position.source_frames-double(decoded.frames()))<.00001,"Chunk restore lost its unsubmitted remainder");
   ++cases;++restores;selected+=observed;
   unsigned canceled{};const auto cancel=audio.play_loop_packet(request,{},[&](const auto& completion){
    need(completion.status==SdlSoundCompletionStatus::Flushed,"Canceled chunk output reported an invented successful completion");++canceled;});
   need(cancel&&observed==2,"Cancellation did not own a real queued chunk voice");(void)audio.render(17);audio.stop();audio.update();
   need(canceled==1&&!audio.voices()&&observed==2,"Flush restarted remaining chunks or duplicated retirement");
   need(std::ranges::all_of(audio.render(31),[](float value){return value==0.f;}),"Flushed chunk voice continued producing PCM");++cancellations;
   report["cases"].push_back({{"name",m.name},{"sound",binding.sound.value},{"pitch",pitch},{"permutation",p},{"chunks",encoded.packet_chunks.size()},{"pcm_frames",decoded.frames()},{"restored_suffix_verified",true},{"flushed_remainder_verified",true}});
  }
 }
 need(expected&&cases==expected&&restores==expected&&backpressure==expected&&selected==expected&&cancellations==expected,"Multi-chunk destruction context incomplete");
 report.update({{"passed",true},{"multi_chunk_permutations",cases},{"expected_from_authenticated_metadata",expected},
  {"playing_restore_cases",restores},{"backpressure_cases",backpressure},{"selected_permutations",selected},{"flush_cases",cancellations},{"original_context",catalog->original_context()}});
 std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<"PASS original multi-chunk output: permutations="<<cases<<" restored_suffixes="<<restores<<" backpressure="<<backpressure<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
