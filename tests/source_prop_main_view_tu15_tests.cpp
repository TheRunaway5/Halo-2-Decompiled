#include "halo2/multiplayer_world.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/collision.hpp"
#include "halo2/scene.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>
using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F&& f,const char* why){bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}check(rejected,why);}
bool equal(std::span<const Vertex> a,std::span<const Vertex> b){return a.size()==b.size() && (a.empty() || !std::memcmp(a.data(),b.data(),a.size_bytes()));}
}
int main(int argc,char**argv)try {
  check(argc==4,"Supply TU15_XBE MAP SHARED_MAPS");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  auto directory=(std::filesystem::temp_directory_path()/"halo2-prop-main-view-XXXXXX").string();
  check(::mkdtemp(directory.data()),"Prop handoff scratch directory failed");
  struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{directory};
  MultiplayerOptions options;options.map=argv[2];options.shared_maps=argv[3];options.gameplay_xbe=argv[1];
  options.source_object_world=true;options.source_object_savegame_directory=directory;
  MultiplayerPreparedWorld prepared(options);prepared.match.step({});
  const auto& cache=*prepared.cache;auto& source=*prepared.source_objects;
  const auto& objects=prepared.match.simulation().scene();
  const auto chosen=std::find_if(objects.objects().begin(),objects.objects().end(),[&](const auto& object) {
    const auto& definition=prepared.content.objects.at(object.placement);
    if(definition.type!=11)return false;
    const auto tag=cache.query_tag({.handle=TagHandle{definition.definition}});
    return tag && tag->name=="scenarios\\objects\\multi\\lockout\\power_module\\power_module";
  });
  check(chosen!=objects.objects().end(),"Real automatic source power-module constructor did not create an owner");
  const auto object=chosen->handle;const auto definition=prepared.content.objects[chosen->placement].definition;
  const auto model=cache.load_initial_model_definition(definition).render_model;
  const auto nodes=source.world_nodes(object);const auto sphere=source.object_sphere(object);
  const CollisionWorld collision(prepared.content.collision);Mesh mesh;
  SourcePropScene scene(cache,prepared.content.objects,0,mesh,collision);scene.synchronize(objects);
  // The retained regular sphere is an explicit component input here; live
  // physics-override admission is covered by the real gameplay capture.
  scene.lighting_query_state([&](std::size_t,std::uint32_t owner,const Transform&,float) {
    const auto bound=source.object_sphere(owner);const auto body=source.object_body(owner);
    const auto flags=Bytes(source.memory().view(body+4u,4)).u32(0);
    const bool frozen=(std::to_integer<unsigned>(source.memory().view(body+0xc1u,1)[0])&1u)!=0;
    return SourcePropQueryState{{bound.center,bound.radius},source.object_linear_velocity(owner),flags,frozen};
  });
  scene.update_pose(1,mesh);
  const auto roots=scene.source_render_models();check(std::find(roots.begin(),roots.end(),model)!=roots.end(),"Prepared BLOC model root was lost");
  SourceLiveModelDrawContextTu15 context;context.entry.eye=sphere.center+Vec3{2,-2,1};
  context.forward=normalized(sphere.center-context.entry.eye);context.entry.distance_scale=1;
  context.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
  context.draw.eye=context.entry.eye;context.draw.viewport={0,0,480,640};
  SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
  projection.outer=projection.safe=context.draw.viewport;const auto projected=source_frustum_projection(projection);
  context.draw.rendering=source_view_projection({context.entry.eye,context.forward,{0,0,1},.015625f,1024.f,{},projected.projection_program});
  context.entry.coverage=source_shader_coverage_camera(context.draw.rendering);
  SourcePortalVisibilitySet visible;const auto count=cache.load_source_bsp_admission(0).clusters.size();
  std::vector<std::uint8_t> fog(count);
  // Declared all-cluster camera input exercises actual source registration;
  // this component check does not claim original portal traversal or pixels.
  const auto camera=source_frustum_camera({context.entry.eye,context.forward,{0,0,1},.015625f,1024.f,projected.clipping_rectangle()});
  visible.groups.push_back(camera);visible.frustums.push_back(camera.full_frustum);
  for(unsigned n=0;n<count;++n){SourceClusterFrustumRanges r;r.source_cluster=std::int16_t(n);r.counts[0]=1;visible.clusters.push_back(r);}
  const SourceModelShaderView shader{context.entry.coverage,source_shader_lod_view(0,480,false,false),1,1,false};
  SourceObjectViewLod view;view.shader=shader;view.detail_policy=source_geometry_lod_policy(1);
  view.input=[&](std::size_t,std::uint32_t owner,Vec3 center,float) {
    const auto flags=Bytes(source.memory().view(source.object_body(owner)+4u,4)).u32(0);
    return SourceObjectLodInput{source_geometry_lod_distance(center,context.entry.eye,1),flags,0};
  };
  std::vector<Vertex> before;std::vector<ObjectLightingBinding> bindings;scene.append_view(before,bindings,view);
  SourceLiveModelBaseSelectionTu15 selected;std::vector<SourceLiveModelBaseDrawTu15> base;
  context.built=[&](const auto& collector){selected.remember(collector);};
  source.begin_model_frame();source.render_model_view([&](PhysicsLiveWorldTu15& world) {
    SourceObjectAdmissionInput input;input.containing_fog=fog;SourceDrawUnitState units;
    auto admission=world.admit_render_objects(visible,input);
    source_live_model_draws_tu15(world,admission,units,context);
    base=selected.copy(world,units,[&](const auto& unit){return unit.words[1]==model;});
    std::erase_if(base,[&](const auto& draw){return draw.object!=object;});
  });
  check(!base.empty(),"Real native main view did not select the power module");
  auto stale=base;for(auto& draw:stale)draw.object^=0x10000u;
  scene.begin_source_frame();check(!scene.retain_source_view(0,stale,nodes,shader),"Stale full owner was accepted");
  auto wrong=base;wrong.front().definition^=0x10000u;
  rejects([&]{scene.retain_source_view(0,wrong,nodes,shader);},"Wrong full definition was accepted");
  auto mixed=base;mixed.push_back(base.front());mixed.back().override_slot=0;
  check(!scene.retain_source_view(0,mixed,nodes,shader),"Mixed unsupported owner was partially retained");
  std::vector<ModelObjectDraw> rendered;scene.append_source_models(rendered,view);
  check(rendered.empty(),"Rejected owner left hidden base geometry");
  check(scene.retain_source_view(0,base,nodes,shader),"Complete ordinary source owner was not retained");
  rejects([&]{scene.retain_source_view(0,base,nodes,shader);},"Duplicate source owner was retained");
  std::vector<Vertex> legacy;bindings.clear();
  rejects([&]{scene.append_view(legacy,bindings,view);},"Forgotten source base submission was hidden");
  scene.append_source_models(rendered,view);check(!rendered.empty(),"Retained source base produced no model draw");
  rejects([&]{scene.append_source_models(rendered,view);},"Source base was submitted twice");
  const std::vector<Vertex> retained(rendered.front().vertices.begin(),rendered.front().vertices.end());
  source.begin_model_frame();check(equal(retained,rendered.front().vertices),"Producer arena reset changed the retained base");
  legacy.clear();bindings.clear();std::vector<SourceObjectViewSelection> legacy_owners;
  scene.append_view(legacy,bindings,view,&legacy_owners);
  check(std::none_of(legacy_owners.begin(),legacy_owners.end(),[&](const auto& selected){return selected.source_object==chosen->placement;}),"Source owner was also submitted by legacy geometry");
  rendered.clear();scene.begin_source_frame();legacy.clear();bindings.clear();scene.append_view(legacy,bindings,view);
  check(equal(before,legacy),"Source WORLD section selection mutated the legacy pose cache");
  std::cout<<"PASS actual BLOC main selection, full identities, atomic rejection, owned lifetime, exactly-once submission and immutable legacy pose\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
