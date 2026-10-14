#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/navigation.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Supply the original Armory cache");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);const auto bsp=cache.load_bsp_content(0);
    NavigationWorld navigation(bsp.navigation);
    const Vec3 start{30.5947f,-48.5443f,3.00016f},goal{33.0895f,-46.1925f,3.64566f};
    const auto from=navigation.locate(start),to=navigation.locate(goal);
    if(from!=6915 || to!=7662)throw FormatError("Armory authored navigation fixture differs");
    // Actual links15903 and17311 have flags0, while both adjacent sectors
    // have flags17. Retail26C380 admits these ordinary walkable neighbors.
    const auto connected=[&](std::size_t a,std::size_t b) {
      for(const auto &portal:bsp.navigation[a].portals)
        if(portal.sector==b && portal.flags==0)return true;
      return false;
    };
    if(!connected(6915,6918)||!connected(6918,6915)||
       !connected(7655,7662)||!connected(7662,7655))
      throw FormatError("Original ordinary walkable adjacency was discarded by link bit5");
    const auto route=navigation.route(start,goal);
    if(route.empty())throw FormatError("Armory authored floor route is disconnected");
    const auto &end=route.back();
    if(end.x!=goal.x||end.y!=goal.y||end.z!=goal.z)
      throw FormatError("Armory route lost its actual destination");
    std::cout<<"Original Armory ordinary sector links retained; route points="<<route.size()<<'\n';
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
