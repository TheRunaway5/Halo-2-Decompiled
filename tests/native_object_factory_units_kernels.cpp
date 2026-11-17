// Oracle harness kernels for track "units" (see the registry header).
#include "halo2/native_object_factory_units_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
#include <array>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::UnitFp fp(const NativeCampaignScriptBank& bank) {
  return {bank.floating_point().x87_control_word,0,bank.floating_point().mxcsr};
}
void store(SourcePhysicsContactMemory& world,std::uint32_t at,std::uint32_t value) {
  const std::array b{std::byte(value),std::byte(value>>8),std::byte(value>>16),std::byte(value>>24)};
  world.write(at,b);
}
// A refusal of one of `owners` (the callee whose entry the original's run
// stops at, see STOPS in the cases) reports the original's boundary EAX.
template<class F>
std::uint32_t at_boundary(F&& run,std::initializer_list<const char*> owners,std::uint32_t boundary) {
  try{return run();}
  catch(const std::runtime_error& error) {
    const std::string what=error.what();
    for(const auto* owner:owners)if(what.rfind(std::string("Native object factory: ")+owner+" ",0)==0) {
      native_object_factory_tests::stopped_at=std::uint32_t(std::stoul(owner,nullptr,16));
      return boundary;
    }
    throw;
  }
}
const Register nodes_changed("units_nodes_changed",[](auto& world,auto& bank,const auto& a){
  f::unit_nodes_changed(world,a.at(0),a.at(1),a.at(2),fp(bank));return 0u;});
const Register fade_start("units_fade_start",[](auto& world,auto& bank,const auto& a){
  f::animation_fade_start(world,a.at(0),std::uint8_t(a.at(1)),a.at(2),fp(bank));return 0u;});
const Register fade_weight("units_fade_weight",[](auto& world,auto& bank,const auto& a){
  return f::animation_fade_weight(world,a.at(0),bank.floating_point().mxcsr);});
const Register ik_weapon("units_ik_next_weapon",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::animation_ik_point_next(world,a.at(0),a.at(1),true));});
const Register ik_look("units_ik_next_look",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::animation_ik_point_next(world,a.at(0),a.at(1),false));});
const Register weapon("units_weapon",[](auto& world,auto&,const auto& a){
  return f::unit_weapon(world,a.at(0),std::int16_t(a.at(1)));});
const Register graph_find("units_graph_find",[](auto& world,auto&,const auto& a){
  store(world,a.at(1),f::animation_graph_find(world,a.at(0),a.at(2)));return 0u;});
const Register look_enabled("units_look_enabled",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::unit_look_enabled(world,a.at(0)));});
const Register aim_enabled("units_aim_enabled",[](auto& world,auto&,const auto& a){
  return std::uint32_t(f::unit_aim_enabled(world,a.at(0)));});
const Register biped_validate("units_biped_validate",[](auto& world,auto& bank,const auto& a){
  f::biped_validate(world,a.at(0),bank.floating_point().mxcsr);return 0u;});
const Register biped_new("units_biped_new",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::biped_new(world,a.at(0),a.at(1),a.at(2),fp(bank)));});
const Register physics_mode("units_physics_mode",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{f::biped_physics_mode_set(world,a.at(0),std::uint8_t(a.at(1)),fp(bank));return 0u;},
      {"1D9510","1D73D0","1D97F0"},0u);});
const Register biped_failed("units_biped_new_failed",[](auto& world,auto&,const auto& a){
  f::biped_new_failed(world,a.at(0));return 0u;});
const Register unit_failed("units_new_failed",[](auto& world,auto&,const auto& a){
  f::unit_new_failed(world,a.at(0));return 0u;});
const Register marker_index("units_marker_index",[](auto& world,auto&,const auto& a){
  return f::model_node_index(world,a.at(0),a.at(1));});
const Register animation_reset("units_animation_reset",[](auto& world,auto&,const auto& a){
  f::unit_animation_reset(world,a.at(0));return 0u;});
const Register variant_record("units_variant_record",[](auto& world,auto&,const auto& a){
  f::unit_variant_record(world,a.at(0),a.at(1));return 0u;});
// These cases stop the original at 111B10's entry (EAX 2 for C6440, 0 for
// F10E0); the native's 111B10 call is overridden by that boundary.
bool stop_at_111b10(SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t,const std::array<std::uint32_t,6>&) {
  throw std::runtime_error("Native object factory: 111B10 boundary of these cases");
}
const Register unit_new("units_new",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{return std::uint32_t(f::unit_new(world,a.at(0),a.at(1),a.at(2),a.at(3),fp(bank),stop_at_111b10));},
      {"111B10"},2u);});
const Register vehicle_new("units_vehicle_new",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{return std::uint32_t(f::vehicle_new(world,a.at(0),a.at(1),a.at(2),fp(bank),stop_at_111b10));},
      {"111B10"},0u);});
const Register animation_state("units_animation_state",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::unit_animation_state_set(world,a.at(1),a.at(0),{a.at(2),a.at(3),a.at(4),a.at(5),a.at(6),a.at(7)},
      fp(bank)));});
const Register channels_refresh("units_channels_refresh",[](auto& world,auto& bank,const auto& a){
  f::unit_animation_channels_refresh(world,a.at(0),fp(bank));return 0u;});
const Register vehicle_settle("units_vehicle_settle",[](auto& world,auto& bank,const auto& a){
  f::vehicle_animation_settle(world,a.at(0),bank.floating_point().mxcsr);return 0u;});
const Register root_origin("units_root_origin",[](auto& world,auto& bank,const auto& a){
  return f::object_root_origin(world,a.at(0),a.at(1),bank.floating_point().mxcsr);});
// C6850 is void: any named refusal ends the case where the original's run
// stops at that callee's entry (STOPS['units_place']: B9B70 and the
// unitactions track's refused callees past E8E50 and CECF0).
const Register place("units_place",[](auto& world,auto& bank,const auto& a){
  try{f::unit_place(world,a.at(0),fp(bank));}
  catch(const std::runtime_error& error) {
    const std::string what=error.what(),prefix="Native object factory: ";
    if(what.rfind(prefix,0)!=0)throw;
    native_object_factory_tests::stopped_at=std::uint32_t(std::stoul(what.substr(prefix.size()),nullptr,16));
  }
  return 0u;});
const Register blend_seconds("units_blend_seconds",[](auto&,auto& bank,const auto& a){
  return f::animation_blend_seconds(a.at(0),a.at(1),std::uint8_t(a.at(2)),std::uint8_t(a.at(3)),a.at(4),
      bank.floating_point().mxcsr);});
const Register fade_remaining("units_fade_remaining",[](auto& world,auto& bank,const auto& a){
  return f::animation_fade_remaining(world,a.at(0),bank.floating_point().mxcsr);});
const Register interpolation("units_interpolation_start",[](auto& world,auto& bank,const auto& a){
  return std::uint32_t(f::animation_interpolation_start(world,a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),fp(bank)));});
const Register object_interpolation("units_object_interpolation",[](auto& world,auto& bank,const auto& a){
  f::object_interpolation_start(world,a.at(0),a.at(1),fp(bank));return 0u;});
const Register overlays("units_overlays_restart",[](auto& world,auto&,const auto& a){
  f::unit_overlays_restart(world,a.at(0));return 0u;});
}
