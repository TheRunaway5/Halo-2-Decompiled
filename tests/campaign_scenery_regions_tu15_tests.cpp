#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_scene_physics.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/model.hpp"
#include "halo2/renderer.hpp"
#include "halo2/original_data.hpp"
#include "helpers/campaign_vehicle_observer_inputs_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
using namespace halo2;
void require(bool value,const char *why){if(!value)throw FormatError(why);}
int main(int argc,char **argv)try {
 require(argc==3,"Supply original TU1.5 XBE and New Mombasa map");
 MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];
 options.shared_maps=options.map.parent_path();options.bsp=0;options.local_players=1;
 options.campaign_difficulty=1;options.source_object_world=true;
 const auto original=OriginalData::load_xbe(argv[1]);Cache input(options.map,options.shared_maps,original);
 const auto content=input.load_content(0,false);
 const auto vehicle=std::find_if(content.objects.begin(),content.objects.end(),[](const auto &o){return o.type==1 && o.placement==3 && !o.model_attachment;});
 require(vehicle!=content.objects.end(),"Actual Pelican observer placement missing");
 const auto starts=test::campaign_vehicle_observer_starts<1>(input,content,0,*vehicle);
 options.native_spawn_inputs.assign(starts.begin(),starts.end());
 options.campaign_spawn_input=[starts](unsigned slot,bool initial)->std::optional<Spawn>{if(initial && slot<starts.size())return starts[slot];return {};};
 MultiplayerPreparedWorld world(options,ScriptProgram{});const auto &cache=*world.cache;
 auto objects=world.content.objects;std::set<std::size_t> selected;
 for(const auto &live:world.match.simulation().scene().objects())if(live.handle!=null_datum) {
  const auto &authored=objects.at(live.placement);
  if(authored.type==6 && cache.load_initial_model_definition(authored.definition).render_model!=null_datum &&
      !cache.load_initial_model_selection(authored.definition,authored.variant).resolved())selected.insert(live.placement);
 }
 require(selected.size()==9,"Actual New Mombasa RNG scenery census changed");
 for(std::size_t n=0;n<objects.size();++n)if(!selected.contains(n))objects[n].definition=null_datum;
 CollisionWorld collision(world.content.collision);Mesh mesh;
 SourceSceneryScene scene(cache,objects,0,mesh,collision,{}, {},true);
 require(scene.stats().render_models==selected.size() && !scene.stats().unresolved_selections,
     "Source union omitted actual RNG-selected owners");
 auto &live_scene=world.match.simulation().scene();
 const SourceSceneryScene::LifetimeSelection regions=[&](std::size_t index,std::uint32_t handle,std::uint64_t born) {
  const auto &live=live_scene.get(handle);
  require(live.placement==index && live.created_tick==born,"Source region selector crossed a lifetime");
  return world.source_objects->object_model_regions(handle).selection;
 };
 // Exercise the complete authored catalog against the actual source world,
 // including other-BSP cinematic resources. No failed visible owner may pass.
 Mesh full_mesh;SourceSceneryScene full(cache,world.content.objects,0,full_mesh,collision,{}, {},true);
 full.synchronize(live_scene,regions);
 for(const auto &boundary:full.deferred_resource_owners()) {
  const auto live=std::find_if(live_scene.objects().begin(),live_scene.objects().end(),
      [&](const auto &o){return o.handle!=null_datum && o.placement==boundary.source_object;});
  require(live==live_scene.objects().end() || !live->visible,"Resource deferral omitted a visible source owner");
  // A declared presentation request must surface the retained error.
  // This isolated visibility fixture supplies no physical state or output.
  auto requested_content=world.content;
  for(std::size_t n=0;n<requested_content.objects.size();++n)
   if(n!=boundary.source_object)requested_content.objects[n].definition=null_datum;
  SceneObjects requested(requested_content,0);
  std::vector<std::uint32_t> old;
  for(const auto &o:requested.objects())if(o.handle!=null_datum)old.push_back(o.handle);
  for(auto handle:old)requested.destroy(handle);
  requested.create_placement(boundary.source_object);
  bool rejected=false;
  try {full.synchronize(requested);}
  catch(const FormatError &e) {rejected=std::string(e.what())=="Visible SCEN placement "+
      std::to_string(boundary.source_object)+" requires source resources: "+boundary.error;}
  require(rejected,"A visible request escaped its retained source resource boundary");
 }
 scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform&,float) {
  require(live_scene.get(handle).placement==index,"Scenery lighting crossed a source owner");
  const auto sphere=world.source_objects->object_sphere(handle);
  const auto body=world.source_objects->source_object_body(handle);
  const Bytes data(world.source_objects->memory().view(body,0xc2));
  return SourceSceneryQueryState{sphere.center,sphere.radius,world.source_objects->object_linear_velocity(handle),
      data.u32(4),(std::to_integer<unsigned>(data.slice(0xc1,1)[0])&1u)!=0};
 });
 Mesh prop_mesh;SourcePropScene props(cache,world.content.objects,0,prop_mesh,collision,true);
 std::optional<SourceInitialModelSelection> prop_regions;
 props.region_state([&](std::uint32_t handle) {
  require(world.content.objects.at(live_scene.get(handle).placement).type==11,"Prop selector crossed a full source owner");
  prop_regions=world.source_objects->object_model_regions(handle).selection;return &*prop_regions;
 });
 auto source_state=world.match.state();
 props.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform&,float) {
  require(live_scene.get(handle).placement==index,"Prop query crossed its full source lifetime");
  const auto root=std::find_if(source_state.source_scene_roots.begin(),source_state.source_scene_roots.end(),
      [&](const auto &o){return o.object==handle;});
  const auto pose=std::find_if(source_state.source_body_poses.begin(),source_state.source_body_poses.end(),
      [&](const auto &o){return o.object==handle;});
  require(root!=source_state.source_scene_roots.end() && pose!=source_state.source_body_poses.end(),
      "Prop query has no completed actual source bounds bank");
  return SourcePropQueryState{{pose->sphere.center,pose->sphere.radius},root->velocity,root->runtime_flags,root->frozen};
 });
 unsigned prop_draws{},prop_vertices{};
 const auto verify_props=[&] {
  source_state=world.match.state();props.synchronize(live_scene);
  props.update_pose(std::int32_t(world.match.simulation().ticks()),prop_mesh);
  for(unsigned lod:{0u,5u}) {
   std::vector<Vertex> output;std::vector<ObjectLightingBinding> lighting;
   std::vector<SourceObjectViewSelection> choices;
   SourceObjectViewLod view{0,1,false,[&](std::size_t,std::uint32_t,Vec3,float) {
       return SourceObjectLodInput{lod?0.f:100000.f,lod?0x80000u:0u,0};}};
   props.append_view(output,lighting,view,&choices);std::size_t at=0;
   for(const auto &choice:choices) {
    require(choice.lod==lod,"Prop source geometry chose a different authored LOD");
    const auto live=std::find_if(live_scene.objects().begin(),live_scene.objects().end(),
        [&](const auto &o){return o.handle!=null_datum && o.placement==choice.source_object;});
    require(live!=live_scene.objects().end() && live->solved_nodes,"Prop draw has no actual WORLD owner");
    const auto &authored=world.content.objects.at(choice.source_object);
    const auto actual=world.source_objects->object_model_regions(live->handle).selection;
    const auto model=cache.load_model(authored.definition,false,authored.variant,actual,lod);
    const auto expected=skin_model_world_nodes(model,*live->solved_nodes);
    require(expected.size()==choice.vertices,"Prop resource union emitted an unselected collector");
    for(const auto &vertex:expected) {
     const auto &draw=output.at(at++);
     require(draw.source_section==vertex.source_section && draw.source_part==vertex.source_part &&
         !std::memcmp(&draw.position,&vertex.position,sizeof(Vec3)) && draw.material<prop_mesh.materials.size(),
         "Prop source union changed actual WORLD geometry, collector order or material residency");
    }
    ++prop_draws;prop_vertices+=unsigned(expected.size());
   }
   require(at==output.size(),"Prop source union emitted an extra collector");
  }
 };
 verify_props();
 std::vector<std::uint32_t> projectiles;unsigned borrowed_attachments{};
 for(const auto &[datum,weapon]:world.content.weapons) {
  if(weapon.projectile!=null_datum)projectiles.push_back(weapon.projectile);
  for(const auto &barrel:weapon.barrel_views)if(barrel->projectile!=null_datum)projectiles.push_back(barrel->projectile);
 }
 for(auto definition:projectiles) {
  const auto tag=cache.load_source_tag_metadata(definition);
  const Bytes data(tag.bytes_at(tag.source_address(),0x9c));
  if(data.u32(0x38)!=null_datum)continue;
  const auto descriptor=read_tag_block_descriptor(data,0x94);
  const auto extent=descriptor.extent(0x18,4095);
  if(!descriptor.count)continue;
  if(descriptor.address<tag.source_address() ||
      std::uint64_t(descriptor.address)+extent>std::uint64_t(tag.source_address())+tag.size()) {
   const auto owner=cache.query_tag({.contains=TagAddressRange{descriptor.address,extent},.require_address=true});
   require(bool(owner),"Original projectile attachment pointer has no actual cache owner");
   (void)cache.tag_payload(owner->handle());
   require(cache.read_tag_bytes(descriptor.address,extent).size()==extent,"Original borrowed attachment rows lost extent");
   ++borrowed_attachments;
  }
 }
 require(borrowed_attachments>0,"Campaign shared projectile attachment regression lost its actual external pointer case");
 Mesh projectile_mesh;NativeWeaponProjectileScene projectile_scene(cache,0,projectile_mesh,collision,
     projectiles,world.match.native_object_capacity());
 unsigned draws{},vertices{};
 const auto verify=[&] {
  scene.synchronize(live_scene,regions);scene.update_pose(world.match.simulation().ticks(),0,mesh);
  for(unsigned lod:{0u,5u}) {
   std::vector<Vertex> output;std::vector<ObjectLightingBinding> lighting;
   std::vector<SourceObjectViewSelection> choices;
   SourceObjectViewLod view{0,1,false,[&](std::size_t,std::uint32_t,Vec3,float) {
       return SourceObjectLodInput{lod?0.f:100000.f,lod?0x80000u:0u,0};}};
   scene.append_view(output,lighting,view,&choices);std::size_t at=0;
   require(choices.size()==selected.size(),"Actual source union lost a live scenery draw");
   for(const auto &choice:choices) {
    require(choice.lod==lod,"Source scenery selected a different authored LOD");
    const auto live=std::find_if(live_scene.objects().begin(),live_scene.objects().end(),
        [&](const auto &o){return o.handle!=null_datum && o.placement==choice.source_object;});
    require(live!=live_scene.objects().end() && live->solved_nodes,"Source scenery lost completed WORLD nodes");
    const auto &authored=objects.at(choice.source_object);const auto actual=regions(choice.source_object,live->handle,live->created_tick);
    const auto model=cache.load_model(authored.definition,false,authored.variant,actual,lod);
    const auto expected=skin_model_world_nodes(model,*live->solved_nodes);
    require(expected.size()==choice.vertices,"Source union emitted an unselected collector");
    for(const auto &vertex:expected) {
     const auto &draw=output.at(at++);
     require(draw.source_section==vertex.source_section && draw.source_part==vertex.source_part &&
         !std::memcmp(&draw.position,&vertex.position,sizeof(Vec3)) && draw.material<mesh.materials.size(),
         "Source union changed section order, WORLD positions or material residency");
    }
    ++draws;vertices+=unsigned(expected.size());
   }
   require(at==output.size(),"Source union emitted an extra collector");
  }
 };
 verify();const auto index=*selected.begin();const auto prior=std::find_if(live_scene.objects().begin(),live_scene.objects().end(),
     [&](const auto &o){return o.handle!=null_datum && o.placement==index;})->handle;
 world.match.destroy_world_object(prior);const auto fresh=world.match.create_world_object(index);
 require(fresh!=prior && fresh!=null_datum,"Original scenery recreation retained its retired generation");
 std::array<Controls,multiplayer_capacity> neutral{};world.match.step(neutral);
 verify();verify_props();
 std::cout<<"PASS actual TU1.5 scenery RNG unions: 9 owners, "<<draws<<" LOD draws, "<<vertices
     <<" exact WORLD vertices; original retirement/recreation retained; "<<full.deferred_resource_owners().size()
     <<" inactive resource boundaries retain visible-request rejection; "<<prop_draws<<" actual prop LOD draws, "
     <<prop_vertices<<" exact prop WORLD vertices; "<<borrowed_attachments<<" original borrowed projectile attachment banks\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
