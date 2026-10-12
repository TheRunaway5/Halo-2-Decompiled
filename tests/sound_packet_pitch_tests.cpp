#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include <SDL.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <optional>
using namespace halo2;
int main(int argc,char **argv){try{
  if(argc!=6)throw FormatError("Supply LOCKOUT TRIPLICATE SHARED UPDATE_XBE FIXTURE");
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  const auto original=OriginalData::load_xbe(argv[4]);
  std::array<Cache,2> caches{Cache(argv[1],argv[3],original),Cache(argv[2],argv[3],original)};
  std::ifstream input(argv[5]);if(!input)throw FormatError("Missing original packet pitch fixture");
  unsigned bank,cases=0,failures=0;std::uint32_t sound,control,range,pitch;
  while(input>>std::dec>>bank>>std::hex>>sound>>control>>range>>pitch){
    std::optional<MultiplayerAudioSubmission> submitted;
    MultiplayerAudio audio(caches.at(bank),0x87e28a31,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
        [&](const auto &packet){submitted=packet;});
    audio.impulse(sound,std::bit_cast<float>(control));
    ++cases;
    if(!submitted || submitted->pitch_range!=range || submitted->source_pitch_bits!=pitch){
      if(failures<5)std::cerr<<"sound="<<std::hex<<sound<<" control="<<control<<" pitch="
          <<(submitted?submitted->source_pitch_bits:0)<<"/"<<pitch<<" range="
          <<(submitted?submitted->pitch_range:0xffff)<<"/"<<range<<'\n';
      ++failures;
    }
    audio.stop();
  }
  if(!input.eof() || cases<30)throw FormatError("Incomplete packet pitch fixture");
  std::cout<<std::dec<<cases<<" actual SDL packet pitch controls, "<<failures<<" mismatches\n";
  if(failures)throw FormatError("Native packet pitch differs from original ordinary voice math");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
