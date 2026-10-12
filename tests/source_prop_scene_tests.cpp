#include "halo2/source_prop_scene.hpp"
#include "halo2/source_prop_initial_state.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/scene.hpp"
#include <filesystem>
#include <iostream>
#include <array>
#include <algorithm>
#include <fstream>
#include <bit>
#include <sstream>
#include <set>
#include <cstdlib>
#include <cstring>
using namespace halo2;
namespace {
void crane_bank(const Mesh &mesh,std::span<const Vertex> submitted){
 std::set<unsigned> used;for(const auto&v:submitted)used.insert(v.material);
 std::vector<std::pair<unsigned,std::uint32_t>> expected;
 std::ifstream fixture("tests/fixtures/retail_main_crane_bank.tsv");
 for(std::string line;std::getline(fixture,line);){
  if(line.empty() || line[0]=='#')continue;
  std::istringstream row(line);unsigned index;std::uint32_t word;
  if(!(row>>index>>std::hex>>word))throw FormatError("crane source bank fixture malformed");
  expected.emplace_back(index,word);
 }
 if(expected.size()!=157)throw FormatError("crane independent source fixture missing");
 unsigned matched=0;
 for(unsigned slot:used){const auto&material=mesh.materials.at(slot);if(material.name=="scenarios\\shaders\\multi\\zanzibar\\zanzibar_crane_prt" ||
     material.name=="scenarios\\shaders\\multi\\zanzibar\\grating_prt"){
  if(!material.environment_pass || !material.environment_pass->vertex_bank)
   throw FormatError("crane omitted original layer5 PRT submission");
  const auto&bank=*material.environment_pass->vertex_bank;
  if(bank.first_constant!=68 || !bank.prt_inputs)throw FormatError("crane source PRT declaration changed");
  //Only section0 shares actual captured node1. The source section0 materials
  //are independently identified by captured raw attributes and indices.
  bool exact=true;for(const auto&[index,word]:expected)
   exact&=index/4<bank.constants.size() && std::bit_cast<std::uint32_t>(bank.constants[index/4][index%4])==word;
  matched+=exact;
 }}
 //The actual capturedLOD4/5 source section0 has two identified materials.
 //Lower resident slots are checked by their own section/LOD input bank.
 if(matched!=2)throw FormatError("crane section0 current PRT bank differs from157 original consumed words");
}
void crane_submission(const Cache&cache,const LevelContent&content,const CollisionWorld&collision){
 const auto source=std::find_if(content.objects.begin(),content.objects.end(),[](const auto&o){
  return o.type==11 && o.definition==0xe93107bcu;
 });
 if(source==content.objects.end())throw FormatError("actual original crane placement missing");
 const std::array objects{*source};Mesh mesh;SourcePropScene scene(cache,objects,0,mesh,collision);
 if(scene.stats().render_models!=1 || scene.stats().prt_materials!=3)
  throw FormatError("original crane acquired an ordinary lighting implementation");
 std::vector<Vertex> geometry;std::vector<ObjectLightingBinding> bindings;scene.append(geometry,bindings);
 crane_bank(mesh,geometry);
 if(geometry.empty() || bindings.size()!=1)throw FormatError("crane mixed authored source material range missing");
 for(const auto&material:mesh.materials)if(!material.environment_pass || material.environment_pass->forward_vertex!=9)
  std::cout<<"crane ordinary consumer: "<<material.name<<'\n';
}
}
int main(int argc,char**argv){try{
#ifdef _WIN32
 _putenv_s("HALO2_VERIFY_STATIC_PROP_POSES","1");
#else
 setenv("HALO2_VERIFY_STATIC_PROP_POSES","1",1);
#endif
 if(argc>1 && std::string(argv[1])=="--sphere-probe"){
  unsigned count;while(std::cin>>count){
   std::vector<SourcePropBodyBounds> groups(count);
   for(auto&g:groups)std::cin>>g.minimum.x>>g.minimum.y>>g.minimum.z>>g.maximum.x>>g.maximum.y>>g.maximum.z>>g.margin;
   const auto sphere=source_prop_physics_sphere(groups);std::cout<<bool(sphere);
   if(sphere)for(float x:{sphere->center.x,sphere->center.y,sphere->center.z,sphere->radius})
    std::cout<<' '<<std::hex<<std::bit_cast<std::uint32_t>(x)<<std::dec;
   std::cout<<'\n';
  }return 0;
 }
 const bool probe=argc>1 && std::string(argv[1])=="--definition-probe";
 const std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
 SourcePropSceneStats totals;unsigned vertices=0,bindings=0;
 for(unsigned i=0;i<names.size();++i){
  Cache cache(std::filesystem::path(i<12?"assets/disc/maps":"assets/map-pack/maps")/(std::string(names[i])+".map"));
  const auto content=cache.load_content(0,true);
  if(probe){for(const auto&object:content.objects)if(object.type==11){
   const auto d=cache.load_source_prop_definition(object.definition);
   std::cout<<names[i]<<' '<<object.placement<<' '<<object.definition<<' '<<d.constructor_admitted<<' '
    <<d.physics_bodies<<' '<<d.initial_prop_flags<<' '<<d.animation_graph<<'\n';}continue;}
  CollisionWorld collision(content.collision);Mesh mesh;SourcePropScene scene(cache,content.objects,0,mesh,collision);
  const auto&s=scene.stats();totals.placements+=s.placements;totals.render_models+=s.render_models;
  totals.dynamic_materials+=s.dynamic_materials;totals.prt_materials+=s.prt_materials;totals.section_banks+=s.section_banks;
  totals.resident_dynamic_materials+=s.resident_dynamic_materials;
  totals.resident_prt_materials+=s.resident_prt_materials;
  totals.union_dynamic_materials+=s.union_dynamic_materials;totals.union_prt_materials+=s.union_prt_materials;
  totals.union_section_banks+=s.union_section_banks;
  SceneObjects objects(content,0);scene.synchronize(objects);
  if(std::string(names[i])=="zanzibar")crane_submission(cache,content,collision);
  std::vector<Vertex> world;std::vector<ObjectLightingBinding> light;scene.append(world,light);
  if(std::string(names[i])=="zanzibar")crane_bank(mesh,world);
  if(std::string(names[i])=="colossus" || std::string(names[i])=="elongation") {
   unsigned created=0;
   for(const auto &placement:content.objects)if(placement.type==11 && (placement.flags&1) && placement.name>=0) {
    const auto handle=objects.create(placement.name,false,17);
    scene.synchronize(objects);std::vector<Vertex> spawned;std::vector<ObjectLightingBinding> live;
    scene.append(spawned,live);
    if(live.size()!=light.size()+1 || spawned.size()<=world.size())throw FormatError("Named script prop has no prepared geometry");
    objects.destroy(handle);scene.synchronize(objects);spawned.clear();live.clear();scene.append(spawned,live);
    if(live.size()!=light.size() || spawned.size()!=world.size())throw FormatError("Script prop destruction retained geometry");
    ++created;
   }
   if(!created)throw FormatError("Actual map has no named script-created prop fixture");
   std::cout<<names[i]<<": "<<created<<" script-created prop generations prepared and retired\n";
  }
  for(const auto&v:world)if(v.material>=mesh.materials.size())throw FormatError("prop vertex lost resident material slot");
  for(const auto&binding:light)if(binding.first_material+binding.material_count>mesh.materials.size())
   throw FormatError("prop object-light bank overlaps another object");
  if(std::string(names[i])=="zanzibar"){
   const auto state=objects;
   objects.destroy_all();scene.synchronize(objects);
   std::vector<Vertex> gone;std::vector<ObjectLightingBinding> dead;scene.append(gone,dead);
   if(!gone.empty() || !dead.empty())throw FormatError("destroyed source props retained resident submission");
   objects.restore(state);scene.synchronize(objects);
   gone.clear();dead.clear();scene.append(gone,dead);
   if(gone.size()!=world.size() || dead.size()!=light.size())throw FormatError("restored full prop handles lost source geometry");
   const auto live=std::find_if(objects.objects().begin(),objects.objects().end(),[&](const auto&o){
    return o.handle!=null_datum && content.objects[o.placement].type==11;});
   if(live==objects.objects().end())throw FormatError("source prop lifetime fixture has no live object");
   const auto handle=live->handle;objects.visible(handle,false);scene.synchronize(objects);
   gone.clear();dead.clear();scene.append(gone,dead);
   if(dead.size()+1!=light.size())throw FormatError("hidden source prop still submitted");
   objects.restore(state);scene.synchronize(objects);
   bool rejected=false;try{scene.update(0,{},1,mesh);}catch(const FormatError&e){
    rejected=std::string(e.what()).find("source cached/physics query state")!=std::string::npos;
   }
   if(!rejected)throw FormatError("source prop query substituted regular bounds for missing physics/cache producer");
  }
  SourcePropInitialState initial;unsigned query_callbacks=0;
  scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform&node,float scale){
   ++query_callbacks;
   const auto definition=content.objects.at(index).definition;
   if(!initial.contains(handle))initial.capture(handle,definition,cache.load_object_shadow_definition(definition),node,scale);
   const auto state=initial.retained(handle,definition,node,scale,false);
   return SourcePropQueryState{{state.sphere.center,state.sphere.radius},state.velocity,state.query_runtime_flags,state.frozen};
  });
  scene.update(0,{},1,mesh);
  if(query_callbacks!=light.size())throw FormatError("visible prop query owner count changed");
  query_callbacks=0;
  if(std::string(names[i])=="zanzibar"){
   const auto retained=objects;
   const auto live=std::find_if(objects.objects().begin(),objects.objects().end(),[&](const auto&o){
    return o.handle!=null_datum && content.objects[o.placement].type==11;});
   objects.visible(live->handle,false);scene.synchronize(objects);scene.update(1,{},1,mesh);
   if(query_callbacks+1!=light.size())throw FormatError("hidden prop advanced source rendering-light cache");
   objects.restore(retained);scene.synchronize(objects);
  }else scene.update(1,{},1,mesh); //Same caller-owned initial physics/root state.
  // A changed transform must retire the retained pose; check it against
  // forced fresh construction, then return to the original complete stream.
  const auto before_move=objects;
  const auto moving=std::find_if(objects.objects().begin(),objects.objects().end(),[&](const auto&o){
   return o.handle!=null_datum && o.visible && content.objects[o.placement].type==11;});
  if(moving!=objects.objects().end()) {
   auto &object=objects.get(moving->handle);object.position.x+=.125f;object.rotation.z+=.25f;object.scale*=.75f;
   scene.synchronize(objects);scene.update_pose(2,mesh);
   std::vector<Vertex> actual,expected;std::vector<ObjectLightingBinding> ignored;
   scene.append(actual,ignored);
#ifdef _WIN32
   _putenv_s("HALO2_REFERENCE_REPOSE_STATIC_PROPS","1");
#else
   setenv("HALO2_REFERENCE_REPOSE_STATIC_PROPS","1",1);
#endif
   scene.update_pose(2,mesh);ignored.clear();scene.append(expected,ignored);
#ifdef _WIN32
   _putenv_s("HALO2_REFERENCE_REPOSE_STATIC_PROPS","");
#else
   unsetenv("HALO2_REFERENCE_REPOSE_STATIC_PROPS");
#endif
   if(actual.size()!=expected.size() || (!actual.empty() && std::memcmp(actual.data(),expected.data(),actual.size()*sizeof(Vertex))))
    throw FormatError("changed static prop transform retained an old complete posed stream");
   scene.update_pose(3,mesh); // Verified retained hit on the new transform.
   objects.restore(before_move);scene.synchronize(objects);scene.update_pose(4,mesh);
   expected.clear();ignored.clear();scene.append(expected,ignored);
   if(expected.size()!=world.size() || (!expected.empty() && std::memcmp(expected.data(),world.data(),expected.size()*sizeof(Vertex))))
    throw FormatError("restored static prop transform changed complete original geometry");
  }
  std::cout<<names[i]<<": "<<s.placements<<" props, "<<s.dynamic_materials<<" dynamic, "<<s.prt_materials
   <<" PRT entries, "<<s.resident_dynamic_materials<<" dynamic + "<<s.resident_prt_materials<<" PRT resident slots, "
   <<s.section_banks<<" section banks, "<<world.size()<<" vertices\n";
  vertices+=unsigned(world.size());bindings+=unsigned(light.size());
 }
 if(probe)return 0;
 if(totals.placements<415 || totals.render_models!=totals.placements || totals.dynamic_materials<807 ||
    totals.prt_materials!=3 || totals.resident_dynamic_materials<784 || totals.resident_prt_materials!=3 ||
    totals.section_banks!=3 || bindings!=415 || vertices!=215511)
  throw FormatError("source initial prop scene inventory changed");
 std::cout<<"TOTAL "<<totals.placements<<' '<<totals.render_models<<' '<<totals.dynamic_materials<<' '
  <<totals.prt_materials<<' '<<totals.section_banks<<' '<<vertices<<' '<<bindings<<"; resident "
  <<totals.resident_dynamic_materials<<' '<<totals.resident_prt_materials<<"; union "<<totals.union_dynamic_materials<<' '
  <<totals.union_prt_materials<<' '<<totals.union_section_banks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
