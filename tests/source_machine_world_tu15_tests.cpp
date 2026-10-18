#include "halo2/multiplayer_world.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/render_interpolation.hpp"
#include <cstring>
#include "helpers/campaign_vehicle_observer_inputs_tu15.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
using namespace halo2;using Json=nlohmann::json;
int main(int argc,char**argv)try {
 if(argc!=4)throw FormatError("Supply original TU1.5 XBE, Floodlab map and output directory");
 const auto original=OriginalData::load_xbe(argv[1]);
 if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("TU1.5 only");
 std::filesystem::path out=argv[3];std::filesystem::create_directories(out);
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
 Json report{{"passed",false},{"reference","English TU1.5 only"},{"contexts",Json::array()},
  {"full_physics_parity",false},{"supplied_physical_output",false}};
 unsigned verified_nodes=0,verified_vertices=0;
 for(unsigned context=0;context<3;++context) {
  const bool campaign=context<2;const unsigned bsp=context==1?5:0;
  const auto map=campaign?std::filesystem::path(argv[2]):std::filesystem::path(argv[2]).parent_path()/"cyclotron.map";
  Cache context_cache(map,map.parent_path(),original);
  const auto content=context_cache.load_content(bsp,!campaign);const auto selected=bsp==0?0:8;
  MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();
  options.gameplay_xbe=argv[1];options.bsp=bsp;options.local_players=2;options.source_object_world=true;
  options.source_object_savegame_directory=out/("source-storage-"+std::to_string(context));
  if(campaign) {
   const auto at=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&p){return p.type==1&&p.placement==selected&&!p.model_attachment;});
   if(at==content.objects.end())throw FormatError("Original campaign vehicle placement is absent");
   const auto starts=test::campaign_vehicle_observer_starts<3>(context_cache,content,bsp,*at);
   options.campaign_difficulty=1;options.native_spawn_inputs.assign(starts.begin(),starts.end());
   options.campaign_spawn_input=[starts](unsigned slot,bool initial)->std::optional<Spawn>{if(!initial||slot>=starts.size())return {};return starts[slot];};
  }
  MultiplayerPreparedWorld world(options,ScriptProgram{});
  Json row{{"map",map.string()},{"campaign",campaign},{"bsp",bsp},{"vehicle_placement",selected},{"scene_owners",Json::array()},{"device_owners",Json::array()}};
  bool authored=false;
  for(std::size_t i=0;i<world.content.objects.size();++i) {
   const auto &p=world.content.objects[i];if(p.type!=7||p.placement!=29)continue;
   authored=true;row["content_index"]=i;row["definition"]=p.definition;
   for(const auto&o:world.match.simulation().scene().objects())if(o.handle!=null_datum&&o.placement==i)
    row["scene_owners"].push_back({{"object",o.handle},{"visible",o.visible},{"bsp_hidden",o.bsp_hidden},{"script_hidden",o.script_hidden}});
   for(const auto &[handle,o]:world.match.simulation().devices().owners())if(o.placement==i)
    row["device_owners"].push_back({{"object",handle},{"visible",o.visible},{"bsp_hidden",o.bsp_hidden}});
  }
  if(campaign&&!authored)throw FormatError("Original machine29 placement is absent");
  for(const auto &o:row["scene_owners"])if(o.at("visible").get<bool>())throw FormatError("Original machine29 is visible in this source context");
  for(const auto &o:row["device_owners"])if(o.at("visible").get<bool>())throw FormatError("Machine29 device owner is visible in this source context");
  const auto &source_cache=*world.cache;
  CollisionWorld collision(world.content.collision);Mesh mesh;
  const auto before_snapshot_query=world.source_objects->memory().checkpoint();
  const auto tick=world.match.state().devices.tick;
  const auto bank_before=world.source_objects->memory().checkpoint();
  row["snapshot_query_changed_regions"]=Json::array();
  for(unsigned i=0;i<before_snapshot_query.regions.size();++i) {
   const auto &a=before_snapshot_query.regions[i],&b=bank_before.regions.at(i);
   if(a!=b)row["snapshot_query_changed_regions"].push_back({{"address",a.address},{"owner",a.owner}});
  }
  SourceMachineScene machines(source_cache,world.content.objects,bsp,mesh,collision,true);
  machines.synchronize(world.match.simulation().scene(),&world.match.simulation().devices());
  machines.update_pose(tick,0,mesh);
  SceneInterpolation history;history.reset(world.match.simulation().ticks(),world.match.simulation().scene().objects());
  machines.interpolate(world.match.simulation().scene(),history,1.f);machines.update_pose(tick,0,mesh);
  row["deferred_resources"]=Json::array();
  for(const auto &boundary:machines.deferred_resource_owners())row["deferred_resources"].push_back({{"source_object",boundary.source_object},{"error",boundary.error}});
  if(campaign && std::none_of(machines.deferred_resource_owners().begin(),machines.deferred_resource_owners().end(),[&](const auto &p){return p.source_object==row.at("content_index").get<unsigned>();}))
    throw FormatError("Original hidden machine29 resource boundary was not retained");
  std::vector<Vertex> submitted;std::vector<ObjectLightingBinding> lighting;machines.append(submitted,lighting);std::size_t vertex=0;
  row["actual_machine_nodes"]=0;row["actual_machine_vertices"]=0;
  for(const auto &owner:machines.effect_owners()) {
   const auto actual=world.source_objects->world_nodes(owner.object);
   if(actual.size()!=owner.nodes.size() || (!actual.empty()&&std::memcmp(actual.data(),owner.nodes.data(),actual.size()*sizeof(Transform))))
     throw FormatError("Machine presentation replaced completed source WORLD nodes");
   verified_nodes+=unsigned(actual.size());row["actual_machine_nodes"]=row["actual_machine_nodes"].get<unsigned>()+unsigned(actual.size());
   const auto &object=world.match.simulation().scene().get(owner.object);if(!object.visible)continue;
   const auto &p=world.content.objects.at(owner.placement);const auto selection=source_cache.load_initial_model_selection(p.definition,p.variant);
   const auto model=source_cache.load_model(p.definition,true,p.variant,selection,5);
   const auto expected=skin_model_world_nodes(model,actual);
   if(vertex+expected.size()>submitted.size())throw FormatError("Rendered machine lost original WORLD vertices");
   for(unsigned i=0;i<expected.size();++i) {
    const auto &a=expected[i],&b=submitted[vertex+i];
    if(std::memcmp(&a.position,&b.position,sizeof(Vec3)) || std::memcmp(&a.normal,&b.normal,sizeof(Vec3)))
      throw FormatError("Rendered machine did not skin its actual source WORLD bank");
   }
   vertex+=expected.size();verified_vertices+=unsigned(expected.size());row["actual_machine_vertices"]=row["actual_machine_vertices"].get<unsigned>()+unsigned(expected.size());
  }
  if(vertex!=submitted.size())throw FormatError("Machine renderer submitted a hidden or extra source owner");
  const auto bank_after=world.source_objects->memory().checkpoint();
  if(bank_after!=bank_before) {
   Json differences=Json::array();
   for(unsigned i=0;i<bank_before.regions.size();++i) {
    const auto &a=bank_before.regions[i],&b=bank_after.regions.at(i);
    if(a!=b)differences.push_back({{"address",a.address},{"owner",a.owner}});
   }
   throw FormatError("Read-only machine presentation mutated source regions: "+differences.dump());
  }
  if(campaign && bsp==0) {
   auto native=world.content;const auto target=native.objects.at(row.at("content_index").get<unsigned>());native.objects={target};
   Mesh strict_mesh;bool rejected=false;
   try {SourceMachineScene strict(source_cache,native.objects,0,strict_mesh,collision);}catch(const FormatError &e){rejected=std::string(e.what())=="missing/truncated bitmap tag";}
   if(!rejected)throw FormatError("Legacy eager machine resource admission changed");
   Mesh isolated;isolated.materials.push_back({});isolated.materials.back().name="retained material sentinel";
   SourceMachineScene deferred(source_cache,native.objects,0,isolated,collision,true);
   if(isolated.materials.size()!=1 || isolated.materials.front().name!="retained material sentinel" || deferred.stats().deferred_resources!=1)
     throw FormatError("Deferred machine preparation leaked partial material slots");
   SceneObjects lifetimes(native,0,SourceSceneGameContext{1,0});deferred.synchronize(lifetimes);
   lifetimes.switch_bsp(2);rejected=false;
   try {deferred.synchronize(lifetimes);}catch(const FormatError &e){rejected=std::string(e.what()).find("requires original source resources: missing/truncated bitmap tag")!=std::string::npos;}
   if(!rejected)throw FormatError("Newly visible original machine bypassed its retained resource boundary");
   row["visible_bsp2_lifetime_rejected"]=true;row["partial_material_rollback_verified"]=true;row["legacy_eager_admission_preserved"]=true;
  }
  report["contexts"].push_back(row);
 }
 if(!verified_nodes||!verified_vertices)throw FormatError("Actual machine WORLD node/vertex corpus is empty");
 report["verified_original_world_nodes"]=verified_nodes;report["verified_original_world_vertices"]=verified_vertices;
 report["passed"]=true;report["boundary"]="Actual TU1.5 world construction, retained machine lifetimes, exact completed WORLD nodes and skinned vertices at Floodlab BSP0/5 and Ivory Tower. Separate native presentation lifetime inputs exercise BSP2 visibility rejection and material rollback without replacing source bodies, poses, contacts or solver outputs. Future physical evolution, live rendered multiplayer and Xbox parity remain required.";
 std::ofstream(out/"report.json")<<report.dump(2)<<'\n';std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
