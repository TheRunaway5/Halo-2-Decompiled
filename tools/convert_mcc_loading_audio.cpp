#include "wwriff.h"
#include <vorbis/codec.h>
#include <ogg/ogg.h>
#include <bit>
#include <cmath>
#include <filesystem>
#include <vector>

namespace {
void word(std::ostream& out,std::uint32_t value,unsigned bytes=4) {
  for(unsigned i=0;i<bytes;++i)out.put(char(value>>(8*i)));
}
struct TemporaryOgg {
  std::filesystem::path path;
  ~TemporaryOgg(){std::error_code ignored;std::filesystem::remove(path,ignored);}
};
struct Vorbis {
  ogg_sync_state sync{};ogg_stream_state stream{};
  vorbis_info info{};vorbis_comment comment{};vorbis_dsp_state dsp{};vorbis_block block{};
  bool stream_open{},dsp_open{},block_open{};
  Vorbis(){ogg_sync_init(&sync);vorbis_info_init(&info);vorbis_comment_init(&comment);}
  ~Vorbis(){if(block_open)vorbis_block_clear(&block);if(dsp_open)vorbis_dsp_clear(&dsp);
    vorbis_comment_clear(&comment);vorbis_info_clear(&info);if(stream_open)ogg_stream_clear(&stream);ogg_sync_clear(&sync);}
};
}
// The caller owns a temporary directory. Never downloads a codec or reads a
// game executable. Reconstruct ordinary Vorbis packets with the unmodified
// BSD ww2ogg converter, then decode with the platform's admitted Vorbis
// dependency. Windows fixes the inverse-transform/overlap operation order in
// its private source-built decoder; the source-owned PCM profile is shared.
int main(int argc,char** argv)try {
  if(argc!=5)throw std::runtime_error("input WEM, output WAV, codebooks and declared frame count required");
  const auto expected=std::stoull(argv[4]);
  if(!expected || expected>44100u*600u || std::filesystem::file_size(argv[1])>16u*1024u*1024u)
    throw std::runtime_error("loading audio extent exceeds its bounded owner");
  TemporaryOgg temporary{std::string(argv[2])+".ogg"};
  if(std::filesystem::exists(temporary.path) || std::filesystem::exists(argv[2]))
    throw std::runtime_error("conversion requires new private outputs");
  {
    Wwise_RIFF_Vorbis input(argv[1],argv[3],false,false,kNoForcePacketFormat);
    std::ofstream output(temporary.path,std::ios::binary);input.generate_ogg(output);
    if(!output)throw std::runtime_error("incomplete reconstructed Ogg stream");
  }
  Vorbis vorbis;
  std::vector<float> pcm;pcm.reserve(expected*2);bool nonzero{};
  std::ifstream ogg(temporary.path,std::ios::binary);unsigned headers{};int serial{};
  while(ogg) {
    auto* buffer=ogg_sync_buffer(&vorbis.sync,8192);ogg.read(buffer,8192);
    if(ogg_sync_wrote(&vorbis.sync,long(ogg.gcount())))throw std::runtime_error("invalid Ogg buffer");
    ogg_page page{};int page_result;
    while((page_result=ogg_sync_pageout(&vorbis.sync,&page))!=0) {
      if(page_result<0)throw std::runtime_error("invalid loading Ogg page");
      if(!vorbis.stream_open){serial=ogg_page_serialno(&page);ogg_stream_init(&vorbis.stream,serial);vorbis.stream_open=true;}
      if(ogg_page_serialno(&page)!=serial || ogg_stream_pagein(&vorbis.stream,&page))
        throw std::runtime_error("loading track must contain one Ogg stream");
      ogg_packet packet{};int packet_result;
      while((packet_result=ogg_stream_packetout(&vorbis.stream,&packet))!=0) {
        if(packet_result<0)throw std::runtime_error("invalid loading Vorbis packet");
        if(headers<3) {
          if(vorbis_synthesis_headerin(&vorbis.info,&vorbis.comment,&packet))throw std::runtime_error("invalid loading Vorbis header");
          if(++headers==3) {
            if(vorbis.info.channels!=2 || vorbis.info.rate!=44100 || vorbis_synthesis_init(&vorbis.dsp,&vorbis.info))
              throw std::runtime_error("loading track must be stereo 44100Hz Vorbis");
            vorbis.dsp_open=true;
            if(vorbis_block_init(&vorbis.dsp,&vorbis.block))throw std::runtime_error("cannot initialize loading Vorbis block");
            vorbis.block_open=true;
          }
          continue;
        }
        // Wwise supplies a RIFF extent, not Ogg end-page granules. Match the
        // verified Wwise packet decoder: keep the last overlap block available
        // and trim only against that source-owned RIFF sample count.
        packet.granulepos=-1;packet.e_o_s=0;
        if(vorbis_synthesis(&vorbis.block,&packet) || vorbis_synthesis_blockin(&vorbis.dsp,&vorbis.block))
          throw std::runtime_error("invalid loading Vorbis synthesis packet");
        float** channels{};int count;
        while((count=vorbis_synthesis_pcmout(&vorbis.dsp,&channels))>0) {
          if(pcm.size()/2+count>expected+65536)throw std::runtime_error("excessive loading Vorbis duration");
          for(int i=0;i<count;++i)for(unsigned channel=0;channel<2;++channel) {
            const auto sample=channels[channel][i];
            if(!std::isfinite(sample))throw std::runtime_error("nonfinite loading sample");
            nonzero|=sample!=0;pcm.push_back(sample);
          }
          vorbis_synthesis_read(&vorbis.dsp,count);
        }
      }
    }
  }
  const auto decoded=pcm.size()/2;
  if(decoded<expected || !nonzero)throw std::runtime_error("incomplete or silent loading track: "+std::to_string(decoded));
  // The Wwise RIFF owner supplies the exact source sample count. Packet-tail
  // padding is not another loop interval or another audible authored sample.
  // Wwise's RIFF sample count owns the final extent. Reconstructed Ogg page
  // granules must not trim the last block; the independent stock-vgmstream
  // comparison covers every output sample, including that packet tail.
  pcm.resize(expected*2);
  std::ofstream out(argv[2],std::ios::binary);
  out.write("RIFF",4);word(out,36+unsigned(pcm.size()*4));out.write("WAVEfmt ",8);
  word(out,16);word(out,3,2);word(out,2,2);word(out,44100);word(out,44100*8);
  word(out,8,2);word(out,32,2);out.write("data",4);word(out,unsigned(pcm.size()*4));
  for(float sample:pcm)word(out,std::bit_cast<std::uint32_t>(sample));
  if(!out)throw std::runtime_error("incomplete loading PCM output");
  std::cout<<"{\"channels\":2,\"rate\":44100,\"frames\":"<<expected
           <<",\"decoded_frames\":"<<decoded<<",\"silent_tail_frames\":"<<(decoded<expected?expected-decoded:0)
           <<",\"finite\":true,\"nonzero\":true}\n";
  return 0;
}catch(const Parse_error& error){std::cerr<<error<<'\n';return 1;}
catch(const File_open_error& error){std::cerr<<error<<'\n';return 1;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
