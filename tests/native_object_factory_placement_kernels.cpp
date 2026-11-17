// Oracle harness kernels for track "placement" (see the registry header).
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
#include <array>
#include <bit>
#include <vector>

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::PlacementFp fp(const NativeCampaignScriptBank& bank) {
  const auto& live=bank.floating_point();
  f::PlacementFp out;out.control_word=live.x87_control_word;out.mxcsr=live.mxcsr;return out;
}
const Register leaf_from_point("bsp3d_leaf_from_point",[](auto& world,auto& bank,const auto& a){
  return f::bsp3d_leaf_from_point(world,a.at(0),a.at(1),a.at(2),fp(bank).mxcsr);});
const Register location_point("location_from_point",[](auto& world,auto& bank,const auto& a){
  f::location_from_point(world,a.at(0),a.at(1),fp(bank).mxcsr);return 0u;});
const Register location_leaf("location_from_leaf",[](auto& world,auto&,const auto& a){
  f::location_from_leaf(world,a.at(0),a.at(1));return 0u;});
const Register object_location("object_get_location",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::object_get_location(world,a.at(0),a.at(1),fp(bank)));});
const Register disconnect("object_disconnect_from_map",[](auto& world,auto& bank,const auto& a){
  auto state=fp(bank);f::object_disconnect_from_map(world,a.at(0),state);return 0u;});
const Register defer("object_defer_physics",[](auto& world,auto& bank,const auto& a){
  auto state=fp(bank);f::object_defer_physics(world,a.at(0),state);return 0u;});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::PlacementFp live(const NativeCampaignScriptBank& bank) {
  const auto& state=bank.floating_point();
  f::PlacementFp out;out.control_word=state.x87_control_word;out.mxcsr=state.mxcsr;return out;
}
float real(std::uint32_t value){return std::bit_cast<float>(value);}
const Register axis("dominant_axis",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::dominant_axis(world,a.at(0),live(bank).mxcsr));});
const Register circle("circle_inside_portal",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::circle_inside_portal(world,a.at(0),a.at(1),real(a.at(2)),std::uint16_t(a.at(3)),live(bank).mxcsr));});
const Register portal("portal_intersects_sphere",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::portal_intersects_sphere(world,std::uint16_t(a.at(0)),a.at(1),a.at(2),real(a.at(3)),live(bank)));});
const Register flood("cluster_portal_flood",[](auto& world,auto& bank,const auto& a){
  const auto out=a.at(4);
  const f::ClusterSink sink=[&](std::uint32_t slot,std::uint16_t cluster) {
    const std::array bytes{std::byte(cluster),std::byte(cluster>>8)};world.write(out+slot*2u,bytes);
  };
  return f::cluster_portal_flood(world,std::uint16_t(a.at(0)),a.at(1),real(a.at(2)),a.at(3),sink,0,live(bank));});
void reconnect_clusters(SourcePhysicsContactMemory& world,const NativeCampaignScriptBank& bank,
    const std::vector<std::uint32_t>& a,std::uint32_t payload_address) {
  std::vector<std::byte> payload;
  if(payload_address){payload.resize(a.at(7));world.read(payload_address,payload);}
  f::ClusterSelection selection;selection.forced=a.at(0);
  f::ClusterLinks links;links.partition=a.at(1);links.owner=a.at(2);links.owner_head=a.at(3);links.center=a.at(4);
  links.radius=a.at(5);links.location=a.at(6);links.payload_size=a.at(7);links.payload=payload;links.overflow=a.at(6);
  f::cluster_partition_reconnect(world,selection,links,live(bank));
}
const Register reconnect_clusters_payload("cluster_partition_reconnect",[](auto& world,auto& bank,const auto& a){
  reconnect_clusters(world,bank,a,a.at(4));return 0u;});
const Register reconnect_clusters_bare("cluster_partition_reconnect_bare",[](auto& world,auto& bank,const auto& a){
  reconnect_clusters(world,bank,a,a.at(0));return 0u;});
const Register in_clusters("object_in_active_clusters",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::object_in_active_clusters(world,a.at(0),a.at(1)));});
const Register activate("object_activate",[](auto& world,auto& bank,const auto& a){
  auto state=live(bank);f::object_activate(world,a.at(0),state);return 0u;});
