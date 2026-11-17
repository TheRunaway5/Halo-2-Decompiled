// Oracle harness kernels for track "nodebuild" (see the registry header).
// Argument words follow tools/native_object_factory_cases_nodebuild.py.
#include "halo2/native_object_factory_nodebuild_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
std::uint32_t mxcsr(const NativeCampaignScriptBank& bank){return bank.floating_point().mxcsr;}
f::NodeRebuildFp controls(const NativeCampaignScriptBank& bank) {
  f::NodeRebuildFp fp;
  fp.control_word=bank.floating_point().x87_control_word;
  fp.mxcsr=bank.floating_point().mxcsr;
  return fp;
}
const Register mask_empty("nodebuild_mask_empty",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::node_mask_empty(world,a.at(0)));});
const Register mask_invert("nodebuild_mask_invert",[](auto& world,auto&,const auto& a){
  f::node_mask_invert(world,a.at(0));return 0u;});
const Register mask_limit("nodebuild_mask_limit",[](auto& world,auto&,const auto& a){
  f::node_mask_limit(world,a.at(0),a.at(1));return 0u;});
const Register mask_last("nodebuild_mask_last",[](auto& world,auto&,const auto& a){
  return f::node_mask_last(world,a.at(0),a.at(1));});
const Register flags_bit("nodebuild_flags_bit",[](auto& world,auto&,const auto& a){
  f::flags_assign_bit(world,a.at(0),a.at(1),(a.at(2)&0xffu)!=0);return 0u;});
const Register frozen("nodebuild_frozen",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::object_animation_frozen(world,a.at(0)));});
const Register manager_valid("nodebuild_manager_valid",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::animation_manager_valid(world,a.at(0)));});
const Register animation("nodebuild_animation",[](auto& world,auto&,const auto& a){
  return f::animation_definition(world,a.at(0),a.at(1));});
const Register manager_animation("nodebuild_manager_animation",[](auto& world,auto&,const auto& a){
  return f::animation_manager_definition(world,a.at(0));});
const Register height("nodebuild_height",[](auto& world,auto& bank,const auto& a){
  return f::biped_physics_height(world,a.at(0),mxcsr(bank));});
const Register default_orientations("nodebuild_default_orientations",[](auto& world,auto&,const auto& a){
  f::model_default_orientations(world,a.at(0),a.at(1));return 0u;});
const Register orientation("nodebuild_orientation",[](auto& world,auto& bank,const auto& a){
  f::orientation_matrix(world,a.at(0),a.at(1),mxcsr(bank));return 0u;});
const Register product("nodebuild_product",[](auto& world,auto& bank,const auto& a){
  return f::matrix_product(world,a.at(0),a.at(1),a.at(2),mxcsr(bank));});
const Register root("nodebuild_root",[](auto& world,auto& bank,const auto& a){
  f::object_root_matrix(world,a.at(0),a.at(1),(a.at(2)&0xffu)!=0,a.at(3),a.at(4),a.at(5),a.at(6),a.at(7),mxcsr(bank));
  return 0u;});
const Register type_hooks("nodebuild_type_hooks",[](auto& world,auto& bank,const auto& a){
  f::object_type_node_hooks(world,a.at(0),a.at(1),mxcsr(bank));return 0u;});
const Register unit_hook("nodebuild_unit_hook",[](auto& world,auto& bank,const auto& a){
  f::unit_node_matrix_hook(world,a.at(0),a.at(1),mxcsr(bank));return 0u;});
const Register scenery_hook("nodebuild_scenery_hook",[](auto& world,auto& bank,const auto& a){
  f::scenery_node_matrix_hook(world,a.at(0),a.at(1),mxcsr(bank));return 0u;});
const Register fraction("nodebuild_fraction",[](auto& world,auto& bank,const auto& a){
  return f::animation_interpolation_fraction(world,a.at(0),mxcsr(bank));});
const Register crossfade("nodebuild_crossfade",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::animation_crossfade(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),fp);return 0u;});
const Register blend_fraction("nodebuild_blend_fraction",[](auto& world,auto& bank,const auto& a){
  return f::animation_blend_fraction(world,a.at(0),mxcsr(bank));});
const Register codec_entry("nodebuild_codec_entry",[](auto& world,auto&,const auto& a){
  return f::animation_codec_entry(world,a.at(0),a.at(1));});
