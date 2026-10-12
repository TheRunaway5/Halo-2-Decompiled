#include "halo2/frontend_model_lighting.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
int main(int argc,char**argv) {
  if(argc!=2)throw std::runtime_error("original fixture path required");
  std::ifstream file(argv[1]);if(!file)throw std::runtime_error("original fixture unavailable");
  unsigned count=0;std::string line;
  while(std::getline(file,line)) {
    std::istringstream in(line);unsigned op;in>>op;
    std::array<float,5> args;std::array<float,20> initial;std::array<std::uint32_t,20> expected;
    in>>std::hex;std::uint32_t word;
    for(auto&v:args){in>>word;v=std::bit_cast<float>(word);}
    for(auto&v:initial){in>>word;v=std::bit_cast<float>(word);}
    for(auto&v:expected)in>>v;
    if(!in)throw std::runtime_error("invalid original fixture");
    ObjectLightingState state{{initial[0],initial[1],initial[2]},
        {initial[3],initial[4],initial[5]},initial[6],initial[7],
        {initial[8],initial[9],initial[10]},{initial[11],initial[12],initial[13]},
        {initial[14],initial[15],initial[16]},{initial[17],initial[18],initial[19]}};
    const Vec3 color{args[2],args[3],args[4]};
    if(op==0)frontend_cinematic_primary(state,args[0],args[1],color);
    else if(op==1)frontend_cinematic_secondary(state,args[0],args[1],color);
    else if(op==2)frontend_cinematic_ambient(state,color);
    else throw std::runtime_error("invalid operation");
    auto actual=state.source_values();
    for(unsigned i=0;i<20;++i)if(std::bit_cast<std::uint32_t>(actual[i])!=expected[i]) {
      std::cerr<<"case "<<count<<" word "<<i<<" actual="<<std::hex<<std::bit_cast<std::uint32_t>(actual[i])<<" original="<<expected[i]<<'\n';return 1;
    }
    ++count;
  }
  if(count!=402)throw std::runtime_error("incomplete original fixture");
  std::cout<<count<<" cinematic lighting cases; "<<count*20<<" exact output words\n";
}
