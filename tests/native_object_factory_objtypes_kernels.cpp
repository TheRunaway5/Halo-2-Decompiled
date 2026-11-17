// Oracle harness kernels for track "objtypes" (see the registry header).
// Arguments follow the routines in tools/native_object_factory_cases_objtypes.py.
#include "halo2/native_object_factory_objtypes_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::ObjectTypeFp floating_point(const NativeCampaignScriptBank& bank) {
  return {bank.floating_point().x87_control_word,0,bank.floating_point().mxcsr};
}
// Empty hooks: BF050 and 1CE340 stay named refusals the fixtures never reach.
f::ObjectTypeServices services(const NativeCampaignScriptBank& bank) {
  f::ObjectTypeServices out;
  out.fp=floating_point(bank);
  return out;
}
const Register device_new("objtypes_device_new",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::device_new(world,a.at(0),a.at(1),a.at(2)));});
const Register machine_new("objtypes_machine_new",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::machine_new(world,a.at(0),a.at(1),a.at(2)));});
const Register sound_scenery_new("objtypes_sound_scenery_new",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::sound_scenery_new(world,a.at(0),a.at(1),a.at(2)));});
const Register crate_new("objtypes_crate_new",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::crate_new(world,a.at(0),a.at(1),a.at(2)));});
const Register scenery_new("objtypes_scenery_new",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::scenery_new(world,a.at(0),a.at(1),a.at(2),services(bank)));});
const Register garbage_new("objtypes_garbage_new",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::garbage_new(world,a.at(0),a.at(1),a.at(2),floating_point(bank)));});
const Register projectile_new("objtypes_projectile_new",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::projectile_new(world,a.at(0),a.at(1),a.at(2),floating_point(bank)));});
const Register projectile_spin("objtypes_projectile_spin",[](auto& world,auto& bank,const auto& a){
  f::projectile_spin(world,a.at(0),floating_point(bank));return 0u;});
const Register device_new_failed("objtypes_device_new_failed",[](auto& world,auto&,const auto& a){
  f::device_new_failed(world,a.at(0));return 0u;});
const Register machine_new_failed("objtypes_machine_new_failed",[](auto& world,auto&,const auto& a){
  f::machine_new_failed(world,a.at(0));return 0u;});
const Register scenery_new_failed("objtypes_scenery_new_failed",[](auto& world,auto& bank,const auto& a){
  f::scenery_new_failed(world,a.at(0),services(bank));return 0u;});
const Register set_animation("objtypes_object_set_animation",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::object_set_animation(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),(a.at(5)&0xffu)!=0,
      (a.at(6)&0xffu)!=0,services(bank)));});
const Register manager_find("objtypes_animation_manager_find",[](auto& world,auto&,const auto& a){
  return f::animation_manager_find(world,a.at(0),a.at(1),a.at(2));});
const Register clear_overlays("objtypes_animation_manager_clear_overlays",[](auto& world,auto&,const auto& a){
  f::animation_manager_clear_overlays(world,a.at(0));return 0u;});
const Register set_frame("objtypes_animation_manager_set_frame",[](auto& world,auto& bank,const auto& a){
  f::animation_manager_set_frame(world,a.at(0),a.at(1),floating_point(bank));return 0u;});
const Register manager_start("objtypes_animation_manager_start",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::animation_manager_start(world,a.at(0),a.at(1),a.at(2),floating_point(bank)));});
const Register header_activate("objtypes_object_header_activate",[](auto& world,auto& bank,const auto& a){
  f::object_header_activate(world,a.at(0),floating_point(bank));return 0u;});
const Register physics_deactivate("objtypes_object_physics_deactivate",[](auto& world,auto&,const auto& a){
  f::object_physics_deactivate(world,a.at(0));return 0u;});
const Register object_wake("objtypes_object_wake",[](auto& world,auto& bank,const auto& a){
  f::object_wake(world,a.at(0),std::uint8_t(a.at(1))!=0,floating_point(bank));return 0u;});
// BBBC0 over the havok track's physics world: 1D9F10 activates the
// component's bodies through the havok track's 315CB0.
const Register object_wake_bodies("objtypes_object_wake_bodies",[](auto& world,auto& bank,const auto& a){
  f::object_wake(world,a.at(0),std::uint8_t(a.at(1))!=0,floating_point(bank));return 0u;});
const Register lightmap_row("objtypes_lightmap_scenery_object_row",[](auto& world,auto&,const auto& a){
  return f::lightmap_scenery_object_row(world,a.at(0),a.at(1));});
}
