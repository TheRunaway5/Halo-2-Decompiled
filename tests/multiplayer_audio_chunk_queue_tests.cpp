#include "halo2/multiplayer_audio.hpp"
#include "halo2/audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/original_data.hpp"
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

int original_reference(const std::filesystem::path& map,const std::filesystem::path& report_path) {
 constexpr auto sound=0xe8bb0746u;
 constexpr unsigned rate=44100;
 const auto cache=std::make_shared<const Cache>(map,map.parent_path(),OriginalData::native_tu15());
 const auto encoded=cache->load_sound(sound,0,1);
 need(encoded.name.ends_with("cov_damage_med") && encoded.codec==1 && encoded.sample_rate==rate &&
     encoded.channels==1 && encoded.data.size()==66132 && encoded.packet_chunks.size()==2,
     "Original Coagulation permutation differs from the selected reference");
 const auto pcm=decode_xbox_adpcm_native(encoded);
 need(pcm.frames()==117568 && pcm.chunk_boundaries==std::vector<PcmSound::ChunkBoundary>{{32760,58240},{66132,117568}},
     "Original Coagulation chunk boundaries differ");
 const auto data=std::make_shared<const std::vector<float>>(pcm.stereo);
 const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
 unsigned submissions{};
 MultiplayerAudio audio(cache,0x5821b36du,manual,[&](const auto& event){
   need(event.sound==sound && event.pitch_range==0 && event.permutation==1 &&
       event.frames==pcm.frames() && event.pcm_sample_rate==rate,"Original game audio lost its native selected packet");
   ++submissions;
 });
 audio.enroll_continuation();
 const auto definition=cache->load_sound_variation(sound,0);
 const auto center=std::bit_cast<std::int16_t>(Bytes(definition.pitch_ranges.at(0).interval).u16(0));
 MultiplayerLoopPacketTu15 request{sound,0xffffffffu,0,0,1,{float(center)+137.f,-3,1}};
 request.single_sound=true;
 const auto played=audio.play_loop_packet(request);
 need(played && submissions==1 && !audio.loop_packet_accepts_input(*played),"Original chunks did not occupy their own packet slots");
 const auto parameters=audio.voice_parameters(*played);
 const auto reference=[&]{
   auto device=std::make_unique<SdlSoundPlatform>(manual);
   auto voice=device->create_voice(1,parameters);
   need(device->submit(voice,{data,0,pcm.frames(),1,{},rate})==SdlSoundPlatform::Submission::Accepted,
       "Contiguous native rate comparison failed to submit");
   return device;
 };
 auto continuous=reference();
 constexpr std::size_t prefix=113;
 same(audio.render(prefix),continuous->render(prefix));
 const auto saved=audio.checkpoint();
 const auto finish=[&](const MultiplayerAudio::VoiceReceipt& voice,SdlSoundPlatform& device) {
   SdlSoundPlaybackPosition position{};
   for(unsigned block=0;audio.voice_active(voice);++block) {
     need(block<1000,"Native chunk playback did not finish");
     const auto frames=std::array<std::size_t,3>{1921,337,513}[block%3];
     same(audio.render(frames),device.render(frames));
     position=audio.playback_position(voice);
     audio.update();
   }
   need(std::abs(position.source_frames-double(pcm.frames())*48000.0/rate)<.00001,
       "Original rate playback changed the 48 kHz source time contract");
 };
 finish(*played,*continuous);
 auto restore=audio.prepare_restore(saved);
 const auto resumed=restore.voice(*played);
 need(bool(resumed),"Native rate restore lost the parent voice");
 {
   auto held=audio.hold_publication(restore);
   need(audio.ready(restore,held) && audio.begin_commit(restore,held) && audio.commit(restore,held),"Native rate restore failed");
 }
 continuous=reference();
 (void)continuous->render(prefix);
 finish(*resumed,*continuous);
 need(submissions==1 && !audio.voices(),"Native rate restore duplicated a packet or leaked its voice");

 audio.listeners(std::array{Vec3{10,0,0}});
 request.position=Vec3{};
 request.parameters.minimum_distance=12.f;
 request.parameters.maximum_distance=30.f;
 const auto located=audio.play_loop_packet(request);
 need(located && audio.voice_parameters(*located).routes.front().linear_gain>0.f,
     "The inherited distance bounds silenced an in-range source");
 request.parameters.minimum_distance=1.f;
 request.parameters.maximum_distance=5.f;
 audio.loop_packet_parameters(*located,request.parameters);
 need(audio.voice_parameters(*located).routes.front().linear_gain==0.f,
     "Updated parent distance bounds did not retire out-of-range output");
 audio.stop();

 unsigned selected{};
 std::vector<SdlSoundCompletion> completed;
 MultiplayerAudio impulse_audio(cache,0x5821b36du,manual,[&](const auto& event){
   need(event.sound==sound && event.pitch_range==0 && event.pcm_sample_rate==rate,
       "Original impulse did not enter the native rate path");
   selected=event.permutation;
 });
 impulse_audio.enroll_continuation();
 std::optional<MultiplayerAudio::VoiceReceipt> impulse;
 for(unsigned attempt=0;attempt<16;++attempt) {
   impulse=impulse_audio.play_impulse_receipt(sound,1,MultiplayerAudioKind::Impulse,0xffffffffu,0,
       [&](const auto& packet){completed.push_back(packet);});
   need(bool(impulse),"Original impulse did not admit a voice");
   if(selected==1)break;
   impulse_audio.stop();
   need(completed.size()==1 && completed.front().status==SdlSoundCompletionStatus::Flushed,
       "Unconsumed original impulse lost its single flush notification");
   completed.clear();
 }
 need(selected==1,"Original impulse did not select the two-chunk reference");
 const auto impulse_parameters=impulse_audio.voice_parameters(*impulse);
 need(impulse_parameters.native.has_value(),"Original impulse lacks a pitch register");
 const auto step=std::exp2(double(impulse_parameters.native->pitch)/4096.0);
 const auto crossing=std::size_t(std::ceil(58240.0/step))+17;
 need(crossing<std::size_t(std::ceil(double(pcm.frames())/step)),"Impulse lacks a first-chunk witness");
 SdlSoundPlatform impulse_reference(manual);
 const auto reference_voice=impulse_reference.create_voice(1,impulse_parameters);
 need(impulse_reference.submit(reference_voice,{data,0,pcm.frames(),1,{},rate})==SdlSoundPlatform::Submission::Accepted,
     "Contiguous impulse comparison failed to submit");
 same(impulse_audio.render(crossing),impulse_reference.render(crossing));
 impulse_audio.update();
 need(completed.empty() && impulse_audio.voice_active(*impulse),"First ADPCM chunk retired the impulse early");
 const auto suffix=std::size_t(std::ceil(double(pcm.frames())/step))-crossing+17;
 same(impulse_audio.render(suffix),impulse_reference.render(suffix));
 need(completed.empty(),"Mixer invoked the impulse completion owner");
 impulse_audio.update();
 impulse_audio.stop();
 need(!impulse_audio.voices() && completed.size()==1 && completed.front().submission==2 &&
     completed.front().pcm_frames==59328 && completed.front().status==SdlSoundCompletionStatus::Success,
     "Original impulse lost its actual terminal packet or retired twice");

 const J result{{"passed",true},{"sound",sound},{"pitch",0},{"permutation",1},{"pcm_sample_rate",rate},
     {"native_frames",pcm.frames()},{"chunk_frames",{58240,59328}},{"bit_identical_contiguous_output",true},
     {"restored_suffix_verified",true},{"source_distance_overrides_verified",true},
     {"impulse_terminal_packet_verified",true},{"retained_large_artifacts",J::array()}};
 std::ofstream(report_path)<<result.dump(2)<<'\n';
 std::cout<<"PASS original Coagulation native-rate packet, split PCM and checkpoint replay\n";
 return 0;
}
}
int main(int argc,char**argv)try{
 if(argc==4 && std::string_view(argv[1])=="--xbox"){
   if(!std::filesystem::is_regular_file(argv[2]))return 77;
   return original_reference(argv[2],argv[3]);
 }
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
   const auto decoded=decode_xbox_adpcm_native(encoded);need(decoded.chunk_boundaries.size()==encoded.packet_chunks.size(),"Missing decoder-owned boundaries");
   unsigned observed{};MultiplayerAudio audio(cache,0x5821b36du,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},[&](const auto& event){
    need(event.sound==binding.sound.value&&event.pitch_range==pitch&&event.permutation==p&&event.frames==decoded.frames()&&
        event.pcm_sample_rate==decoded.rate&&event.rng_before==event.rng_after,"Chunk delivery changed the selected controller permutation or rate");++observed;});
   audio.enroll_continuation();audio.bind_native_loop_metadata(metadata);
   const auto definition=metadata.load(*cache,binding.sound,pitch);
   const auto center=std::bit_cast<std::int16_t>(Bytes(definition.pitch_ranges[pitch].interval).u16(0));
   MultiplayerLoopPacketTu15 request{binding.sound.value,0xffffffffu,0,pitch,p,{float(center)+137.f,-3,1},{}};
   request.single_sound=true;
   const auto voice=audio.play_loop_packet(request);need(voice&&observed==1,"Selected multi-chunk sound was not delivered");
   need(!audio.loop_packet_accepts_input(*voice),"Multi-chunk permutation occupied only one native packet slot");
   need(!audio.queue_loop_packet(*voice,p)&&observed==1,"Full chunk queue consumed another controller selection");++backpressure;
   const auto parameters=audio.voice_parameters(*voice);const auto data=std::make_shared<const std::vector<float>>(decoded.stereo);
   SdlSoundPlatform::Voice reference_voice{};
   const auto make_reference=[&]{auto device=std::make_unique<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
    reference_voice=device->create_voice(0x1234,parameters);need(device->submit(reference_voice,{data,0,decoded.frames(),0x888,{},decoded.rate})==SdlSoundPlatform::Submission::Accepted,"Reference PCM was not accepted");return device;};
   need(parameters.native.has_value(),"Original authored voice lost its native pitch register");
   const auto rate=std::exp2(double(parameters.native->pitch)/4096.0);
   const auto normalized_frames=double(decoded.frames())*48000.0/decoded.rate;
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
   finish();need(std::abs(position.source_frames-normalized_frames)<.00001,"Chunk queue lost or invented consumed source frames");
   const auto physical=audio.mixed_frames();auto replacement=audio.prepare_restore(checkpoint);const auto target=replacement.voice(*voice);need(bool(target),"Chunk publication lost its retained voice");
   {auto held=audio.hold_publication(replacement);need(audio.ready(replacement,held)&&audio.begin_commit(replacement,held)&&audio.commit(replacement,held),"Playing chunk publication failed");}
   need(audio.mixed_frames()==physical&&!audio.voice_active(*voice),"Chunk restore rewound output or revived a stale receipt");
   current=*target;reference=make_reference();(void)reference->render(137);blocks=0;finish();need(observed==1,"Chunk restore duplicated its permutation selection");
   need(std::abs(position.source_frames-normalized_frames)<.00001,"Chunk restore lost its unsubmitted remainder");
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