const Register sample("nodebuild_sample",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::object_sample_animation(world,a.at(2),a.at(3),a.at(4),a.at(5),a.at(0),a.at(1),fp);return 0u;});
// Codec track decoders: one kernel per original entry.
template<std::uint32_t decoder>struct CodecTrack {
  explicit CodecTrack(const char* name):r(name,[](auto& world,auto& bank,const auto&){
    f::animation_codec_track(world,decoder,mxcsr(bank));return 0u;}){}
  Register r;
};
const CodecTrack<0x296180u> codec_296180("nodebuild_codec_296180");
const CodecTrack<0x296680u> codec_296680("nodebuild_codec_296680");
const CodecTrack<0x296450u> codec_296450("nodebuild_codec_296450");
const CodecTrack<0x296240u> codec_296240("nodebuild_codec_296240");
const CodecTrack<0x2963d0u> codec_2963d0("nodebuild_codec_2963d0");
const CodecTrack<0x2961f0u> codec_2961f0("nodebuild_codec_2961f0");
const CodecTrack<0x296410u> codec_296410("nodebuild_codec_296410");
const CodecTrack<0x296220u> codec_296220("nodebuild_codec_296220");
const CodecTrack<0x2cec60u> codec_2cec60("nodebuild_codec_2cec60");
const CodecTrack<0x296c60u> codec_296c60("nodebuild_codec_296c60");
const CodecTrack<0x296db0u> codec_296db0("nodebuild_codec_296db0");
const CodecTrack<0x296ad0u> codec_296ad0("nodebuild_codec_296ad0");
const CodecTrack<0x297270u> codec_297270("nodebuild_codec_297270");
const CodecTrack<0x297400u> codec_297400("nodebuild_codec_297400");
const CodecTrack<0x296700u> codec_296700("nodebuild_codec_296700");
const CodecTrack<0x296890u> codec_296890("nodebuild_codec_296890");
const CodecTrack<0x2969e0u> codec_2969e0("nodebuild_codec_2969e0");
// Composite codec owners (tools/native_object_factory_cases_nodebuild.py).
const Register codec_frames("nodebuild_codec_frames",[](auto& world,auto& bank,const auto&){
  f::animation_codec_frames(world,mxcsr(bank));return 0u;});
const Register codec_setup("nodebuild_codec_setup",[](auto& world,auto& bank,const auto& a){
  f::AnimationCodecSetup s;
  s.block=a.at(0);s.frame=a.at(1);s.weight=a.at(2);s.graph=a.at(3);s.animation=a.at(4);s.count=a.at(5);
  s.entry=a.at(6);s.mask=a.at(7);s.orientations=a.at(8);s.force=a.at(9);s.flag=a.at(10);s.second_frame=a.at(11);
  s.second_weight=a.at(12);
  f::animation_codec_setup(world,s,mxcsr(bank));return 0u;});
const Register resource_data("nodebuild_resource_data",[](auto& world,auto&,const auto& a){
  return f::animation_resource_data(world,a.at(0),a.at(1));});
const Register codec("nodebuild_codec",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);
  f::animation_decode(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),a.at(5),a.at(6),a.at(7),(a.at(8)&0xffu)!=0,fp);
  return 0u;});
const Register device_hook("nodebuild_device_hook",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::device_orientation_hook(world,a.at(0),a.at(1),a.at(2),a.at(3),fp);return 0u;});
const Register vehicle_hook("nodebuild_vehicle_hook",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::vehicle_orientation_hook(world,a.at(0),a.at(1),a.at(2),a.at(3),fp);return 0u;});
const Register unit_hook_orientations("nodebuild_unit_orientation_hook",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::unit_orientation_hook(world,a.at(0),a.at(1),a.at(2),a.at(3),fp);return 0u;});
const Register aim_channel("nodebuild_aim_channel",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::animation_aim_channel(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),a.at(5),a.at(6),fp);
  return 0u;});
const Register fraction_channel("nodebuild_fraction_channel",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::animation_fraction_channel(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),a.at(5),fp);
  return 0u;});
