#include "halo2/source_machine_scene.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/scene.hpp"
#include "halo2/reference_player.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/scenery.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <bit>
#include <filesystem>
#include <iostream>
#include <set>
#include <map>
#include <thread>
using namespace halo2;
namespace {
std::vector<Vertex> material_vertices(const std::vector<Vertex>&world,const ObjectLightingBinding &binding){
 std::vector<Vertex> result;for(const auto&v:world)if(v.material>=binding.first_material &&
  v.material<binding.first_material+binding.material_count)result.push_back(v);return result;
}
void same_pose(const std::vector<Vertex>&a,const std::vector<Vertex>&b){
 if(a.empty() || a.size()!=b.size())throw FormatError("recreated source machine lost authored geometry");
 for(unsigned i=0;i<a.size();++i)if(a[i].position.x!=b[i].position.x || a[i].position.y!=b[i].position.y ||
  a[i].position.z!=b[i].position.z || a[i].normal.x!=b[i].normal.x || a[i].normal.y!=b[i].normal.y ||
  a[i].normal.z!=b[i].normal.z || a[i].prt_cluster!=b[i].prt_cluster || a[i].prt_weights!=b[i].prt_weights)
  throw FormatError("machine lifetime retained previous pose or advanced from render time");
}
void prepared_machine_birth(){
 Cache cache("assets/map-pack/maps/triplicate.map");auto content=cache.load_content(0,true);
 const auto at=std::find_if(content.objects.begin(),content.objects.end(),[](const auto&o){return o.type==7 && scenery_automatic_creation_allowed(o,2);});
 if(at==content.objects.end())throw FormatError("prepared machine fixture missing authored resource");
 const auto placement=std::size_t(at-content.objects.begin());
 for(std::size_t i=0;i<content.objects.size();++i)if(i!=placement)content.objects[i].definition=null_datum;
 content.objects[placement].flags|=1u; // Declared automatic-birth suppression input.
 CollisionWorld collision(content.collision);Mesh mesh;SourceMachineScene scene(cache,content.objects,0,mesh,collision);
 if(scene.stats().render_models!=1)throw FormatError("Deferred machine resource was not prepared");
 const auto absent=[&]{std::vector<Vertex> v;std::vector<ObjectLightingBinding> b;scene.append(v,b);
  if(!v.empty() || !b.empty())throw FormatError("Prepared machine geometry submitted before actual birth");};
 absent();SceneObjects objects(content,0,SourceSceneGameContext{2,2});scene.synchronize(objects);absent();
 const auto born=objects.create_placement(placement,7);scene.synchronize(objects);scene.update_pose(7,0,mesh);
 std::vector<Vertex> v;std::vector<ObjectLightingBinding>b;scene.append(v,b);
 if(born==null_datum || v.empty() || b.size()!=1)throw FormatError("Actual deferred machine birth did not activate its prepared resource");
 objects.destroy(born);scene.synchronize(objects);absent();
 std::cout<<"Prepared machine: no prebirth submission, actual deferred birth and retirement\n";
}
void triplicate_lifetimes(){
 Cache cache("assets/map-pack/maps/triplicate.map");const auto content=cache.load_content(0,true);
 CollisionWorld collision(content.collision);Mesh mesh;SourceMachineScene scene(cache,content.objects,0,mesh,collision);
 SceneObjects objects(content,0);scene.synchronize(objects);scene.update(0,0,{},1,mesh);
 const auto retained=objects;const auto resident_materials=mesh.materials.size();
 std::map<std::size_t,std::vector<std::uint8_t>> retained_permutations;
 for(const auto &o:scene.effect_owners())retained_permutations[o.placement]={o.permutations.begin(),o.permutations.end()};
 std::vector<Vertex> initial;std::vector<ObjectLightingBinding> bindings;scene.append(initial,bindings);
 if(bindings.size()!=7)throw FormatError("Triplicate source machine lifetime inventory");
 const auto initial_portals=scene.portal_owners();
 for(unsigned view=0;view<4;++view){std::vector<Vertex> draw;std::vector<ObjectLightingBinding> light;
  scene.append_view(draw,light,SourceObjectViewLod{view,4,false,[](auto,auto,auto,auto){return SourceObjectLodInput{0,0x80000u,0};}});}
 const auto exact=scene.checkpoint();
 if(!exact.valid()||exact.tick()!=0||!scene.owns_checkpoint(exact))
  throw FormatError("Machine checkpoint lost its actual same-owner completed cut");
 const auto east=material_vertices(initial,bindings[0]);
 scene.update(1,1,{},1,mesh);std::vector<Vertex> one;std::vector<ObjectLightingBinding> first;scene.append(one,first);
 const auto east_one=material_vertices(one,first[0]);
 bool moved=false;for(unsigned i=0;i<east.size();++i)moved|=east[i].position.x!=east_one[i].position.x;
 if(!moved)throw FormatError("Triplicate source gear fixture did not advance");
 //These names/ordinals are retained from actual compiled authored syntax;
 //the renderer module receives lifetimes and has no map/script-name branch.
 for(int name:{4,6,5,7})objects.destroy(objects.resolve(name));
 scene.synchronize(objects);scene.update(130,130,{},1,mesh);
 std::vector<Vertex> gone;std::vector<ObjectLightingBinding> remaining;scene.append(gone,remaining);
 if(remaining.size()!=3 || !material_vertices(gone,bindings[0]).empty() ||
    !material_vertices(gone,bindings[1]).empty())throw FormatError("destroyed train submitted stale geometry");
 const auto replacement=objects.create(4,false,130);scene.synchronize(objects);scene.update(130,130,{},1,mesh);
 std::vector<Vertex> born;std::vector<ObjectLightingBinding> alive;scene.append(born,alive);
 if(alive.size()!=4 || objects.get(replacement).created_tick!=130)throw FormatError("source train creation tick lost");
 same_pose(east,material_vertices(born,bindings[0]));
 //Multiple rendering polls within one game tick never advance the gear.
 scene.synchronize(objects);scene.update(130,131,{},1,mesh);born.clear();alive.clear();scene.append(born,alive);
 same_pose(east,material_vertices(born,bindings[0]));
 scene.update(131,132,{},1,mesh);born.clear();alive.clear();scene.append(born,alive);
 same_pose(east_one,material_vertices(born,bindings[0]));
 objects.visible(replacement,false);scene.synchronize(objects);born.clear();alive.clear();scene.append(born,alive);
 if(alive.size()!=3)throw FormatError("source hidden machine submitted geometry");
 bool reversed=false;try{scene.update_pose(0,0,mesh);}catch(const FormatError&){reversed=true;}
 if(!reversed)throw FormatError("Unannounced machine clock rollback was accepted");
 // The exact historical snapshot remains eligible after removal, recreation,
 // hiding and131 later controller ticks. It restores the captured owners and
 // caches themselves, independently of the current SceneObjects lifetimes.
 Mesh foreign_mesh;SourceMachineScene foreign(cache,content.objects,0,foreign_mesh,collision);
 if(foreign.owns_checkpoint(exact))throw FormatError("Foreign machine scene claimed a historical checkpoint");
 bool foreign_rejected=false;try{(void)foreign.prepare_restore(exact);}catch(const FormatError&){foreign_rejected=true;}
 if(!foreign_rejected)throw FormatError("Foreign machine scene prepared another resource owner's state");
 std::vector<Vertex> later;std::vector<ObjectLightingBinding> later_light;scene.append(later,later_light);
 {auto cancelled=scene.prepare_restore(exact);
  if(!scene.restore_ready(cancelled)||foreign.restore_ready(cancelled)||foreign.commit_restore(cancelled))
   throw FormatError("Prepared machine checkpoint lost same-owner cancellation/foreign guards");}
 std::vector<Vertex> cancelled;std::vector<ObjectLightingBinding> cancelled_light;scene.append(cancelled,cancelled_light);
 same_pose(later,cancelled);
 if(scene.checkpoint().tick()!=131)throw FormatError("Cancelled machine restore changed the live controller clock");
 auto stale=scene.prepare_restore(exact);scene.update_pose(131,133,mesh);
 if(scene.restore_ready(stale)||scene.commit_restore(stale)||!scene.owns_checkpoint(exact))
  throw FormatError("Later machine mutation kept a stale prepared token or retired the owning historical checkpoint");
 auto prepared=scene.prepare_restore(exact);
 bool thread_capture_rejected{},thread_prepare_rejected{},thread_ready{},thread_commit{},thread_owns{};
 std::thread other([&]{
  try{(void)scene.checkpoint();}catch(const FormatError&){thread_capture_rejected=true;}
  try{(void)scene.prepare_restore(exact);}catch(const FormatError&){thread_prepare_rejected=true;}
  thread_ready=scene.restore_ready(prepared);thread_commit=scene.commit_restore(prepared);thread_owns=scene.owns_checkpoint(exact);
 });other.join();
 if(!thread_capture_rejected||!thread_prepare_rejected||thread_ready||thread_commit||thread_owns||!scene.restore_ready(prepared))
  throw FormatError("Foreign executor captured/published machine state or invalidated a quiescent local preparation");
 if(!scene.commit_restore(prepared)||scene.restore_ready(prepared)||scene.commit_restore(prepared)||scene.checkpoint().tick()!=0)
  throw FormatError("Exact machine restore did not publish its captured clock once");
 std::vector<Vertex> restored;std::vector<ObjectLightingBinding> restored_light;scene.append(restored,restored_light);
 same_pose(initial,restored);
 if(restored_light.size()!=bindings.size()||mesh.materials.size()!=resident_materials)
  throw FormatError("Exact machine restore changed resident material slots or lighting inventory");
 for(unsigned i=0;i<bindings.size();++i)if(bindings[i].first_material!=restored_light[i].first_material||
   bindings[i].material_count!=restored_light[i].material_count||bindings[i].state.source_values()!=restored_light[i].state.source_values())
  throw FormatError("Exact machine restore reconstructed different completed lighting values");
 const auto restored_portals=scene.portal_owners();
 if(restored_portals.size()!=initial_portals.size())throw FormatError("Exact machine restore lost captured active portal owners");
 for(unsigned i=0;i<initial_portals.size();++i){const auto &a=initial_portals[i],&b=restored_portals[i];
  if(a.source_object!=b.source_object||a.created_tick!=b.created_tick||a.owner.handle!=b.owner.handle||
    a.door.current_position!=b.door.current_position||a.door.group_position!=b.door.group_position||
    a.door.device_flags!=b.door.device_flags||a.initialized_group_state_only!=b.initialized_group_state_only)
   throw FormatError("Exact machine restore lost captured salted lifetimes or private device channels");
 }
 scene.update(1,1,{},1,mesh);restored.clear();restored_light.clear();scene.append(restored,restored_light);
 same_pose(east_one,material_vertices(restored,bindings[0]));
 std::cout<<"PASS exact machine presentation restore: historical salted lifetimes/private controller/pose/lighting, cancellation, stale token, foreign owner/executor, once-only publication and resident material slots\n";
 scene.reset_lifetimes();born.clear();alive.clear();scene.append(born,alive);
 if(!born.empty()||!alive.empty())throw FormatError("Retired machine lifetimes still submitted geometry");
 scene.synchronize(retained);scene.update_pose(0,0,mesh);scene.append(born,alive);
 same_pose(initial,born);
 if(mesh.materials.size()!=resident_materials || alive.size()!=bindings.size())
  throw FormatError("Machine world adoption rebuilt resident slots or lost preserved handles");
 for(unsigned i=0;i<initial.size();++i)if(initial[i].material!=born[i].material)
  throw FormatError("Machine world adoption changed an uploaded material index");
 for(const auto &o:scene.effect_owners())if(!std::ranges::equal(o.permutations,retained_permutations.at(o.placement)))
  throw FormatError("Machine world adoption discarded authored attachment permutation ownership");
 std::cout<<"Machine world adoption: same salted handles, tick131 to tick0, no stale draws, original geometry/material slots\n";
 std::cout<<"Triplicate lifetime: four source removals, fresh generation/pose, exact tick age and hidden submission\n";
}
void reference_devices(){
 const auto directory=std::filesystem::path(__FILE__).parent_path()/"fixtures";
 const auto reference=load_reference_player_state(directory/"retail_zanzibar_player.reference");
 if(reference.devices.size()!=2)throw FormatError("source device image fixture missing");
 Cache cache("assets/disc/maps/zanzibar.map");const auto content=cache.load_content(0,true);
 CollisionWorld collision(content.collision);Mesh mesh;SourceMachineScene scene(cache,content.objects,0,mesh,collision);
 const auto reject=[&](std::vector<ReferenceDeviceState> bank){
  bool rejected=false;try{scene.set_reference_devices(bank,mesh);}catch(const FormatError&){rejected=true;}
  if(!rejected)throw FormatError("unmatched reference device admitted");
 };
 auto bad=reference.devices;bad[0].definition=0xffffffffu;reject(bad);
 bad=reference.devices;bad[1]=bad[0];reject(bad);
 bad=reference.devices;bad[0].render_model=0xffffffffu;reject(bad);
 scene.set_reference_devices(reference.devices,mesh);
 SceneObjects objects(content,0,SourceSceneGameContext{2,2});scene.synchronize(objects);
 // Both literal authored material identities belong to the source paired
 // geometry. The expected bank is captured GPU memory, not native output.
 std::ifstream fixture(directory/"retail_zanzibar_device_banks.tsv");std::string line;unsigned checked=0;
 const std::array<std::string,2> names{cache.load_material(0xebe50a70).name,cache.load_material(0xec0b0a96).name};
 std::vector<std::vector<ShaderVector>> initial;
 while(std::getline(fixture,line)){
  if(line.empty() || line[0]=='#')continue;
  std::istringstream row(line);unsigned definition,model,placement,count;row>>std::hex>>definition>>model>>std::dec>>placement>>count;
  if(checked>=2 || reference.devices[checked].definition!=definition || reference.devices[checked].render_model!=model ||
     reference.devices[checked].scenario_placement!=placement || count!=168)throw FormatError("device source bank identity changed");
  const auto material=std::find_if(mesh.materials.begin(),mesh.materials.end(),[&](const auto&m){return m.name==names[checked];});
  if(material==mesh.materials.end() || !material->environment_pass || material->environment_pass->forward_vertex!=9)
   throw FormatError("source reference device PRT material missing");
  const auto&bank=*material->environment_pass->vertex_bank;initial.push_back(bank.constants);
  if(bank.first_constant!=68 || bank.constants.size()*4!=count)throw FormatError("reference device bank range changed");
  for(unsigned i=0;i<count;++i){unsigned expected;row>>std::hex>>expected;
   if(i==3 || (i>=8 && (i-8)%16==3))continue;
   const auto actual=bank.constants[i/4][i%4];
   if(i<8 ? std::bit_cast<unsigned>(actual)!=expected : std::abs(actual-std::bit_cast<float>(expected))>9e-8f)
    throw FormatError("reference device differs from independently captured source bank");
  }
  if(!row)throw FormatError("truncated source device bank fixture");++checked;
 }
 if(checked!=2)throw FormatError("source reference device bank coverage changed");
 const auto selected=[&](const std::vector<Vertex>&world){std::vector<Vertex>out;for(const auto&v:world)
  if(mesh.materials[v.material].name==names[0] || mesh.materials[v.material].name==names[1])out.push_back(v);return out;};
 std::vector<Vertex>world;std::vector<ObjectLightingBinding>bindings;scene.append(world,bindings);const auto first=selected(world);
 MaterialPresentationContinuity materials(mesh);
 const auto material_cut=materials.checkpoint();
 const auto exact=scene.checkpoint();
 std::vector<std::pair<unsigned,std::shared_ptr<const VertexProgramBank>>> captured_banks;
 for(unsigned i=0;i<mesh.materials.size();++i){const auto &pass=mesh.materials[i].environment_pass;
  if(pass&&pass->forward_vertex==9)captured_banks.emplace_back(i,pass->vertex_bank);}
 if(captured_banks.empty())throw FormatError("Exact machine bank fixture has no actual resident PRT link");
 materials.mutating();scene.synchronize(objects);scene.update(30,30,reference.camera_position,1,mesh);
 world.clear();bindings.clear();scene.append(world,bindings);same_pose(first,selected(world));
 for(unsigned i=0;i<2;++i){const auto material=std::find_if(mesh.materials.begin(),mesh.materials.end(),[&](const auto&m){return m.name==names[i];});
  if(material->environment_pass->vertex_bank->constants!=initial[i])throw FormatError("diagnostic device advanced its frozen source image phase");}
 std::vector<std::shared_ptr<const VertexProgramBank>> later_banks;
 for(const auto &[slot,bank]:captured_banks)later_banks.push_back(mesh.materials[slot].environment_pass->vertex_bank);
 if(later_banks.front()==captured_banks.front().second)
  throw FormatError("Machine fixture did not generate a later actual PRT bank owner");
 const std::weak_ptr<const VertexProgramBank> retired=later_banks.front();
 {auto cancelled=scene.prepare_restore(exact);if(!scene.restore_ready(cancelled))throw FormatError("Actual PRT restore was not prepared");}
 for(unsigned i=0;i<captured_banks.size();++i)if(mesh.materials[captured_banks[i].first].environment_pass->vertex_bank!=later_banks[i])
  throw FormatError("Cancelled machine restore changed a live PRT link");
 {auto prepared=scene.prepare_restore(exact),sibling=scene.prepare_restore(exact);
  auto &pass=mesh.materials[captured_banks.front().first].environment_pass;
  const auto live_pass=pass;pass.reset();
  if(scene.restore_ready(prepared)||scene.commit_restore(prepared))throw FormatError("Replaced external material pass kept restore eligibility");
  pass=live_pass;pass->vertex_bank=captured_banks.front().second;
  if(scene.restore_ready(prepared)||scene.commit_restore(prepared))throw FormatError("Changed live PRT bank kept restore eligibility");
  pass->vertex_bank=later_banks.front();later_banks.clear();
  if(!scene.restore_ready(prepared)||!scene.commit_restore(prepared)||scene.restore_ready(sibling)||scene.commit_restore(sibling)||retired.expired())
   throw FormatError("Machine PRT restore did not publish once and retain displaced immutable bank owners");
  for(const auto &[slot,bank]:captured_banks)if(mesh.materials[slot].environment_pass->vertex_bank!=bank)
   throw FormatError("Exact machine restore recomputed rather than restored a captured PRT link");
 }
 if(!retired.expired())throw FormatError("Machine restore retained displaced PRT bank owners after caller retirement");
 if(scene.checkpoint().tick()!=0)throw FormatError("PRT scene restore lost its actual historical clock");
 // The real lighting producer may detach a shared child through its normal
 // COW rule. Scene methods accept that new graph, while this subowner refuses
 // to restore an old graph without the separate material-animation owner.
 const auto slot=captured_banks.front().first;
 materials.mutating();
 apply_object_lighting(mesh.materials[slot],bindings.front().state);
 scene.update(1,1,reference.camera_position,1,mesh);world.clear();bindings.clear();scene.append(world,bindings);
 bool graph_rejected=false;try{(void)scene.prepare_restore(exact);}catch(const FormatError&){graph_rejected=true;}
 if(!graph_rejected||!scene.owns_checkpoint(exact))throw FormatError("Detached material graph bypassed its external owner boundary");
 const auto current_cut=scene.checkpoint();auto current_restore=scene.prepare_restore(current_cut);
 if(!scene.restore_ready(current_restore)||!scene.commit_restore(current_restore))
  throw FormatError("Normal material COW prevented a new actual scene checkpoint from restoring");
 std::cout<<"PASS exact machine PRT restore: actual captured immutable bank identity, external pass/bank refusal, cancellation, stale sibling, deferred retirement and normal lighting COW with explicit external graph boundary\n";
 const auto future_pass=mesh.materials[slot].environment_pass;
 const auto future_bank=future_pass->vertex_bank;
 {auto wrong_material=materials.prepare_restore(materials.checkpoint());
  bool wrong=false;try{(void)scene.prepare_restore(exact,wrong_material.target_graph());}catch(const FormatError&){wrong=true;}
  if(!wrong)throw FormatError("Machine scene accepted another historical material target graph");}
 {auto cancelled_material=materials.prepare_restore(material_cut);
  auto cancelled_scene=scene.prepare_restore(exact,cancelled_material.target_graph());
  if(!materials.restore_ready(cancelled_material)||!scene.restore_ready(cancelled_scene)||scene.commit_restore(cancelled_scene))
   throw FormatError("Machine/material preparation omitted complete preflight or allowed scene-first publication");}
 if(mesh.materials[slot].environment_pass!=future_pass||future_pass->vertex_bank!=future_bank||scene.checkpoint().tick()!=1)
  throw FormatError("Cancelled composed material/scene restore changed live owners");
 {auto stale_material=materials.prepare_restore(material_cut);
  auto stale_scene=scene.prepare_restore(exact,stale_material.target_graph());materials.mutating();
  if(scene.restore_ready(stale_scene)||scene.commit_restore(stale_scene)||materials.commit_restore(stale_material))
   throw FormatError("Changed material transaction retained a dependent scene token");}
 auto material_restore=materials.prepare_restore(material_cut);
 auto restored_scene=scene.prepare_restore(exact,material_restore.target_graph());
 const auto target=material_restore.target_graph();
 if(!materials.restore_ready(material_restore)||!scene.restore_ready(restored_scene)||
   !materials.commit_restore(material_restore)||!target.applied()||!scene.restore_ready(restored_scene)||
   !scene.commit_restore(restored_scene)||!target.applied()||scene.commit_restore(restored_scene)||scene.checkpoint().tick()!=0)
  throw FormatError("Historical material graph and exact machine state did not publish in one prepared order");
 for(const auto &[index,bank]:captured_banks)if(mesh.materials[index].environment_pass->vertex_bank!=bank)
  throw FormatError("Composed machine commit changed the material owner's exact target PRT link");
 world.clear();bindings.clear();scene.append(world,bindings);same_pose(first,selected(world));
 const auto budget=material_cut.stats();
 std::cout<<"PASS composed original machine/material COW restore: private target graph proof before mutation, mismatched/cancelled/stale refusal, material-first publication, exact captured pass/PRT identity and historical pose; roots="
  <<budget.roots<<" nodes="<<budget.nodes<<" logical_bytes="<<budget.logical_bytes<<" resident_schema_bytes="<<budget.resident_schema_bytes<<'\n';
 std::cout<<"Source device image fixture: 2 matched placements, 314 consumed source bank words within measured bounds, frozen through synchronize/tick30\n";
}
void simulation_devices(){
 Cache cache("assets/disc/maps/zanzibar.map");const auto content=cache.load_content(0,true);
 CollisionWorld collision(content.collision);Mesh mesh;SourceMachineScene machines(cache,content.objects,0,mesh,collision);
 SceneObjects objects(content,0);SourceDeviceWorld devices(content);devices.synchronize(objects);
 machines.synchronize(objects,&devices);machines.update_pose(0,0,mesh);
 std::vector<Vertex> initial;std::vector<ObjectLightingBinding> bindings;machines.append(initial,bindings);
 if(bindings.size()!=6)throw FormatError("Zanzibar live source machine render inventory");
 const auto gate=material_vertices(initial,bindings[1]);std::uint32_t control=~0u;
 for(const auto&[id,o]:devices.owners())if(content.devices[o.placement]->control)control=id;
 if(control==~0u || !devices.activate_admitted_control(control).changed)throw FormatError("Zanzibar admitted shared control failed");
 for(unsigned i=0;i<300;++i)devices.step(1.f/30.f);
 machines.synchronize(objects,&devices);machines.update_pose(300,1,mesh);
 std::vector<Vertex> opened;std::vector<ObjectLightingBinding> next;machines.append(opened,next);
 const auto open_gate=material_vertices(opened,bindings[1]);
 if(open_gate.size()!=gate.size() || gate.empty())throw FormatError("moving source gate lost rendered geometry");
 bool moved=false;for(unsigned i=0;i<gate.size();++i)moved|=gate[i].position.x!=open_gate[i].position.x ||
  gate[i].position.y!=open_gate[i].position.y || gate[i].position.z!=open_gate[i].position.z;
 if(!moved)throw FormatError("simulation group changed without moving rendered gate");
 for(const auto&p:machines.portal_owners()){
  const auto*owner=devices.find(p.owner.handle);
  if(!owner || p.initialized_group_state_only || p.door.current_position!=owner->motion.position ||
   p.door.group_position!=devices.position_group(*owner).value || p.door.device_flags!=owner->motion.flags)
   throw FormatError("machine portal inputs lost actual simulation state");
 }
 machines.update_pose(300,2,mesh);opened.clear();next.clear();machines.append(opened,next);
 same_pose(open_gate,material_vertices(opened,bindings[1]));
 // A late-joining client has unrelated local scene lifetimes. The replicated
 // host handles, root transforms and current channels own rendering/portals.
 SceneObjects client_objects(content,0);client_objects.destroy_all();SourceDeviceWorld replica(content);
 replica.apply_snapshot(devices.snapshot(300));Mesh peer_mesh;
 SourceMachineScene peer(cache,content.objects,0,peer_mesh,collision);peer.synchronize(client_objects,&replica);
 peer.update_pose(300,0,peer_mesh);std::vector<Vertex> peer_vertices;std::vector<ObjectLightingBinding> peer_bindings;
 peer.append(peer_vertices,peer_bindings);
 if(peer_bindings.size()!=bindings.size())throw FormatError("replicated machines depended on client scene lifetimes");
 same_pose(open_gate,material_vertices(peer_vertices,peer_bindings[1]));
 const auto host_portals=machines.portal_owners(),peer_portals=peer.portal_owners();
 if(host_portals.size()!=peer_portals.size())throw FormatError("replicated machine portal inventory lost");
 for(unsigned i=0;i<host_portals.size();++i){const auto&a=host_portals[i];const auto&b=peer_portals[i];
  if(a.owner.handle!=b.owner.handle || a.created_tick!=b.created_tick || b.initialized_group_state_only ||
    a.door.current_position!=b.door.current_position || a.door.group_position!=b.door.group_position || a.door.device_flags!=b.door.device_flags)
   throw FormatError("replicated machine portal state differs from authority");
 }
 peer.update_pose(301,1,peer_mesh);peer_vertices.clear();peer_bindings.clear();peer.append(peer_vertices,peer_bindings);
 same_pose(open_gate,material_vertices(peer_vertices,peer_bindings[1]));
 const auto gate_binding=peer_bindings[1];const auto old_gate=host_portals[1].owner.handle;
 const auto gate_name=content.objects[devices.find(old_gate)->placement].name;
 objects.destroy(old_gate);devices.synchronize(objects);replica.apply_snapshot(devices.snapshot(301));
 peer.synchronize(client_objects,&replica);peer.update_pose(301,2,peer_mesh);
 peer_vertices.clear();peer_bindings.clear();peer.append(peer_vertices,peer_bindings);
 if(peer_bindings.size()!=5 || !material_vertices(peer_vertices,gate_binding).empty())
  throw FormatError("removed authoritative gate submitted stale client geometry");
 const auto replacement=objects.create(gate_name,false,302);devices.synchronize(objects);
 replica.apply_snapshot(devices.snapshot(302));peer.synchronize(client_objects,&replica);peer.update_pose(302,3,peer_mesh);
 peer_vertices.clear();peer_bindings.clear();peer.append(peer_vertices,peer_bindings);
 if(replacement==old_gate || peer_bindings.size()!=6)throw FormatError("recreated authoritative gate did not publish client geometry");
 same_pose(open_gate,material_vertices(peer_vertices,gate_binding));
 std::cout<<"Zanzibar simulation owner: shared control moves rendered gate, exact portal inputs, repeated-view stability\n";
}
void presentation_devices(){
 Cache cache("assets/map-pack/maps/triplicate.map");const auto content=cache.load_content(0,true);
 CollisionWorld collision(content.collision);Mesh mesh;SourceMachineScene scene(cache,content.objects,0,mesh,collision);
 SceneObjects objects(content,0);SourceDeviceWorld devices(content);devices.synchronize(objects);
 scene.synchronize(objects,&devices);scene.update_pose(0,0,mesh);
 std::vector<Vertex> start;std::vector<ObjectLightingBinding> bindings;scene.append(start,bindings);
 const auto previous=devices.snapshot(0);devices.step(1.f/30.f);
 const auto portals=[&] {
  for(const auto &packet:scene.portal_owners()) {
   const auto *owner=devices.find(packet.owner.handle);
   if(!owner || packet.door.current_position!=owner->motion.position ||
      packet.door.group_position!=devices.position_group(*owner).value || packet.door.device_flags!=owner->motion.flags)
    throw FormatError("interpolated machine pose changed authoritative portal/collision channels");
  }
 };
 std::array<std::vector<Vertex>,5> draws;
 for(unsigned i=0;i<draws.size();++i) {
  scene.synchronize(objects,&devices);scene.interpolate(previous,float(i)/4);
  scene.update_pose(1,int(i+1),mesh);bindings.clear();scene.append(draws[i],bindings);portals();
 }
 same_pose(start,draws[0]);
 for(unsigned sample=1;sample<draws.size();++sample) {
  if(draws[sample].size()!=start.size())throw FormatError("fractional machine pose changed geometry inventory");
  bool moved=false;
  for(unsigned v=0;v<start.size();++v) {
   const auto &a=draws[sample-1][v].position,&b=draws[sample][v].position;
   moved|=a.x!=b.x || a.y!=b.y || a.z!=b.z;
  }
  if(!moved)throw FormatError("30 Hz device failed to move at a fractional render sample");
 }
 scene.synchronize(objects,&devices);scene.update_pose(1,6,mesh);
 std::vector<Vertex> endpoint;bindings.clear();scene.append(endpoint,bindings);same_pose(draws.back(),endpoint);
 std::cout<<"Triplicate presentation: four fractional geometry steps per simulation tick; portal channels unchanged\n";
}
}
int main(){try{
 prepared_machine_birth();reference_devices();triplicate_lifetimes();simulation_devices();presentation_devices();SourceMachineSceneStats totals;unsigned vertices=0,bindings=0,prt_banks=0;
 const std::string maps=" ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar containment warlock deltatap turf backwash dune elongation gemini triplicate ";
 for(const auto&directory:{"assets/disc/maps","assets/map-pack/maps"})for(const auto&entry:std::filesystem::directory_iterator(directory)){
  if(entry.path().extension()!=".map" || maps.find(" "+entry.path().stem().string()+" ")==std::string::npos)continue;
  Cache cache(entry.path());auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);Mesh mesh;
  // This original census covers the automatic initial MP2/current-BSP corpus.
  for(auto &o:content.objects)if(o.type==7 && (!scenery_automatic_creation_allowed(o,2) || o.origin_bsp>0))o.definition=null_datum;
  SourceMachineScene scene(cache,content.objects,0,mesh,collision);
  SceneObjects objects(content,0,SourceSceneGameContext{2,2});scene.synchronize(objects);const auto&s=scene.stats();
  totals.placements+=s.placements;totals.render_models+=s.render_models;totals.position_overlays+=s.position_overlays;
  totals.dynamic_materials+=s.dynamic_materials;totals.prt_materials+=s.prt_materials;totals.initial_state_only+=s.initial_state_only;
  totals.union_dynamic_materials+=s.union_dynamic_materials;totals.union_prt_materials+=s.union_prt_materials;
  totals.union_section_banks+=s.union_section_banks;
  //Initial source cadence does not depend on view distance; later tests
  //supply their explicit first-view distance input, rather than a camera proxy.
  scene.update(0,0,{},1,mesh);scene.update(1,1,{},1,mesh);
  std::vector<Vertex> world;std::vector<ObjectLightingBinding> light;scene.append(world,light);
  for(const auto&v:world)if(v.material>=mesh.materials.size())throw FormatError("machine vertex lost material slot");
  std::set<const VertexProgramBank*> banks;
  for(const auto&material:mesh.materials)if(material.environment_pass && material.environment_pass->forward_vertex==9){
   const auto&bank=material.environment_pass->vertex_bank;
   if(!bank || !bank->prt_inputs || !banks.insert(bank.get()).second)throw FormatError("machine sections share a mutable PRT bank");++prt_banks;
  }
  for(const auto&b:light)if(b.first_material+b.material_count>mesh.materials.size())throw FormatError("machine lighting range lost slots");
  vertices+=world.size();bindings+=light.size();
 }
 if(totals.placements!=49 || totals.render_models!=48 || totals.position_overlays!=37 ||
    totals.dynamic_materials!=97 || totals.prt_materials!=7 || bindings!=48 || !vertices || !prt_banks)
  throw FormatError("source initial machine scene inventory changed");
 std::cout<<"machine scene: "<<totals.placements<<" placements, "<<totals.render_models<<" models, "
  <<totals.dynamic_materials<<" dynamic + "<<totals.prt_materials<<" PRT source materials, "<<prt_banks
  <<" section banks, "<<vertices<<" vertices, "<<totals.initial_state_only<<" initial-state-only devices; resident union "<<totals.union_dynamic_materials<<" dynamic + "
  <<totals.union_prt_materials<<" PRT slots / "<<totals.union_section_banks<<" section banks\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
