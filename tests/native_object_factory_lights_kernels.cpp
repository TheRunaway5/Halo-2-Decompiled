// Oracle harness kernels for track "lights" (see the registry header).
#include "halo2/native_object_factory_lights_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
NativeTu15PhysicsFrameFp live(const NativeCampaignScriptBank& bank) {
  const auto& state=bank.floating_point();
  NativeTu15PhysicsFrameFp out;out.control_word=state.x87_control_word;out.mxcsr=state.mxcsr;return out;
}
// C5290: light, update-owner byte.
const Register remove("light_delete",[](auto& world,auto&,const auto& a){
  f::light_delete(world,a.at(0),(a.at(1)&0xffu)!=0);return 0u;});
// C3430: no arguments.
const Register tick("lights_update",[](auto& world,auto& bank,const auto&){
  auto state=live(bank);f::lights_update(world,state);return 0u;});
// C3430 again over the attachments track's light rows (function-driven
// intensities, owners, timed fades).
const Register tick_functions("lights_update_functions",[](auto& world,auto& bank,const auto&){
  auto state=live(bank);f::lights_update(world,state);return 0u;});
}
