#include "halo2/frontend_lobby_team.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
using namespace halo2;
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply the original local profile team fixture");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("Missing original local profile team fixture");
    unsigned count{};std::set<std::pair<std::string,int>> controllers;
    std::set<std::int32_t> values;
    for(std::string line;std::getline(input,line);) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);std::string version,byte;int slot,expected;std::int32_t committed;
      if(!(row>>version>>slot>>committed>>expected>>byte))throw std::runtime_error("Truncated profile fixture");
      const auto actual=source_local_profile_team(committed);
      if(actual!=expected || std::uint8_t(actual)!=std::stoul(byte,nullptr,16))
        throw std::runtime_error("Signed profile team differs from complete original converter/update");
      controllers.emplace(version,slot);values.insert(committed);++count;
    }
    if(controllers.size()!=12 || values.size()!=337 || count!=4044)
      throw std::runtime_error("Incomplete three-image/four-controller original profile fixture");
    std::cout<<"PASS "<<count<<" complete original profile team conversions/updates, including signed32 edges and byte aliases\n";
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
