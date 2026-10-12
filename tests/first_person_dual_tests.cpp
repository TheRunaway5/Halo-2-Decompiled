#include "halo2/first_person_dual.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv){try {
  if(argc!=2)throw FormatError("Supply original dual WORLD fixture");
  std::ifstream file(argv[1]);if(!file)throw FormatError("Missing original dual WORLD fixture");
  unsigned cases=0,checked=0;std::string line;
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    const auto separator=line.find('|');if(separator==std::string::npos)throw FormatError("Invalid dual fixture");
    std::istringstream in(line.substr(0,separator)),expected(line.substr(separator+1));in>>std::hex;expected>>std::hex;
    auto word=[&](){std::uint32_t v;if(!(in>>v))throw FormatError("Truncated dual fixture");return v;};
    const auto count=word(),node=word(),parent=word();std::array<std::uint32_t,13> root;
    for(auto &v:root)v=word();std::vector<BonePose> locals(count);std::vector<int> parents(count);
    for(unsigned i=0;i<count;++i) {
      parents[i]=std::bit_cast<std::int32_t>(word());auto &p=locals[i];
      for(auto &q:p.rotation)q=std::bit_cast<float>(word());
      p.translation={std::bit_cast<float>(word()),std::bit_cast<float>(word()),std::bit_cast<float>(word())};
      p.scale=std::bit_cast<float>(word());
    }
    const auto result=source_first_person_dual_nodes(locals,parents,{int(node),int(parent)},std::bit_cast<Transform>(root));
    for(unsigned i=0;i<result.size();++i)for(auto v:std::bit_cast<std::array<std::uint32_t,13>>(result[i])) {
      std::uint32_t e;if(!(expected>>e))throw FormatError("Truncated original dual outputs");
      if(e!=v)throw FormatError("Original dual WORLD differs case="+std::to_string(cases)+" node="+std::to_string(i)+
          " word="+std::to_string(checked%13));++checked;
    }
    ++cases;
  }
  if(cases<100)throw FormatError("Incomplete dual WORLD coverage");
  std::cout<<"PASS "<<cases<<" original dual WORLD cases, "<<checked<<" exact float words\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
