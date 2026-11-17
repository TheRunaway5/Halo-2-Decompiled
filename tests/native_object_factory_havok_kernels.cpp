// Oracle harness kernels for track "havok" (see the registry header).
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "halo2/native_tu15_island_activation.hpp"
#include "native_object_factory_test_registry.hpp"
#include <string>

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
f::HavokFp live(const NativeCampaignScriptBank& bank) {
  const auto& state=bank.floating_point();
  return f::HavokFp{state.x87_control_word,0,state.mxcsr};
}
const Register allocate("havok_array_allocate",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);return f::havok_array_allocate(world,a.at(0),a.at(1),fp);});
const Register release("havok_array_release",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_array_release(world,a.at(0),a.at(1),a.at(2),fp);return 0u;});
const Register grow("havok_grow_array",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_grow_array(world,a.at(0),a.at(1),fp);return 0u;});
const Register activate_body("havok_activate_body",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_activate_body(world,a.at(0),fp);return 0u;});
const Register component_activate("havok_component_activate",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_component_activate(world,a.at(0),fp);return 0u;});
const Register component_velocities("havok_object_component_velocities",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_object_component_velocities(world,a.at(0),a.at(1),a.at(2),
      (a.at(3)&0xffu)!=0,fp);return 0u;});
const Register reserved("havok_reserved_body",[](auto& world,auto&,const auto& a){
  return std::uint32_t(native_tu15_reserved_body_reference(world,{a.at(0)}));});
const Register phantom_info("havok_aabb_phantom_info",[](auto& world,auto&,const auto& a){
  return f::havok_aabb_phantom_info(world,a.at(0));});
const Register phantom_construct("havok_aabb_phantom_construct",[](auto& world,auto&,const auto& a){
  return f::havok_aabb_phantom_construct(world,a.at(0),a.at(1));});
const Register component_reset("havok_component_reset",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_component_reset(world,a.at(0),fp);return 0u;});
// The last argument of the kernels below is the original entry ESP (the
// harness SP), which names the stack local the original publishes.
// A refusal of one of `owners` ends the case where the oracle stops the
// original, at that callee's entry (observed stop entries): 1CC310 at
// 1D73D0, 1D73D0 at its shape builders.
template<class F> std::uint32_t at_entry(F&& run,std::initializer_list<const char*> owners) {
  try{run();}
  catch(const std::runtime_error& error) {
    const std::string what=error.what(),prefix="Native object factory: ";
    bool stop=false;
    for(const char* owner:owners)stop=stop || what.rfind(prefix+owner+" ",0)==0;
    if(!stop)throw;
    native_object_factory_tests::stopped_at=std::uint32_t(std::stoul(what.substr(prefix.size()),nullptr,16));
  }
  return 0u;
}
const Register state_flags("havok_component_state_flags",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);
  return at_entry([&]{f::havok_component_state_flags(world,a.at(0),a.at(1),fp,a.size()>2?a.at(2):0u);},
      {"1D9510","1D73D0"});});
const Register component_refresh("havok_component_refresh",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);
  return at_entry([&]{f::havok_component_refresh(world,a.at(0),fp);},{"1DEE30","1DD9A0","1DDBF0","1DD950","1DEF50"});});
const Register component_remove("havok_component_remove",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_component_remove(world,a.at(0),fp,a.at(1));return 0u;});
const Register component_add("havok_component_add",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_component_add(world,a.at(0),fp,a.at(1));return 0u;});
const Register remove_entity("havok_remove_entity",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_world_remove_entity(world,a.at(0),a.at(1),a.at(2),fp,a.at(3));return 0u;});
const Register add_entity("havok_add_entity",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);return f::havok_world_add_entity(world,a.at(0),a.at(1),fp,a.at(2));});
const Register broadphase_add("havok_broadphase_add",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_broadphase_add_object(world,a.at(0),a.at(1),a.at(2),a.at(3),fp);return 0u;});
const Register broadphase_remove("havok_broadphase_remove",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_broadphase_remove_object(world,a.at(0),a.at(1),a.at(2),fp);return 0u;});
const Register add_phantom("havok_add_phantom",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);return f::havok_world_add_phantom(world,a.at(0),a.at(1),fp);});
const Register remove_phantom("havok_remove_phantom",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_world_remove_phantom(world,a.at(0),a.at(1),fp);return 0u;});
const Register phantom_wake("havok_phantom_wake",[](auto& world,auto& bank,const auto& a){
  auto fp=live(bank);f::havok_phantom_wake(world,a.at(0),fp);return 0u;});
}
