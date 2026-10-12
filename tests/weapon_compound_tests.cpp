#include "halo2/source_model_palette.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv){try {
  if(argc!=2)throw FormatError("Supply original compound palette fixture");
  std::ifstream file(argv[1]);if(!file)throw FormatError("Missing compound palette fixture");
  std::string line;unsigned cases=0,checked=0;
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream in(line);in>>std::hex;unsigned nodes,compounds;in>>nodes>>compounds;
    auto word=[&](){std::uint32_t v;if(!(in>>v))throw FormatError("Truncated compound fixture");return v;};
    auto real=[&](){return std::bit_cast<float>(word());};
    std::vector<Transform> world(nodes),inverse(nodes);
    for(auto &t:world){std::array<float,13> words;for(auto &v:words)v=real();t=std::bit_cast<Transform>(words);}
    std::vector<SourceModelCompoundNode> definitions(compounds);
    for(auto &d:definitions){for(auto &n:d.nodes)n=std::uint8_t(word());for(auto &w:d.weights)w=real();}
    auto palette=source_model_skin_palette(world,inverse);append_source_model_compounds(palette,definitions);
    for(const auto &m:palette)for(auto v:m){if(std::bit_cast<std::uint32_t>(v)!=word())throw FormatError("Original compound matrix differs");++checked;}
    std::string extra;if(in>>extra)throw FormatError("Trailing compound fixture");++cases;
  }
  if(cases<10)throw FormatError("Insufficient original compound coverage");
  std::cout<<"PASS "<<cases<<" original compound palette cases, "<<checked<<" exact words\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
