#include "halo2/source_weapon_shader_input.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original primary-charge fixture");
  std::ifstream input(argv[1]);if(!input)throw std::runtime_error("Missing primary-charge fixture");
  nlohmann::json rows;input>>rows;
  for(const auto &row:rows) {
    const auto real=[&](const char *name){return std::bit_cast<float>(row.at(name).get<std::uint32_t>());};
    const auto actual=source_weapon_primary_charge(row.at("mode"),row.at("ticks"),real("charging"),real("step"));
    if(std::bit_cast<std::uint32_t>(actual.value)!=row.at("value").get<std::uint32_t>() || actual.enabled!=bool(row.at("enabled").get<unsigned>()))
      throw std::runtime_error("Original primary-trigger charge value/enable mismatch: "+row.dump());
  }
  std::cout<<"PASS whole original primary charge: "<<rows.size()<<" cases\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
