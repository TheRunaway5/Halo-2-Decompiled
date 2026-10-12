#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include <SDL.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
using namespace halo2;
namespace {double peak(std::span<const float> pcm){double result=0;for(auto sample:pcm)result=std::max(result,std::abs(double(sample)));return result;}}
int main(int argc,char **argv){try{
  if(argc!=6)throw FormatError("Supply LOCKOUT TRIPLICATE SHARED UPDATE_XBE FIXTURE");
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  const auto original=OriginalData::load_xbe(argv[4]);
  std::array<Cache,2> caches{Cache(argv[1],argv[3],original),Cache(argv[2],argv[3],original)};
  std::ifstream input(argv[5]);if(!input)throw FormatError("Missing original packet gain fixture");
  unsigned bank,cases=0,failures=0;int permutation_gain;std::uint32_t sound,control,combined,packet_gain,volume;
  while(input>>std::dec>>bank>>std::hex>>sound>>control>>combined>>std::dec>>permutation_gain>>std::hex>>packet_gain>>volume){
    std::optional<MultiplayerAudioSubmission> submitted;
    MultiplayerAudio audio(caches.at(bank),0x87e28a31,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
        [&](const auto &packet){submitted=packet;});
    audio.impulse(sound,std::bit_cast<float>(control));
    if(!submitted)throw FormatError("Audible original gain input submitted no native packet");
    const auto pcm=decode_sound(caches.at(bank).load_sound(sound,submitted->pitch_range,submitted->permutation));
    // Pitch is exactly zero for these authored single-range controls. This
    // compares actual mixed samples with the original integer volume request;
    // amplitude conversion remains the separately identified native service.
    const auto expected=peak(pcm.stereo)*std::pow(10.,double(std::bit_cast<std::int32_t>(volume))/2000.);
    const auto actual=peak(audio.render(pcm.frames()));++cases;
    if(expected<=0 || std::abs(actual-expected)>1e-6){
      if(failures<5)std::cerr<<"sound="<<std::hex<<sound<<" control="<<control<<std::dec
          <<" PCM peak="<<actual<<"/"<<expected<<" original volume="<<std::bit_cast<std::int32_t>(volume)<<'\n';
      ++failures;
    }
    audio.stop();
  }
  if(!input.eof() || cases<30)throw FormatError("Incomplete packet gain fixture");
  std::cout<<cases<<" actual mixed PCM gain controls, "<<failures<<" mismatches\n";
  if(failures)throw FormatError("Native packet gain differs from original ordinary voice math");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
