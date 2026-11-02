#include "halo2/audio.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
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
}
int main()try{
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
