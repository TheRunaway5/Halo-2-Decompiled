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
#include <map>
#include <thread>
#include <optional>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F &&call,const char *why){bool rejected=false;try{call();}catch(const FormatError &){rejected=true;}check(rejected,why);}
bool same(std::span<const Vertex> a,std::span<const Vertex> b){return a.size()==b.size()&&(a.empty()||!std::memcmp(a.data(),b.data(),a.size_bytes()));}
bool same(const std::vector<ObjectLightingBinding> &a,const std::vector<ObjectLightingBinding> &b){
 if(a.size()!=b.size())return false;
 for(std::size_t n=0;n<a.size();++n){
  const auto x=a[n].state.source_values(),y=b[n].state.source_values();
  if(a[n].first_material!=b[n].first_material||a[n].material_count!=b[n].material_count||std::memcmp(x.data(),y.data(),sizeof(x)))return false;
 }return true;
}
void checkpoint_roundtrip(const Cache &cache,const LevelContent &content,const CollisionWorld &collision,
 SourcePropScene &scene,const SceneObjects &objects,Mesh &mesh){
 const auto material_count=mesh.materials.size();
 SourcePropScene::Checkpoint saved;bool inspect_callback=true,callback_checked=false;unsigned callbacks{};
 scene.observe_draws([&](unsigned,std::uint32_t,std::uint32_t,std::span<const Vertex>){
  ++callbacks;if(!inspect_callback)return;inspect_callback=false;
  rejects([&]{(void)scene.checkpoint();},"Prop callback captured an incomplete presentation call");
  rejects([&]{(void)scene.prepare_restore(saved);},"Prop callback prepared an incomplete presentation call");
  rejects([&]{scene.begin_source_frame();},"Prop callback admitted a reentrant mutation");
  const auto occupied=scene.effect_owners().size();scene.reset_lifetimes();
  check(scene.effect_owners().size()==occupied,"Reentrant noexcept prop reset changed active lifetimes");callback_checked=true;
 });
 scene.begin_source_frame();scene.update_pose(5,mesh);saved=scene.checkpoint();
 SourceObjectViewLod view;view.input=[](std::size_t,std::uint32_t,Vec3,float){return SourceObjectLodInput{1,0x80000,0};};
 std::vector<Vertex> view_vertices;std::vector<ObjectLightingBinding> view_light;
 scene.append_view(view_vertices,view_light,view);
 check(callback_checked&&!view_vertices.empty(),"Prop checkpoint callback control did not execute");
 saved=scene.checkpoint();check(saved.valid()&&scene.owns_checkpoint(saved),"Completed prop checkpoint was not owned");
 std::vector<Vertex> expected;std::vector<ObjectLightingBinding> expected_light;scene.append(expected,expected_light);
 struct Owner {std::size_t placement;std::uint32_t object;std::uint64_t created;std::vector<Transform> nodes;std::vector<std::uint8_t> permutations;};
 std::vector<Owner> owners;for(const auto &o:scene.effect_owners())owners.push_back({o.placement,o.object,o.created_tick,{o.nodes.begin(),o.nodes.end()},{o.permutations.begin(),o.permutations.end()}});
 struct Bank {unsigned slot;std::shared_ptr<Material> pass;std::shared_ptr<const VertexProgramBank> bank;};
 std::vector<Bank> banks;for(unsigned slot=0;slot<mesh.materials.size();++slot){
  const auto &pass=mesh.materials[slot].environment_pass;if(pass&&pass->forward_vertex==9)banks.push_back({slot,pass,pass->vertex_bank});
 }
 check(!banks.empty(),"PRT prop checkpoint control lacks actual material banks");
 {Mesh other_mesh;SourcePropScene other(cache,content.objects,0,other_mesh,collision);
  check(!other.owns_checkpoint(saved),"Foreign prop scene owned another scene's checkpoint");
  rejects([&]{(void)other.prepare_restore(saved);},"Foreign prop scene prepared another scene's checkpoint");
  auto mine=scene.prepare_restore(saved);check(!other.restore_ready(mine)&&!other.commit_restore(mine)&&scene.restore_ready(mine),"Foreign prop scene committed another scene's preparation");
 }
 check(!scene.owns_checkpoint({}),"Missing prop checkpoint was owned");
 auto changed=objects;const auto moving=std::find_if(changed.objects().begin(),changed.objects().end(),[&](const auto &o){return o.handle!=null_datum&&o.visible&&content.objects[o.placement].type==11;});
 check(moving!=changed.objects().end(),"Prop checkpoint has no actual movable source owner");
 auto &moved=changed.get(moving->handle);moved.position.x+=.375f;moved.rotation.z+=.25f;
 scene.synchronize(changed);scene.update_pose(17,mesh);
 std::vector<Vertex> future;std::vector<ObjectLightingBinding> future_light;scene.append(future,future_light);
 check(!same(expected,future),"Prop checkpoint future control did not change the actual pose");
 check(scene.owns_checkpoint(saved),"Later prop frames expired historical ownership");
 const auto unchanged=[&]{std::vector<Vertex> actual;std::vector<ObjectLightingBinding> light;scene.append(actual,light);return same(future,actual)&&same(future_light,light);};
 {auto cancelled=scene.prepare_restore(saved);check(scene.restore_ready(cancelled),"Prop restore cancellation control was not ready");}
 check(unchanged(),"Cancelled prop restore changed the live pose/light bank");
 auto stale=scene.prepare_restore(saved);scene.begin_source_frame();
 check(!scene.restore_ready(stale)&&!scene.commit_restore(stale)&&unchanged(),"Mutated prop owner accepted a stale preparation");
 auto prepared=scene.prepare_restore(saved);
 bool foreign_ready=true,foreign_owned=true,foreign_capture=false;
 std::thread observer([&]{foreign_ready=scene.restore_ready(prepared);foreign_owned=scene.owns_checkpoint(saved);scene.reset_lifetimes();try{(void)scene.checkpoint();}catch(const FormatError &){foreign_capture=true;}});observer.join();
 check(!foreign_ready&&!foreign_owned&&foreign_capture&&scene.restore_ready(prepared)&&unchanged(),"Prop restore/reset admitted a foreign executor");
 const auto old_bank=banks.front().pass->vertex_bank;
 banks.front().pass->vertex_bank=banks.front().bank;
 check(old_bank!=banks.front().bank&&!scene.restore_ready(prepared)&&!scene.commit_restore(prepared)&&unchanged(),"Changed external PRT bank escaped prop restore preflight");
 banks.front().pass->vertex_bank=old_bank;
 banks.front().pass->vertex_bank.reset();
 check(!scene.restore_ready(prepared)&&!scene.commit_restore(prepared),"Missing external PRT bank escaped prop restore preflight");
 rejects([&]{(void)scene.checkpoint();},"Fresh prop checkpoint accepted a missing original PRT bank");banks.front().pass->vertex_bank=old_bank;
 auto &resident=mesh.materials[banks.front().slot];const auto old_pass=resident.environment_pass;
 resident.environment_pass.reset();check(!scene.restore_ready(prepared)&&!scene.commit_restore(prepared)&&unchanged(),"Replaced resident PRT pass escaped prop restore preflight");resident.environment_pass=old_pass;
 const auto before_commit=callbacks;
 check(scene.restore_ready(prepared)&&scene.commit_restore(prepared)&&!scene.restore_ready(prepared)&&!scene.commit_restore(prepared),"Exact prop restore did not commit exactly once");
 check(callbacks==before_commit&&mesh.materials.size()==material_count,"Prop restore invoked callbacks or changed material residency");
 for(const auto &bank:banks)check(mesh.materials[bank.slot].environment_pass==bank.pass&&bank.pass->vertex_bank==bank.bank,"Prop restore lost the actual immutable PRT link");
 std::vector<Vertex> restored;std::vector<ObjectLightingBinding> restored_light;scene.append(restored,restored_light);
 check(same(expected,restored)&&same(expected_light,restored_light),"Prop restore changed exact posed geometry/light values");
 const auto actual_owners=scene.effect_owners();check(actual_owners.size()==owners.size(),"Prop restore changed occupied source lifetimes");
 for(std::size_t n=0;n<owners.size();++n){const auto &a=actual_owners[n];const auto &b=owners[n];
  check(a.placement==b.placement&&a.object==b.object&&a.created_tick==b.created&&std::ranges::equal(a.permutations,b.permutations)&&a.nodes.size()==b.nodes.size()&&
      (a.nodes.empty()||!std::memcmp(a.nodes.data(),b.nodes.data(),a.nodes.size_bytes())),"Prop restore changed full salts, nodes or region selections");
 }
 restored.clear();restored_light.clear();scene.append_view(restored,restored_light,view);
 check(same(view_vertices,restored)&&same(view_light,restored_light),"Prop restore changed retained per-view selection/light output");
 auto rebound=scene.prepare_restore(saved);scene.observe_draws({});
 check(!scene.owns_checkpoint(saved)&&!scene.restore_ready(rebound)&&!scene.commit_restore(rebound),"Prop callback replacement kept an old cut eligible");
 rejects([&]{(void)scene.prepare_restore(saved);},"Prop checkpoint crossed a replaced callback binding");
 const auto before_cow=scene.checkpoint();auto cow_prepared=scene.prepare_restore(before_cow);
 const auto lighting=std::find_if(expected_light.begin(),expected_light.end(),[&](const auto &binding){return banks.front().slot>=binding.first_material&&banks.front().slot-binding.first_material<binding.material_count;});
 check(lighting!=expected_light.end(),"PRT COW control lost its actual object's lighting binding");
 const auto pass_before_cow=resident.environment_pass.get();apply_object_lighting(resident,lighting->state);
 check(resident.environment_pass.get()!=pass_before_cow&&!scene.owns_checkpoint(before_cow)&&
     !scene.restore_ready(cow_prepared)&&!scene.commit_restore(cow_prepared),"Actual lighting COW kept a different historical material graph eligible");
 scene.update_pose(18,mesh);restored.clear();restored_light.clear();scene.append_view(restored,restored_light,view);
 check(same(view_vertices,restored),"Ordinary prop pose/draw failed after actual lighting COW");
 const auto after_cow=scene.checkpoint();check(scene.owns_checkpoint(after_cow),"Fresh prop capture was permanently invalid after material COW");
 auto fresh=scene.prepare_restore(after_cow);check(scene.restore_ready(fresh)&&scene.commit_restore(fresh),"Fresh actual material graph could not prepare/restore");
 // An enclosing material owner can restore a historical graph even after the
 // real lighting producer detached that pass through its normal COW path.
 MaterialPresentationContinuity materials(mesh);
 const auto material_cut=materials.checkpoint();const auto scene_cut=scene.checkpoint();
 const auto paired_pass=resident.environment_pass;const auto paired_bank=paired_pass->vertex_bank;
 std::vector<Vertex> paired_vertices;std::vector<ObjectLightingBinding> paired_light;scene.append(paired_vertices,paired_light);
 scene.synchronize(changed);
 {auto batch=materials.begin_mutation();apply_object_lighting(resident,lighting->state);scene.update_pose(19,mesh);}
 check(resident.environment_pass!=paired_pass&&!scene.owns_checkpoint(scene_cut),"Actual second lighting COW did not detach historical prop material graph");
 std::vector<Vertex> paired_future;std::vector<ObjectLightingBinding> paired_future_light;scene.append(paired_future,paired_future_light);
 check(!same(paired_vertices,paired_future),"Paired prop/material future did not change actual geometry");
 const auto future_unchanged=[&]{std::vector<Vertex> actual;std::vector<ObjectLightingBinding> light;scene.append(actual,light);
  return same(paired_future,actual)&&same(paired_future_light,light);};
 {auto wrong=materials.prepare_restore(materials.checkpoint());
  rejects([&]{(void)scene.prepare_restore(scene_cut,wrong.target_graph());},"Prop accepted a different historical material graph");}
 {auto cancelled_material=materials.prepare_restore(material_cut);
  auto cancelled_scene=scene.prepare_restore(scene_cut,cancelled_material.target_graph());
  check(materials.restore_ready(cancelled_material)&&scene.restore_ready(cancelled_scene),"Paired prop/material cancellation was not ready");}
 check(future_unchanged(),"Cancelled paired prop/material preparation changed live geometry");
 {auto stale_material=materials.prepare_restore(material_cut);auto stale_scene=scene.prepare_restore(scene_cut,stale_material.target_graph());
  scene.begin_source_frame();check(!scene.restore_ready(stale_scene)&&!scene.commit_restore(stale_scene)&&future_unchanged(),"Paired prop/material preparation survived scene mutation");}
 auto material_restore=materials.prepare_restore(material_cut);const auto target=material_restore.target_graph();
 auto scene_restore=scene.prepare_restore(scene_cut,target);
 check(target.ready()&&!target.applied()&&scene.restore_ready(scene_restore)&&!scene.commit_restore(scene_restore)&&future_unchanged(),"Prop published before its prepared material graph");
 check(materials.commit_restore(material_restore)&&target.applied()&&scene.restore_ready(scene_restore),"Material publication did not enable its exact prop target");
 check(scene.commit_restore(scene_restore)&&!scene.commit_restore(scene_restore),"Paired prop/material restore failed exact single publication");
 restored.clear();restored_light.clear();scene.append(restored,restored_light);
 check(same(paired_vertices,restored)&&same(paired_light,restored_light)&&resident.environment_pass==paired_pass&&
  resident.environment_pass->vertex_bank==paired_bank&&mesh.materials.size()==material_count,"Paired prop/material restore changed exact geometry/light/PRT graph");
 std::cout<<"PASS exact prop material target: real lighting COW, wrong target, cancellation, stale scene, material-first publication and historical PRT graph\n";
 std::cout<<"PASS exact prop checkpoint: salted lifetimes, posed/view/light caches, PRT links, cancellation, stale/foreign/executor/callback guards\n";
}
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
  if(std::string(names[i])=="coagulation") {
   const auto vehicle=std::find_if(content.objects.begin(),content.objects.end(),[](const auto &o){
    return o.type==1 && o.name<0 && o.definition!=null_datum;
   });
   if(vehicle==content.objects.end())throw FormatError("prop unrelated-lifetime fixture has no vehicle placement");
   auto repeated=objects;const auto placement=std::size_t(vehicle-content.objects.begin());
   const auto first=repeated.create_placement(placement,19),second=repeated.create_placement(placement,20);
   if(first==second)throw FormatError("unrelated vehicle fixture lost distinct full generations");
   scene.synchronize(repeated);scene.synchronize(objects);
   std::cout<<"prop lifetime: two vehicle generations sharing a placement stay outside the prop bank\n";
  }
  if(std::string(names[i])=="zanzibar") {
   auto repeated_content=content;
   const auto prop=std::find_if(repeated_content.objects.begin(),repeated_content.objects.end(),[](const auto &o){
    return o.type==11 && o.definition!=null_datum;
   });
   if(prop==repeated_content.objects.end())throw FormatError("duplicate prop lifetime fixture missing");
   const auto placement=std::size_t(prop-repeated_content.objects.begin());prop->name=-1;
   SceneObjects repeated(repeated_content,0);repeated.create_placement(placement,20);
   repeated.create_placement(placement,21);
   bool rejected=false;try{scene.synchronize(repeated);}catch(const FormatError &e){
    rejected=std::string(e.what()).find("duplicate live source placement")!=std::string::npos;
   }
   if(!rejected)throw FormatError("duplicate owned prop generations escaped lifetime validation");
   scene.synchronize(objects);
  }
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
  if(std::string(names[i])=="backwash") {
   unsigned created=0;
   for(unsigned index=0;index<content.objects.size();++index) {
    const auto &placement=content.objects[index];
    if(placement.type!=11 || !(placement.flags&1) || placement.name>=0)continue;
    const auto handle=objects.create_placement(index,17);
    scene.synchronize(objects);std::vector<Vertex> spawned;std::vector<ObjectLightingBinding> live;
    scene.append(spawned,live);
    if(live.size()!=light.size()+1 || spawned.size()<=world.size())
      throw FormatError("Unnamed initially absent prop has no prepared live geometry");
    objects.destroy(handle);scene.synchronize(objects);spawned.clear();live.clear();scene.append(spawned,live);
    if(live.size()!=light.size() || spawned.size()!=world.size())
      throw FormatError("Unnamed initially absent prop retirement retained geometry");
    ++created;
   }
   if(created!=2)throw FormatError("Original Backwash absent prop inventory differs");
   std::cout<<"Backwash: two unnamed initially absent prop placements prepared, created and retired\n";
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
  if(std::string(names[i])=="zanzibar")checkpoint_roundtrip(cache,content,collision,scene,objects,mesh);
  const auto resident_materials=mesh.materials.size();
  std::map<std::size_t,std::vector<std::uint8_t>> retained_permutations;
  for(const auto &o:scene.effect_owners())retained_permutations[o.placement]={o.permutations.begin(),o.permutations.end()};
  scene.reset_lifetimes();std::vector<Vertex> reset;std::vector<ObjectLightingBinding> reset_bindings;
  scene.append(reset,reset_bindings);
  if(!reset.empty()||!reset_bindings.empty())throw FormatError("Retired prop world retained draw admission");
  scene.synchronize(before_move);scene.update_pose(0,mesh);scene.append(reset,reset_bindings);
  if(reset.size()!=world.size() || reset_bindings.size()!=light.size() || mesh.materials.size()!=resident_materials ||
     (!reset.empty()&&std::memcmp(reset.data(),world.data(),reset.size()*sizeof(Vertex))))
   throw FormatError("Prop world adoption changed the complete original stream or resident slots");
  for(const auto &o:scene.effect_owners())if(!std::ranges::equal(o.permutations,retained_permutations.at(o.placement)))
   throw FormatError("Prop world adoption discarded authored attachment permutation ownership");
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
