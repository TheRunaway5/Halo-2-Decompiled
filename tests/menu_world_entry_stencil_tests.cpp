#include "halo2/menu_world_entry_stencil.hpp"
#include "halo2/menu_world_raster.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

using namespace halo2;
namespace {
constexpr std::uint32_t owner=0xe1000001u;
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
template<class Function>void rejects(Function function,unsigned &count) {
  bool caught=false;try{function();}catch(const FormatError &){caught=true;}
  require(caught,"unowned/unsupported source entry stencil context was admitted");++count;
}
MenuWorldRasterSnapshotInput raster_input() {
  return {MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,
      std::array<std::int32_t,2>{1,1},true};
}
unsigned integration() {
  // This uses the actual admitted queues, rather than assigning a default
  // stencil ref from a material or title-camera capture. Spatial membership
  // and the sky-map availability are explicit bounded caller inputs.
  SourcePortalVisibilitySet visible;visible.groups.resize(1);
  SourceClusterFrustumRanges ranges;ranges.source_cluster=0;
  ranges.counts[0]=1;visible.clusters.push_back(ranges);
  SourceCameraFrustum frustum;frustum.bounds={-10,10,-10,10,-10,10};
  frustum.planes={ShaderVector{1,0,0,10},ShaderVector{-1,0,0,10},
      ShaderVector{0,1,0,10},ShaderVector{0,-1,0,10},
      ShaderVector{0,0,1,10},ShaderVector{0,0,-1,10}};
  visible.frustums.push_back(frustum);
  const std::array<std::uint8_t,2> fog{0,0};
  const SourceObjectAdmissionObject object{owner,0,6,{{0,0,0},1}};
  ObjectLightSpatialLists collidable(2),noncollidable(2);
  const std::array<std::int16_t,2> memberships{0,1};
  collidable.insert(owner,{6,0,{0,0,0},1},memberships);
  SourceObjectAdmissionInput in;in.containing_fog=fog;
  const auto normal=source_object_admission(visible,collidable,noncollidable,
      std::span(&object,1),in);
  require(normal.objects.size()==1 && normal.objects[0].flags==0xf800,
      "actual ordinary queue differs from unchanged133050 input case");
  require(!menu_world_object_entry_stencil_marked(normal,owner,{}),
      "ordinary queue4000 visibility-side bit was mistaken for entry4000");
  in.sky_cluster=1;
  const auto sky=source_object_admission(visible,collidable,noncollidable,
      std::span(&object,1),in);
  require(sky.objects.size()==1 && sky.objects[0].flags==0xfa00,
      "actual sky revisit differs from unchanged133050 normal-to-skip case");
  const bool marked=menu_world_object_entry_stencil_marked(sky,owner,{});
  require(marked,"actual sky bypass200 was not propagated to entry4000");
  MenuWorldRasterContext raster;raster.viewport_reset({640,480});raster.game_initialize_offset();
  raster.layer_begin(MenuWorldRasterLayer::base_3);raster.entry_begin({marked,false,false});
  require(raster.snapshot(raster_input()).stencil.values->reference==255,
      "queue-owned stencil predicate was not consumed by the raster context");
  raster.entry_end();
  const std::array child{MenuWorldObjectCollectorCopy::attached_child};
  require(menu_world_object_entry_stencil_marked(sky,owner,child),
      "attached child lost its inherited source stencil predicate");
  const std::array effect{MenuWorldObjectCollectorCopy::recursive_effect_80};
  const bool effect_marked=menu_world_object_entry_stencil_marked(sky,owner,effect);
  require(!effect_marked,"recursive source effect retained replaced+1AC");
  raster.entry_begin({effect_marked,false,false});
  require(raster.snapshot(raster_input()).stencil.values->reference==0,
      "recursive effect predicate did not restore the ordinary raster mode");
  raster.entry_end();
  SourceBspAdmissionBsp bsp;bsp.definitions.push_back({1});
  bsp.instances.push_back({0,{0,0,0},1});
  SourceBspAdmissionCluster cluster;cluster.geometry_count=1;
  cluster.bounds=frustum.bounds;cluster.instances.push_back(0);bsp.clusters.push_back(cluster);
  SourceBspAdmissionInput bsp_input;bsp_input.containing_fog=std::span(fog).first(1);
  const auto bsp_queue=source_bsp_admission(bsp,visible,bsp_input);
  require(bsp_queue.clusters.size()==1 && bsp_queue.instances.size()==1,
      "source BSP test did not admit the supplied owners");
  require(!menu_world_bsp_entry_stencil_marked(bsp_queue,0) &&
      !menu_world_bsp_entry_stencil_marked(bsp_queue,-2),
      "ordinary BSP queue-side flags invented a stencil override");
  return 6;
}
unsigned controls() {
  unsigned count{};SourceObjectAdmission empty;
  rejects([&]{(void)menu_world_object_entry_stencil_marked(empty,owner,{});},count);
  rejects([&]{(void)menu_world_object_entry_stencil_marked(empty,0xffffffffu,{});},count);
  SourceObjectAdmission objects;objects.objects.push_back({owner,0x200,0xffff,0});
  rejects([&]{(void)menu_world_object_entry_stencil_marked(objects,owner^0x10000u,{});},count);
  for(auto code:{MenuWorldObjectCollectorCopy::unspecified,static_cast<MenuWorldObjectCollectorCopy>(99)}) {
    const std::array path{code};
    rejects([&]{(void)menu_world_object_entry_stencil_marked(objects,owner,path);},count);
  }
  auto duplicate=objects;duplicate.objects.push_back(duplicate.objects[0]);
  rejects([&]{(void)menu_world_object_entry_stencil_marked(duplicate,owner,{});},count);
  auto reused=objects;reused.objects.push_back({owner^0x10000u,0,0xffff,0});
  rejects([&]{(void)menu_world_object_entry_stencil_marked(reused,owner,{});},count);
  SourceBspAdmission bsp;
  for(int encoding:{-1,0,-2,512,-32770,std::numeric_limits<int>::min()})
    rejects([&]{(void)menu_world_bsp_entry_stencil_marked(bsp,encoding);},count);
  bsp.clusters={{0,0,0xffff,0},{0,0x200,0xffff,0}};
  bsp.instances={{0,0,0xffff,0},{0,0x200,0xffff,0}};
  rejects([&]{(void)menu_world_bsp_entry_stencil_marked(bsp,0);},count);
  rejects([&]{(void)menu_world_bsp_entry_stencil_marked(bsp,-2);},count);
  // The old full-word adapter is explicit and independent: arbitrary true
  // predicates are not inferred from an unknown or unrelated flag word.
  for(unsigned bit=0;bit<32;++bit) {
    const auto full=menu_world_raster_entry_from_source_flags(1u<<bit,false,false);
    require(full.stencil_marked_4000==std::optional<bool>(bit==14),
        "complete source entry conversion consumed the wrong flag domain");
  }
  const auto unknown=menu_world_raster_entry_from_source_flags(0x4000u,std::nullopt,false);
  MenuWorldRasterContext context;context.layer_begin(MenuWorldRasterLayer::base_3);
  rejects([&]{context.entry_begin(unknown);},count);
  return count;
}
}
int main(int argc,char **argv) {try {
  require(argc==2,"source entry stencil test requires its original-instruction fixture");
  std::ifstream file(argv[1]);require(bool(file),"missing source entry stencil fixture");
  std::string line;require(bool(std::getline(file,line)) && line=="H2_MENU_WORLD_ENTRY_STENCIL_V1",
      "invalid source entry stencil fixture header");
  unsigned object_count{},bsp_count{};
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='O') {
      unsigned flags,association,n,expected;row>>flags>>association>>n;
      require(flags<=65535 && association<=65535 && n<=2,"invalid source object stencil fixture");
      std::vector<MenuWorldObjectCollectorCopy> path;
      for(unsigned i=0;i<n;++i){unsigned code;row>>code;path.push_back(static_cast<MenuWorldObjectCollectorCopy>(code));}
      row>>expected;require(expected<=1,"invalid source stencil predicate fixture");
      SourceObjectAdmission queue;queue.objects.push_back({owner,std::uint16_t(flags),std::uint16_t(association),0});
      require(menu_world_object_entry_stencil_marked(queue,owner,path)==bool(expected),
          "ordinary object stencil differs from unchanged source collector/consumer");++object_count;
    } else if(kind=='B') {
      unsigned type,flags,expected;row>>type>>flags>>expected;
      require(type<=1 && flags<=65535 && expected<=1,"invalid source BSP stencil fixture");
      SourceBspAdmission queue;
      (type?queue.instances:queue.clusters).push_back({0,std::uint16_t(flags),0xffff,0});
      require(menu_world_bsp_entry_stencil_marked(queue,type?-2:0)==bool(expected),
          "ordinary BSP stencil differs from unchanged source flag producer");++bsp_count;
    } else throw FormatError("unknown source entry stencil fixture record");
    require(bool(row),"truncated source entry stencil fixture");std::string extra;
    require(!(row>>extra),"trailing source entry stencil fixture input");
  }
  require(file.eof() && object_count==2560 && bsp_count==256,
      "incomplete unchanged source entry stencil coverage");
  const auto integrated=integration(),rejected=controls();
  std::cout<<object_count<<" original object copy paths + "<<bsp_count
      <<" original BSP mappings exact; "<<integrated<<" queue/raster integration checks; "
      <<rejected<<" unowned/unknown controls rejected; no live visibility admission\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
