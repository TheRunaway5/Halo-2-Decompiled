#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

// Read-only layout inspection. Generated geometry belongs in HALO2_TEST_SCRATCH.
int main(int argc,char **argv) {
  try {
    if(argc!=5)throw halo2::FormatError("Supply MCC map directory, marker report, output JSON and geometry directory");
    nlohmann::json rows;std::ifstream(argv[2])>>rows;
    const std::filesystem::path maps=argv[1],geometry=argv[4];
    std::filesystem::create_directories(geometry);
    for(auto &[name,row]:rows.items()) {
      halo2::Cache cache(maps/(name+".map"));
      auto bsp=cache.load_bsp_content(0);
      const halo2::CollisionWorld collision(bsp.collision);
      nlohmann::json triangles=nlohmann::json::array();
      for(const auto &t:bsp.collision)triangles.push_back({{t.a.x,t.a.y,t.a.z},{t.b.x,t.b.y,t.b.z},{t.c.x,t.c.y,t.c.z}});
      std::ofstream(geometry/(name+".json"))<<triangles;
      for(auto &flag:row["flags"]) {
        const auto &p=flag["position"];halo2::Vec3 point{p[0],p[1],p[2]};
        const auto floor=collision.raycast(point+halo2::Vec3{0,0,.25f},{0,0,-1},5.f);
        if(floor && floor->normal.z>.65f) {
          flag["floor"]={floor->position.x,floor->position.y,floor->position.z};
          flag["clearance"]=!collision.overlaps(floor->position+halo2::Vec3{0,0,.03f},.65f,.18f);
        }
      }
      std::cout<<name<<": "<<bsp.collision.size()<<" collision triangles\n";
    }
    std::ofstream(argv[3])<<rows.dump(2)<<'\n';
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
