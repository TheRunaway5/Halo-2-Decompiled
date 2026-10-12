#include "halo2/vista_audio_framing.hpp"
#include "halo2/audio.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace halo2 {
namespace {
void check_vista(int result,const char* context) {
  if(result>=0)return;
  char message[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(result,message,sizeof(message));
  throw FormatError(std::string(context)+": "+message);
}
}
PcmSound decode_vista_serialized_sound(const EncodedSound& input) {
  if(input.codec!=4||!is_vista_serialized_audio(input.data))
    throw FormatError("not an admitted Vista serialized WMA sound");
  const auto media=parse_vista_serialized_media(input.data);
  // This family is independently observed in the supplied PC announcer chunks.
  // Other serialized codec types remain explicit unsupported inputs.
  constexpr unsigned char wma2_guid[16]={0x61,0x01,0,0,0,0,0x10,0,
                                         0x80,0,0,0xaa,0,0x38,0x9b,0x71};
  if(!std::equal(media.subtype.begin(),media.subtype.end(),wma2_guid,
                 [](std::byte a,unsigned char b){return std::to_integer<unsigned>(a)==b;})||
      media.format_tag!=0x161||media.channels<1||media.channels>2||
      media.sample_rate<8000||media.sample_rate>96000||!media.block_align||
      !media.average_bytes_per_second||media.packets.empty()||
      input.channels!=media.channels||input.sample_rate!=media.sample_rate)
    throw FormatError("unsupported or inconsistent Vista serialized WMA format");
  const auto* codec=avcodec_find_decoder(AV_CODEC_ID_WMAV2);
  if(!codec)throw FormatError("FFmpeg lacks Vista WMA2 decoder");
  struct Resources {
    AVCodecContext* decoder{};
    AVFrame* frame=av_frame_alloc();
    AVPacket* packet=av_packet_alloc();
    SwrContext* resampler{};
    ~Resources() {
      swr_free(&resampler);av_frame_free(&frame);av_packet_free(&packet);
      avcodec_free_context(&decoder);
    }
  } r;
  r.decoder=avcodec_alloc_context3(codec);
  if(!r.decoder||!r.frame||!r.packet)throw FormatError("cannot allocate Vista WMA decoder");
  r.decoder->sample_rate=int(media.sample_rate);
  av_channel_layout_default(&r.decoder->ch_layout,int(media.channels));
  r.decoder->block_align=int(media.block_align);
  r.decoder->bits_per_coded_sample=int(media.bits_per_sample);
  r.decoder->bit_rate=std::int64_t(media.average_bytes_per_second)*8;
  r.decoder->codec_tag=media.format_tag;
  if(!media.extra.empty()) {
    r.decoder->extradata=static_cast<std::uint8_t*>(
        av_mallocz(media.extra.size()+AV_INPUT_BUFFER_PADDING_SIZE));
    if(!r.decoder->extradata)throw FormatError("cannot allocate Vista WMA extra data");
    std::memcpy(r.decoder->extradata,media.extra.data(),media.extra.size());
    r.decoder->extradata_size=int(media.extra.size());
  }
  check_vista(avcodec_open2(r.decoder,codec,nullptr),"open Vista WMA2 decoder");
  PcmSound output;output.name=input.name;output.sound_class=input.sound_class;
  constexpr std::size_t maximum_samples=48000ull*600*2;
  auto append=[&](int capacity,const std::uint8_t** data,int samples) {
    if(capacity<0||std::size_t(capacity)>maximum_samples/2||
       output.stereo.size()>maximum_samples-std::size_t(capacity)*2)
      throw FormatError("decoded Vista WMA duration limit");
    const auto offset=output.stereo.size();
    output.stereo.resize(offset+std::size_t(capacity)*2);
    auto* target=reinterpret_cast<std::uint8_t*>(output.stereo.data()+offset);
    const int count=swr_convert(r.resampler,&target,capacity,data,samples);
    check_vista(count,"resample Vista WMA frame");
    output.stereo.resize(offset+std::size_t(count)*2);
    return count;
  };
  auto receive=[&] {
    for(;;) {
      const int result=avcodec_receive_frame(r.decoder,r.frame);
      if(result==AVERROR(EAGAIN)||result==AVERROR_EOF)break;
      check_vista(result,"decode Vista WMA frame");
      if(!r.resampler) {
        AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
        check_vista(swr_alloc_set_opts2(&r.resampler,&stereo,AV_SAMPLE_FMT_FLT,48000,
            &r.frame->ch_layout,AVSampleFormat(r.frame->format),r.decoder->sample_rate,
            0,nullptr),"configure Vista WMA resampler");
        check_vista(swr_init(r.resampler),"initialize Vista WMA resampler");
      }
      const auto capacity=av_rescale_rnd(
          swr_get_delay(r.resampler,r.decoder->sample_rate)+r.frame->nb_samples,
          48000,r.decoder->sample_rate,AV_ROUND_UP);
      if(capacity>std::numeric_limits<int>::max())
        throw FormatError("Vista WMA resampler size overflow");
      append(int(capacity),const_cast<const std::uint8_t**>(r.frame->extended_data),
             r.frame->nb_samples);
      av_frame_unref(r.frame);
    }
  };
  for(const auto& packet:media.packets) {
    if(packet.bytes.size()>std::size_t(std::numeric_limits<int>::max()))
      throw FormatError("Vista WMA packet exceeds decoder size");
    check_vista(av_new_packet(r.packet,int(packet.bytes.size())),"allocate Vista WMA packet");
    std::memcpy(r.packet->data,packet.bytes.data(),packet.bytes.size());
    check_vista(avcodec_send_packet(r.decoder,r.packet),"submit Vista WMA packet");
    av_packet_unref(r.packet);receive();
  }
  check_vista(avcodec_send_packet(r.decoder,nullptr),"flush Vista WMA decoder");receive();
  if(r.resampler)for(;;) {
    const int capacity=swr_get_out_samples(r.resampler,0);
    if(capacity<=0||!append(capacity,nullptr,0))break;
  }
  if(output.stereo.empty())throw FormatError("empty decoded Vista WMA stream");
  for(auto& sample:output.stereo) {
    if(!std::isfinite(sample))throw FormatError("nonfinite decoded Vista WMA sample");
    sample=std::clamp(sample,-1.f,1.f);
  }
  return output;
}
} // namespace halo2
