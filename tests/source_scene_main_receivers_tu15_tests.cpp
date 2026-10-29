#include "halo2/multiplayer_world.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/renderer.hpp"
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
using Json=nlohmann::json;Json results=Json::array();
void need(bool value,const char* why){if(!value)throw FormatError(why);}
template<class F>void reject(F&& fn){try{fn();}catch(const FormatError&){return;}throw FormatError("Foreign native scene receiver was admitted");}
template<class Scene>bool verify(Scene& scene,MultiplayerPreparedWorld& prepared,Mesh& mesh,unsigned type) {
 auto& source=*prepared.source_objects;const auto& cache=*prepared.cache;const auto& objects=prepared.match.simulation().scene();
 const auto roots=scene.source_render_models();
 const auto chosen=std::find_if(objects.objects().begin(),objects.objects().end(),[&](const auto& object){
  if(object.placement>=prepared.content.objects.size() || prepared.content.objects[object.placement].type!=type)return false;
  const auto model=cache.load_initial_model_definition(prepared.content.objects[object.placement].definition).render_model;
  return std::find(roots.begin(),roots.end(),model)!=roots.end();
 });
 if(chosen==objects.objects().end())return false;
 const auto object=chosen->handle;const auto definition=prepared.content.objects[chosen->placement].definition;
 const auto model=cache.load_initial_model_definition(definition).render_model;
 const auto nodes=source.world_nodes(object);const auto sphere=source.object_sphere(object);
 SourceLiveModelDrawContextTu15 context;context.entry.eye=sphere.center+Vec3{2,-2,1};context.forward=normalized(sphere.center-context.entry.eye);
 context.entry.distance_scale=1;context.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
 context.draw.eye=context.entry.eye;context.draw.viewport={0,0,480,640};
 // The actual shared draw-unit arena below owns this logical source token,
 // as in SourceBspEffectLights main collection. It is never a GPU pointer.
 constexpr std::uint32_t main_mask_arena=0x71000000u;
 context.draw.draw.mask_arena_address=main_mask_arena;
 SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;projection.outer=projection.safe=context.draw.viewport;
 const auto projected=source_frustum_projection(projection);context.draw.rendering=source_view_projection({context.entry.eye,context.forward,{0,0,1},.015625f,1024.f,{},projected.projection_program});
 context.entry.coverage=source_shader_coverage_camera(context.draw.rendering);
 const auto camera=source_frustum_camera({context.entry.eye,context.forward,{0,0,1},.015625f,1024.f,projected.clipping_rectangle()});
 SourcePortalVisibilitySet visible;visible.groups.push_back(camera);visible.frustums.push_back(camera.full_frustum);
 const auto count=cache.load_source_bsp_admission(0).clusters.size();std::vector<std::uint8_t> fog(count);
 for(unsigned n=0;n<count;++n){SourceClusterFrustumRanges row;row.source_cluster=std::int16_t(n);row.counts[0]=1;visible.clusters.push_back(row);}
 SourceLiveModelBaseSelectionTu15 selected;std::vector<SourceLiveModelBaseDrawTu15> draws;unsigned masked_units=0;
 context.built=[&](const auto& collector){selected.remember(collector);};
 source.begin_model_frame();source.render_model_view([&](auto& world){SourceObjectAdmissionInput input;input.containing_fog=fog;SourceDrawUnitState units;
  source_bsp_range_arena_begin(units.arena);source_draw_units_begin(units,0);
  auto admission=world.admit_render_objects(visible,input);source_live_model_draws_tu15(world,admission,units,context);
  for(unsigned n=0;n<units.draw_count;++n)if(units.units[n].words[0]&0x40u) {
   const auto pointer=units.units[n].words[5];
   need(pointer>=main_mask_arena&&((pointer-main_mask_arena)&3u)==0&&
       (pointer-main_mask_arena)/4u<units.arena.words.size(),"Native main draw mask lost its actual shared arena backing");
   ++masked_units;
  }
  draws=selected.copy(world,units,[&](const auto& unit){return unit.words[1]==model;});std::erase_if(draws,[&](const auto& draw){return draw.object!=object;});
 // Retain/consume the completed native PRT cache before any later view can
 // run begin_model_view and retire it. This is the production callback life.
 need(!draws.empty(),"Genuine native main collector omitted the chosen scene receiver");
 need(masked_units>0,"Scene receiver fixture must exercise actual subpart mask ownership");
 const SourceModelShaderView shader{context.entry.coverage,source_shader_lod_view(0,480,false,false),1,1,false};
 const auto retain=[&](const auto& rows){if constexpr(std::is_same_v<Scene,SourceMachineScene>)return scene.retain_source_view(0,rows,nodes,shader,world);else return scene.retain_source_view(0,rows,nodes,shader);};
 scene.begin_source_frame();auto wrong=draws;wrong.front().definition^=65536u;reject([&]{retain(wrong);});
 auto stale=draws;for(auto& draw:stale)draw.object^=65536u;need(!retain(stale),"Stale scene lifetime was retained");
 auto mixed=draws;mixed.back().override_slot=0;need(!retain(mixed),"Unsupported scene override was partly retained");
 need(retain(draws),"Genuine completed main scene receiver was not retained");reject([&]{retain(draws);});
 const auto saved=scene.checkpoint();SourceObjectViewLod view;view.shader=shader;view.detail_policy=source_geometry_lod_policy(1);
 view.source_bound=[&](auto owner){return source.object_sphere(owner);};
 view.input=[&](auto,auto owner,Vec3 center,float){need(owner==object || source.object_definition(owner)!=~0u,"Scene input has no native owner");return SourceObjectLodInput{source_geometry_lod_distance(center,context.entry.eye,1),0,0};};
 std::vector<ModelObjectDraw> output;scene.append_source_models(output,view);need(!output.empty(),"Retained scene receiver produced no real model bank");
 std::size_t vertices=0;for(const auto& draw:output){need(draw.source_model_selection.has_value(),"Scene receiver lost original ordinal metadata");
  const auto& owner=*draw.source_model_selection;need(owner.object==object&&owner.definition==definition&&owner.render_model==model&&owner.source_vertices.size()==draw.vertices.size(),"Scene receiver changed its native owner or extent");
  for(unsigned n=0;n<draw.vertices.size();++n){const auto& origin=owner.model->vertex_origins.at(owner.source_vertices[n]);need(origin.section==draw.vertices[n].source_section&&origin.part==draw.vertices[n].source_part,"Scene receiver changed its original part/vertex identity");}vertices+=draw.vertices.size();}
 reject([&]{scene.append_source_models(output,view);});
 auto restore=scene.prepare_restore(saved);need(scene.restore_ready(restore)&&scene.commit_restore(restore),"Native scene source-view checkpoint did not restore atomically");
 // The consumed token retains borrowed old banks until both draw sets retire.
 std::vector<ModelObjectDraw> restored;scene.append_source_models(restored,view);need(restored.size()==output.size(),"Restored scene source selection changed section count");
 for(unsigned n=0;n<output.size();++n)need(output[n].vertices.size()==restored[n].vertices.size()&&!std::memcmp(output[n].vertices.data(),restored[n].vertices.data(),output[n].vertices.size_bytes()),"Restored native scene receiver changed complete WORLD vertices");
 restored.clear();output.clear();scene.begin_source_frame();
 results.push_back({{"type",type},{"object",object},{"definition",definition},{"model",model},{"main_units",draws.size()},{"masked_main_units",masked_units},{"vertices",vertices},{"negative_controls",4},{"checkpoint_exact",true}});
 });
 return true;
}
}
int main(int argc,char** argv)try{
 need(argc==5,"Supply actual TU15 XBE, original map, shared maps and report");
 if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
 const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch&&*scratch,"Scene main receiver test needs owned scratch");
 MultiplayerOptions options;options.map=argv[2];options.shared_maps=argv[3];options.gameplay_xbe=argv[1];options.source_object_world=true;options.source_object_savegame_directory=std::filesystem::path(scratch)/"scene-native-world";
 MultiplayerPreparedWorld prepared(options);prepared.match.step({});auto& cache=*prepared.cache;auto& source=*prepared.source_objects;
 need(source.owns_physics(),"Scene receiver fixture requires its genuine native source owner");
 const auto& objects=prepared.match.simulation().scene();
 // Use the same completed native WORLD endpoints that production synchronizes
 // into each scene. Do not run a second local controller or choose a default
 // region/permutation to make the collector's selection fit a resident model.
 unsigned solved_owners=0;
 for(const auto& object:objects.objects())if(object.handle!=null_datum) {
  const auto type=prepared.content.objects.at(object.placement).type;if(type!=6&&type!=7)continue;
  need(object.solved_transform.has_value()&&object.solved_nodes.has_value(),"Native scene receiver has no completed solved WORLD endpoint");
  const auto nodes=source.world_nodes(object.handle);
  need(nodes.size()==object.solved_nodes->size()&&(nodes.empty()||!std::memcmp(nodes.data(),object.solved_nodes->data(),nodes.size()*sizeof(Transform))),"Scene receiver solved WORLD endpoint differs from its actual native source");
  ++solved_owners;
 }
 CollisionWorld collision(prepared.content.collision);Mesh mesh;
 SourceMachineScene machines(cache,prepared.content.objects,0,mesh,collision,true);
 machines.synchronize(objects,&prepared.match.simulation().devices());
 machines.update_pose(prepared.match.state().devices.tick,0,mesh);
 const SourceSceneryScene::LifetimeSelection regions=[&](std::size_t index,std::uint32_t handle,std::uint64_t born) {
  const auto& object=objects.get(handle);
  need(object.placement==index&&object.created_tick==born,"SCEN native regions lost their actual full lifetime");
  return source.object_model_regions(handle).selection;
 };
 SourceSceneryScene scenery(cache,prepared.content.objects,0,mesh,collision,{}, {},true);
 // These are the current native cached sphere, velocity and query flags, not
 // a test-created frozen/default lighting state. The selected source WORLD
 // bank owns animation; invoking a separate clock would be a fixture defect.
 scenery.lighting_query_state([&](std::size_t index,std::uint32_t owner,const Transform&,float) {
  need(objects.get(owner).placement==index,"SCEN native query lost its actual source placement");
  const auto sphere=source.object_sphere(owner);const auto body=source.object_body(owner);
  const auto flags=Bytes(source.memory().view(body+4u,4)).u32(0);
  const bool frozen=(std::to_integer<unsigned>(source.memory().view(body+0xc1u,1)[0])&1u)!=0;
  return SourceSceneryQueryState{sphere.center,sphere.radius,source.object_linear_velocity(owner),flags,frozen};
 });
 scenery.animation_frames([](auto,auto,auto,auto,const AnimationClip&)->AnimationFrameWindow {
  throw FormatError("Native SCEN source receiver must retain its actual solved WORLD bank");
 });
 scenery.synchronize(objects,regions);scenery.update_pose(prepared.match.state().tick,0,mesh);
 const bool machine=verify(machines,prepared,mesh,7),scen=verify(scenery,prepared,mesh,6);
 need(machine&&scen,"Fixture must exercise actual supported MACH and SCEN native main receivers");
 std::ofstream out(argv[4]);out<<Json{{"passed",true},{"map",cache.name()},{"cases",results},{"native_solved_scene_owners",solved_owners},{"source_region_union",true},{"scope","Actual native main model collection, native lifetime region selection and solved WORLD endpoints with declared all-cluster camera service; exact retained geometry/ordinal/checkpoint owners. No original portal visibility or framebuffer claim."}}.dump(2)<<'\n';need(bool(out),"Cannot retain scene receiver report");
 std::cout<<"PASS actual MACH/SCEN main receivers: "<<results.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
