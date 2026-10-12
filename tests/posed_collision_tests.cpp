#include "halo2/posed_collision.hpp"
#include "halo2/content.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
using namespace halo2;
namespace {
std::uint32_t bits(std::istringstream &row) {
  std::string word;if(!(row>>word))throw FormatError("Missing collision fixture float");
  return std::stoul(word,nullptr,16);
}
float real(std::istringstream &row) {return std::bit_cast<float>(bits(row));}
Vec3 point(std::istringstream &row) {const float x=real(row),y=real(row),z=real(row);return {x,y,z};}
void check(bool value,const std::string &name) {if(!value)throw FormatError("Original collision differs: "+name);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply project and local collision fixtures");
    if(!std::filesystem::exists(argv[2]))return 77;
    const std::filesystem::path root(argv[1]);
    Cache cache(root/"assets/disc/maps/lockout.map");
    const auto content=cache.load_content(0,true);
    const auto model=cache.load_model_collision(content.player.model);
    const auto original=OriginalData::load_xbe(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03");
    std::unordered_map<unsigned,std::vector<Transform>> poses;
    std::ifstream file(argv[2]);unsigned queries=0,hits=0,edges=0,accepts=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);std::string kind;row>>kind;
      if(kind=="pose") {
        unsigned id,count;row>>id>>count;std::vector<Transform> matrices;
        for(unsigned i=0;i<count;++i) {
          std::array<float,13> values;for(auto &value:values)value=real(row);
          matrices.push_back(std::bit_cast<Transform>(values));
        }
        poses[id]=std::move(matrices);
      } else if(kind=="query") {
        std::string name;unsigned pose,flags,upper,count;row>>name>>pose>>flags>>upper;
        const float previous=real(row);row>>count;std::vector<std::uint8_t> active;
        for(unsigned i=0;i<count;++i) {unsigned v;row>>v;active.push_back(std::uint8_t(v));}
        const Vec3 origin=point(row),delta=point(row);unsigned expected;row>>expected;
        const auto result=posed_collision(model,poses.at(pose),active,*original,origin,delta,flags,previous);
        check(bool(result)==bool(expected),name+" hit");
        if(result) {
          const auto fraction=bits(row),packed=bits(row);int region,node,leaf,surface,plane,material;
          unsigned surface_flags,breakable;row>>region>>node>>leaf>>surface>>plane>>material>>surface_flags>>breakable;
          check(std::bit_cast<std::uint32_t>(result->fraction)==fraction,name+" fraction");
          check(result->packed_bsp==packed && result->region==region && result->node==node,name+" BSP");
          check(result->leaf==leaf && result->surface==surface && result->signed_plane==plane,name+" surface");
          check(result->model_material==material && result->flags==surface_flags && result->breakable==breakable,name+" attributes");
          if(upper) {
            for(float value:{result->plane.normal.x,result->plane.normal.y,result->plane.normal.z,result->plane.distance})
              check(std::bit_cast<std::uint32_t>(value)==bits(row),name+" world plane");
            int global;row>>global;check(result->global_material==global,name+" global material");
          }
          ++hits;
        }
        ++queries;
      } else if(kind=="edge") {
        unsigned id,region,bsp,surface,axis,orientation,expected;
        row>>id>>region>>bsp>>surface>>axis>>orientation;
        const float x=real(row),y=real(row);row>>expected;
        const auto result=collision_surface_contains(model.regions.at(region).at(0).at(bsp),surface,
            {x,y},original->collision_projection.at(axis*2+orientation));
        check(result==bool(expected),"edge "+std::to_string(id));++edges;if(result)++accepts;
      } else throw FormatError("Unknown collision fixture row");
      if(!row)throw FormatError("Truncated collision fixture row");
      std::string extra;if(row>>extra)throw FormatError("Trailing collision fixture fields");
    }
    check(queries && edges,"empty suite");
    std::cout<<queries<<" original posed queries / "<<hits<<" hits and "<<edges
        <<" original surface edge cases / "<<accepts<<" accepts matched\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
