#include "halo2/audio.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
extern "C" {
#include <libswresample/swresample.h>
}
using namespace halo2;
namespace {
void need(bool v,const char* why){if(!v)throw FormatError(why);}
template<class F>void rejects(F f){try{f();}catch(const FormatError&){return;}throw FormatError("Invalid stock chunk extent admitted");}
struct Reference {
 std::vector<float> pcm;std::vector<PcmSound::ChunkBoundary> boundaries;
};
// Bypass the production codec: known interleaved S16 samples go straight to
// an independently owned resampler, one whole resource chunk per call.
Reference resample(std::span<const std::int16_t> input,const EncodedSound& encoded){
 struct Owned {SwrContext* p{};~Owned(){swr_free(&p);}}r;
 AVChannelLayout from;av_channel_layout_default(&from,int(encoded.channels));AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
 need(swr_alloc_set_opts2(&r.p,&stereo,AV_SAMPLE_FMT_FLT,48000,&from,AV_SAMPLE_FMT_S16,int(encoded.sample_rate),0,nullptr)>=0,"Reference resampler allocation failed");
 av_channel_layout_uninit(&from);need(swr_init(r.p)>=0,"Reference resampler initialization failed");
 Reference out;std::size_t source{},bytes{};
 const auto convert=[&](const std::uint8_t* data,int frames){
  const auto capacity=swr_get_out_samples(r.p,frames);need(capacity>=0,"Reference capacity failed");
  const auto at=out.pcm.size();out.pcm.resize(at+std::size_t(capacity)*2);
  auto* destination=reinterpret_cast<std::uint8_t*>(out.pcm.data()+at);
  const auto emitted=swr_convert(r.p,&destination,capacity,data?&data:nullptr,frames);need(emitted>=0,"Reference conversion failed");
  out.pcm.resize(at+std::size_t(emitted)*2);return emitted;
 };
 for(const auto& chunk:encoded.packet_chunks){const auto samples=chunk.bytes/2,frames=samples/encoded.channels;
  convert(reinterpret_cast<const std::uint8_t*>(input.data()+source),int(frames));source+=samples;bytes+=chunk.bytes;
  out.boundaries.push_back({bytes,out.pcm.size()/2});}
 while(convert(nullptr,0)){}
 out.boundaries.back().end_frame=out.pcm.size()/2;return out;
}

void native_adpcm(){
 const std::array blocks{std::array<std::int16_t,2>{-32768,32767},
                         std::array<std::int16_t,2>{-7123,2930}};
 for(unsigned channels:{1,2}){
  EncodedSound input;input.codec=1;input.channels=channels;input.sample_rate=44100;
  for(const auto& block:blocks){
   const auto end=input.data.size()+36*channels;
   // IMA index zero and all zero deltas leave the predictor unchanged.
   for(unsigned ch=0;ch<channels;++ch){
    const auto word=std::bit_cast<std::uint16_t>(block[ch]);
    input.data.insert(input.data.end(),{std::byte(word),std::byte(word>>8),std::byte{},std::byte{}});
   }
   input.data.resize(end);input.packet_chunks.push_back({36*channels,0,false});
  }
  const auto decoded=decode_xbox_adpcm_native(input);
  const std::vector<PcmSound::ChunkBoundary> boundaries{{36*channels,64},{72*channels,128}};
  need(decoded.rate==44100&&decoded.frames()==128&&decoded.chunk_boundaries==boundaries,
       "Native Xbox ADPCM changed its sample rate or block extent");
  for(std::size_t frame=0;frame<decoded.frames();++frame){
   const auto& expected=blocks[frame/64];
   need(decoded.stereo[frame*2]==expected[0]/32768.f&&
        decoded.stereo[frame*2+1]==expected[channels-1]/32768.f,
        "Native Xbox ADPCM changed the signed sample values");
  }
  auto whole=input;whole.packet_chunks.clear();
  const auto unsplit=decode_xbox_adpcm_native(whole);
  need(unsplit.chunk_boundaries.empty()&&unsplit.stereo==decoded.stereo,
       "Native Xbox ADPCM changed across the resource boundary");
  need(decode_sound(input).rate==48000,"Existing stock decoder lost its 48 kHz output");
  --input.packet_chunks.front().bytes;
  rejects([&]{(void)decode_xbox_adpcm_native(input);});
 }
}

void original_adpcm_fixture(const char* shared_map){
 std::ifstream owner(shared_map,std::ios::binary);
 need(bool(owner),"Cannot open original ADPCM resource owner");
 EncodedSound input;input.codec=1;input.channels=1;input.sample_rate=44100;
 // cov_damage_med10, authenticated original shared.map extents from the
 // prepared original sound catalog. The optional fixture stays read-only.
 for(const auto [offset,count]:{std::pair{52873728,32760},std::pair{52906496,33372}}){
  const auto first=input.data.size();input.data.resize(first+count);
  owner.seekg(offset);owner.read(reinterpret_cast<char*>(input.data.data()+first),count);
  need(bool(owner),"Original ADPCM fixture resource read failed");
  input.packet_chunks.push_back({std::uint32_t(count),0,false});
 }
 const auto decoded=decode_xbox_adpcm_native(input);
 const std::vector<PcmSound::ChunkBoundary> boundaries{{32760,58240},{66132,117568}};
 need(decoded.rate==44100&&decoded.frames()==117568&&decoded.chunk_boundaries==boundaries,
      "Actual original ADPCM changed its codec block boundaries");
 std::uint64_t digest=0xcbf29ce484222325ull;
 for(float value:decoded.stereo){
  const auto bits=std::bit_cast<std::uint32_t>(value);
  for(unsigned shift=0;shift<32;shift+=8)digest=(digest^std::uint8_t(bits>>shift))*0x100000001b3ull;
 }
 // Reference fingerprint from a scalar decode of the exact 66,132 bytes.
 need(digest==0xa30081b0b7d3f495ull,"Original ADPCM native sample bytes differ from scalar reference");
 std::cout<<"PASS original cov_damage_med10: native_frames=117568 chunk_end_frames=58240,117568\n";
}
}
int main()try{
 native_adpcm();
 if(const auto* shared=std::getenv("HALO2_TEST_ORIGINAL_SHARED_MAP"))original_adpcm_fixture(shared);
 unsigned cases{},zero_frame_chunks{},delayed_boundaries{},controls{};
 for(unsigned rate:{22050,32000,44100,48000})for(unsigned channels:{1,2,4})for(unsigned codec:{0,3}){
  EncodedSound e;e.name="declared native PCM";e.codec=codec;e.channels=channels;e.sample_rate=rate;
  std::vector<std::int16_t> source;
  for(unsigned frames:{1,7,9,61,1027,13}){e.packet_chunks.push_back({frames*channels*2,0,false});
   for(unsigned n=0;n<frames*channels;++n){const auto v=std::int16_t(int((source.size()*9176u)%20001u)-10000);source.push_back(v);
    const auto word=std::bit_cast<std::uint16_t>(v);e.data.push_back(std::byte(codec==3?word:word>>8));e.data.push_back(std::byte(codec==3?word>>8:word));}}
  const auto expected=resample(source,e);const auto actual=decode_sound(e);
  need(actual.chunk_boundaries==expected.boundaries,"Stock chunk map differs from independent emitted-frame reference");
  need(actual.stereo==expected.pcm,"Stock chunk decode changed independently resampled known PCM");
  auto unsliced=e;unsliced.packet_chunks.clear();const auto legacy=decode_sound(unsliced);
  need(legacy.stereo==actual.stereo&&legacy.chunk_boundaries.empty(),"Resource boundaries changed continuous PCM or invented missing metadata");
  std::size_t before{};for(const auto& b:actual.chunk_boundaries){if(b.end_frame==before)++zero_frame_chunks;
   if(b.encoded_end!=e.data.size()&&b.end_frame*e.data.size()!=actual.frames()*b.encoded_end)++delayed_boundaries;before=b.end_frame;}
  auto bad=e;bad.packet_chunks.front().bytes=0;rejects([&]{(void)decode_sound(bad);});++controls;
  bad=e;bad.packet_chunks.front().bytes=1;rejects([&]{(void)decode_sound(bad);});++controls;
  bad=e;bad.packet_chunks.back().bytes-=channels*2;rejects([&]{(void)decode_sound(bad);});++controls;
  bad=e;bad.packet_chunks.back().bytes+=channels*2;rejects([&]{(void)decode_sound(bad);});++controls;
  bad=e;bad.packet_chunks.front().bytes=0xffffffffu;rejects([&]{(void)decode_sound(bad);});++controls;
  ++cases;
 }
 EncodedSound adpcm;adpcm.codec=1;adpcm.channels=1;adpcm.sample_rate=44100;adpcm.data.resize(72);
 adpcm.packet_chunks={{35,0,false},{37,0,false}};rejects([&]{(void)decode_sound(adpcm);});++controls;
 need(cases==24&&zero_frame_chunks>0&&delayed_boundaries>0,"Stock mapping lacks delayed/empty boundary witnesses");
 std::cout<<"PASS stock chunk mapping: independent_resampler_cases="<<cases<<" zero_frame_chunks="<<zero_frame_chunks<<" non_proportional_boundaries="<<delayed_boundaries<<" refusals="<<controls<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
