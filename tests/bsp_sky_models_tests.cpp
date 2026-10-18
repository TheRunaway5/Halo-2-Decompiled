#include "halo2/sky.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/appearance.hpp"
#include "halo2/scene.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <set>
#include <tuple>
using namespace halo2;
namespace {
void require(bool b,const char *why){if(!b)throw FormatError(why);}
template<class F>void rejects(F f,const char *why){bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}require(rejected,why);}
bool same(Vec3 a,Vec3 b){return std::bit_cast<std::array<std::uint32_t,3>>(a)==std::bit_cast<std::array<std::uint32_t,3>>(b);}
std::size_t check(const Cache &cache,const BspSkyModels &bank,const Mesh &destination,
                  const BspFogSample &view,Vec3 eye,std::size_t prefix) {
  const auto actual=bank.pose(view,eye);
  if(view.visible_sky<0){require(actual.empty(),"no-sky view retained a preceding backdrop");return 0;}
  const auto definition=cache.load_sky(unsigned(view.visible_sky));
  if(!definition){require(actual.empty(),"authored NONE/model-less sky acquired a replacement");return 0;}
  auto source=cache.load_model(definition->model);prepare_forward_materials(cache,source.mesh);
  const auto expected=pose_sky(source,*definition,eye);
  require(!expected.empty() && actual.size()==expected.size(),"admitted sky geometry was omitted");
  std::set<unsigned> used;
  for(unsigned i=0;i<actual.size();++i) {
    const auto &a=actual[i],&b=expected[i];
    require(same(a.position,b.position) && same(a.normal,b.normal) && same(a.tangent,b.tangent) &&
        same(a.binormal,b.binormal) && a.source_part==b.source_part && a.source_section==b.source_section,
        "selected original sky transform or authored part sequence changed");
    require(a.material>=prefix && a.material<destination.materials.size(),"sky escaped its current BSP material namespace");
    const auto &material=destination.materials[a.material],&original=source.mesh.materials.at(b.material);
    require(material.name==original.name && material.source_shader_datum==original.source_shader_datum &&
        material.passes.size()==original.passes.size() && material.forward_raster==original.forward_raster,
        "sky selected another authored shader/pass/raster binding");
    require(material.diffuse.pixels==original.diffuse.pixels,"sky texture pixels do not belong to selected material");
    used.insert(a.material);
  }
  require(!used.empty(),"sky has no resident material binding");
  return actual.size();
}
void opening(const std::filesystem::path &maps) {
  Cache cache(maps/"00a_introduction.map");const auto atmosphere=cache.load_bsp_atmosphere(0);
  // Before this integration the production caller caught this exception and
  // rendered an empty background for every current camera in the BSP.
  rejects([&]{cache.load_visible_sky(0);},"opening no longer exercises multi-sky omission regression");
  Mesh mesh;mesh.materials.resize(7);BspSkyModels bank(cache,atmosphere,mesh);
  // Literal real normal-main TU1.5 halo-main-draw-v1 caller camera at tick1770.
  // Capture bank SHA 532d1ba370dadd372a8f3bca948ccdb6310e0fa1bc0b249027aa926758477579.
  // This asserts source draw-camera/map selection; matched framebuffer pixels are a separate gate.
  const Vec3 eye{193.7972869873047f,320.6581726074219f,53.08562469482422f};
  const Vec3 forward{-.8953210115432739f,.44136491417884827f,-.05997834727168083f};
  const auto actual_camera=atmosphere.sample(eye,forward);
  require(actual_camera.cluster==27 && actual_camera.visible_sky==10,"original Halo camera selected another BSP sky");
  require(check(cache,bank,mesh,actual_camera,eye,7)>0,"original Halo camera still has a black omitted backdrop");
  // Actual normal-main fleet draw, tick1558/frame32800. The original main
  // caller carries cluster27 in EDX and visible-sky10 as arg10 (14DE90 output).
  const Vec3 fleet_eye{349.0115661621094f,340.0783386230469f,52.2694091796875f};
  const Vec3 fleet_forward{.5321619510650635f,.8273915648460388f,.17951834201812744f};
  const auto fleet=atmosphere.sample(fleet_eye,fleet_forward);
  require(fleet.cluster==27 && fleet.visible_sky==10,"original fleet draw camera selected another sky");
  check(cache,bank,mesh,fleet,fleet_eye,7);
  // Decode each target camera from its authored graph/anchor, then use the
  // production camera sampler. This does not infer a source game/render join.
  const auto content=cache.load_content(0,false);
  Presentation presentation(content,[&](auto graph,auto name){return cache.load_named_animation(graph,name);});
  presentation.control(true,{});
  for(const auto &[name,sid,elapsed]:std::array{
      std::tuple{"fleet",0x06001c9eu,65},std::tuple{"intact-Halo",0x06001c9fu,93},
      std::tuple{"destruction",0x06001ca2u,111}}) {
    presentation.camera_animation(0xe9d90864u,sid,2);
    for(int tick=0;tick<elapsed;++tick)presentation.tick();
    const auto camera=presentation.camera({},0);
    const auto view=atmosphere.sample(camera.position,camera.forward);
    require(view.cluster==27 && view.visible_sky==10,"authored cinematic camera selected another original sky");
    check(cache,bank,mesh,view,camera.position,7);
    std::cout<<"authored "<<name<<" elapsed="<<elapsed<<" cluster="<<view.cluster<<" sky="<<view.visible_sky<<'\n';
  }
  // The same live BSP also has a different, real visible-sky ordinal15.
  const auto chamber=atmosphere.sample(eye,forward,45);
  require(chamber.visible_sky==15 && chamber.visible_sky!=actual_camera.visible_sky,"opening fixture lost independent per-view selection");
  check(cache,bank,mesh,chamber,eye,7);
  const auto repeated=bank.pose(actual_camera,eye);
  require(repeated.size()==bank.pose(actual_camera,eye).size(),"returning camera lost its sky");
  auto none=actual_camera;none.visible_sky=-1;none.clear=Vec3{.125f,.25f,.5f};
  check(cache,bank,mesh,none,eye,7);
  require(none.clear && same(*none.clear,{.125f,.25f,.5f}),"sky resource selection changed view clear policy");
  auto invalid=none;invalid.visible_sky=int(atmosphere.skies.size());
  rejects([&]{bank.pose(invalid,eye);},"invalid visible-sky ordinal accepted");
  invalid.visible_sky=7;
  rejects([&]{bank.pose(invalid,eye);},"unprepared unrelated scenario sky accepted");
  // Slot7 is an actual NONE reference in the original introduction scenario.
  auto empty_atmosphere=atmosphere;empty_atmosphere.scenario_flags=0;
  for(auto &cluster:empty_atmosphere.clusters)cluster.visible_sky=7;
  Mesh empty_mesh;empty_mesh.materials.resize(3);BspSkyModels empty_bank(cache,empty_atmosphere,empty_mesh);
  none.visible_sky=7;check(cache,empty_bank,empty_mesh,none,eye,3);
  require(empty_mesh.materials.size()==3,"NONE sky appended replacement materials");
  auto malformed=empty_atmosphere;malformed.clusters.front().visible_sky=999;
  rejects([&]{BspSkyModels bad(cache,malformed,empty_mesh);},"invalid authored BSP sky accepted");
  require(empty_mesh.materials.size()==3,"failed sky admission changed destination materials");
  std::cout<<"PASS actual TU1.5 Halo camera: cluster27/sky10, independent sky15, no-sky/NONE, invalid admission and original bindings\n";
}
void bsp_transitions(const std::filesystem::path &maps) {
  Cache cache(maps/"01b_spacestation.map");std::optional<BspSkyModels> bank;Mesh mesh;
  unsigned visits=0;
  for(const auto [bsp,prefix]:std::array{std::array{0u,11u},std::array{5u,31u},std::array{0u,4u}}) {
    bank.reset();mesh=Mesh{};mesh.materials.resize(prefix);
    const auto atmosphere=cache.load_bsp_atmosphere(bsp);bank.emplace(cache,atmosphere,mesh);
    std::set<int> skies;
    for(unsigned cluster=0;cluster<atmosphere.clusters.size();++cluster) {
      const auto sample=atmosphere.sample({},{1,0,0},int(cluster));
      if(skies.insert(sample.visible_sky).second)check(cache,*bank,mesh,sample,{},prefix);
    }
    require(skies.contains(0)==(bsp==0),"BSP fixture no longer changes authored sky index bank");
    ++visits;
  }
  require(visits==3,"BSP retirement/re-entry fixture incomplete");
  Cache single(maps/"zanzibar.map");const auto atmosphere=single.load_bsp_atmosphere(0);
  Mesh destination;destination.materials.resize(2);BspSkyModels single_bank(single,atmosphere,destination);
  const auto authored=single.load_visible_sky(0);require(bool(authored),"single-sky compatibility fixture missing");
  const auto sample=atmosphere.sample({},{1,0,0},0);require(sample.visible_sky==0,"single-sky fixture ordinal differs");
  check(single,single_bank,destination,sample,{},2);
  std::cout<<"PASS actual Cairo BSP0 -> BSP5 -> BSP0 retirement/material namespaces and Zanzibar single-sky compatibility\n";
}
}
int main(int argc,char **argv)try {
  require(argc==2,"Original maps directory required");opening(argv[1]);bsp_transitions(argv[1]);
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
