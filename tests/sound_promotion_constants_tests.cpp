#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
}
int main(int argc,char **argv) {
  try {
    require(argc==5,"Supply owned constants fixture and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    require(!halo2::OriginalData::load_xbe(argv[2])->sound_promotion_constants,
            "Unverified base promotion constants were supplied");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),
                            halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream input(argv[1]);std::size_t count=0;
    for(std::string line;std::getline(input,line);) {
      std::istringstream row(line);char kind;unsigned index;
      std::string milliseconds,seconds,one,extra;
      require(bool(row>>kind>>index>>milliseconds>>seconds>>one)&&
              kind=='C'&&index==count+1&&index<=2&&!(row>>extra),
              "Incomplete original promotion constant row");
      const auto &constants=images[index-1]->sound_promotion_constants;
      require(bool(constants),"Verified promotion constants unavailable");
      require(constants->milliseconds_per_second_bits==std::stoul(milliseconds,nullptr,16)&&
              constants->seconds_per_millisecond_bits==std::stoul(seconds,nullptr,16)&&
              constants->one_bits==std::stoul(one,nullptr,16),
              "Imported original promotion arithmetic words differ");
      ++count;
    }
    require(count==2,"Incomplete source image coverage");
    std::cout<<"supportedImages=2 immutablePromotionBytes=24 baseUnknown=absent mutableClockAndCounters=separate\n";
    return 0;
  }catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
