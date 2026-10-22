// Export actual Halo 2 palm/wreck meshes and measure retail map budgets.
// Invoke under tools/run_test.py with an owned output directory.
#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/posed_collision.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Usage: export_dust2_props MAP SHARED_MAPS OUTPUT_DIRECTORY props|budget");
  Cache cache(argv[1],argv[2]);std::filesystem::path output(argv[3]);
  std::filesystem::create_directories(output);
  if(std::string(argv[4])=="budget") {
    const auto mesh=cache.load_bsp(0,false);
    std::ofstream(output/(cache.name()+"-budget.json"))<<Json{{"map",cache.name()},
      {"source",cache.path().string()},{"triangles",mesh.vertices.size()/3},
      {"clusters",mesh.clusters},{"instances",mesh.instances}}.dump(2)<<'\n';return 0;
  }
  Json report{{"schema","H2_DUST2_NATIVE_PROP_EXPORT_V1"},{"source",cache.path().string()},
              {"models",Json::array()}};
  for(const auto &tag:cache.tags()) {
    const bool palm=tag.group=="scen" && tag.name.find("palmtrees\\palmtree_")!=std::string::npos;
    const bool hog=tag.group=="mode" && tag.name=="objects\\vehicles\\warthog\\warthog";
    if(!palm && !hog)continue;
    Json selection=Json::array();std::vector<unsigned> sections,permutations;
    if(hog) {
      const Bytes root(cache.read_tag_bytes(tag.address+28,8));
      for(unsigned i=0;i<root.u32(0);++i) {
        const auto at=root.u32(4)+i*16;const Bytes r(cache.read_tag_bytes(at,16));
        int chosen=-1;unsigned chosen_permutation=0;std::string chosen_name;
        for(unsigned p=0;p<r.u32(8);++p) {
          const Bytes perm(cache.read_tag_bytes(r.u32(12)+p*16,16));
          const auto name=cache.string_id(perm.u32(0));
          if(p==0 || name=="destroyed" || (chosen_name!="destroyed" && name=="medium")) {
            chosen=std::int16_t(perm.u16(14));chosen_name=name;chosen_permutation=p;
          }
        }
        selection.push_back({{"region",cache.string_id(r.u32(0))},{"permutation",chosen_name},{"section",chosen}});
        if(chosen>=0)sections.push_back(unsigned(chosen));
        permutations.push_back(chosen_permutation);
      }
    }
    auto model=hog?cache.load_model_sections(tag.datum,true,0,5,sections):cache.load_model(tag.datum,true);
    const auto vertices=pose_model(model);if(vertices.empty() || vertices.size()%3)throw FormatError("Empty or invalid prop mesh");
    std::string name=hog?"warthog_destroyed":tag.name.substr(tag.name.find_last_of('\\')+1);
    Json materials=Json::array();for(const auto &m:model.mesh.materials)materials.push_back(m.name);
    std::ofstream file(output/(name+".bin"),std::ios::binary);
    for(const auto &v:vertices) {
      const float values[]{v.position.x,v.position.y,v.position.z,v.normal.x,v.normal.y,v.normal.z,v.uv.x,v.uv.y};
      file.write(reinterpret_cast<const char*>(values),sizeof(values));
      file.write(reinterpret_cast<const char*>(&v.material),4);
    }
    if(!file)throw FormatError("Could not write prop mesh");
    Json collision_triangles=Json::array();
    if(hog) {
      const auto *definition=cache.query_tag({.group=TagGroup::from_string("hlmt"),.name=tag.name,.require_address=true});
      if(!definition)throw FormatError("Wreck collision model missing");
      const auto collision=cache.load_model_collision(definition->datum);
      if(collision.model_regions.size()!=permutations.size())throw FormatError("Wreck render/collision region mismatch");
      std::set<std::pair<unsigned,unsigned>> visited;
      for(unsigned region=0;region<permutations.size();++region) {
        const auto &binding=collision.model_regions[region];
        if(binding.collision==255 || permutations[region]>=binding.permutations.size())continue;
        const auto permutation=binding.permutations[permutations[region]];
        if(permutation==255 || !visited.emplace(binding.collision,permutation).second)continue;
        const auto &parts=collision.regions.at(binding.collision).at(permutation);
        for(const auto &bsp:parts) {
          const auto transform=inverse_transform(collision.authored_inverse.at(unsigned(bsp.node)));
          const auto point=[&](Vec3 v) {
            const auto &b=transform.basis;const auto s=transform.scale;
            return Vec3{transform.position.x+s*(b[0].x*v.x+b[1].x*v.y+b[2].x*v.z),
              transform.position.y+s*(b[0].y*v.x+b[1].y*v.y+b[2].y*v.z),
              transform.position.z+s*(b[0].z*v.x+b[1].z*v.y+b[2].z*v.z)};
          };
          for(unsigned surface=0;surface<bsp.surfaces.size();++surface) {
            const auto &s=bsp.surfaces[surface];if(s.flags&16)continue;
            std::vector<Vec3> polygon;unsigned edge=s.first_edge;
            do {
              if(edge>=bsp.edges.size() || polygon.size()>bsp.edges.size())throw FormatError("Wreck collision edge cycle invalid");
              const auto &e=bsp.edges[edge];const bool left=e.surfaces[0]==surface;
              if(!left && e.surfaces[1]!=surface)throw FormatError("Wreck collision surface mismatch");
              polygon.push_back(point(bsp.vertices.at(e.vertices[left?0:1])));edge=e.next[left?0:1];
            }while(edge!=s.first_edge);
            for(unsigned v=1;v+1<polygon.size();++v) {
              Json triangle=Json::array();for(auto p:{polygon[0],polygon[v],polygon[v+1]})triangle.push_back({p.x,p.y,p.z});
              collision_triangles.push_back(std::move(triangle));
            }
          }
        }
      }
      if(collision_triangles.empty())throw FormatError("Destroyed Warthog collision is empty");
    }
    report["models"].push_back({{"name",name},{"tag",tag.name},{"group",tag.group},
      {"triangles",vertices.size()/3},{"materials",materials},{"selection",selection},
      {"geometry",name+".bin"},{"collision_triangles",collision_triangles},
      {"vertex_record","little-endian float3 position, float3 normal, float2 uv, uint32 material"}});
  }
  std::ofstream(output/"props.json")<<report.dump(2)<<'\n';
  std::cout<<report["models"].size()<<" original Halo 2 prop meshes exported\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
