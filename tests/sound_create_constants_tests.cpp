#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
std::array<std::uint32_t,11> words(const halo2::SoundCreateConstants &value) {
  return {value.default_extra_gain_bits,value.minimum_gain_bits,value.maximum_float_bits,
          value.rng_scale_bits,value.unsigned_conversion_correction_bits,value.one_bits,
          value.upper_gain_bits,value.db_exponent_scale_bits,value.db_exponent_log_bits,
          value.linear_db_scale_bits,value.fade_epsilon_bits};
}
}
int main(int argc,char **argv) {
  try {
    require(argc==5,"Supply owned constants fixture and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    require(!halo2::OriginalData::load_xbe(argv[2])->sound_create_constants,
            "Unverified base sound-create constants were supplied");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),
                            halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream input(argv[1]);std::size_t count=0;
    for(std::string line;std::getline(input,line);) {
      std::istringstream row(line);char kind;unsigned index;std::string text;
      require(bool(row>>kind>>index)&&kind=='C'&&index==count+1&&index<=2,
              "Incomplete original sound-create constant row");
      const auto &constants=images[index-1]->sound_create_constants;
      require(bool(constants),"Verified sound-create constants unavailable");
      for(const auto value:words(*constants)) {
        require(bool(row>>text),"Incomplete original sound-create words");
        require(value==std::stoul(text,nullptr,16),"Imported original sound-create word differs");
      }
      require(!(row>>text),"Unexpected original sound-create operand");++count;
    }
    require(count==2,"Incomplete source image coverage");
    std::cout<<"supportedImages=2 immutableSoundCreateBytes=88 baseUnknown=absent mutableDelayScalePoolsAndRng=separate\n";
    return 0;
  }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
