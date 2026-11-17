// Oracle harness kernels for track "birth" (see the registry header).
#include "halo2/native_object_factory_birth_tu15.hpp"
#include "native_object_factory_test_registry.hpp"

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
// B9B70 up to its block-allocation failure (or datum/body refusal): every
// service is left empty, so reaching a not-yet-native owner refuses.
const Register early("object_new_early_failure",[](auto& world,auto& bank,const auto& a){
  f::ObjectFactoryServices services;
  services.mxcsr=bank.floating_point().mxcsr;
  services.entry_stack=a.at(1);
  return f::object_new(world,a.at(0),services);});
const Register place("object_place_variant_children",[](auto& world,auto& bank,const auto& a){
  f::ObjectFactoryServices services;services.mxcsr=bank.floating_point().mxcsr;
  f::object_place_variant_children(world,a.at(0),services);return 0u;});
}
