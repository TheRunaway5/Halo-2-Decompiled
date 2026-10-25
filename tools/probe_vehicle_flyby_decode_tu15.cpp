#include "halo2/audio.hpp"
#include "halo2/prepared_original_sound_bank.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

// Probe only the production native decoder. Encoded bytes, codec metadata and
// packet descriptors are read independently from authenticated asset owners.
int main(int argc,char** argv) {
  try {
    if(argc!=2&&argc!=4)throw std::runtime_error("Expected an owned input manifest and optional prepared bank/digest");
    const auto bank=argc==4?halo2::PreparedOriginalSoundBank::load(argv[2],argv[3]):nullptr;
    std::ifstream manifest(argv[1]);
    std::string input,output;
    unsigned cases{};
    while(manifest>>input>>output) {
      halo2::EncodedSound sound;
      unsigned chunks{};
      if(!(manifest>>sound.sample_rate>>sound.channels>>sound.codec>>sound.samples>>sound.sound_class>>chunks))
        throw std::runtime_error("Truncated decoder input metadata");
      for(unsigned n=0;n<chunks;++n) {
        halo2::EncodedSound::PacketChunk chunk;
        unsigned reset{};
        if(!(manifest>>chunk.bytes>>chunk.pcm_bytes>>reset)||reset>1)
          throw std::runtime_error("Truncated decoder chunk metadata");
        chunk.reset_after=reset;
        sound.packet_chunks.push_back(chunk);
      }
      if(bank&&cases<6)sound=bank->encoded(0,cases);
      else {
        std::ifstream encoded(input,std::ios::binary);
        const std::vector<char> raw{std::istreambuf_iterator<char>(encoded),{}};
        for(char c:raw)sound.data.push_back(std::byte(static_cast<unsigned char>(c)));
      }
      const auto pcm=halo2::decode_sound(sound);
      if(pcm.rate!=48000||!pcm.frames())throw std::runtime_error("Missing native PCM frames");
      std::ofstream decoded(output,std::ios::binary);
      decoded.write(reinterpret_cast<const char*>(pcm.stereo.data()),std::streamsize(pcm.stereo.size()*sizeof(float)));
      if(!decoded)throw std::runtime_error("Failed writing owned temporary PCM");
      ++cases;
    }
    if(!manifest.eof()||cases!=12)throw std::runtime_error("Incomplete six original/six MCC permutation cohort");
    std::cout<<"Native flyby decoder cases="<<cases<<'\n';
    if(bank)std::cout<<"Native prepared original sound cases=6\n";
  } catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
