#include "halo2/source_camera.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
std::optional<Vec2> project(const std::array<float,22> &v) {
  return source_hud_target_anchor({{v[0],v[1],v[2]},{v[3],v[4],v[5]},
      {v[6],v[7],v[8]},v[9],bool(v[10])},{v[11],v[12],v[13],v[14]},
      {v[15],v[16],v[17],v[18]},{v[19],v[20],v[21]});
}
}
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply original target-anchor fixtures or --probe");
  if(std::string(argv[1])=="--probe") {
    std::array<float,22> row;
    while(std::cin>>row[0]) {
      for(unsigned n=1;n<row.size();++n)std::cin>>row[n];
      const auto result=project(row);
      std::cout<<bool(result)<<' '<<(result?result->x:0)<<' '<<(result?result->y:0)<<'\n';
    }
    return 0;
  }
  std::ifstream file(argv[1]);if(!file)return 77;
  unsigned rows{};
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream input(line);std::array<float,22> row;bool visible;Vec2 expected;
    for(auto &v:row)input>>v;
    input>>visible>>expected.x>>expected.y;
    if(!input)throw FormatError("Malformed original target-anchor fixture");
    const auto result=project(row);
    if(bool(result)!=visible || (result && (result->x!=expected.x || result->y!=expected.y)))
      throw FormatError("Original lock target projection differs at row "+std::to_string(rows));
    ++rows;
  }
  if(!rows)throw FormatError("Empty target-anchor fixture");
  std::cout<<rows<<" original lock target projections matched\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