const Register deactivate("object_deactivate",[](auto& world,auto& bank,const auto& a){
  auto state=live(bank);f::object_deactivate(world,a.at(0),state);return 0u;});
const Register protected_delete("object_delete_protected",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::object_delete_protected(world,a.at(0)));});
const Register create("object_create_physics",[](auto& world,auto& bank,const auto& a){
  auto state=live(bank);f::object_create_physics(world,a.at(0),state);return 0u;});
const Register create_tree("object_create_physics_tree",[](auto& world,auto& bank,const auto& a){
  auto state=live(bank);f::object_create_physics_tree(world,a.at(0),a.at(1),state);return 0u;});
const Register islands("process_dirty_islands",[](auto& world,auto& bank,const auto&){
  auto state=live(bank);f::process_dirty_islands(world,state);return 0u;});
const Register reconnect("object_reconnect_to_map",[](auto& world,auto& bank,const auto& a){
  auto state=live(bank);f::object_reconnect_to_map(world,a.at(0),a.at(1),state);return 0u;});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::PlacementFp state_of(const NativeCampaignScriptBank& bank) {
  const auto& state=bank.floating_point();
  f::PlacementFp out;out.control_word=state.x87_control_word;out.mxcsr=state.mxcsr;return out;
}
f::PlacementVector at(std::uint32_t address){f::PlacementVector v;v.address=address;return v;}
const Register dirty("simulation_entity_dirty",[](auto& world,auto&,const auto& a){
  f::simulation_entity_dirty(world,a.at(0),a.at(1));return 0u;});
const Register physics_position("object_update_physics_position",[](auto& world,auto&,const auto& a){
  f::object_update_physics_position(world,a.at(0),a.at(1)&0xffu,a.at(2)&0xffu);return 0u;});
const Register biped_filter("biped_relocation_filter",[](auto& world,auto& bank,const auto& a){
  auto state=state_of(bank);f::biped_relocation_filter(world,a.at(0),at(a.at(2)),at(a.at(3)),state);return 0u;});
const Register type_relocation("object_type_relocation",[](auto& world,auto& bank,const auto& a){
  auto state=state_of(bank);f::object_type_relocation(world,a.at(0),at(a.at(1)),at(a.at(2)),at(a.at(3)),state);return 0u;});
// Eight argument words: rebuild_nodes is the position's low byte (0).
const Register relocate("object_relocate",[](auto& world,auto& bank,const auto& a){
  auto state=state_of(bank);f::RelocationOptions options;
  options.location=a.at(4);options.rebuild_nodes=a.at(1)&0xffu;options.update_physics=a.at(5)&0xffu;
  options.physics_force=a.at(6);options.keep_links=a.at(7)&0xffu;
  return std::uint32_t(f::object_relocate(world,a.at(0),at(a.at(1)),at(a.at(2)),at(a.at(3)),options,state));});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::PlacementFp node_state(const NativeCampaignScriptBank& bank) {
  const auto& state=bank.floating_point();
  f::PlacementFp out;out.control_word=state.x87_control_word;out.mxcsr=state.mxcsr;return out;
}
f::PlacementVector node_vector(std::uint32_t address){f::PlacementVector v;v.address=address;return v;}
// B9460 with rebuild_nodes set (BF050 through track nodebuild).
const Register relocate_nodes("object_relocate_nodes",[](auto& world,auto& bank,const auto& a){
  auto state=node_state(bank);f::RelocationOptions options;
  options.location=a.at(4);options.rebuild_nodes=true;options.update_physics=a.at(5)&0xffu;
  options.physics_force=a.at(6);options.keep_links=a.at(7)&0xffu;
  return std::uint32_t(f::object_relocate(world,a.at(0),node_vector(a.at(1)),node_vector(a.at(2)),
      node_vector(a.at(3)),options,state));});
const Register set_position("object_set_position",[](auto& world,auto& bank,const auto& a){
  auto state=node_state(bank);f::object_set_position(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),a.at(5),state);
  return 0u;});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
