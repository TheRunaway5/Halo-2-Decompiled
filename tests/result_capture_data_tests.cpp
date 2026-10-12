#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstring>

using namespace halo2;
int main(int argc,char** argv) {
  try {
    if(argc!=5)throw FormatError("Supply capture fixture and retail/TU1.1/U5 XBEs");std::ifstream stream(argv[1]);nlohmann::json fixture;stream>>fixture;
    const char* names[]{"retail","tu11","u5"};
    for(unsigned n=0;n<3;++n) {
      if(!std::filesystem::is_regular_file(argv[n+2]))return 77;const auto original=OriginalData::load_xbe(argv[n+2]);
      if(!original->result_capture_constants)throw FormatError("Original capture constants unavailable");const auto& c=*original->result_capture_constants;
      const auto& v=fixture.at("versions").at(names[n]);const auto raw=v.at("weapon_descriptors_hex").get<std::string>();
      const auto* bytes=reinterpret_cast<const std::uint8_t*>(c.weapon_descriptors.data());
      for(unsigned i=0;i<112;++i)if(bytes[i]!=std::stoul(raw.substr(i*2,2),nullptr,16))throw FormatError("Imported weapon descriptor or unclassified tail differs");
      for(unsigned cause=0;cause<256;++cause)if(c.cause_weapons[cause&63]!=v.at("cause_mapping")[cause])throw FormatError("Imported compiled reporting-type dispatch differs");
      if(c.multikill_seconds!=4)throw FormatError("Original imported multikill operand differs");
    }
    std::cout<<"PASS original7 weapon descriptors,64-entry reporting-type dispatch and multikill operand in all3 images\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
