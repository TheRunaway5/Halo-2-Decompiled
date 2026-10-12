#include "halo2/player_action_edges.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
std::vector<unsigned char> bytes(const std::string &hex) {
  std::vector<unsigned char> result;
  for(std::size_t i=0;i<hex.size();i+=2)result.push_back(std::stoul(hex.substr(i,2),nullptr,16));
  return result;
}
std::uint32_t get(const auto &b,unsigned at,unsigned n) {
  std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(b.at(at+i))<<(8*i);return v;
}
void put(auto &b,unsigned at,std::uint32_t v,unsigned n) {
  for(unsigned i=0;i<n;++i)b.at(at+i)=v>>(8*i);
}
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
}
int main(int argc,char **argv){try {
  check(argc==2,"Supply original player-action fixture");std::ifstream file(argv[1]);
  check(bool(file),"Missing original player-action fixture");unsigned cases=0;std::string line;
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream in(line);std::array<std::string,4> h;for(auto &v:h)in>>v;
    auto p=bytes(h[0]),a=bytes(h[1]);
    SourcePlayerActionEdges state{get(p,0x38,4),std::uint16_t(get(p,0x3c,2)),std::uint16_t(get(p,2,2))};
    auto game=get(a,0,4);auto actions=std::uint16_t(get(a,0x1c,2));
    source_player_action_edges(state,game,actions);
    put(p,0x38,state.previous_gameplay,4);put(p,0x3c,state.previous_buttons,2);put(p,2,state.flags,2);
    put(a,0,game,4);put(a,0x1c,actions,2);
    check(p==bytes(h[2]) && a==bytes(h[3]),"Original150DD0 complete buffer differs");++cases;
  }
  check(cases==4096,"Incomplete original action-edge coverage");
  check(source_pickup_hold_threshold(.2333f)==233,"Authored threshold conversion");
  check(source_pickup_buttons(true,232,false,0,233)==1,"232ms pickup boundary");
  check(source_pickup_buttons(true,233,false,0,233)==3,"233ms pickup boundary");
  check(source_pickup_buttons(false,0,true,233,233)==0x30,"Secondary threshold bits");
  std::cout<<"PASS "<<cases<<" original full player/action buffers and pickup threshold boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
