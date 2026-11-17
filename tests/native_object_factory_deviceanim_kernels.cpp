// Oracle harness kernels for track "deviceanim" (see the registry header).
// Frame out-parameters are bound to their declared bank addresses.
#include "halo2/native_object_factory_deviceanim_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace d=native_object_factory_tu15::device_animation;
using native_object_factory_tests::Register;
using Args=std::vector<std::uint32_t>;
using World=SourcePhysicsContactMemory;
d::FrameWord bound(World& world,std::uint32_t address){d::FrameWord word;word.bind(world,address);return word;}

const Register find_named("deviceanim_find_named_animation",[](World& w,auto&,const Args& v){
  auto out=bound(w,v.at(1));d::find_named_animation(w,v.at(0),out,v.at(2));return v.at(1);});
// Words: key3, graph, result, r0, r1, r2, p, q; the out-pointers are p, q, p.
const Register overlay_lookup("deviceanim_overlay_lookup",[](World& w,auto&,const Args& v){
  auto result=bound(w,v.at(2));
  const std::uint32_t addresses[3]{v.at(6),v.at(7),v.at(6)};
  d::FrameWord outs[3];d::FrameWord* pointers[3]{};
  for(int n=0;n<3;++n)if(addresses[n]){outs[n]=bound(w,addresses[n]);pointers[n]=&outs[n];}
  d::overlay_lookup(w,v.at(1),v.at(0),v.at(3),v.at(4),v.at(5),pointers[0],pointers[1],pointers[2],result);
  return v.at(2);});
const Register start_named("deviceanim_start_named_animation",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(d::start_named_animation(w,v.at(0),v.at(1),v.at(2),v.at(3),bank.floating_point()));});
const Register overlay_channel("deviceanim_overlay_channel",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(d::device_start_overlay_channel(w,v.at(1),v.at(0),bank.floating_point()));});
const Register position_channel("deviceanim_position_channel",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(d::device_start_position_channel(w,v.at(1),v.at(0),bank.floating_point()));});
const Register overlay_track("deviceanim_set_overlay_track",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(d::device_set_overlay_track(w,v.at(0),v.at(1),bank.floating_point()));});
const Register normalize("deviceanim_normalize3",[](World& w,auto& bank,const Args& v){
  return d::normalize3(w,v.at(0),bank.floating_point());});
const Register basis("deviceanim_basis_matrix",[](World& w,auto& bank,const Args& v){
  d::basis_matrix(w,v.at(0),v.at(1),v.at(2),v.at(3),bank.floating_point());return 0u;});
const Register inverse("deviceanim_matrix_inverse",[](World& w,auto& bank,const Args& v){
  d::matrix_inverse(w,v.at(0),v.at(1),bank.floating_point());return 0u;});
const Register quaternion("deviceanim_rotation_quaternion",[](World& w,auto& bank,const Args& v){
  d::rotation_quaternion(w,v.at(0),v.at(1),bank.floating_point());return 0u;});
const Register orientation("deviceanim_matrix_orientation",[](World& w,auto& bank,const Args& v){
  d::matrix_orientation(w,v.at(0),v.at(1),bank.floating_point());return 0u;});
const Register product("deviceanim_quaternion_product",[](World& w,auto& bank,const Args& v){
  d::quaternion_product(w,v.at(0),v.at(1),v.at(2),bank.floating_point());return 0u;});
const Register compose("deviceanim_orientation_compose",[](World& w,auto& bank,const Args& v){
  d::orientation_compose(w,v.at(0),v.at(1),v.at(2),bank.floating_point());return 0u;});
const Register node_index("deviceanim_node_index",[](World& w,auto&,const Args& v){
  return d::graph_node_index(w,v.at(0),v.at(1));});
const Register codec_entry("deviceanim_codec_entry",[](World& w,auto&,const Args& v){
  return d::codec_entry(w,v.at(0),v.at(1));});
const Register static_pose("deviceanim_static_pose",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(d::static_pose(w,v.at(0),v.at(1),v.at(2),bank.floating_point()));});
const Register channel_seconds("deviceanim_channel_seconds",[](World& w,auto& bank,const Args& v){
  return d::channel_seconds(w,v.at(0),bank.floating_point());});
const Register orientations("deviceanim_has_node_orientations",[](World& w,auto&,const Args& v){
  return std::uint32_t(d::object_has_node_orientations(w,v.at(0)));});
// Composite owners: the codec (283A90), node rebuild (BF050) and set
// position (B95D0) services are absent, so reaching one is its refusal.
const Register sample_orientation("deviceanim_sample_orientation",[](World& w,auto& bank,const Args& v){
  d::sample_orientation(w,v.at(0),v.at(1),v.at(2),v.at(3),bank.floating_point(),{});return 0u;});
const Register sample_root("deviceanim_sample_root",[](World& w,auto& bank,const Args& v){
  d::sample_root(w,v.at(0),v.at(1),v.at(2),v.at(3),bank.floating_point(),{});return 0u;});
const Register velocity("deviceanim_sample_root_velocity",[](World& w,auto& bank,const Args& v){
  d::sample_root_velocity(w,v.at(0),v.at(1),v.at(2),v.at(3),v.at(4),bank.floating_point(),{});return 0u;});
const Register position_track("deviceanim_set_position_track",[](World& w,auto& bank,const Args& v){
  return std::uint32_t(d::device_set_position_track(w,v.at(0),v.at(1),v.at(2),bank.floating_point(),{}));});
}
