#include "halo2/particle_effect_color.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original TU1.5 particle color fixture");
  std::ifstream file(argv[1]);if(!file)return 77;
  const auto fixture=nlohmann::json::parse(file);unsigned count{};
  for(const auto &c:fixture.at("cases")) {
    const auto actual=halo2::source_particle_effect_color(c.at("previous"),c.at("a"),c.at("b"),c.at("flags"));
    if(actual!=c.at("result").get<std::uint32_t>())
      throw std::runtime_error("Original particle effect color differs at case "+std::to_string(count));
    ++count;
  }
  std::cout<<"PASS original TU1.5 particle effect color: "<<count<<" cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
