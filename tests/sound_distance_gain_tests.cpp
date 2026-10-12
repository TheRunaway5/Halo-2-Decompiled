#include "halo2/sound_driver_parameters.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc,char **argv){try{
  if(argc!=2)throw std::runtime_error("Supply original distance-gain fixtures");
  std::ifstream in(argv[1]);char kind;
  halo2::SoundDriverParameterConstants constants{};
  in>>kind>>std::hex>>constants.normalized_parameter_range_epsilon_bits>>constants.mono_channel_count_bits;
  if(!in || kind!='K')throw std::runtime_error("Missing original distance constants");
  std::uint32_t minimum,maximum,distance,expected;unsigned cases=0,mismatches=0;
  while(in>>minimum>>maximum>>distance>>expected){
    const auto actual=halo2::source_sound_distance_gain(minimum,maximum,distance,constants,{0x027f,0x1f80});
    if(actual!=expected){
      if(mismatches<5)std::cerr<<"Distance case "<<std::dec<<cases<<std::hex<<": minimum="<<minimum
        <<" maximum="<<maximum<<" distance="<<distance<<" gain="<<actual<<'/'<<expected<<'\n';
      ++mismatches;
    }
    ++cases;
  }
  if(!in.eof() || cases<300)throw std::runtime_error("Incomplete original distance controls");
  std::cout<<std::dec<<cases<<" original distance-gain controls, "<<mismatches<<" mismatches\n";
  if(mismatches)throw std::runtime_error("Original distance falloff differs");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
