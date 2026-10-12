#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
std::array<std::uint32_t,7> words(const halo2::SoundDriverStreamConstants &value) {
  return {value.one_bits,value.stereo_bits,value.pcm_queue,value.adpcm_queue,
          value.pcm_initialize_method,value.adpcm_initialize_method,value.completion_callback};
}
}
int main(int argc,char **argv) {
  try {
    require(argc==5,"Supply owned constants fixture and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    require(!halo2::OriginalData::load_xbe(argv[2])->sound_driver_stream_constants,
            "Unverified base stream constants were supplied");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream input(argv[1]);std::size_t count=0;
    for(std::string line;std::getline(input,line);) {
      std::istringstream row(line);char kind;unsigned index;std::string text;
      require(bool(row>>kind>>index)&&kind=='C'&&index==count+1&&index<=2,
              "Incomplete original stream constant row");
      const auto &constants=images[index-1]->sound_driver_stream_constants;
      require(bool(constants),"Verified stream constants unavailable");
      for(const auto value:words(*constants)) {
        require(bool(row>>text),"Incomplete original stream words");
        require(value==std::stoul(text,nullptr,16),"Imported original stream word differs");
      }
      require(!(row>>text),"Unexpected original stream operand");++count;
    }
    require(count==2,"Incomplete source image coverage");
    std::cout<<"supportedImages=2 immutableStreamBytes=56 baseUnknown=absent mutableQueuesDriverAndScratch=separate\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
