#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char**argv) {
  try {
    if(argc!=4)throw FormatError("Supply original countdown fixture, repository root and TU1.5 XBE");
    const std::filesystem::path root=argv[2];
    const std::array<std::filesystem::path,3> paths{root/"assets/disc/default.xbe",
        root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03",argv[3]};
    for(const auto& path:paths)if(!std::filesystem::exists(path))return 77;
    nlohmann::json data;std::ifstream(argv[1])>>data;
    const std::array<const char*,3> versions{"retail","tu11","u5"};
    for(unsigned i=0;i<3;++i) {
      const auto imported=OriginalData::load_xbe(paths[i]);
      if(!imported->game_over_constants)throw FormatError("Original countdown operands unavailable");
      const auto& c=*imported->game_over_constants;
      const std::array<float,3> values{c.seconds,c.fade_threshold_seconds,c.ambient_linear_gain};
      for(unsigned word=0;word<3;++word) {
        const auto hex=data.at("versions").at(versions[i]).at("constants").at(word).get<std::string>();
        std::uint32_t expected=0;
        for(unsigned b=0;b<4;++b)expected|=std::uint32_t(std::stoul(hex.substr(b*2,2),nullptr,16))<<(b*8);
        if(std::bit_cast<std::uint32_t>(values[word])!=expected)throw FormatError("Selected XBE countdown/fade operand differs");
      }
      if(std::bit_cast<std::uint32_t>(c.fade_seconds)!=0x3f800000)throw FormatError("Original immediate fade duration differs");
    }
    std::cout<<"PASS selected retail/TU1.1/TU1.5 OriginalData countdown and audio operands match original instruction fixtures\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
