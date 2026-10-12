#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
int main(int argc,char** argv) {
  try {
    if(argc!=5)throw FormatError("Supply original result-stat fixture and retail/TU1.1/U5 XBEs");
    std::ifstream stream(argv[1]);nlohmann::json fixture;stream>>fixture;
    const char* names[]{"retail","tu11","u5"};
    for(unsigned image=0;image<3;++image) {
      if(!std::filesystem::is_regular_file(argv[image+2]))return 77;
      const auto data=OriginalData::load_xbe(argv[image+2]);
      if(!data->result_stat_descriptors)throw FormatError("Original result-stat descriptor import unavailable");
      const auto& rows=fixture.at("versions").at(names[image]).at("descriptors");
      for(unsigned i=0;i<45;++i) {
        const auto& d=data->result_stat_descriptors->at(i);
        if(d.minimum!=rows[i].at("minimum") || d.maximum!=rows[i].at("maximum"))throw FormatError("Imported statistic bounds differ");
        const auto tail=rows[i].at("remaining_hex").get<std::string>();
        for(unsigned n=0;n<12;++n)
          if(d.remaining[n]!=std::stoul(tail.substr(n*2,2),nullptr,16))throw FormatError("Unclassified descriptor tail changed");
      }
    }
    std::cout<<"PASS all45 original16-byte result-stat descriptors from retail/TU1.1/U5 images\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
