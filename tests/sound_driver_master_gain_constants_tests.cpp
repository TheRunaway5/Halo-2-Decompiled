#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
}
int main(int argc,char **argv){
  try{
    require(argc==5,"Supply owned constant fixture and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    require(!halo2::OriginalData::load_xbe(argv[2])->sound_driver_master_gain_constants,
            "Unverified base master-gain constants were supplied");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream input(argv[1]);unsigned count=0;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;unsigned index;std::string text;
      require(bool(row>>kind>>index)&&kind=='C'&&index==count+1&&index<=2,"Original master-gain constant row");
      const auto &value=images[index-1]->sound_driver_master_gain_constants;
      require(bool(value),"Verified master-gain constants unavailable");
      for(const auto word:{value->one_bits,value->logarithm_scale_bits,value->quarter_bits}){
        require(bool(row>>text),"Incomplete original master-gain words");
        require(word==std::stoul(text,nullptr,16),"Imported original master-gain word differs");
      }
      require(!(row>>text),"Unexpected master-gain operand");++count;
    }
    require(count==2,"Incomplete original image coverage");
    std::cout<<"supportedImages=2 immutableMasterGainBytes=24 baseUnknown=absent mutableDriverAndOutputGlobals=separate\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
