#include "halo2/sound_dsp_encoder_stereo_tu15.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::uint32_t word(std::istream &in,unsigned bytes=4) {
  std::uint32_t value{};
  for(unsigned i=0;i<bytes;++i){const auto c=in.get();if(c==EOF)throw std::runtime_error("Incomplete EP fixture");value|=std::uint32_t(c)<<(i*8);}
  return value;
}
void check(std::uint32_t actual,std::uint32_t expected,const std::string &where) {
  if(actual!=expected)throw std::runtime_error(where+": "+std::to_string(actual)+" != "+std::to_string(expected));
}
}
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original EP stereo fixture directory");
  const std::filesystem::path root=argv[1];if(!std::filesystem::exists(root))return 77;
  std::ifstream manifest(root/"manifest.txt");if(!manifest)throw std::runtime_error("Missing EP fixture manifest");
  std::size_t cases{},compared{};
  for(std::string name;std::getline(manifest,name);) {
    if(name.empty())continue;
    std::ifstream input(root/(name+".input"),std::ios::binary),expected(root/(name+".output"),std::ios::binary);
    halo2::SoundDspEncoderStereoInputTu15 state{};
    state.scale_enabled=word(input);state.scale_word=word(input);
    for(auto &channel:state.channels)for(auto &sample:channel)sample=word(input);
    if(input.get()!=EOF)throw std::runtime_error("Unexpected EP input bytes");
    const auto actual=halo2::source_sound_dsp_encoder_stereo_tu15(state);
    for(unsigned i=0;i<256;++i)check(actual.left[i],word(expected),name+" left "+std::to_string(i));
    for(unsigned i=0;i<256;++i)check(actual.right[i],word(expected),name+" right "+std::to_string(i));
    for(unsigned i=0;i<512;++i)check(std::bit_cast<std::uint16_t>(actual.interleaved[i]),word(expected,2),name+" FIFO "+std::to_string(i));
    if(expected.get()!=EOF)throw std::runtime_error("Unexpected EP expected bytes");
    ++cases;compared+=1024;
  }
  if(!cases)throw std::runtime_error("No original EP stereo fixtures compared");
  std::cout<<"PASS "<<cases<<" original EP downmix/scale/FIFO cases, "<<compared<<" exact words\n";
  std::cout<<"LIMIT numerical stereo stage; GP/EP boot, routing, meters, native playback and timing remain separate\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