const Register physics_nodes("nodebuild_physics_nodes",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::object_physics_nodes(world,a.at(0),a.at(1),a.at(2),fp);return 0u;});
const Register crate_sphere("nodebuild_crate_sphere",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);return std::uint32_t(f::crate_physics_sphere(world,a.at(0),a.at(1),a.at(2),fp));});
const Register bounds("nodebuild_bounds",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);return std::uint32_t(f::object_update_bounds(world,a.at(0),fp));});
const Register node_matrices("nodebuild_node_matrices",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::object_update_node_matrices(world,a.at(0),fp);return 0u;});
const Register rebuild("nodebuild_rebuild",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::object_rebuild_nodes(world,a.at(0),fp);return 0u;});
// 16A4DD's bound callees at their entries.
const Register sample_channels("nodebuild_sample_channels",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::animation_sample_channels(world,a.at(0),a.at(2),a.at(3),a.at(4),a.at(7),fp);return 0u;});
const Register function_overlays("nodebuild_function_overlays",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::object_function_overlays(world,a.at(0),a.at(2),a.at(3),a.at(4),a.at(5),fp);return 0u;});
const Register sample_channel("nodebuild_sample_channel",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::animation_sample_channel(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),fp);return 0u;});
// BF640 and its callees (argument words as the cases pass them).
const Register deferred("nodebuild_deferred",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);return f::object_deferred_node_matrices(world,a.at(0),a.at(1),a.at(2),fp);});
const Register frame_overlays("nodebuild_frame_overlays",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::unit_frame_overlays(world,a.at(0),a.at(1),a.at(3),a.at(4),a.at(5),a.at(6),fp);return 0u;});
const Register aim_overlays("nodebuild_aim_overlays",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::unit_aim_overlays(world,a.at(0),a.at(1),a.at(3),a.at(4),a.at(5),a.at(6),fp);return 0u;});
const Register unit_hand_ik("nodebuild_unit_hand_ik",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::unit_hand_ik(world,a.at(0),a.at(1),a.at(2),a.at(3),fp);return 0u;});
const Register hand_ik("nodebuild_hand_ik",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::hand_ik(world,a.at(0),a.at(1),a.at(2),fp);return 0u;});
const Register euler("nodebuild_euler",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::euler_matrix(world,a.at(0),a.at(1),a.at(2),a.at(3),fp);return 0u;});
const Register node_flags("nodebuild_node_flags",[](auto& world,auto&,const auto& a){
  f::animation_node_flags(world,a.at(0),a.at(1));return 0u;});
// 16A4DD, 16AD45 and 169863 (first-person animation). No residual owners
// are bound: 17A570, 171440 and 128E70 refuse where the original stops.
const Register first_person("nodebuild_first_person",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::first_person_animation(world,a.at(0),a.at(1),fp);return 0u;});
const Register first_person_rebuild("nodebuild_first_person_rebuild",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::first_person_rebuild(world,a.at(0),a.at(1),fp);return 0u;});
const Register first_person_state("nodebuild_first_person_state",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::first_person_state(world,a.at(0),a.at(1),a.at(2),(a.at(3)&0xffu)!=0,fp);return 0u;});
// 169C46 (user): 18BE50, 17A570, 171440 and 128E70 refuse where the original stops.
const Register first_person_update("nodebuild_first_person_update",[](auto& world,auto& bank,const auto& a){
  auto fp=controls(bank);f::first_person_update(world,a.at(0),fp);return 0u;});
// The same owners on MCC-named graphs: the oracle applies the translated
// route's key joins (query_names), the natives search through the cache's
// names (weapon_animation::QueryNamespace).
using MccNames=f::weapon_animation::QueryNamespace;
const Register first_person_rebuild_mcc("nodebuild_first_person_rebuild_mcc",[](auto& world,auto& bank,const auto& a){
  const MccNames names(true);auto fp=controls(bank);f::first_person_rebuild(world,a.at(0),a.at(1),fp);return 0u;});
const Register first_person_state_mcc("nodebuild_first_person_state_mcc",[](auto& world,auto& bank,const auto& a){
  const MccNames names(true);auto fp=controls(bank);
  f::first_person_state(world,a.at(0),a.at(1),a.at(2),(a.at(3)&0xffu)!=0,fp);return 0u;});
const Register first_person_update_mcc("nodebuild_first_person_update_mcc",[](auto& world,auto& bank,const auto& a){
  const MccNames names(true);auto fp=controls(bank);f::first_person_update(world,a.at(0),fp);return 0u;});
}
