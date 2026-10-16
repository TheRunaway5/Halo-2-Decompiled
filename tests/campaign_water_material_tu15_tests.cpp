#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/water.hpp"
#include <iostream>
using namespace halo2;
void require(bool value,const char *message){if(!value)throw FormatError(message);}
int main(int argc,char **argv){
 require(argc==3,"Supply the actual campaign map and English TU1.5 image");
 const auto original=OriginalData::load_xbe(argv[2]);
 require(original->image==OriginalData::Image::TitleUpdate15,"Water material requires TU1.5");
 Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
 const auto water=cache.load_bsp_water(0);
 require(!water.definitions.empty(),"Actual Delta Towers water record missing");
 const auto tag=cache.query_tag({.group=TagGroup::from_string("shad"),
     .name="scenarios\\shaders\\solo\\deltatemple\\tunnel_pool_water"});
 require(tag!=nullptr,"Actual late pool shader missing");
 auto material=cache.load_material(tag->datum);
 require(prepare_water_material(cache,material) && material.forward_vertex==7,
     "Actual tunnel pool's original pass was replaced");
 require(material.water_definition==-1,"Late material unexpectedly has a BSP binding");
 const auto program=material.forward_program.words;
 const auto constants=material.program_constants;
 const auto vertex=material.vertex_program;
 // Registration after an earlier binding pass reproduces the live failure.
 std::vector<Material> roots;
 bind_water_pool_materials(water,roots);
 roots.push_back(material);
 bind_water_pool_materials(water,roots);
 require(roots.front().water_definition==0,"Late pool lost TU1.5's first water record");
 require(roots.front().forward_program.words==program &&
     roots.front().program_constants==constants && roots.front().vertex_program==vertex,
     "Water binding changed the original shader program/constants");
 std::vector<Material> absent{material};
 bind_water_pool_materials({},absent);
 require(absent.front().water_definition==-1,"Missing BSP water invented a definition");
 std::cout<<"PASS actual TU1.5 late tunnel pool: first BSP water record bound; original program retained\n";
}