std::uint32_t marker_mxcsr(const NativeCampaignScriptBank& bank){return bank.floating_point().mxcsr;}
const Register attachment("object_attachment_marker",[](auto& world,auto&,const auto& a){
  return f::object_attachment_marker(world,a.at(0),std::uint16_t(a.at(1)));});
const Register root_location("object_root_location",[](auto& world,auto&,const auto& a){
  f::object_root_location(world,a.at(0),a.at(1));return 0u;});
const Register marker_owner("object_marker_owner",[](auto& world,auto&,const auto& a){
  return f::object_marker_owner(world,a.at(0));});
const Register marker_index("model_marker_index",[](auto& world,auto&,const auto& a){
  return f::model_marker_index(world,a.at(0),a.at(1));});
const Register rotation("quaternion_rotation",[](auto& world,auto& bank,const auto& a){
  f::quaternion_rotation(world,a.at(0),a.at(1),marker_mxcsr(bank));return 0u;});
// 1E1200: index, model, regions, nodes, mirrored, out, maximum (no node map).
const Register marker_matrices("model_marker_matrices",[](auto& world,auto& bank,const auto& a){
  return f::model_marker_matrices(world,a.at(0),a.at(1),a.at(2),a.at(7),a.at(3),a.at(4)&0xffu,a.at(5),a.at(6),marker_mxcsr(bank));});
const Register object_markers("object_marker_matrices",[](auto& world,auto& bank,const auto& a){
  return f::object_marker_matrices(world,a.at(0),a.at(1),std::uint16_t(a.at(2)),a.at(3)&0xffu,a.at(4),marker_mxcsr(bank));});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
const Register light("light_reconnect",[](auto& world,auto& bank,const auto& a){
  const auto& live=bank.floating_point();
  f::PlacementFp state;state.control_word=live.x87_control_word;state.mxcsr=live.mxcsr;
  f::light_reconnect(world,a.at(0),state);return 0u;});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
// BA630 again, with sphere-light attachments in its cases.
const Register reconnect_lights("object_reconnect_lights",[](auto& world,auto& bank,const auto& a){
  const auto& live=bank.floating_point();
  f::PlacementFp state;state.control_word=live.x87_control_word;state.mxcsr=live.mxcsr;
  f::object_reconnect_to_map(world,a.at(0),a.at(1),state);return 0u;});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
const Register sphere("points_bounding_sphere",[](auto& world,auto& bank,const auto& a){
  const auto& live=bank.floating_point();
  f::PlacementFp state;state.control_word=live.x87_control_word;state.mxcsr=live.mxcsr;
  f::points_bounding_sphere(world,a.at(0),a.at(1),a.at(2),a.at(3),state);return 0u;});
}
namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
// 16D740: flags, centre, radius, two ignored objects, out centre, out
// radius. The fitted centre (x, y, z) and then the radius are stored as the
// original stores them; AL is the result.
std::uint32_t fit_sphere(SourcePhysicsContactMemory& world,const NativeCampaignScriptBank& bank,
    const std::vector<std::uint32_t>& a) {
  const auto& live=bank.floating_point();
  f::PlacementFp state;state.control_word=live.x87_control_word;state.mxcsr=live.mxcsr;
  const auto load=[&](std::uint32_t at){std::array<std::byte,4> b{};world.read(at,b);return std::bit_cast<float>(b);};
  const auto store=[&](std::uint32_t at,float value){const auto b=std::bit_cast<std::array<std::byte,4>>(value);world.write(at,b);};
  const std::array<float,3> center{load(a.at(1)),load(a.at(1)+4u),load(a.at(1)+8u)};
  const auto fit=f::collision_sphere_fit(world,a.at(0),center,std::bit_cast<float>(a.at(2)),state);
  for(std::uint32_t k=0;k<3u;++k)store(a.at(5)+4u*k,fit.center[k]);
  store(a.at(6),fit.radius);
  return std::uint32_t(fit.fitted);
}
const Register sphere_fit("collision_sphere_fit",[](auto& world,auto& bank,const auto& a){return fit_sphere(world,bank,a);});
// 16D740 again, over instanced geometry (16CF00) and object chains (flags
// bits 2 and 3).
const Register sphere_fit_instances("collision_sphere_fit_instances",[](auto& world,auto& bank,const auto& a){
  return fit_sphere(world,bank,a);});
}
