#include "halo2/audio.hpp"
#include "halo2/vista_audio_framing.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>

// Offline conversion only: no SDL audio initialization/device/playback.
int main(int argc,char** argv)try {
  std::map<std::string,std::string> options;
  for(int i=1;i<argc;i+=2) {
    if(i+1>=argc||!options.emplace(argv[i],argv[i+1]).second)
      throw halo2::FormatError("missing or duplicate Vista decode option");
  }
  for(const auto& [key,value]:options)
    if(key!="--input"&&key!="--output"&&key!="--rate"&&key!="--channels"&&
       key!="--class"&&key!="--name")throw halo2::FormatError("unknown Vista decode option: "+key);
  for(const auto* key:{"--input","--output","--rate","--channels","--class","--name"})
    if(!options.contains(key))throw halo2::FormatError(
        "usage: decode_vista_audio --input cue.auds --output cue.wav --rate 44100 --channels 2 --class 50 --name tag_name");
  auto number=[&](const char* key) {
    std::size_t used=0;const auto value=std::stoul(options.at(key),&used);
    if(used!=options.at(key).size()||value>0xfffffffful)
      throw halo2::FormatError("invalid Vista decode numeric option");
    return unsigned(value);
  };
  const std::filesystem::path output=options.at("--output");
  if(std::filesystem::exists(output))throw halo2::FormatError("refusing to overwrite existing WAV");
  std::ifstream stream(options.at("--input"),std::ios::binary|std::ios::ate);
  if(!stream)throw halo2::FormatError("cannot open Vista audio chunk");
  const auto size=stream.tellg();
  if(size<=0||size>std::streamoff(128*1024*1024))throw halo2::FormatError("invalid Vista audio chunk extent");
  halo2::EncodedSound sound;sound.name=options.at("--name");sound.codec=4;
  sound.sample_rate=number("--rate");sound.channels=number("--channels");sound.sound_class=number("--class");
  if(sound.sound_class>=halo2::sound_class_count)throw halo2::FormatError("invalid Vista sound class");
  sound.data.resize(static_cast<std::size_t>(size));stream.seekg(0);
  stream.read(reinterpret_cast<char*>(sound.data.data()),std::streamsize(size));
  if(!stream)throw halo2::FormatError("truncated Vista audio chunk");
  if(!halo2::is_vista_serialized_audio(sound.data))
    throw halo2::FormatError("input is not the source-qualified PC serialized audio family");
  const auto media=halo2::parse_vista_serialized_media(sound.data);
  const auto pcm=halo2::decode_sound(sound);
  if(!output.parent_path().empty())std::filesystem::create_directories(output.parent_path());
  halo2::write_wav(pcm,output);
  if(!std::filesystem::exists(output)||std::filesystem::file_size(output)!=44+pcm.stereo.size()*2)
    throw halo2::FormatError("decoded WAV output extent mismatch");
  float peak=0;for(float value:pcm.stereo)peak=std::max(peak,std::abs(value));
  std::cout<<"{\"rate\":"<<pcm.rate<<",\"channels\":2,\"frames\":"<<pcm.frames()
           <<",\"peak\":"<<peak<<",\"source_packets\":"<<media.packets.size()<<"}\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
