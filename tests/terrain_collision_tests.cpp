#include "halo2/posed_collision.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const std::string &message) {if(!value)throw FormatError(message);}
std::uint32_t bits(std::istringstream &row) {
  std::string value;if(!(row>>value))throw FormatError("Missing original terrain float");
  return std::stoul(value,nullptr,16);
}
Vec3 point(std::istringstream &row) {
  const auto x=std::bit_cast<float>(bits(row)),y=std::bit_cast<float>(bits(row)),z=std::bit_cast<float>(bits(row));
  return {x,y,z};
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply project and original terrain fixtures");
    if(!std::filesystem::exists(argv[2]))return 77;
    const std::filesystem::path root(argv[1]);
    Cache cache(root/"assets/disc/maps/lockout.map");
    const auto terrain=cache.load_raw_terrain_collision(0);
    check(terrain.bsps.size()==1,"Original terrain BSP count differs");
    const auto &bsp=terrain.bsps[0];
    const auto original=OriginalData::load_xbe(root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03");
    std::array<std::uint32_t,8> intact;intact.fill(0xffffffffu);
    std::ifstream file(argv[2]);unsigned imports=0,queries=0,hits=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);char kind;row>>kind;
      if(kind=='I') {
        for(auto size:{bsp.nodes.size(),bsp.planes.size(),bsp.leaves.size(),bsp.references.size(),
            bsp.nodes2.size(),bsp.surfaces.size(),bsp.edges.size(),bsp.vertices.size()}) {
          std::size_t expected;row>>expected;check(size==expected,"Original terrain block count differs");
        }
        std::size_t count;row>>count;
        check(terrain.global_materials.size()==count,"Original terrain material count differs");
        for(auto material:terrain.global_materials) {
          int expected;row>>expected;check(material==expected,"Original terrain global material differs");
        }
        ++imports;
      } else if(kind=='Q') {
        unsigned id,flags;row>>id>>flags;const auto origin=point(row),delta=point(row);
        unsigned expected;row>>expected;
        const auto result=raw_collision(bsp,*original,origin,delta,flags,intact,256);
        const auto name="Original terrain query "+std::to_string(id);
        check(bool(result.hit)==bool(expected),name+" hit differs");
        if(result.hit) {
          const auto fraction=bits(row);int crossing,leaf,surface,plane,material,surface_flags,breakable;
          row>>crossing>>leaf>>surface>>plane>>material>>surface_flags>>breakable;
          const auto &hit=*result.hit;
          check(std::bit_cast<std::uint32_t>(hit.fraction)==fraction,name+" fraction differs");
          check(hit.crossing_plane==crossing && hit.leaf==leaf && hit.surface==surface &&
              hit.signed_plane==plane && hit.material==material && hit.flags==surface_flags &&
              hit.breakable==breakable,name+" contact identity differs");++hits;
        }
        int leaf,classification,plane;unsigned count;row>>leaf>>classification>>plane>>count;
        check(result.prior_leaf==leaf && result.prior_class==classification && result.prior_plane==plane,
            name+" traversal descriptor differs");
        check(result.visited_leaves.size()==count,name+" visited count differs");
        for(auto actual:result.visited_leaves) {
          int expected_leaf;row>>expected_leaf;check(actual==expected_leaf,name+" visited order differs");
        }
        ++queries;
      } else throw FormatError("Unknown original terrain fixture");
      check(!row.fail(),"Incomplete original terrain fixture");std::string extra;
      check(!(row>>extra),"Trailing original terrain fixture");
    }
    check(imports==1 && queries==1506 && hits==1112,"Incomplete original terrain suite");
    std::cout<<queries<<" original raw terrain queries / "<<hits
        <<" hits, contact identities, descriptor state and visited-leaf order matched\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
