#include "halo2/particle_collision.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original TU1.5 collision decisions");
  std::ifstream file(argv[1]);if(!file)throw std::runtime_error("Missing original collider decisions");
  unsigned count{};
  for(std::string line;std::getline(file,line);) {
    const auto row=nlohmann::json::parse(line);
    if(halo2::source_particle_collision_dies(row.at("flags"),row.at("type"))!=row.at("dead"))
      throw std::runtime_error("Collider retirement differs from TU1.5 at case "+std::to_string(count));
    ++count;
  }
  if(count!=27648)throw std::runtime_error("Incomplete original collision bank");
  std::cout<<"PASS "<<count<<" original TU1.5 collision retirement decisions\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
